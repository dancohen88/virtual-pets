// Core2 v1.3 hardware constants, confirmed by hand on real boards (see issue #3).
// Shared by the game's motion code and the self-test screen, so they can't disagree.
#pragma once

namespace hw {

// IMU axis signs: tilting the device right must lean the egg right. X was mirrored on a
// Core2 v1.3 (+1 leaned the egg the wrong way), so it's -1.
constexpr float kImuSignX = -1.0f;
constexpr float kImuSignY = -1.0f;

// Shake: total acceleration strays this far from 1 g. 0.45 g = a firm, ordinary shake;
// picking it up or tilting stays under ~0.3 g. (1 g needed a hard shake.)
constexpr float kShakeG = 0.45f;
constexpr int kShakeCooldownFrames = 15;  // ~0.5 s at 30 fps, so one shake counts once

// Face-down: screen pointing at the floor. Sign still to confirm (needs the napping baby).
constexpr float kFaceDownZ = -0.7f;

}  // namespace hw
