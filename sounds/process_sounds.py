"""Turn curated picks into clean, game-ready master clips.

Usage (from the sounds/ folder):
    python3 process_sounds.py

Reads curation.json + raw/manifest.json; writes processed/<pet>/*.wav, processed/manifest.json
and review.html (a listening page for the results).

Each master is mono 16-bit 44.1 kHz WAV:
  * cut to the curated selection (start/end), leading/trailing silence removed
  * loudness-matched: average level brought to TARGET_MEAN_DB, peaks held under PEAK_DB by a
    limiter (RMS normalisation + limiting — keeps a crunch and a bark at similar perceived level)
  * 5 ms fade-in / 30 ms fade-out so clips start and stop without clicks
  * long bark recordings are split into single barks at the silences between them

Not done here, on purpose:
  * device format (lower sample rate) — a separate export step once the audio code exists
  * baby/teen pitch — done at playback by raising the sample rate, so no pitched copies are stored
"""
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TARGET_MEAN_DB = -20.0
PEAK_DB = -1.0
MAX_LIMIT_DB = 12.0
SPLIT_IF_ROLE_HAS = "bark"
SPLIT_MIN_SECONDS = 4.0
LONG_CLIP_SECONDS = 8.0
PET_DIR = {"sausage": "sausage", "husky": "husky", "both": "shared"}


def run(args):
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"{args[0]} failed: {r.stderr[-400:]}")
    return r


def duration(path):
    r = run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", path])
    return float(r.stdout.strip())


def sounding_regions(path, noise="-32dB", min_silence=0.12, min_len=0.15, pad=0.04):
    """Regions between silences, via ffmpeg's silencedetect filter."""
    total = duration(path)
    err = run(["ffmpeg", "-hide_banner", "-i", path, "-af",
               f"silencedetect=noise={noise}:d={min_silence}", "-f", "null", "-"]).stderr
    starts = [float(x) for x in re.findall(r"silence_start: (-?[\d.]+)", err)]
    ends = [float(x) for x in re.findall(r"silence_end: ([\d.]+)", err)]
    regions, cur = [], 0.0
    for s, e in zip(starts, ends + [total]):
        if s - cur >= min_len:
            regions.append((max(0.0, cur - pad), min(total, s + pad)))
        cur = e
    if total - cur >= min_len:
        regions.append((max(0.0, cur - pad), total))
    return regions or [(0.0, total)]


def levels(path):
    """(mean, peak) in dB. ffmpeg 9 may print a throwaway filter instance first
    (n_samples: 0), so use the last reported values."""
    err = run(["ffmpeg", "-hide_banner", "-i", path, "-af", "volumedetect", "-f", "null", "-"]).stderr
    means = re.findall(r"mean_volume: (-?[\d.]+) dB", err)
    peaks = re.findall(r"max_volume: (-?[\d.]+) dB", err)
    if not means:
        return -999.0, -999.0  # no audio at all
    return float(means[-1]), float(peaks[-1])


# Fades only: 5 ms in, 30 ms out (areverse lets afade's fade-in act as a fade-out).
# Silence trimming is done with silencedetect timings instead of the silenceremove filter,
# whose options changed meaning in ffmpeg 6.1 and silently removed everything here.
CLEAN = "afade=t=in:d=0.005,areverse,afade=t=in:d=0.03,areverse"


def master(src, dst, a=None, b=None):
    """Cut [a, b] from src, clean it, loudness-match it, write dst."""
    raw_cut, tmp = dst + ".cut.wav", dst + ".tmp.wav"
    cut = ["-ss", f"{a:.3f}"] if a is not None else []
    cut += ["-to", f"{b:.3f}"] if b is not None else []
    run(["ffmpeg", "-hide_banner", "-y", "-i", src, *cut, "-ac", "1", "-ar", "44100", raw_cut])
    # Trim dead air: keep from the first to the last sounding moment (quiet threshold, small pad).
    regions = sounding_regions(raw_cut, noise="-45dB", min_silence=0.05, min_len=0.02, pad=0.02)
    lo, hi = regions[0][0], regions[-1][1]
    run(["ffmpeg", "-hide_banner", "-y", "-i", raw_cut, "-ss", f"{lo:.3f}", "-to", f"{hi:.3f}",
         "-af", CLEAN, "-c:a", "pcm_s16le", tmp])
    os.remove(raw_cut)
    mean, peak = levels(tmp)
    if peak < -50:  # nothing audible left after trimming silence: the selection missed the sound
        os.remove(tmp)
        raise ValueError(f"selection is near-silent (peak {peak:.0f} dB)")
    # Raise the average to the target; a brick-wall limiter shaves the few transient peaks
    # (crunches, clicks) that would otherwise clip, up to MAX_LIMIT_DB of gain reduction.
    gain = min(TARGET_MEAN_DB - mean, PEAK_DB - peak + MAX_LIMIT_DB)
    ceiling = 10 ** (PEAK_DB / 20)
    run(["ffmpeg", "-hide_banner", "-y", "-i", tmp, "-af",
         f"volume={gain:.2f}dB,alimiter=limit={ceiling:.3f}:attack=1:release=50:level=false",
         "-ar", "44100", "-c:a", "pcm_s16le", dst])
    os.remove(tmp)
    return duration(dst)


def slug(s):
    return re.sub(r"[^a-z0-9]+", "-", s.lower()).strip("-")


def main():
    for tool in ("ffmpeg", "ffprobe"):
        if not shutil.which(tool):
            sys.exit(f"{tool} not found — install with: brew install ffmpeg")
    picks = json.load(open(os.path.join(HERE, "curation.json"), encoding="utf-8"))
    raw = {c["key"]: c for c in json.load(open(os.path.join(HERE, "raw", "manifest.json"), encoding="utf-8"))}
    out_root = os.path.join(HERE, "processed")
    if os.path.isdir(out_root):
        shutil.rmtree(out_root)  # fully regenerated from curation.json every run
    out_manifest, long_clips = [], []

    for p in picks:
        if p.get("verdict") in (None, "reject"):
            continue
        src = os.path.join(HERE, "raw", p["file"])
        pet = PET_DIR[p["verdict"]]
        os.makedirs(os.path.join(out_root, pet), exist_ok=True)
        base = f"{slug(p['role'])}__{p['key']}"
        a = p.get("start")
        b = p.get("end")

        # First cut the selection to a temp file so splitting works on just that part.
        sel = os.path.join(out_root, pet, base + ".sel.wav")
        cut = (["-ss", f"{a:.3f}"] if a else []) + (["-to", f"{b:.3f}"] if b else [])
        run(["ffmpeg", "-hide_banner", "-y", "-i", src, *cut, "-ac", "1", "-ar", "44100", sel])
        sel_len = duration(sel)

        if SPLIT_IF_ROLE_HAS in p["role"].lower() and sel_len > SPLIT_MIN_SECONDS:
            regions = sounding_regions(sel)
        else:
            regions = [(None, None)]

        for i, (ra, rb) in enumerate(regions, 1):
            name = f"{base}_{i:02d}.wav" if len(regions) > 1 else f"{base}.wav"
            rel = f"{pet}/{name}"
            try:
                length = master(sel, os.path.join(out_root, rel), ra, rb)
            except ValueError as e:
                print(f"  SKIPPED {rel}: {e}")
                continue
            if length > LONG_CLIP_SECONDS:
                long_clips.append((rel, length))
            src_meta = raw.get(p["key"], {})
            out_manifest.append(dict(
                key=f"{p['key']}#{i}" if len(regions) > 1 else p["key"], file=rel,
                title=p["title"] + (f" — part {i}" if len(regions) > 1 else ""),
                desc=p.get("notes") or src_meta.get("desc", ""),
                length=f"{length:.2f}s", licence=src_meta.get("licence", "?"),
                page=src_meta.get("page", ""), source=src_meta.get("source", ""),
                pet=p["verdict"], role=p["role"]))
        os.remove(sel)
        print(f"{p['key']:<28} {pet:<8} {p['role']:<20} -> {len(regions)} clip(s)")

    json.dump(out_manifest, open(os.path.join(out_root, "manifest.json"), "w"), indent=1, ensure_ascii=False)
    subprocess.run([sys.executable, "-I", os.path.join(HERE, "make_curate.py"),
                    os.path.join(out_root, "manifest.json"), os.path.join(HERE, "review.html"),
                    "processed/", "pet-sound-review-v1", "Processed sound review"], check=True)
    print(f"\n{len(out_manifest)} masters written to processed/")
    for rel, length in long_clips:
        print(f"  long ({length:.1f}s), consider trimming: {rel}")


if __name__ == "__main__":
    main()
