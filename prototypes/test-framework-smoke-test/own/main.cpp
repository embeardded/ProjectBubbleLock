// PROTOTYPE (issue #18): own runner entry point on bare metal.
#include "board.hpp"
#include "minitest.hpp"

int main() {
    uart_init();
    minitest::set_output(&uart_write);
    finish(minitest::run_all());
}
