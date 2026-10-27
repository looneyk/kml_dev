#if !defined(QUETZAL_MATH_INTERVAL_HPP)
#define QUETZAL_MATH_INTERVAL_HPP
//------------------------------------------------------------------------------
// math
// Interval.hpp
//------------------------------------------------------------------------------

// Closed interval

#include "floating_point.hpp"
#include "math_util.hpp"
#include "quetzal/common/string_util.hpp"
#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename T>
    class Interval
    {
    public:

        using value_type = T;

        constexpr Interval();
        explicit constexpr Interval(const T& length); // [0, length]
        constexpr Interval(const T& lower, const T& upper);
        constexpr Interval(const Interval&) = default;
        constexpr Interval(Interval&&) = default;
        virtual ~Interval() = default;

        Interval& operator=(const Interval&) = default;
        Interval& operator=(Interval&&) = default;

        T lower() const;
        void set_lower(const T& t);

        T upper() const;
        void set_upper(const T& t);

        void set_bounds(const T& lower, const T& upper);

        // strict functions are for standard dimensionless parametric intervals
        T length() const;
        bool proper() const; // lower < upper
        bool strict_proper() const; // values in the range [0, 1], implies proper
        bool unit() const; // length == 1, implies proper
        bool strict_unit() const; // [0, 1], implies unit

        bool contains(T t) const;
        T lerp(T t) const;

    private:

        T m_lower;
        T m_upper;
//        std::array<T, 2> m_bounds;
    };

    template<typename T>
    Interval<T> strict_unit_interval();

    template<typename T>
    Interval<T> shift(const Interval<T>& interval, T t);

    template<typename T>
    std::istream& operator>>(std::istream& is, Interval<T>& interval);

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Interval<T>& interval);

    template<typename T>
    std::from_chars_result from_chars(const char* first, const char* last, Interval<T>& interval, const FormatSpec& spec = {});

    template<typename T>
    std::to_chars_result to_chars(char* first, char* last, const Interval<T>& interval, const FormatSpec& spec = {});

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
constexpr quetzal::math::Interval<T>::Interval() :
    Interval(T(0))
{
}

//------------------------------------------------------------------------------
template<typename T>
constexpr quetzal::math::Interval<T>::Interval(const T& length) :
    Interval(T(0), length)
{
}

//------------------------------------------------------------------------------
template<typename T>
constexpr quetzal::math::Interval<T>::Interval(const T& lower, const T& upper) :
    m_lower(lower),
    m_upper(upper)
{
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::lower() const
{
    return m_lower;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_lower(const T& t)
{
    m_lower = t;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::upper() const
{
    return m_upper;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_upper(const T& t)
{
    m_upper = t;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_bounds(const T& lower, const T & upper)
{
    m_lower = lower;
    m_upper = upper;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::length() const
{
    return m_upper - m_lower;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::proper() const
{
    if constexpr (std::floating_point<T>)
    {
        return float_gt(m_upper, m_lower);
    }
    else
    {
        return m_upper > m_lower;
    }
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::strict_proper() const
{
    return clamped01(m_lower) && clamped01(m_upper) && proper();
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::unit() const
{
    if constexpr (std::floating_point<T>)
    {
        return float_eq(length(), T(1));
    }
    else
    {
        return length() == T(1);
    }
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::strict_unit() const
{
    if constexpr (std::floating_point<T>)
    {
        return float_eq(m_lower, T(0)) && float_eq(m_upper, T(1));
    }
    else
    {
        return m_lower == T(0) && m_upper == T(1);
    }
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::contains(T t) const
{
    if constexpr (std::floating_point<T>)
    {
        return float_ge(t, m_lower) && float_le(t, m_upper);
    }
    else
    {
        return t >= m_lower && t <= m_upper;
    }
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::lerp(T t) const
{
    if (m_lower == m_upper)
    {
        return m_lower;
    }

    return math::lerp(m_lower, m_upper, t);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Interval<T> quetzal::math::strict_unit_interval()
{
    return Interval(T(0), T(1));
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Interval<T> quetzal::math::shift(const Interval<T>& interval, T t)
{
    return {interval.lower() + t, interval.upper() + t};
}

//------------------------------------------------------------------------------
template<typename T>
std::istream& quetzal::math::operator>>(std::istream& is, Interval<T>& interval)
{
    std::array<T, 2> bounds {};
    if (quetzal::read_series(is, bounds, quetzal::bounds_delimiter_default))
    {
        interval.set_bounds(bounds[0], bounds[1]);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename T>
std::ostream& quetzal::math::operator<<(std::ostream& os, const Interval<T>& interval)
{
    os << quetzal::to_string(interval, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename T>
std::from_chars_result quetzal::math::from_chars(const char* first, const char* last, Interval<T>& interval, const FormatSpec& spec)
{
    std::array<T, 2> bounds {};
    auto result = quetzal::series_from_chars(first, last, bounds, spec, quetzal::bounds_delimiter_default);
    if (result.ec == std::errc {})
    {
        interval.set_bounds(bounds[0], bounds[1]);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename T>
std::to_chars_result quetzal::math::to_chars(char* first, char* last, const Interval<T>& interval, const FormatSpec& spec)
{
    const std::array<T, 2> bounds {interval.lower(), interval.upper()};
    return quetzal::series_to_chars(first, last, bounds, spec, quetzal::bounds_delimiter_default);
}

//------------------------------------------------------------------------------
template<typename T>
struct std::formatter<quetzal::math::Interval<T>> : quetzal::series_formatter<T>
{
    constexpr formatter() :
        quetzal::series_formatter<T>({.series_delimiter = std::string(quetzal::bounds_delimiter_default)})
    {
    }

    template<typename FormatContext>
    auto format(const quetzal::math::Interval<T>& interval, FormatContext& ctx) const
    {
        // The conditionals translate negative zero
        const std::array<T, 2> bounds {interval.lower() == T(0) ? T(0) : interval.lower(), interval.upper() == T(0) ? T(0) : interval.upper()};
        return this->format_series(bounds, ctx);
    }
};

#endif // QUETZAL_MATH_INTERVAL_HPP
