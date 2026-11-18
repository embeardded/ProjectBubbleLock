#pragma once
// PROTOTYPE (issue #18): just enough board support to print over USART2. Throwaway.
#include <cstddef>
#include <string_view>

void uart_init() noexcept;
void uart_write(std::string_view text) noexcept;
void uart_write_uint(std::size_t value) noexcept;

// Bytes of stack touched since reset (the stack is painted before main).
std::size_t stack_high_water_bytes() noexcept;

// Prints the stack high-water mark and an end marker, then halts.
[[noreturn]] void finish(int failed_tests) noexcept;
