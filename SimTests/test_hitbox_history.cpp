#include "third_party/doctest.h"
#include "HitboxHistory.h"

using namespace bsim;

static std::vector<Capsule> At(float X)
{
    return {Capsule{{X, 0, 0}, {X, 0, 100}, 30.f, HitZone::Body}};
}

TEST_CASE("SampleAt interpolates between snapshots")
{
    HitboxHistory H;
    H.Record(0.0, At(0));
    H.Record(1.0, At(100));
    const auto S = H.SampleAt(0.5);
    REQUIRE(S.has_value());
    CHECK((*S)[0].A.X == doctest::Approx(50.f));
    CHECK((*S)[0].B.Z == doctest::Approx(100.f));
}

TEST_CASE("SampleAt older than history is a miss, newer than history is the latest")
{
    HitboxHistory H;
    H.Record(0.0, At(0));
    H.Record(1.0, At(100));
    CHECK_FALSE(H.SampleAt(-0.1).has_value());
    const auto S = H.SampleAt(2.0);
    REQUIRE(S.has_value());
    CHECK((*S)[0].A.X == doctest::Approx(100.f));
}

TEST_CASE("Teleport snapshots are never interpolated across")
{
    HitboxHistory H;
    H.Record(0.0, At(0));
    H.Record(1.0, At(5000), /*bTeleported=*/true);
    const auto Before = H.SampleAt(0.99);
    REQUIRE(Before.has_value());
    CHECK((*Before)[0].A.X == doctest::Approx(0.f));
    const auto After = H.SampleAt(1.0);
    REQUIRE(After.has_value());
    CHECK((*After)[0].A.X == doctest::Approx(5000.f));
}

TEST_CASE("Prune drops snapshots older than MaxAge")
{
    HitboxHistory H(3.0);
    for (int T = 0; T <= 5; ++T)
    {
        H.Record(T, At(static_cast<float>(T)));
    }
    H.Prune(5.0);
    CHECK(H.Num() == 4);
    CHECK_FALSE(H.SampleAt(1.5).has_value());
}

TEST_CASE("Out-of-order records are ignored")
{
    HitboxHistory H;
    H.Record(1.0, At(0));
    H.Record(0.5, At(10));
    CHECK(H.Num() == 1);
}

TEST_CASE("Different shape counts fall back to the earlier snapshot")
{
    HitboxHistory H;
    H.Record(0.0, At(0));
    auto Two = At(100);
    Two.push_back(Capsule{{100, 0, 150}, {100, 0, 150}, 15.f, HitZone::Head});
    H.Record(1.0, Two);
    const auto S = H.SampleAt(0.5);
    REQUIRE(S.has_value());
    CHECK(S->size() == 1);
    CHECK((*S)[0].A.X == doctest::Approx(0.f));
}
