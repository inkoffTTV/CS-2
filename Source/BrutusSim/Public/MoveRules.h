#pragma once

#include "BrutusSimApi.h"
#include "Vec3.h"

namespace bsim
{
// Starting tuning values for hero movement (cm, cm/s, cm/s^2).
struct MoveConfig
{
    float DashSpeed = 1500.f;
    int AirJumps = 1;
    float SlideMinStartSpeed = 500.f;
    float SlideStartBoost = 250.f;
    float SlideDecel = 400.f;
    float SlideMaxSpeed = 1800.f;
    float SlideExitSpeed = 300.f;
    float Gravity = 980.f;
};

// Horizontal dash velocity along WishDir, or along Facing when there is no input.
BRUTUSSIM_API Vec3 ComputeDashVelocity(const Vec3& WishDir, const Vec3& Facing, const MoveConfig& Config);

struct AirJumpState
{
    int Used = 0;
};

BRUTUSSIM_API bool TryAirJump(AirJumpState& State, const MoveConfig& Config);
BRUTUSSIM_API void ResetAirJumps(AirJumpState& State);

BRUTUSSIM_API bool CanStartSlide(float HorizontalSpeed, bool bGrounded, const MoveConfig& Config);

// Velocity right after entering a slide: boost along horizontal travel, clamped to SlideMaxSpeed.
BRUTUSSIM_API Vec3 StartSlideVelocity(const Vec3& Velocity, const MoveConfig& Config);

// One slide step: velocity is turned into the floor plane (keeping its speed), gains gravity
// along the downhill direction, loses SlideDecel, and is clamped to [0, SlideMaxSpeed].
BRUTUSSIM_API Vec3 StepSlide(const Vec3& Velocity, const Vec3& FloorNormal, float Dt, const MoveConfig& Config);

BRUTUSSIM_API bool ShouldExitSlide(const Vec3& Velocity, const MoveConfig& Config);
} // namespace bsim
