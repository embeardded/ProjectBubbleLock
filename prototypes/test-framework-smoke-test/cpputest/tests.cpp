// PROTOTYPE (issue #18): the same two tests, rewritten in CppUTest syntax.
// CppUTest can't compile common/tests.cpp unchanged:
// - group and test names must be identifiers, not strings;
// - its plain CHECK(a <= b) prints no values, so CHECK_COMPARE is the closest match;
// - every check is fatal: a failure longjmps out of the test body.
#include "CppUTest/TestHarness.h"

namespace {
[[gnu::noinline]] float tilt_after_correction_deg(float measured_deg, float correction_deg) {
    return measured_deg - correction_deg;
}
} // namespace

TEST_GROUP(Tilt){};

TEST(Tilt, StaysWithinTheTiltLimitAfterCorrection) {
    const float tilt_limit_deg = 5.0f;
    const float tilt_deg = tilt_after_correction_deg(12.0f, 9.5f);
    CHECK_COMPARE(tilt_deg, <=, tilt_limit_deg);
}

TEST(Tilt, DeliberateFailureTiltExceedsTheTiltLimit) {
    const float tilt_limit_deg = 5.0f;
    const float tilt_deg = tilt_after_correction_deg(12.0f, 4.5f);
    CHECK_COMPARE(tilt_deg, <=, tilt_limit_deg);
}
