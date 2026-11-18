#pragma once
// PROTOTYPE (issue #18): minimal C++20 test runner. Throwaway.
// - Tests register themselves at startup (static objects in an intrusive list, no heap).
// - CHECK is non-fatal; it decomposes `a op b` and prints both values on failure.
// - Failure locations come from std::source_location.
// - All output goes through one function the caller supplies.
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <string_view>
#include <type_traits>

namespace minitest {

using output_function = void (*)(std::string_view text) noexcept;
void set_output(output_function out) noexcept;

using test_function = void (*)();

struct test_case {
    std::string_view name;
    test_function function;
    std::source_location location;
    test_case* next = nullptr;
};

struct registrar {
    explicit registrar(test_case& test) noexcept;
};

// Runs every registered test in registration order. Returns the number of failed tests.
int run_all() noexcept;

// Fixed-capacity text, for printing values without a heap. Truncates when full.
class text {
public:
    void append(std::string_view s) noexcept;
    void append_unsigned(std::uint64_t value) noexcept;
    void append_signed(std::int64_t value) noexcept;
    void append_float(float value) noexcept;
    std::string_view view() const noexcept { return {data_, size_}; }

private:
    char data_[48];
    std::size_t size_ = 0;
};

template <typename T>
void append_value(text& out, const T& value) noexcept {
    if constexpr (std::same_as<T, bool>) {
        out.append(value ? "true" : "false");
    } else if constexpr (std::is_enum_v<T>) {
        append_value(out, static_cast<std::underlying_type_t<T>>(value));
    } else if constexpr (std::floating_point<T>) {
        out.append_float(static_cast<float>(value));
    } else if constexpr (std::signed_integral<T>) {
        out.append_signed(value);
    } else if constexpr (std::unsigned_integral<T>) {
        out.append_unsigned(value);
    } else if constexpr (std::convertible_to<const T&, std::string_view>) {
        out.append("\"");
        out.append(std::string_view(value));
        out.append("\"");
    } else if constexpr (std::is_pointer_v<T>) {
        out.append("0x");
        out.append_unsigned(reinterpret_cast<std::uintptr_t>(value)); // decimal is fine here
    } else {
        out.append("{?}");
    }
}

void report_pass() noexcept;
void report_failure(std::source_location where, std::string_view expression, std::string_view lhs,
                    std::string_view op, std::string_view rhs) noexcept;

// ---- expression decomposition: `decomposer{} <= a op b` ----

template <typename L, typename R>
struct binary_expression {
    const L& lhs;
    const R& rhs;
    std::string_view op;
    bool passed;
};

template <typename L>
struct value_expression {
    const L& value;

#define MINITEST_BINARY_OPERATOR(OP)                                                               \
    template <typename R>                                                                          \
    binary_expression<L, R> operator OP(const R& rhs) && noexcept {                                \
        return {value, rhs, #OP, static_cast<bool>(value OP rhs)};                                 \
    }
    MINITEST_BINARY_OPERATOR(==)
    MINITEST_BINARY_OPERATOR(!=)
    MINITEST_BINARY_OPERATOR(<)
    MINITEST_BINARY_OPERATOR(<=)
    MINITEST_BINARY_OPERATOR(>)
    MINITEST_BINARY_OPERATOR(>=)
#undef MINITEST_BINARY_OPERATOR
};

struct decomposer {
    template <typename L>
    value_expression<L> operator<=(const L& value) const noexcept {
        return {value};
    }
};

template <typename L, typename R>
void check(const binary_expression<L, R>& e, std::string_view expression,
           std::source_location where = std::source_location::current()) noexcept {
    if (e.passed) return report_pass();
    text lhs, rhs;
    append_value(lhs, e.lhs);
    append_value(rhs, e.rhs);
    report_failure(where, expression, lhs.view(), e.op, rhs.view());
}

template <typename L>
void check(const value_expression<L>& e, std::string_view expression,
           std::source_location where = std::source_location::current()) noexcept {
    if (static_cast<bool>(e.value)) return report_pass();
    text value;
    append_value(value, e.value);
    report_failure(where, expression, value.view(), {}, {});
}

} // namespace minitest

#define MINITEST_CAT2(a, b) a##b
#define MINITEST_CAT(a, b) MINITEST_CAT2(a, b)
#define MINITEST_TEST_CASE(name, fn)                                                               \
    static void fn();                                                                              \
    namespace {                                                                                    \
    ::minitest::test_case MINITEST_CAT(fn, _case){name, &fn, std::source_location::current()};     \
    const ::minitest::registrar MINITEST_CAT(fn, _registrar){MINITEST_CAT(fn, _case)};             \
    }                                                                                              \
    static void fn()

#define TEST_CASE(name) MINITEST_TEST_CASE(name, MINITEST_CAT(minitest_test_, __COUNTER__))

// Non-fatal: records the failure and carries on. A test returns early itself when needed.
#define CHECK(...)                                                                                 \
    do {                                                                                           \
        _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wparentheses\"")         \
            ::minitest::check(::minitest::decomposer{} <= __VA_ARGS__, #__VA_ARGS__);              \
        _Pragma("GCC diagnostic pop")                                                              \
    } while (false)
