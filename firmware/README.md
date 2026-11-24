# Firmware

The virtual pet's code: one pure C++ game core that runs in three places.

| Environment | What it is | Command |
|---|---|---|
| `native` | Unit tests for the game core on your computer | `pio test -e native` |
| `sim` | Desktop simulator: the real drawing code in an SDL2 window | `pio run -e sim -t exec` |
| `core2` | The M5Stack Core2 device | `pio run -e core2` (build), `pio run -e core2 -t upload` (flash) |

Run every command from this `firmware/` folder.

## Layout

```
firmware/
├── platformio.ini        the three environments
├── lib/petcore/          the game core — pure C++, no Arduino or M5 code
│   └── src/petcore/
│       ├── Tuning.h      every gameplay number (drain rates, multipliers, step size)
│       ├── Needs.h       hunger, affection, energy, hygiene (0..100, 100 = fully met)
│       ├── Simulation.*  fixed-step simulation, catch-up, hidden health, actions
│       ├── Schedule.*    weekday/weekend routine, school hours, bedtime, holiday switch
│       ├── Clock.h       Clock interface, monotonic ElapsedTracker, AcceleratedClock (debug speed)
│       ├── TimeUtil.*    calendar maths on local time
│       ├── Platform.h    the hardware abstraction boundary: Display, Input, Motion, Sound, Storage
│       └── App.*         Stage 1 placeholder screen (needs bars, pet blob, action strip)
├── src/
│   ├── m5/               Display/Input/Sound built on M5Unified (shared by sim and core2)
│   ├── sim/main.cpp      simulator entry point: computer clock, keyboard stand-ins, speed keys
│   └── core2/main.cpp    device entry point: RTC clock, IMU face-down detection
└── test/                 one folder per test suite (Unity)
```

The **hardware abstraction boundary** is the key idea: the game core only talks to the small
interfaces in `Platform.h` and `Clock.h`. That's why it can be unit-tested on a laptop and run
unchanged in the simulator and on the device.

## Tests

```sh
pio test -e native                        # all suites
pio test -e native -f test_catchup        # one suite (folder name under test/)
pio test -e native -f test_needs -v       # verbose: show each assertion's output
```

## Simulator controls

| Key / input | Does |
|---|---|
| ← ↓ → | Core2 buttons A / B / C (B opens the action strip; A/C move; B again confirms) |
| Mouse click | Touch (click the pet to stroke it, or an action in the strip) |
| `]` / `[` | Faster / slower time: paused, ×1, ×60, ×600, ×3600, ×86400 (one day per second) |
| `J` | Jump forward one hour |
| `F` | Toggle "lying face-down" (the pet naps) |
| `1`–`6`, `R` / `L` | Window zoom and rotation (built into M5GFX's SDL port) |

## Prerequisites

- macOS: `brew install platformio sdl2 pkg-config`
- Linux: PlatformIO plus `libsdl2-dev` and `pkg-config` (`sudo apt install libsdl2-dev pkg-config`)

## How time works

- Need levels are 0..100; rates in `Tuning.h` are per hour of game time.
- The simulation advances in fixed one-minute steps and carries leftovers forward, so catching
  up after days switched off gives exactly the same result as playing straight through
  (`test_catchup` checks this bit-for-bit).
- Times are **local** milliseconds since 1970; the device's RTC is set to local time.
- A clock that goes backwards counts as no time passing (monotonic time).
- Needs drain much more slowly while the owner is at school, at night, or when the device is
  face-down (asymmetric decay).

## Known gaps (Stage 1)

- `Storage` is an interface only; the pet's state isn't saved between runs yet.
- The Core2 face-down check (`az < -0.7`) and RTC behaviour need confirming on real hardware.
- The screen is a placeholder; real art arrives in Stage 3.
