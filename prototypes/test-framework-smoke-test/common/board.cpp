// PROTOTYPE (issue #18): startup, USART2 output and runtime stubs. Throwaway.
// Linked into all three images (baseline, snitch, own runner), so it cancels out
// when the framework's share is measured.
#include "board.hpp"

#include <cstdint>
#include <exception>

namespace {
constexpr std::uintptr_t usart2 = 0x40004400;
constexpr std::uint32_t stack_paint = 0xC0FFEE55;

volatile std::uint32_t& reg(std::uintptr_t address) {
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}
} // namespace

extern "C" {
extern std::uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _sstack, _estack;
extern void (*__init_array_start[])();
extern void (*__init_array_end[])();

int main();

[[noreturn]] void Reset_Handler() {
    for (std::uint32_t *src = &_sidata, *dst = &_sdata; dst < &_edata;) *dst++ = *src++;
    for (std::uint32_t* dst = &_sbss; dst < &_ebss;) *dst++ = 0;

    // Paint the stack below the current frame so the high-water mark can be measured.
    std::uint32_t* sp;
    asm volatile("mov %0, sp" : "=r"(sp));
    for (std::uint32_t* p = &_sstack; p < sp - 16; ++p) *p = stack_paint;

#if defined(__ARM_FP)
    reg(0xE000ED88) |= (0xFu << 20); // CPACR: enable the FPU before any float code runs.
#endif

    // Static constructors: this is where tests register themselves.
    for (auto* f = __init_array_start; f < __init_array_end; ++f) (*f)();

    main();
    for (;;) {}
}

[[noreturn]] void Default_Handler() {
    uart_write("\n!! fault\n");
    for (;;) {}
}

__attribute__((section(".isr_vector"), used)) void (*const vector_table[])() = {
    reinterpret_cast<void (*)()>(&_estack), Reset_Handler,
    Default_Handler, // NMI
    Default_Handler, // HardFault
    Default_Handler, // MemManage
    Default_Handler, // BusFault
    Default_Handler, // UsageFault
};

// Runtime stubs. The Firmware never exits, so destructors of statics never run.
void* __dso_handle = nullptr;
int __cxa_atexit(void (*)(void*), void*, void*) { return 0; }
void __cxa_pure_virtual() { Default_Handler(); }

// newlib's abort() and assert handler pull in signal() and fprintf(), and fprintf needs
// _sbrk (malloc). Bare-metal firmware replaces both. Only linked if something calls them.
[[noreturn]] void abort() {
    uart_write("\n!! abort\n");
    for (;;) {}
}
[[noreturn]] void __assert_func(const char* file, int line, const char*, const char* expr) {
    uart_write("\n!! assert: ");
    uart_write(expr);
    uart_write(" at ");
    uart_write(file);
    uart_write(":");
    uart_write_uint(static_cast<std::size_t>(line));
    uart_write("\n");
    for (;;) {}
}
}

// libstdc++'s version (std::string_view::substr out of range) formats with fprintf.
namespace std {
[[noreturn]] void __throw_out_of_range_fmt(const char*, ...) {
    uart_write("\n!! out_of_range\n");
    for (;;) {}
}
} // namespace std

// Replaces libsupc++'s std::terminate, whose default handler pulls in the demangler,
// which calls malloc. Reached by snitch's REQUIRE() when exceptions are off.
[[noreturn]] void std::terminate() noexcept {
    uart_write("\n!! std::terminate\n");
    for (;;) {}
}

void uart_init() noexcept {
    reg(usart2 + 0x0C) = (1u << 13) | (1u << 3); // CR1: UE | TE
}

void uart_write(std::string_view text) noexcept {
    for (char c : text) {
        while ((reg(usart2 + 0x00) & (1u << 7)) == 0) {} // SR.TXE
        reg(usart2 + 0x04) = static_cast<std::uint8_t>(c);
    }
}

void uart_write_uint(std::size_t value) noexcept {
    char digits[12];
    std::size_t n = 0;
    do {
        digits[n++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (n != 0) uart_write(std::string_view(&digits[--n], 1));
}

std::size_t stack_high_water_bytes() noexcept {
    std::uint32_t* p = &_sstack;
    while (p < &_estack && *p == stack_paint) ++p;
    return reinterpret_cast<std::uintptr_t>(&_estack) - reinterpret_cast<std::uintptr_t>(p);
}

void finish(int failed_tests) noexcept {
    uart_write("stack high-water: ");
    uart_write_uint(stack_high_water_bytes());
    uart_write(" bytes\nfailed tests: ");
    uart_write_uint(static_cast<std::size_t>(failed_tests));
    uart_write("\n=== END OF RUN ===\n");
    for (;;) {}
}
