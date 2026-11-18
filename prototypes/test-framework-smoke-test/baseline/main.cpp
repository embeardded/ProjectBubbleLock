// PROTOTYPE (issue #18): baseline image with no test framework.
#include "board.hpp"
#include "test_framework.hpp"

namespace {
baseline_test tests[4];
int test_count = 0;
} // namespace

void baseline_register(baseline_test test) noexcept { tests[test_count++] = test; }

int main() {
    uart_init();
    uart_write("baseline: running tests without a framework\n");
    for (int i = 0; i < test_count; ++i) tests[i]();
    finish(0);
}
