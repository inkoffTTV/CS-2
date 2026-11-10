#include "third_party/doctest.h"
#include "TeamVision.h"

using namespace bsim;

namespace
{
VisionActor Observer() { return VisionActor{1, 0, {0, 0, 0}, 2000.f, true, false}; }
VisionActor Enemy(float X, bool bHidden = false) { return VisionActor{2, 1, {X, 0, 0}, 2000.f, true, bHidden}; }
const LineOfSightFn Clear = [](const VisionActor&, const VisionActor&) { return true; };
const LineOfSightFn Blocked = [](const VisionActor&, const VisionActor&) { return false; };
} // namespace

TEST_CASE("Enemy in sight radius with line of sight is visible")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(1500)}, Clear);
    CHECK(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Enemy behind a wall is not visible")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(1500)}, Blocked);
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Seen enemy stays visible for the short default grace period")
{
    // The net driver keeps a channel open ~1 s longer on its own, so sim grace is short.
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(1500)}, Clear);
    V.Update(0.15, {Observer(), Enemy(1500)}, Blocked);
    CHECK(V.IsVisibleToTeam(2, 0));
    V.Update(0.3, {Observer(), Enemy(1500)}, Blocked);
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Hidden enemy is revealed only up close")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(1500, true)}, Clear);
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
    V.Update(0.1, {Observer(), Enemy(500, true)}, Clear);
    CHECK(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Hidden enemy gets no grace period")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(500, true)}, Clear);
    V.Update(0.1, {Observer(), Enemy(1500, true)}, Clear);
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Enemy beyond sight radius is not visible")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(2500)}, Clear);
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
}

TEST_CASE("Own team is always visible, unknown ids never")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(2500)}, Blocked);
    CHECK(V.IsVisibleToTeam(1, 0));
    CHECK(V.IsVisibleToTeam(2, 1));
    CHECK_FALSE(V.IsVisibleToTeam(99, 0));
}

TEST_CASE("Id missing from Update is dropped and gets no inherited grace on return")
{
    TeamVision V;
    V.Update(0.0, {Observer(), Enemy(1500)}, Clear);
    CHECK(V.IsVisibleToTeam(2, 0));
    V.Update(0.05, {Observer()}, Clear); // enemy left (despawned)
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
    V.Update(0.1, {Observer(), Enemy(1500)}, Blocked); // reappears within the old grace window
    CHECK_FALSE(V.IsVisibleToTeam(2, 0));
}
