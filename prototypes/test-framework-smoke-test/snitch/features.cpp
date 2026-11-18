// PROTOTYPE (issue #18): one extra test per snitch feature, picked with -DFEATURE_<NAME>.
// snitch-costs.sh builds each one and compares it with FEATURE_PLAIN (one extra plain test),
// so the cost of the extra test itself cancels out. Inputs are volatile, so nothing folds.
#include <snitch/snitch.hpp>

namespace {
volatile float measured_deg = 3.0f;
volatile float limit_deg = 5.0f;
volatile int count = 3;
volatile int max_count = 5;
constexpr float corrected(float deg) { return deg - 1.0f; }
} // namespace

#if defined(FEATURE_PLAIN)
TEST_CASE("extra test") { CHECK(measured_deg <= limit_deg); }
#elif defined(FEATURE_INT_VALUES)
TEST_CASE("extra test") { CHECK(count <= max_count); }
#elif defined(FEATURE_TAGS)
TEST_CASE("extra test", "[tilt][fast]") { CHECK(measured_deg <= limit_deg); }
#elif defined(FEATURE_SECTION)
TEST_CASE("extra test") {
    SECTION("first") { CHECK(measured_deg <= limit_deg); }
    SECTION("second") { CHECK(measured_deg <= limit_deg); }
}
#elif defined(FEATURE_CAPTURE)
TEST_CASE("extra test") {
    const float tilt_deg = measured_deg;
    CAPTURE(tilt_deg);
    CHECK(measured_deg <= limit_deg);
}
#elif defined(FEATURE_INFO)
TEST_CASE("extra test") {
    INFO("tilt limit is ", static_cast<float>(limit_deg), " deg");
    CHECK(measured_deg <= limit_deg);
}
#elif defined(FEATURE_CHECK_THAT)
TEST_CASE("extra test") {
    std::string_view status = count > 0 ? "tilt within limit" : "fault";
    CHECK_THAT(status, snitch::matchers::contains_substring{"tilt"});
}
#elif defined(FEATURE_CHECK_FALSE)
TEST_CASE("extra test") { CHECK_FALSE(measured_deg > limit_deg); }
#elif defined(FEATURE_REQUIRE)
TEST_CASE("extra test") { REQUIRE(measured_deg <= limit_deg); }
#elif defined(FEATURE_FAIL_CHECK)
TEST_CASE("extra test") {
    if (measured_deg > limit_deg) FAIL_CHECK("tilt over limit");
}
#elif defined(FEATURE_SKIP_CHECK)
TEST_CASE("extra test") {
    if (count > 100) SKIP_CHECK("not on this board");
    CHECK(measured_deg <= limit_deg);
}
#elif defined(FEATURE_TEMPLATE_TEST_CASE)
TEMPLATE_TEST_CASE("extra test", "[template]", int, float) {
    TestType value = static_cast<TestType>(count);
    CHECK(value <= static_cast<TestType>(max_count));
}
#elif defined(FEATURE_TEST_CASE_METHOD)
struct tilt_fixture {
    float limit = limit_deg;
};
TEST_CASE_METHOD(tilt_fixture, "extra test") { CHECK(measured_deg <= limit); }
#elif defined(FEATURE_CONSTEXPR_CHECK)
TEST_CASE("extra test") { CONSTEXPR_CHECK(corrected(4.0f) <= 5.0f); }
#endif
