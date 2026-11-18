#pragma once
// PROTOTYPE (issue #18): "no framework" stand-in. The test bodies still compile and run
// (so their code counts in the baseline), but nothing is reported.
using baseline_test = void (*)();
void baseline_register(baseline_test test) noexcept;

#define BASELINE_CAT2(a, b) a##b
#define BASELINE_CAT(a, b) BASELINE_CAT2(a, b)
#define BASELINE_TEST(fn)                                                                          \
    static void fn();                                                                              \
    static const int BASELINE_CAT(fn, _registered) = (baseline_register(&fn), 0);                  \
    static void fn()
#define TEST_CASE(name) BASELINE_TEST(BASELINE_CAT(baseline_test_, __COUNTER__))
#define CHECK(...)                                                                                 \
    do {                                                                                           \
        volatile bool result = (__VA_ARGS__);                                                      \
        (void)result;                                                                              \
    } while (false)
