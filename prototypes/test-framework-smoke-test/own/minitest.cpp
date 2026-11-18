// PROTOTYPE (issue #18): minimal C++20 test runner. Throwaway.
#include "minitest.hpp"

namespace minitest {
namespace {
// Constant-initialised, so registration order across files does not matter.
output_function output = nullptr;
test_case* first = nullptr;
test_case* last = nullptr;

struct counters {
    unsigned tests = 0, failed_tests = 0, checks = 0, failed_checks = 0;
} counts;
unsigned failures_in_current_test = 0;

void write(std::string_view s) noexcept {
    if (output != nullptr) output(s);
}

void write_count(unsigned n) noexcept {
    text t;
    t.append_unsigned(n);
    write(t.view());
}
} // namespace

void set_output(output_function out) noexcept { output = out; }

registrar::registrar(test_case& test) noexcept {
    if (last == nullptr) first = &test;
    else last->next = &test;
    last = &test;
}

void report_pass() noexcept { ++counts.checks; }

void report_failure(std::source_location where, std::string_view expression, std::string_view lhs,
                    std::string_view op, std::string_view rhs) noexcept {
    ++counts.checks;
    ++counts.failed_checks;
    ++failures_in_current_test;
    write(where.file_name());
    write(":");
    write_count(where.line());
    write(": FAILED: CHECK(");
    write(expression);
    write(")\n  with values: ");
    write(lhs);
    if (!op.empty()) {
        write(" ");
        write(op);
        write(" ");
        write(rhs);
    }
    write("\n");
}

int run_all() noexcept {
    for (test_case* test = first; test != nullptr; test = test->next) {
        ++counts.tests;
        failures_in_current_test = 0;
        write("starting: ");
        write(test->name);
        write("\n");
        test->function();
        if (failures_in_current_test != 0) ++counts.failed_tests;
        write(failures_in_current_test == 0 ? "passed:   " : "FAILED:   ");
        write(test->name);
        write("\n");
    }
    write("== ");
    write_count(counts.tests);
    write(" tests, ");
    write_count(counts.failed_tests);
    write(" failed; ");
    write_count(counts.checks);
    write(" checks, ");
    write_count(counts.failed_checks);
    write(" failed ==\n");
    return static_cast<int>(counts.failed_tests);
}

void text::append(std::string_view s) noexcept {
    for (char c : s) {
        if (size_ == sizeof(data_)) return;
        data_[size_++] = c;
    }
}

void text::append_unsigned(std::uint64_t value) noexcept {
    char digits[20];
    std::size_t n = 0;
    // 32-bit division where possible: Cortex-M0 has no divide instruction.
    if (value <= UINT32_MAX) {
        auto v = static_cast<std::uint32_t>(value);
        do { digits[n++] = static_cast<char>('0' + v % 10); v /= 10; } while (v != 0);
    } else {
        do { digits[n++] = static_cast<char>('0' + value % 10); value /= 10; } while (value != 0);
    }
    while (n != 0) append(std::string_view(&digits[--n], 1));
}

void text::append_signed(std::int64_t value) noexcept {
    if (value < 0) {
        append("-");
        append_unsigned(0 - static_cast<std::uint64_t>(value));
    } else {
        append_unsigned(static_cast<std::uint64_t>(value));
    }
}

// Fixed-point with 4 decimals: enough to read a failure message, not a general formatter.
void text::append_float(float value) noexcept {
    if (value != value) return append("nan");
    if (value < 0) {
        append("-");
        value = -value;
    }
    if (value > 4.0e9f) return append("inf-or-huge");
    auto whole = static_cast<std::uint32_t>(value);
    auto fraction = static_cast<std::uint32_t>((value - static_cast<float>(whole)) * 10000.0f + 0.5f);
    if (fraction >= 10000) {
        ++whole;
        fraction -= 10000;
    }
    append_unsigned(whole);
    append(".");
    for (std::uint32_t scale = 1000; scale != 0; scale /= 10) {
        const char digit = static_cast<char>('0' + (fraction / scale) % 10);
        append(std::string_view(&digit, 1));
    }
}

} // namespace minitest
