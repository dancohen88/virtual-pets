# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Two virtual pets (a cheeky sausage dog, a boisterous husky) on M5Stack Core2 handhelds: C++ firmware with a desktop simulator, Blender-scripted cel-shaded art, curated open-licence sounds.

## Before you change anything

- **Game behaviour or design question** → read [`docs/design.md`](docs/design.md) first; it is the single source of truth. Record any new decision there in the same change.
- **Which stage are we in** → the build plan in `docs/design.md`. One branch + pull request per stage (`stage-N-name`), small commits on the branch.
- **Art direction** → [`docs/moodboard-brief.md`](docs/moodboard-brief.md); moodboards live in the gitignored `art/moodboards/`.

## Public repo: privacy guardrail

The repo is public. Committed files describe the players neutrally: "player 1 / player 2", "the sausage dog / the husky", "launch" and "first update". Personal context (who the pets are for, ages, the real occasion) lives only in the gitignored `docs/private.md`; read it for context, never copy from it into a committed file, commit message or PR. Commits use the repo-local GitHub noreply email already configured.

## Commands

| Area | Command (run from) |
|---|---|
| Host tests | `pio test -e native` (`firmware/`); one suite: `-f test_catchup` |
| Simulator | `pio run -e sim -t exec` (`firmware/`); keys in `firmware/README.md` |
| Device | `pio run -e core2`, flash with `-t upload` (`firmware/`) |
| Pet models | `blender --background --factory-startup --python art/models/<pet>.py` (repo root) |
| Sounds | `python3 process_sounds.py` (`sounds/`); restore missing raw clips: `python3 fetch_sounds.py` |

## Architecture

**Firmware** (`firmware/`, PlatformIO envs `native` / `sim` / `core2`):
- `lib/petcore/` is the pure C++ game core. It never includes Arduino or M5 headers; it talks to hardware only through the interfaces in `petcore/Platform.h` and `petcore/Clock.h` (headers are included as `<petcore/...>`). This hardware abstraction boundary is what lets the same game run in `native` tests, the SDL simulator and on the device.
- `src/m5/` holds the M5Unified-backed platform, shared by `sim` and `core2`; `src/sim/` and `src/core2/` are thin entry points.
- Every gameplay number lives in `lib/petcore/src/petcore/Tuning.h`.
- `Simulation` advances in fixed one-minute steps with carried remainders, so catch-up is deterministic however elapsed time is split; the catch-up tests compare bit-for-bit, so keep stepping deterministic. Elapsed time is monotonic (a clock going backwards counts as zero).

**Art** (`art/`): scripts are the source; `renders/` folders are gitignored outputs. `art/lib/toon.py` is the shared toolkit (cel ramp, inverted-hull outlines, shape keys). `art/prototype/pup_model.py` is an older duplicate kept for the prototype scripts. EEVEE gotchas already solved in `toon.py`: keep materials opaque (EEVEE dithers semi-transparency into speckle), give outline shells `visible_shadow = False`, reset shape keys to 0 after creating them, use the Standard view transform.

**Sounds** (`sounds/`): `curation.json` is the source of truth; `process_sounds.py` regenerates `processed/` and `review.html` from it. ffmpeg 9 gotchas: trim silence with `silencedetect` timings (the `silenceremove` filter's options changed in 6.1 and empty the file), and read the *last* `volumedetect` values (ffmpeg prints a throwaway `n_samples: 0` instance first). Baby/teen pitch is applied at playback by raising the sample rate, so no pitched copies are stored.

## Licences and credits

Code is MIT, original art CC BY-NC 4.0 (`LICENSE-ART.md`). Every committed third-party clip is credited in `CREDITS.md`; when adding a clip, add its credit in the same commit. BigSoundBank requires the exact line "Additional sounds: Joseph SARDIN - BigSoundBank.com", which must also appear on the device's credits screen.

## Working style

The builder is learning, GitHub especially: name the concept behind non-obvious git/GitHub or engineering steps in a sentence. Each stage ends with a field guide (an interactive page) covering what was built and one GitHub skill.
