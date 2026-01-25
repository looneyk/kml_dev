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

    // This is not ulp, rename ...

    constexpr int ulpDefault = 64;

    // Three-way comparison
    template<typename T> requires std::floating_point<T>
    std::partial_ordering float_compare(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_eq(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_ne(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_lt(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_le(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_gt(T lhs, T rhs, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_ge(T lhs, T rhs, int ulp = ulpDefault);

    // Three-way comparison
    template<typename T> requires std::floating_point<T>
    std::partial_ordering float_compare0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_eq0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_ne0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_lt0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_le0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_gt0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_ge0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_clamped(T t, T tMin, T tMax, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    bool float_clamped01(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    int float_sign(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    T denoise0(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    T denoise01(T t, int ulp = ulpDefault);

    template<typename T> requires std::floating_point<T>
    T denoise011(T t, int ulp = ulpDefault);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
std::partial_ordering quetzal::math::float_compare(T lhs, T rhs, int ulp)
{
    return float_eq(lhs, rhs, ulp) ? 0 : (lhs < rhs ? -1 : 1);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_eq(T lhs, T rhs, int ulp)
{
    // this? ...
    if (float_eq0(lhs, ulp) || float_eq0(rhs, ulp))
    {
        return float_eq0(lhs - rhs, ulp);
    }

    // absolute
    if (float_eq0(lhs - rhs, ulp))
    {
        return true;
    }

    // relative
//    T diff = std::abs(lhs - rhs);
//    return diff <= (std::numeric_limits<T>::epsilon() * std::max(std::abs(lhs), std::abs(rhs)) * ulp);
    return std::abs(lhs - rhs) <= std::max(std::abs(lhs), std::abs(rhs)) * std::numeric_limits<T>::epsilon() * ulp;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ne(T lhs, T rhs, int ulp)
{
    return !float_eq(lhs, rhs, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_lt(T lhs, T rhs, int ulp)
{
    return lhs <= rhs && !float_eq(lhs, rhs, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_le(T lhs, T rhs, int ulp)
{
    return lhs <= rhs || float_eq(lhs, rhs, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_gt(T lhs, T rhs, int ulp)
{
    return lhs > rhs && !float_eq(lhs, rhs, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ge(T lhs, T rhs, int ulp)
{
    return lhs >= rhs || float_eq(lhs, rhs, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
std::partial_ordering quetzal::math::float_compare0(T t, int ulp)
{
    return float_eq0(t, ulp) ? std::partial_ordering::equivalent : (t < T(0) ? std::partial_ordering::less : std::partial_ordering::greater);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_eq0(T t, int ulp)
{
    return abs(t) <= std::numeric_limits<T>::epsilon() * ulp;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ne0(T t, int ulp)
{
    return !float_eq0(t, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_lt0(T t, int ulp)
{
    return t < T(0) && !float_eq0(t, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_le0(T t, int ulp)
{
    return t <= T(0) || float_eq0(t, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_gt0(T t, int ulp)
{
    return t > T(0) && !float_eq0(t, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_ge0(T t, int ulp)
{
    return t >= T(0) || float_eq0(t, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_clamped(T t, T tMin, T tMax, int ulp)
{
    return float_ge(t, tMin, ulp) && float_le(t, tMax, ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
bool quetzal::math::float_clamped01(T t, int ulp)
{
    return float_ge0(t, ulp) && float_le(t, T(1), ulp);
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
int quetzal::math::float_sign(T t, int ulp)
{
    if (float_eq0(t, ulp))
    {
        return 0;
    }

    return t < 0 ? -1 : 1;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise0(T t, int ulp)
{
    return float_eq0(t, ulp) ? T(0) : t;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise01(T t, int ulp)
{
    if (float_eq0(t, ulp)) return T(0);
    if (float_eq(t, T(1), ulp)) return T(1);
    return t;
}

//------------------------------------------------------------------------------
template<typename T> requires std::floating_point<T>
T quetzal::math::denoise011(T t, int ulp)
{
    if (float_eq0(t, ulp)) return T(0);
    if (float_eq(t, T(1), ulp)) return T(1);
    if (float_eq(t, T(-1), ulp)) return T(-1);
    return t;
}

#endif // QUETZAL_MATH_FLOATING_POINT_HPP
