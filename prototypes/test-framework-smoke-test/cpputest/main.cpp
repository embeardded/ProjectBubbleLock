// PROTOTYPE (issue #18): CppUTest entry point on bare metal. CommandLineTestRunner is
// skipped: it news up its output objects and parses argv, and we have no argv.
#include "CppUTest/TestOutput.h"
#include "CppUTest/TestRegistry.h"
#include "CppUTest/TestResult.h"

#include "board.hpp"

std::size_t cpputest_pool_high_water() noexcept;

int main() {
    uart_init();
    ConsoleTestOutput output;
    output.verbose(TestOutput::level_verbose); // print each test's name
    TestResult result(output);
    TestRegistry::getCurrentRegistry()->runAllTests(result);
    uart_write("allocator pool high-water: ");
    uart_write_uint(cpputest_pool_high_water());
    uart_write(" bytes\n");
    finish(static_cast<int>(result.getFailureCount()));
}
