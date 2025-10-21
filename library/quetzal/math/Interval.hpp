#if !defined(QUETZAL_MATH_INTERVAL_HPP)
#define QUETZAL_MATH_INTERVAL_HPP
//------------------------------------------------------------------------------
// math
// Interval.hpp
//------------------------------------------------------------------------------

// Closed interval

#include "floating_point.hpp"
#include "math_util.hpp"
#include <concepts>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename T>
    class Interval
    {
    public:

        using value_type = T;

        Interval();
        explicit Interval(const T& tLength);
        Interval(const T& tLower, const T& tUpper);
        Interval(const Interval&) = default;
        Interval(Interval&&) = default;
        virtual ~Interval() = default;

        Interval& operator=(const Interval&) = default;
        Interval& operator=(Interval&&) = default;

        T lower() const;
        void set_lower(const T& t);

        T upper() const;
        void set_upper(const T& t);

        void set_bounds(const T& tLower, const T& tUpper);

        T length() const;
        bool proper() const; // lower < upper
        bool unit() const; // length == 1, implies proper
        bool strict_unit() const; // [0, 1], implies unit

        bool contains(T t) const;
        T lerp(T t) const;

    private:

        T m_tLower;
        T m_tUpper;
//        std::array<T, 2> m_bounds;
    };

    template<typename T>
    Interval<T> strict_unit_interval();

    template<typename T>
    Interval<T> shift(const Interval<T>& interval, T t);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Interval<T>::Interval() :
    Interval(T(0))
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Interval<T>::Interval(const T& tLength) :
    Interval(T(0), tLength)
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Interval<T>::Interval(const T& tLower, const T& tUpper) :
    m_tLower(tLower),
    m_tUpper(tUpper)
{
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::lower() const
{
    return m_tLower;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_lower(const T& t)
{
    m_tLower = t;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::upper() const
{
    return m_tUpper;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_upper(const T& t)
{
    m_tUpper = t;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Interval<T>::set_bounds(const T& tLower, const T & tUpper)
{
    m_tLower = tLower;
    m_tUpper = tUpper;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::length() const
{
    return m_tUpper - m_tLower;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::proper() const
{
    if constexpr (std::floating_point<T>)
    {
        return float_gt(m_tUpper, m_tLower);
    }
    else
    {
        return m_tUpper > m_tLower;
    }
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
        return float_eq(m_tLower, T(0)) && float_eq(m_tUpper, T(1));
    }
    else
    {
        return m_tLower == T(0) && m_tUpper == T(1);
    }
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Interval<T>::contains(T t) const
{
    if constexpr (std::floating_point<T>)
    {
        return float_ge(t, m_tLower) && float_le(t, m_tUpper);
    }
    else
    {
        return t >= m_tLower && t <= m_tUpper;
    }
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::math::Interval<T>::lerp(T t) const
{
    if (m_tLower == m_tUpper)
    {
        return m_tLower;
    }

    return math::lerp(m_tLower, m_tUpper, t);
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

#endif // QUETZAL_MATH_INTERVAL_HPP
