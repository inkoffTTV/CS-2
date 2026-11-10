#include "third_party/doctest.h"
#include "Vec3.h"

using namespace bsim;

TEST_CASE("Vec3 basic math")
{
    CHECK(Length(Vec3{3, 4, 0}) == doctest::Approx(5.f));
    const Vec3 Zero = Normalized(Vec3{0, 0, 0});
    CHECK(Zero.X == 0.f);
    CHECK(Zero.Y == 0.f);
    CHECK(Zero.Z == 0.f);
    CHECK(Lerp(Vec3{0, 0, 0}, Vec3{10, 0, 0}, 0.25f).X == doctest::Approx(2.5f));
    CHECK(Flatten(Vec3{1, 2, 3}).Z == 0.f);
    CHECK(Dot(Vec3{1, 0, 0}, Vec3{0, 1, 0}) == 0.f);
}
