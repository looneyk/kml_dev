#if !defined(QUETZAL_MATH_FLOATING_POINT_HPP)
#define QUETZAL_MATH_FLOATING_POINT_HPP
//------------------------------------------------------------------------------
// math
// floating_point.hpp
//------------------------------------------------------------------------------

// originally based on cppreference.com epsilon example
// updated with Boost implementation of Knuth algorithm

#include <algorithm>
#include <compare>
#include <concepts>
#include <limits>
#include <cmath>

namespace quetzal::math
{

    constexpr int tolerance_default = 64;

    // Three-way comparison
    template<typename T> requires std::floating_point<T>
    std::partial_ordering float_compare(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_eq(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_ne(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_lt(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_le(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_gt(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_ge(T lhs, T rhs, int tolerance = tolerance_default);

    // Three-way comparison
    template<typename T> requires std::floating_point<T>
    std::partial_ordering float_compare0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_eq0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_ne0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_lt0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_le0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_gt0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_ge0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_clamped(T t, T tMin, T tMax, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    bool float_clamped01(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    int float_sign(T t, int tolerance = tolerance_default);

    // The two values are not approximately exact; they are either exactly equal, or if unequal, then by more than the float equality threshold
    template<typename T> requires std::floating_point<T>
    bool ne_or_exact(T lhs, T rhs, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    T denoise0(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    T denoise01(T t, int tolerance = tolerance_default);

    template<typename T> requires std::floating_point<T>
    T denoise011(T t, int tolerance = tolerance_default);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
std::partial_ordering quetzal::math::float_compare(T lhs, T rhs, int tolerance)
{
    return float_eq(lhs, rhs, tolerance) ? 0 : (lhs < rhs ? -1 : 1);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_eq(T lhs, T rhs, int tolerance)
{
    // this? ...
    if (float_eq0(lhs, tolerance) || float_eq0(rhs, tolerance))
    {
        return float_eq0(lhs - rhs, tolerance);
    }

    // absolute
    if (float_eq0(lhs - rhs, tolerance))
    {
        return true;
    }

    // relative
//    T diff = std::abs(lhs - rhs);
//    return diff <= (std::numeric_limits<T>::epsilon() * std::max(std::abs(lhs), std::abs(rhs)) * tolerance);
    return std::abs(lhs - rhs) <= std::max(std::abs(lhs), std::abs(rhs)) * std::numeric_limits<T>::epsilon() * tolerance;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ne(T lhs, T rhs, int tolerance)
{
    return !float_eq(lhs, rhs, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_lt(T lhs, T rhs, int tolerance)
{
    return lhs <= rhs && !float_eq(lhs, rhs, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_le(T lhs, T rhs, int tolerance)
{
    return lhs <= rhs || float_eq(lhs, rhs, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_gt(T lhs, T rhs, int tolerance)
{
    return lhs > rhs && !float_eq(lhs, rhs, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ge(T lhs, T rhs, int tolerance)
{
    return lhs >= rhs || float_eq(lhs, rhs, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
std::partial_ordering quetzal::math::float_compare0(T t, int tolerance)
{
    return float_eq0(t, tolerance) ? std::partial_ordering::equivalent : (t < T(0) ? std::partial_ordering::less : std::partial_ordering::greater);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_eq0(T t, int tolerance)
{
    return abs(t) <= std::numeric_limits<T>::epsilon() * tolerance;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ne0(T t, int tolerance)
{
    return !float_eq0(t, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_lt0(T t, int tolerance)
{
    return t < T(0) && !float_eq0(t, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_le0(T t, int tolerance)
{
    return t <= T(0) || float_eq0(t, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_gt0(T t, int tolerance)
{
    return t > T(0) && !float_eq0(t, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ge0(T t, int tolerance)
{
    return t >= T(0) || float_eq0(t, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_clamped(T t, T tMin, T tMax, int tolerance)
{
    return float_ge(t, tMin, tolerance) && float_le(t, tMax, tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_clamped01(T t, int tolerance)
{
    return float_ge0(t, tolerance) && float_le(t, T(1), tolerance);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
int quetzal::math::float_sign(T t, int tolerance)
{
    if (float_eq0(t, tolerance))
    {
        return 0;
    }

    return t < 0 ? -1 : 1;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::ne_or_exact(T a, T b, int tolerance)
{
    return float_ne(a, b, tolerance) || a == b;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise0(T t, int tolerance)
{
    return float_eq0(t, tolerance) ? T(0) : t;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise01(T t, int tolerance)
{
    if (float_eq0(t, tolerance)) return T(0);
    if (float_eq(t, T(1), tolerance)) return T(1);
    return t;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise011(T t, int tolerance)
{
    if (float_eq0(t, tolerance)) return T(0);
    if (float_eq(t, T(1), tolerance)) return T(1);
    if (float_eq(t, T(-1), tolerance)) return T(-1);
    return t;
}

#endif // QUETZAL_MATH_FLOATING_POINT_HPP
