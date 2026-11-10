#include "MoveRules.h"

#include <algorithm>

namespace bsim
{
namespace
{
Vec3 WithSpeed(const Vec3& Direction, float Speed)
{
    return Normalized(Direction) * Speed;
}
} // namespace

Vec3 ComputeDashVelocity(const Vec3& WishDir, const Vec3& Facing, const MoveConfig& Config)
{
    Vec3 Dir = Normalized(Flatten(WishDir));
    if (LengthSq(Dir) < 1e-6f)
    {
        Dir = Normalized(Flatten(Facing));
    }
    return Dir * Config.DashSpeed;
}

bool TryAirJump(AirJumpState& State, const MoveConfig& Config)
{
    if (State.Used >= Config.AirJumps)
    {
        return false;
    }
    ++State.Used;
    return true;
}

void ResetAirJumps(AirJumpState& State)
{
    State.Used = 0;
}

bool CanStartSlide(float HorizontalSpeed, bool bGrounded, const MoveConfig& Config)
{
    return bGrounded && HorizontalSpeed >= Config.SlideMinStartSpeed;
}

Vec3 StartSlideVelocity(const Vec3& Velocity, const MoveConfig& Config)
{
    const Vec3 Boosted = Velocity + Normalized(Flatten(Velocity)) * Config.SlideStartBoost;
    const float Speed = Length(Boosted);
    return Speed > Config.SlideMaxSpeed ? WithSpeed(Boosted, Config.SlideMaxSpeed) : Boosted;
}

Vec3 StepSlide(const Vec3& Velocity, const Vec3& FloorNormal, float Dt, const MoveConfig& Config)
{
    const Vec3 N = Normalized(FloorNormal);
    const float Speed = Length(Velocity);

    // Redirect into the floor plane without losing speed.
    Vec3 V = WithSpeed(Velocity - N * Dot(Velocity, N), Speed);

    // Gravity along the slope: |projection of down onto the plane| equals sin(slope angle).
    const Vec3 Down{0.f, 0.f, -1.f};
    const Vec3 Downhill = Down - N * Dot(Down, N);
    V = V + Downhill * (Config.Gravity * Dt);

    const float NewSpeed = std::clamp(Length(V) - Config.SlideDecel * Dt, 0.f, Config.SlideMaxSpeed);
    return WithSpeed(V, NewSpeed);
}

bool ShouldExitSlide(const Vec3& Velocity, const MoveConfig& Config)
{
    return Length(Velocity) < Config.SlideExitSpeed;
}
} // namespace bsim
