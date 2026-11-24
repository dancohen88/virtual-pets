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
│       ├── Life.*        the whole life on one time line: Egg -> Hatching -> Baby
│       ├── Egg.*         warmth, balance (toppling), incubation, hatch-day attention
│       ├── Simulation.*  the baby's needs: fixed-step simulation, catch-up, hidden health, actions
│       ├── Schedule.*    weekday/weekend routine, school hours, bedtime, holiday switch
│       ├── Clock.h       Clock interface, monotonic ElapsedTracker, AcceleratedClock (debug speed)
│       ├── TimeUtil.*    calendar maths on local time
│       ├── Platform.h    the hardware abstraction boundary: Display, Input, Motion, Sound, Storage
│       └── App.*         placeholder screens: the egg, the hatch, then needs bars + pet blob
├── src/
│   ├── m5/               Display/Input/Sound built on M5Unified (shared by sim and core2)
│   ├── sim/main.cpp      simulator entry point: computer clock, keyboard stand-ins, speed keys
│   └── core2/main.cpp    device entry point: RTC clock, IMU gravity / shake / face-down
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
| Mouse click | Touch: tap the egg (stands a fallen egg up), tap the baby (stroke), or an action in the strip |
| Mouse drag on the egg | Rub it to warm it up |
| `A` / `D` | Tilt the device left / right by 10° (the egg leans; past 30° it falls over) |
| `S` | Level the device again (tilt 0°) |
| `X` | Shake (the egg wobbles dizzily) |
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

## How the egg works

All numbers are in `Tuning.h`; the rules are tested in `test_egg` and `test_hatch`.

- **Warmth** (0..100) starts at 80 and drains 12/hour with the owner around, with the same
  school / night / face-down slow-downs as the needs. Rubbing the egg adds 2 per stroke.
  A **toppled** egg cools twice as fast.
- **Balance:** the lean is the angle of gravity away from the screen's vertical axis (so it
  works held upright or lying flat). Past 30° the egg falls over and rolls to that edge.
  Tilting 15° the other way rolls it back up; so does a tap, as long as the device isn't still
  tilted past 30°. It is never damaged. A shake only makes it wobble.
- **Incubation:** time counts while warmth is at least 50. After 8 hours of warm time the
  **first crack** appears.
- **Hatch day** opens at the first wake-up time after the crack (a crack in the afternoon waits
  for tomorrow morning; one in the small hours opens that same morning).
- **Attention minutes** count only on hatch day, with the screen on, for time within 10 s of
  the egg being touched, stroked or tilted (by 5°+). More cracks at 5 and 10 minutes; it
  **hatches at 15**, so the hatch always happens while someone is playing with it.
- The egg runs in **1-second steps** (the baby's needs use 1-minute steps), still split-invariant,
  so cracks and the hatch land while the player is there. After a 5-second hatching moment the
  baby's needs simulation starts, full.

## Known gaps

- `Storage` is an interface only; the pet's state isn't saved between runs yet.
- On real hardware, still to confirm: which IMU axes map to the screen's right/down (the
  `kSignX` / `kSignY` constants in `src/core2/main.cpp`), the face-down sign (`az < -0.7`),
  the shake threshold (|total − 1 g| > 1 g), and RTC behaviour.
- The device has no screen-off yet, so attention assumes the screen is on (Stage 6 adds sleep).
- The screens are placeholders; real art arrives in Stage 3.
