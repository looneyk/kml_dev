#if !defined(QUETZAL_MATH_ROTATION_HPP)
#define QUETZAL_MATH_ROTATION_HPP
//------------------------------------------------------------------------------
// math
// Rotation.hpp
//------------------------------------------------------------------------------

#include "HomogeneousTraits.hpp"
#include "Quaternion.hpp"
#include "Vector.hpp"
#include <iostream>
#include <tuple>
#include <cassert>

namespace quetzal::math
{

    // Angles are in radians
    // Internally rotations are represented by unit quaternions
    // Operations with scalars (value_type) only affect the angle, the axis of rotation does not change
    // Multiplication and division operations with two Rotations compose them accordingly
    // Vector rotation can use Vector-Rotation multiplication

    //--------------------------------------------------------------------------
    template<typename T>
    class Rotation
    {
    public:

        static constexpr size_t dimension = 3;
        static constexpr size_t component_count = 4;
        using value_type = T;
        using traits_type = HomogeneousTraits<value_type, dimension>;
        using vector_type = Vector<traits_type>;
        using rep_type = std::array<value_type, component_count>;

        Rotation();
        Rotation(const vector_type& axis, value_type angle);
        Rotation(const Quaternion<T>& quaternion);
        Rotation(const Rotation&) = default;
        Rotation(Rotation&&) = default;
        virtual ~Rotation() = default;

        constexpr static Rotation<T> Identity();

        Rotation& operator=(const Rotation&) = default;
        Rotation& operator=(Rotation&& other) = default;
        Rotation& operator=(const Quaternion<T>& quaternion);

        Rotation operator+() const;
        Rotation operator-() const;
        Rotation& operator+=(value_type t);
        Rotation& operator-=(value_type t);
        Rotation& operator*=(const Rotation& other);
        Rotation& operator*=(value_type t);
        Rotation& operator/=(const Rotation& other); // *= inverse(other)
        Rotation& operator/=(value_type t);

        // Returns an arbitrary axis for the 0 angle rotation case
        vector_type axis() const;
        value_type angle() const;
        std::tuple<vector_type, value_type> axis_angle() const;

        void set(const vector_type& axis, value_type angle);

        const Rotation& inverse() const;

        void reset();

        bool identity(int ulp = ulpDefault) const;

        const rep_type& rep() const;
        rep_type& rep();

    protected:

        void normalize(); // optimized for unit quaternion

        Quaternion<T> m_rep;
    };

    // Compose two rotations
    template<typename T>
    Rotation<T> operator*(Rotation<T> lhs, const Rotation<T>& rhs);

    // Applies to angle
    template<typename T>
    Rotation<T> operator*(const Rotation<T>& lhs, const T& rhs);

    // Applies to angle
    template<typename T>
    Rotation<T> operator*(const T& lhs, const Rotation<T>& rhs);

    // Compose two rotations
    template<typename T>
    Rotation<T> operator/(Rotation<T> lhs, const Rotation<T>& rhs);

    // Applies to angle
    template<typename T>
    Rotation<T> operator/(const Rotation<T>& lhs, const T& rhs);

    template<typename T>
    bool operator==(const Rotation<T>& lhs, const Rotation<T>& rhs);

    template<typename T>
    bool operator!=(const Rotation<T>& lhs, const Rotation<T>& rhs);

    template<typename T>
    typename Rotation<T>::vector_type& operator*=(typename Rotation<T>::vector_type& v, const Rotation<T>& rotation);

    template<typename T>
    typename Rotation<T>::vector_type operator*(typename Rotation<T>::vector_type v, const Rotation<T>& rotation);

    template<typename T>
    typename Rotation<T>::vector_type rotate(const typename Rotation<T>::vector_type& v, const Rotation<T>& rotation);

    template<typename T>
    typename Rotation<T>::vector_type slerp(const typename Rotation<T>::vector_type& lhs, const typename Rotation<T>::vector_type& rhs, T t);

    template<typename T>
    bool float_eq(const Rotation<T>& lhs, const Rotation<T>& rhs, int ulp = ulpDefault);

    template<typename T>
    std::istream& operator>>(std::istream& is, Rotation<T>& rotation);

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Rotation<T>& rotation);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>::Rotation() :
    m_rep{T(0), T(0), T(0), T(1)}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>::Rotation(const vector_type& axis, value_type angle) :
    m_rep()
{
    set(axis, angle);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>::Rotation(const Quaternion<T>& quaternion) :
    m_rep(quaternion)
{
    normalize();
}

//------------------------------------------------------------------------------
template<typename T>
constexpr static quetzal::math::Rotation<T> Identity()
{
    return Rotation<T>();
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator=(const Quaternion<T>& quaternion)
{
    m_rep = quaternion;
    normalize();
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::Rotation<T>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::Rotation<T>::operator-() const
{
    auto q = m_rep;
    q[3] = -q[3];
    return q;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator+=(value_type t)
{
    auto [axis, angle] = axis_angle();
    angle += t;
    set(axis, angle);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator-=(value_type t)
{
    auto [axis, angle] = axis_angle();
    angle -= t;
    set(axis, angle);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator*=(const Rotation<T>& other)
{
    m_rep *= other.m_rep;
    normalize();
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator*=(value_type t)
{
    auto [axis, angle] = axis_angle();
    angle *= t;
    set(axis, angle);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator/=(const Rotation<T>& other)
{
    *this *= other.inverse();
    normalize();
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::operator/=(value_type t)
{
    auto [axis, angle] = axis_angle();
    angle /= t;
    set(axis, angle);
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::vector_type quetzal::math::Rotation<T>::axis() const
{
    vector_type axis = {m_rep[0], m_rep[1], m_rep[2]};
    if (!float_eq0(axis.norm_squared()))
    {
        axis.normalize();
    }
    else
    {
        assert(float_eq(m_rep[3], T(1)));
        axis = vector_type::z_direction();
    }
    return axis;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::value_type quetzal::math::Rotation<T>::angle() const
{
    return std::acos(m_rep[3]) * 2;
}

//------------------------------------------------------------------------------
template<typename T>
std::tuple<typename quetzal::math::Rotation<T>::vector_type, typename quetzal::math::Rotation<T>::value_type> quetzal::math::Rotation<T>::axis_angle() const
{
    auto angle = this->angle();
    auto axis = this->axis();
    return std::make_tuple(axis, angle);
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Rotation<T>::set(const typename quetzal::math::Rotation<T>::vector_type& axis, typename quetzal::math::Rotation<T>::value_type angle)
{
    assert(float_eq(axis[3], T(0)));
    auto sin_angle = std::sin(angle / T(2));
    m_rep[0] = axis[0] * sin_angle;
    m_rep[1] = axis[1] * sin_angle;
    m_rep[2] = axis[2] * sin_angle;
    m_rep[3] = std::cos(angle / T(2));
    normalize();
    return;
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::math::Rotation<T>& quetzal::math::Rotation<T>::inverse() const
{
    auto r = *this;
    r.m_rep[0] = -r.m_rep[0];
    r.m_rep[1] = -r.m_rep[1];
    r.m_rep[2] = -r.m_rep[2];
    return r;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Rotation<T>::reset()
{
    m_rep = {T(0), T(0), T(0), T(1)};
    return;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Rotation<T>::identity(int ulp) const
{
    return float_eq0(m_rep[0], ulp) && float_eq0(m_rep[1], ulp) && float_eq0(m_rep[2], ulp) && float_eq(m_rep[3], T(1), ulp);
}

//------------------------------------------------------------------------------
template<typename T>
const typename quetzal::math::Rotation<T>::rep_type& quetzal::math::Rotation<T>::rep() const
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::rep_type& quetzal::math::Rotation<T>::rep()
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Rotation<T>::normalize()
{
    auto denom = m_rep.norm_squared();
    assert(!float_eq0(denom));
    if (!float_eq(denom, T(1)))
    {
        m_rep /= sqrt(denom);
    }
    return;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::operator*(Rotation<T> lhs, const Rotation<T>& rhs)
{
    lhs *= rhs;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::operator*(const Rotation<T>& lhs, const T& rhs)
{
    auto [axis, angle] = lhs.axis_angle();
    return Rotation<T>(axis, angle * rhs);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::operator*(const T& lhs, const Rotation<T>& rhs)
{
    auto [axis, angle] = rhs.axis_angle();
    return Rotation<T>(axis, angle * lhs);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::operator/(Rotation<T> lhs, const Rotation<T>& rhs)
{
    lhs /= rhs;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Rotation<T> quetzal::math::operator/(const Rotation<T>& lhs, const T& rhs)
{
    assert(!float_eq0(rhs));
    auto [axis, angle] = lhs.axis_angle();
    return Rotation<T>(axis, angle / rhs);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::operator==(const Rotation<T>& lhs, const Rotation<T>& rhs)
{
    return lhs.rep() == rhs.rep();
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::operator!=(const Rotation<T>& lhs, const Rotation<T>& rhs)
{
    return !(lhs == rhs);
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::vector_type& quetzal::math::operator*=(typename quetzal::math::Rotation<T>::vector_type& v, const quetzal::math::Rotation<T>& rotation)
{
    v = rotate(v, rotation);
    return v;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::vector_type quetzal::math::operator*(typename quetzal::math::Rotation<T>::vector_type v, const quetzal::math::Rotation<T>& rotation)
{
    v = rotate(v, rotation);
    return v;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::vector_type quetzal::math::rotate(const typename quetzal::math::Rotation<T>::vector_type& v, const quetzal::math::Rotation<T>& rotation)
{
    const auto& q = rotation.rep();
    typename Rotation<T>::vector_type vq = {q[0], q[1], q[2]};
    auto t = T(2) * cross(vq, v);
    return v + q[3] * t + cross(vq, t);
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::Rotation<T>::vector_type quetzal::math::slerp(const typename quetzal::math::Rotation<T>::vector_type& lhs, const typename quetzal::math::Rotation<T>::vector_type& rhs, T t)
{
    const auto& q1 = lhs.rep();
    const auto& q2 = rhs.rep();

    auto d = clamp11(dot(q1, q2));
    if (float_eq(d, T(1))) // might need greater buffer than default float_eq provides ...
    {
        return lerp(q1, q2, t);
    }

    auto theta0 = acos(d);
    auto theta1 = (T(1) - t) * theta0;
    auto theta2 = t * theta0;

    auto sin_theta0 = sin(theta0);
    auto s1 = sin(theta1) / sin_theta0;
    auto s2 = sin(theta2) / sin_theta0;
    return {s1 * q1 + s2 * q2};
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::float_eq(const Rotation<T>& lhs, const Rotation<T>& rhs, int ulp)
{
    return float_eq(lhs.rep(), rhs.rep(), ulp);
}

//------------------------------------------------------------------------------
template<typename T>
std::istream& quetzal::math::operator>>(std::istream& is, Rotation<T>& rotation)
{
    typename Rotation<T>::vector_type axis;
    T angle;
    is >> axis >> angle;
    rotation = {axis, angle};
    return is;    
}

//------------------------------------------------------------------------------
template<typename T>
std::ostream& quetzal::math::operator<<(std::ostream& os, const Rotation<T>& rotation)
{
    auto [axis, angle] = rotation.axis_angle();
    os << axis << ' ' << angle;
    return os;
}

#endif // QUETZAL_MATH_ROTATION_HPP
