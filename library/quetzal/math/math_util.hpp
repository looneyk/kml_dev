#if !defined(QUETZAL_MATH_MATH_UTIL_HPP)
#define QUETZAL_MATH_MATH_UTIL_HPP
//------------------------------------------------------------------------------
// math
// math_util.hpp
//
// Angles in radians
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include <algorithm>
#include <concepts>
#include <cmath>
#include <cassert>

namespace quetzal::math
{

    template<typename T>
    constexpr T Pi = T(3.1415926535897932384626433832795);

    template<typename T>
    constexpr T PiTwo = T(6.283185307179586476925286766559);

    template<typename T>
    constexpr T PiHalf = T(1.5707963267948966192313216916398);

    template<typename T>
    constexpr T PiThird = T(1.0471975511965977461542144610932);

    template<typename T>
    constexpr T PiFourth = T(0.78539816339744830961566084581988);

    template<typename T>
    constexpr T PiFifth = T(0.6283185307179586476925286766559);

    template<typename T>
    constexpr T PiSixth = T(0.52359877559829887307710723054658);

    template<typename T>
    constexpr T PiTwoThird = T(2.0943951023931954923084289221863);

    template<typename T>
    constexpr T E = T(2.7182818284590452353602874713527);

    template<typename T>
    constexpr T GoldenRatio = T(1.6180339887498948482045868343656);

    template<typename T> requires std::integral<T>
    bool even(T t);

    template<typename T> requires std::integral<T>
    bool odd(T t);

    template<typename T> requires std::integral<T>
    T sext(T t, int nSignificantBits);

    template<typename T>
    int valid(T t);

    template<typename T>
    int signum(T t);

    template<typename T>
    T clamp0(T t);

    template<typename T>
    T clamp01(T t);

    template<typename T>
    bool clamped(T t, T tMin, T tMax);

    template<typename T>
    bool clamped01(T t);

    template<typename X, typename T>
    X lerp(const X& x0, const X& x1, T t);

    // |arc| == 2 * Pi
    template<typename T>
    bool full_circle(T arc);

    // |phi1 - phi0| == 2 * Pi
    template<typename T>
    bool full_circle(T phi0, T phi1);

    // Angle between a and b
    template<typename V>
    typename V::value_type angle_between(const V& a, const V& b);

    // Angle between a and b where a and b are unit vectors
    template<typename V>
    typename V::value_type angle_between_unit(const V& a, const V& b);

    // Angle from from to to
    template<typename V> requires (V::dimension == 2)
    typename V::value_type angle_full(const V& from, const V& to);

    // Angle from from to to where from and to are unit vectors
    template<typename V> requires (V::dimension == 2)
    typename V::value_type angle_full_unit(const V& from, const V& to);

    // Angle from from to to around axis
    template<typename V> requires (V::dimension == 3)
    typename V::value_type angle_full(const V& from, const V& to, const V& axis);

    // Angle from from to to around axis where all vectors are unit vectors
    template<typename V> requires (V::dimension == 3)
    typename V::value_type angle_full_unit(const V& from, const V& to, const V& axis);
/*
    // Axis and angle between a and b
    template<typename V>
    typename V::value_type axis_angle(const V& a, const V& b);

    // Axis and angle between a and b
    template<typename V>
    typename V::value_type axis_angle_unit(const V& a, const V& b);
*/
    template<typename V>
    typename V::value_type azimuth_radius(const V& v);

    template<typename V>
    typename V::value_type azimuth_angle(const V& v);

    template<typename V>
    typename V::value_type elevation_angle(const V& v);

    template<typename V> requires (V::dimension == 3)
    V perpendicular_unit_vector(const V& v);

    // Algorithm from https://www.freesteel.co.uk/wpblog/2009/06/05/encoding-2d-angles-without-trigonometry/
    template<typename V> requires (V::dimension == 2)
    typename V::value_type diamond_angle(const V& v);

    size_t triangular_number(size_t n);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T> requires std::integral<T>
bool quetzal::math::even(T t)
{
    return (t & 1) == 0;
}

//------------------------------------------------------------------------------
template<typename T> requires std::integral<T>
bool quetzal::math::odd(T t)
{
    return (t & 1) == 1;
}

//------------------------------------------------------------------------------
template<typename T> requires std::integral<T>
T quetzal::math::sext(T t, int nSignificantBits)
{
    if (t & (0x01 << (nSignificantBits - 1)))
    {
        t |= (T(-1) << nSignificantBits);
    }
    return t;
}

//------------------------------------------------------------------------------
template<typename T>
int quetzal::math::valid(T t)
{
    return isnormal(t) || t == T(0);
}

//------------------------------------------------------------------------------
template<typename T>
int quetzal::math::signum(T t)
{
    return (T(0) < t) - (t < T(0));
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::clamp0(T t)
{
    return std::max(t, T(0));
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::clamp01(T t)
{
    return std::clamp(t, T(0), T(1));
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::clamped(T t, T tMin, T tMax)
{
    return t >= tMin && t <= tMax;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::clamped01(T t)
{
    return t >= T(0) && t <= T(1);
}

//------------------------------------------------------------------------------
template<typename X, typename T>
X quetzal::math::lerp(const X& x0, const X& x1, T t)
{
    return (T(1) - t) * x0 + t * x1;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::full_circle(T arc)
{
    return math::float_eq(abs(arc), PiTwo<T>);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::full_circle(T phi0, T phi1)
{
    return full_circle(phi1 - phi0);
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type quetzal::math::angle_between(const V& a, const V& b)
{
    return angle_between_unit(normalize(a), normalize(b));
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type quetzal::math::angle_between_unit(const V& a, const V& b)
{
    assert(a.unit());
    assert(b.unit());

    return std::acos(std::clamp(dot(a, b), V::value_type(-1), V::value_type(1)));
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 2)
typename V::value_type quetzal::math::angle_full(const V& from, const V& to)
{
    typename V::value_type theta = angle_between(from, to);
    return dot(perp(from), to) < typename V::value_type(0) ? math::PiTwo<typename V::value_type> - theta : theta;
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 2)
typename V::value_type quetzal::math::angle_full_unit(const V& from, const V& to)
{
    typename V::value_type theta = angle_between_unit(from, to);
    return dot(perp(from), to) < typename V::value_type(0) ? math::PiTwo<typename V::value_type> - theta : theta;
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 3)
typename V::value_type quetzal::math::angle_full(const V& from, const V& to, const V& axis)
{
    return angle_full_unit(normalize(from), normalize(to), axis);
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 3)
typename V::value_type quetzal::math::angle_full_unit(const V& from, const V& to, const V& axis)
{
    typename V::value_type theta = angle_between_unit(from, to);
    return dot(cross(from, to), axis) < typename V::value_type(0) ? math::PiTwo<typename V::value_type> - theta : theta;
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type quetzal::math::azimuth_radius(const V& v)
{
    return sqrt(v.x() * v.x() + v.y() * v.y());
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type quetzal::math::azimuth_angle(const V& v)
{
    return atan2(v.y(), v.x());
}

//------------------------------------------------------------------------------
template<typename V>
typename V::value_type quetzal::math::elevation_angle(const V& v)
{
    return atan2(v.z(), sqrt(v.x() * v.x() + v.y() * v.y()));
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 3)
V quetzal::math::perpendicular_unit_vector(const V& v)
{
    size_t i = 0;
    size_t iMin = 0;
    auto lowest = v[0];
    for (i = 1; i < V::dimension; ++i)
    {
        if (std::abs(v[i]) < lowest)
        {
            iMin = i;
            lowest = v[i];
        }
    }

    V u{};
    u[iMin] = typename V::value_type(1);

    return normalize(cross(v, u));
}

//------------------------------------------------------------------------------
template<typename V> requires (V::dimension == 2)
typename V::value_type quetzal::math::diamond_angle(const V& v)
{
    using T = V::value_type;

    auto x = v.x();
    auto y = v.y();

    if (y >= T(0))
    {
        return x >= T(0) ? y / (x + y) : T(1) - x / (-x + y);
    }

    return x < T(0) ? T(2) - y / (-x - y) : T(3) + x / (x - y);
}

#endif // QUETZAL_MATH_MATH_UTIL_HPP
