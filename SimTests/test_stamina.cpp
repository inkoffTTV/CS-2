#include "third_party/doctest.h"
#include "Stamina.h"

using namespace bsim;

TEST_CASE("Full stamina allows exactly MaxCharges consumptions and never goes negative")
{
    StaminaConfig Cfg;
    StaminaState S;
    CHECK(TryConsume(S, Cfg));
    CHECK(TryConsume(S, Cfg));
    CHECK(TryConsume(S, Cfg));
    CHECK_FALSE(TryConsume(S, Cfg));
    CHECK(S.Charges == doctest::Approx(0.f));
}

TEST_CASE("Regen restores one charge per RegenSecondsPerCharge")
{
    StaminaConfig Cfg;
    StaminaState S{0.f};
    Regen(S, Cfg, 2.5f);
    CHECK(S.Charges == doctest::Approx(1.f));
}

TEST_CASE("Partial regen is not enough to spend a charge")
{
    StaminaConfig Cfg;
    StaminaState S{0.f};
    Regen(S, Cfg, 1.25f);
    CHECK(S.Charges == doctest::Approx(0.5f));
    CHECK_FALSE(TryConsume(S, Cfg));
    CHECK(S.Charges == doctest::Approx(0.5f));
}

TEST_CASE("Regen clamps at MaxCharges")
{
    StaminaConfig Cfg;
    StaminaState S{0.f};
    Regen(S, Cfg, 100.f);
    CHECK(S.Charges == doctest::Approx(3.f));
}

TEST_CASE("Multi-charge cost fails without side effects when short")
{
    StaminaConfig Cfg;
    StaminaState S{1.9f};
    CHECK_FALSE(TryConsume(S, Cfg, 2));
    CHECK(S.Charges == doctest::Approx(1.9f));
}
