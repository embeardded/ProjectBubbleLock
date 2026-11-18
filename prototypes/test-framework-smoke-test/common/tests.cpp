// PROTOTYPE (issue #18): the same two tests, compiled unchanged against each candidate.
// "test_framework.hpp" resolves to baseline/, snitch/ or own/ depending on the build.
#include "test_framework.hpp"

namespace {
// Stand-in for a domain calculation. Not constexpr, so values show up at run time.
[[gnu::noinline]] float tilt_after_correction_deg(float measured_deg, float correction_deg) {
    return measured_deg - correction_deg;
}
} // namespace

TEST_CASE("tilt stays within the tilt limit after correction") {
    const float tilt_limit_deg = 5.0f;
    const float tilt_deg = tilt_after_correction_deg(12.0f, 9.5f);
    CHECK(tilt_deg <= tilt_limit_deg);
}

TEST_CASE("deliberate failure: tilt exceeds the tilt limit") {
    const float tilt_limit_deg = 5.0f;
    const float tilt_deg = tilt_after_correction_deg(12.0f, 4.5f);
    CHECK(tilt_deg <= tilt_limit_deg);
}
