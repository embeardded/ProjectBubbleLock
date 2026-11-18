// PROTOTYPE (issue #18): snitch entry point on bare metal.
#include "board.hpp"

#include <snitch/snitch.hpp>

int main() {
    uart_init();
    // Must be set before any other snitch call (README, "Freestanding").
    snitch::cli::console_print = &uart_write;
    // Not in the README: the registry keeps its own print callback, which defaults to
    // stdout_print (= std::terminate when freestanding). snitch::main() resets it through
    // configure(argv); we have no argv, so set it directly.
    snitch::tests.print_callback = &uart_write;
    snitch::tests.with_color = false;
    snitch::tests.verbose = snitch::registry::verbosity::high; // print each test's name
    const bool passed = snitch::tests.run_tests("emulated-tests");
    finish(passed ? 0 : 1);
}
