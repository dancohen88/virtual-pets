"""Re-download any raw sound clips listed in raw/manifest.json that are missing locally.

Usage (from the sounds/ folder):
    python3 fetch_sounds.py

All clips are normally committed; this restores any that are missing (e.g. after pruning
raw/ locally) before running process_sounds.py.

Be polite to the hosts: downloads are spaced out, and Wikimedia rate-limits aggressive clients.
"""
import json
import os
import time
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
UA = {"User-Agent": "VirtualPetsSoundFetcher/1.0 (hobby project; python-urllib)"}
DELAY_SECONDS = 3


def source_url(clip):
    if clip["key"].startswith("bsb-"):
        return f"https://bigsoundbank.com/UPLOAD/mp3/{clip['key'][4:]}.mp3"
    if clip["source"] == "Wikimedia Commons":
        # Special:FilePath redirects to the original upload for a given file title.
        return "https://commons.wikimedia.org/wiki/Special:FilePath/" + urllib.parse.quote(clip["title"])
    raise ValueError(f"don't know how to fetch {clip['key']}")


def main():
    manifest = json.load(open(os.path.join(HERE, "raw", "manifest.json"), encoding="utf-8"))
    fetched = skipped = failed = 0
    for clip in manifest:
        dest = os.path.join(HERE, "raw", clip["file"])
        if os.path.exists(dest) and os.path.getsize(dest) > 0:
            skipped += 1
            continue
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        try:
            req = urllib.request.Request(source_url(clip), headers=UA)
            with urllib.request.urlopen(req, timeout=60) as r, open(dest, "wb") as f:
                f.write(r.read())
            fetched += 1
            print("ok     ", clip["file"])
        except Exception as e:  # keep going; report at the end
            failed += 1
            print("FAILED ", clip["file"], "-", e)
        time.sleep(DELAY_SECONDS)
    print(f"\n{fetched} fetched, {skipped} already present, {failed} failed")


if __name__ == "__main__":
    main()
