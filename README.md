# Virtual Pets

Two virtual pets, a cheeky sausage dog and a boisterous husky, living on [M5Stack Core2](https://docs.m5stack.com/en/core/Core2_v1.3) handhelds. Each hatches from an egg, needs feeding, stroking, bathing and sleep, and grows up differently depending on how it's cared for. Built as a learning project.

- **Hardware:** M5Stack Core2 v1.3 (ESP32, 320×240 touchscreen, speaker, vibration motor, IMU, real-time clock).
- **Firmware:** C++ with PlatformIO and M5Unified, plus a desktop simulator so the game can be built and tested without a device.
- **Art:** scripted in Blender, cel-shaded, pre-rendered to sprites.
- **Sound:** open-licence recordings, curated and processed with FFmpeg.

## Repository layout

| Folder | What's there |
|---|---|
| [`docs/`](docs/) | [Design decisions](docs/design.md) and the [moodboard brief](docs/moodboard-brief.md) |
| `firmware/` | PlatformIO project: game core, simulator, device build (arrives in Stage 1) |
| [`art/`](art/) | Blender scripts that build and render the pets |
| [`sounds/`](sounds/) | Sound fetching, curation and processing scripts |

## Status

Built in nine stages, each ending with a pull request and a field guide. See the [build plan](docs/design.md#build-plan-ordered-stages-dates-dont-matter).

## Licences

Code: [MIT](LICENSE). Original art: [CC BY-NC 4.0](LICENSE-ART.md). Third-party sounds: see [CREDITS](CREDITS.md).
