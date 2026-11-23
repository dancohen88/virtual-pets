# Virtual Pets: design

Record of the design decisions for two virtual pets on M5Stack Core2 hardware. The design interview is complete (2026-11-23). Remaining details are settled inside the build stage that needs them, and recorded here when they are.

_Last updated: 2026-11-23_

## The project

- **What:** a care-sim virtual pet running on the **M5Stack Core2 v1.3** (ESP32, 320×240 capacitive touchscreen, speaker, vibration motor, BMI270 IMU, battery-backed RTC).
- **Why:** a learning project with two real players. Both expect the pets to have **personality**.
- **Target:** a polished launch build by late December; adults and more content in a first update in January.
- **Hardware:** due ~2026-12-05. Until then, everything is built in a desktop simulator.

## Settled decisions

### Platform & tooling
- **C++ / Arduino framework / PlatformIO / M5Unified** (M5GFX for graphics).
- **Desktop simulator first:** M5GFX renders to an SDL2 window via a PlatformIO `native` env (proven by M5Stack's `lv_m5_emulator`). On Apple Silicon: Homebrew `sdl2` + `pkg-config`, arm64 build flag, `/opt/homebrew` include/lib paths.
- **Hardware abstraction boundary:** game logic never calls M5 APIs directly; it goes through small `Input` / `Display` / `Sound` / `Motion` interfaces with simulator and device implementations.
- To verify on first flash: M5Unified's IMU auto-detect with v1.3's BMI270 (README only names Core2 / Core2 v1.1). Don't stack v1.3 on M5 "Base" bottoms (vibration motor interferes).

### Devices & pets
- **Two devices, one pet each**, same firmware. **Per-pet asset packs:** each device stores only its own pet's sprites and voice, plus a small **visitor pack** of the other pet for when they meet.
- **Player 1: the sausage dog, cheeky.**
- **Player 2: the husky, boisterous.** Stylised to match the sausage dog.
- **Both hatch from an egg.**

### Core loop
- **Care sim**, played through **short check-ins**. No mini-games at launch (backlog).
- **Needs:** hunger, affection, energy, hygiene, plus hidden derived **health**. Too little *and* too much both have outcomes, many funny:

  | Need | Too little | Too much |
  |---|---|---|
  | Hunger | thin, grumpy | plump → round |
  | Affection | sulks, turns its back | clingy, follows your finger |
  | Energy | yawns, dozes off mid-visit | hyper, zooms about |
  | Hygiene | smelly cloud → sick | — |
  | *Health* (hidden) | sick after long neglect; runs away if prolonged | — |

- **Branching growth:** how the pet is raised decides what it becomes (egg → baby → teen → adult), with humour built in.
- **Stakes:** sick, sulking, running away, all possible depending on care. **No permanent death.**
- **Recovery takes effort over a day or two, never one tap:** sulk → turns back after a few affectionate visits; sick → medicine + rest across a sleep cycle; run away → leaves a note, put out favourite food, returns sheepish.
- **Personality:** fixed core traits per pet + care-driven moods, expressed through idle behaviours and short **thought bubbles** (weighted random selection). Separate tone per pet.
- **Hardware → actions:** feed = **tap the bowl** → kibble pours → the pet eats *in character* (sausage dog inhales it; husky spills it); **treats are flicked** up the screen and caught or comically missed. Meals fill hunger; treats give happiness and are what make it plump. Affection = stroke on screen (head pat vs belly rub react differently); sleep = rock the device gently; bath = rub to scrub, then **shake the device and the dog shakes itself dry**; play = tilt to roll a ball it chases; medicine = tap. Vibration for thumps, tail-wag rumbles, sneezes.
- **Needs visibility:** behaviour is the main signal; small icon meters in a top strip appear on tap and hide after a few seconds. Health stays hidden.
- **Controls:** the Core2's three touch buttons under the screen. Middle (B) opens an icon strip of actions (feed, treat, bath, sleep, medicine, shop); left/right (A/C) are back/next. Everything else is direct touch on the pet.
- **Gestures: one gesture, context decides, no hidden modes.** Shake while wet = dry off; shake the egg = dizzy wobble (harmless); shake when asleep = wakes grumpy; shake when awake = dizzy and indignant.
- **Evolution tree:** 1 baby, 1 teen (plump via shape key), **3 adults per pet** picked by dominant care pattern: balanced → best self; spoiled → round and lazy; scruffy/neglected → lovable rebel. No adult is a punishment. **Care stats are recorded from day one**; adult art lands in the first update.
- **Shop (must-ship, smallest form):** a simple item list with prices and a buy button; no shopkeeper yet.
  - **Currency:** **coins**, earned from a daily care allowance (more for balanced care, never taken away) **with randomness**, plus **bones** the pet digs up and brings you (worth a random number of coins). No real money, ever.
  - **Launch stock:** 3 treats (each pet has a secret favourite), 2 toys (squeaky toy; rope for tug-of-war by dragging; the ball is free), 2 home skins per pet. More stock comes with updates.
  - **Prices:** treats cheap (a day's care buys a few, used up); toys a few days (permanent); skins about a week of good care (permanent, the big goal).
- **Awards book:** **stickers** from toy play milestones (first squeak, 50 squeaks, won tug-of-war), care firsts and streaks (first bath, a week of breakfasts) and funny moments (caught 20 treats in a row, got plump, shook dry right next to you). ~24 at launch; some **secret** (silhouette until earned). A **completed page unlocks an exclusive item** you can't buy.
- **Ignoring an alert costs nothing extra**; needs just keep draining.
- **Egg care:** the egg must be kept **stable** and **stroked to keep it warm**. Its "down" follows real gravity (IMU): tilt and it leans; past ~30° it topples and rolls to the edge; tap or tilt to roll it upright. Toppled = cools twice as fast; never damaged.
- **Egg temperature** is shown **on the egg** (frost creeping in when cold; rosy glow + gentle heartbeat wobble when warm) plus a small thermometer (**diegetic UI**).
- **Hatching:** with the egg's needs met, a **crack appears after ~8 hours**; it **hatches the next day after ~15 attention minutes**: cumulative time with the screen on and the egg touched/handled in the last ~10 s, across any number of visits; extra cracks at 5 / 10 / 15 min. The hatch always happens in front of the player.
- **Growth after hatching** (baseline: baby ~3 days, teen ~2 weeks, adult ~4 weeks; held at teen until adult art exists) must **feel dynamic**: **growth points** from good care plus milestones (meals, baths, nights slept, play), within a window (e.g. teen in 10–18 days); telltale signs a day before (restless, appetite spikes); transitions happen in front of the player; the wait for adult art is disguised as a "lazy teen phase".

### Time
- **Real time with catch-up:** on wake, elapsed time is read from the RTC and the sim catches up.
- **Away detection:** a **parent-set schedule** (school hours, bedtime) **plus face-down = nap** (gyro).
- **Device mostly in timer sleep**, woken by the RTC alarm; the pet may **alert** (chirp/buzz) only within the schedule.
- **Alert frequency:** between "rare" and "persistent", **adjustable**.
- **Parent menu** on-device (hidden gesture + PIN): weekday/weekend profiles, holiday switch.
- **Clock:** NTP sync over home Wi-Fi from the parent menu, manual fallback; logic uses **monotonic time** (clock going backwards = zero elapsed).

### Sound
- **Tooling:** ffmpeg (Homebrew). A **Freesound** API key (kept outside the repo at `~/.config/freesound/api_key`) is approved for filling gaps.
- **Dog voices:** barks, growls, whines and the like, plus obvious sounds (eating, snoring, splashing…).
- **Sourcing:** open-licence recordings, curated per pet. Candidates gathered by [`sounds/fetch_sounds.py`](../sounds/fetch_sounds.py) into `sounds/raw/` (BigSoundBank + Wikimedia Commons). Curate in `sounds/curate.html` (generated by [`sounds/make_curate.py`](../sounds/make_curate.py)): pick pet, role, trim points; export JSON.
- **Curation round 1:** 30 clips picked, saved in [`sounds/curation.json`](../sounds/curation.json): sausage dog 6 (barks, growl-barks, sneeze, splash), husky 8 (talk/howl ×2, barks, grumpy bark, whine, panting, yawn), shared 16 (kibble, crunch, lick, sniff, snore loop, squeaky toys, egg cracks, bath water/bubbles, puppy whine, "2 dogs barking" for when the pets meet). Picks went by character, not breed.
- **Processing:** [`sounds/process_sounds.py`](../sounds/process_sounds.py) regenerates `sounds/processed/` from `curation.json`: mono 16-bit 44.1 kHz WAV masters, cut to the selection, dead air trimmed, loudness-matched (−20 dB mean, limiter holding peaks at −1 dB), click-free fades; bark recordings over 4 s auto-split into single barks. 30 picks → 67 masters. Review in `sounds/review.html`. Device-format export comes later; **baby/teen pitch is done at playback by raising the sample rate**, so no pitched copies are stored.
- **Approved:** the processed set as it stands, including the 23-way split of "2 dogs barking".
- **Optional later trims:** snore (65 s), squeaky toy #2 (16 s), bath water (19 s), bath bubbles (18 s), puppy whine (47 s).
- **Gaps:** sausage dog has no whine, yawn, panting or happy yip of its own (could borrow, pitched); husky has no growl (Commons large-dog growl is a candidate) and no long howl; nothing yet for drinking/lapping, the bath shake-off, burp, tummy rumble or hiccup (the overfeeding jokes).

### Art
- **Blender, pre-rendered to 2D sprites**; no real-time 3D on device.
- **Division of labour:** Claude scripts all modelling, rigging, rendering and conversion via headless Blender (`bpy`); the builder art-directs (moodboards, render feedback, approval). Fallback: the builder hand-sculpts a face if a pet isn't landing.
- **Moodboards in PureRef**, one per pet, four sections: shape & proportion, material & colour, faces & expressions, personality & humour. Material and faces shared so the pets feel like one world. Brief: [`docs/moodboard-brief.md`](moodboard-brief.md).
- Premium plush toys are a reference for *qualities* (floppy, squashy, fabric-soft, deadpan faces); the creatures themselves are original designs.
- **View:** ¾ view angled toward the player, head can turn to look at you; each pet has its own **home** background. **Static images for now**, maybe a few more later.
- **Animation:** body frames rendered from Blender, **face as swappable layers** (face region rendered per expression for the same frames, composited on device), procedural squash/stretch and gyro wobble on top.
- **Launch animation list per stage:** idle (breathe, blink, tail wag), walk, eat, sleep, happy bounce, being stroked, sulk, sick, bath shake, plump idle; plus shared egg wobble/crack/hatch.
- **Character over smoothness → limited animation:** strong key poses, holds, 8–12 fps, exaggerated squash & stretch and anticipation.
- **Style chosen: cel-shaded with ink outlines** (toon shading in EEVEE, inverted-hull outlines). Flat colour bands also compress well for flash storage. Exploration in [`art/prototype/style_tests.py`](../art/prototype/style_tests.py).

## Build plan (ordered stages; dates don't matter)

1. **Foundations:** simulator project, core simulation with tests, time-acceleration debug mode, moodboards, base models of both dogs.
2. **Egg & hatch** in the simulator.
3. **Sprite pipeline + baby art** (Blender → device files, baby animations).
4. **Hardware bring-up** whenever the boards land, alongside the current stage (touch, IMU, speaker, RTC, sleep).
5. **Baby loop on device:** needs, feeding/treats, stroking, sleep, bath + shake-dry, sounds.
6. **Daily life:** schedule, alerts, parent menu, battery.
7. **Shop, currency & awards book.**
8. **Teen & growth system.**
9. **Finish:** second device, soak test with accelerated time, feature freeze, wrap.

After launch: adults, pets meeting, mini-games, more shop stock.

**Field guides:** each stage ends with a field guide for the builder: an interactive page styled as a naturalist's notebook about "the creatures we're building". Sections: what we built, named ideas, a diagram, things to try, and a **GitHub corner** teaching one skill per stage. Each guide has its own stickers to earn.

**Firmware updates:** USB re-flash for launch; over-the-air updates over home Wi-Fi from GitHub Releases from the first update on (a public repo means no login is needed). A failed download must never brick a device.

## Repo, workflow & licences

- **Public GitHub repo**, created in Stage 1, with a neutral name and README; presentable enough to share.
- **Personal context stays out of the repo:** committed files refer to "player 1 / player 2", "the sausage dog / the husky". Anything personal lives in the gitignored `docs/private.md`.
- **Commit sources, not build outputs:** scripts, curation picks, Blender sources committed; anything a script regenerates (processed sounds, renders) gitignored; no Git LFS; finished device asset packs and firmware go on **GitHub Releases**.
- **Third-party sounds:** only clips whose licence allows redistribution are committed (CC0, public domain, CC BY / BY-SA, BigSoundBank's licence, checked 2026-11-23), all credited in [`CREDITS.md`](../CREDITS.md). The fetch script + manifest can recreate any that go missing.
- **Licences:** **MIT** for code; **CC BY-NC 4.0** for original art (pets, homes, stickers); third-party sounds keep their own licences.
- **A branch + pull request per build stage**; merging closes the stage and links its field guide. Small commits within the branch.
- **CI from Stage 1:** GitHub Actions runs host-side tests and builds both pets' firmware on every push; a release workflow attaches per-pet firmware.
- **Tracking:** a GitHub **milestone per stage**, **issues** for tasks and the backlog. This doc stays the source for design decisions.

## Prototype

[`art/prototype/plush_pup.py`](../art/prototype/plush_pup.py) is a plush sausage-dog-inspired pup built entirely from code, proving the scripted-Blender pipeline:

```sh
blender --background --factory-startup --python art/prototype/plush_pup.py
```

Outputs in `art/prototype/renders/`: hero sprite (transparent + soft shadow), normal-vs-plump comparison (one **shape key**), exact 320×240 Core2-size render (+3× blow-up), and an editable `.blend`.

## Decided inside their stages

- Sprite size and on-screen layout (Stage 3).
- Storage format and flash budget (Stage 3).
- Adult designs per pet (after moodboards; first update).
- Palette (after moodboards).
- Shop and awards-book screens; full sticker list (Stage 7).

## Project backlog (after launch)

- **Mini-games** using gyro/touch (tilt maze, catch, rhythm with the speaker).
- **Pets meeting across devices** (ESP-NOW): v1 = visits + item swaps + a few reactions; later play together, rivalry, friendship meter. Design for it now (pet IDs, tradeable items).
