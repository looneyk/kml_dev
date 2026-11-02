#if !defined(QUETZAL_TESTING_NAMED_CHECKS_HPP)
#define QUETZAL_TESTING_NAMED_CHECKS_HPP
//------------------------------------------------------------------------------
// testing
// named_checks.hpp
//------------------------------------------------------------------------------

#include <doctest/doctest.h>
#include <concepts>
#include <format>
#include <string>
#include <string_view>
#include <utility>

// Checks a condition and reports failure with a name and optional detail
// The arguments are evaluated once, in order, so a condition that fills in its detail does so before the detail is read
// They are macros so that a failure reports the line of the check
#define CHECK_CASE(name, ...)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        const ::quetzal::testing::detail::CaseResult quetzal_case_result {name, __VA_ARGS__};                          \
        DOCTEST_CHECK_MESSAGE(quetzal_case_result.passed, quetzal_case_result.message());                              \
    }                                                                                                                  \
    while (false)

// Checks that a value equals the value expected and reports failure with a name and both values
#define CHECK_CASE_EQUAL(name, value, value_expected)                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        const auto& quetzal_case_value = (value);                                                                      \
        const auto& quetzal_case_value_expected = (value_expected);                                                    \
        const ::quetzal::testing::detail::CaseResult quetzal_case_result {name,                                        \
            ::quetzal::testing::detail::equal(quetzal_case_value, quetzal_case_value_expected),                        \
            std::format("expected {}, got {}", quetzal_case_value_expected, quetzal_case_value)};                      \
        DOCTEST_CHECK_MESSAGE(quetzal_case_result.passed, quetzal_case_result.message());                              \
    }                                                                                                                  \
    while (false)

namespace quetzal::testing::detail
{

    struct CaseResult
    {
        std::string name;
        bool passed;
        std::string detail;

        CaseResult(std::string_view name, bool passed, std::string_view detail = {}) :
            name(name),
            passed(passed),
            detail(detail)
        {
        }

        // Returns the name, followed by the detail if there is one
        std::string message() const
        {
            if (detail.empty())
            {
                return name;
            }

            return std::format("{}: {}", name, detail);
        }
    };

    // Compares integers with std::cmp_equal, which compares their mathematical values whatever their signedness, and
    // anything else with ==
    template<typename A, typename B>
    bool equal(const A& a, const B& b)
    {
        if constexpr (std::integral<A> && std::integral<B> && !std::same_as<A, bool> && !std::same_as<B, bool>)
        {
            return std::cmp_equal(a, b);
        }
        else
        {
            return a == b;
        }
    }

} // namespace quetzal::testing::detail

#endif // QUETZAL_TESTING_NAMED_CHECKS_HPP
