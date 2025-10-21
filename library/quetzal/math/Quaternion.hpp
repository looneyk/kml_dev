#if !defined(QUETZAL_MATH_QUATERNION_HPP)
#define QUETZAL_MATH_QUATERNION_HPP
//------------------------------------------------------------------------------
// math
// Quaternion.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include <array>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename Traits> requires (Traits::dimension == 3 && Traits::size == 4)
    class Quaternion
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using size_type = Traits::size_type;
        using rep_type = Traits::rep_type;
        static constexpr size_type dimension = Traits::dimension;

        Quaternion();
//        Quaternion(value_type a, value_type b, value_type c, value_type d);
        Quaternion(std::span<value_type, size> s);
//        Quaternion(std::initializer_list<value_type> elements); // waiting for the fix ...
        Quaternion(const rep_type& rep);
        Quaternion(const Quaternion&) = default;
        Quaternion(Quaternion&&) = default;
        virtual ~Quaternion() = default;

        Quaternion& operator=(const Quaternion&) = default;
        Quaternion& operator=(Quaternion&& other) = default;
        Quaternion& operator=(std::span<value_type, dimension> s);

        Quaternion operator+() const;
        Quaternion operator-() const;
        Quaternion& operator+=(const Quaternion& other);
        Quaternion& operator-=(const Quaternion& other);
        Quaternion& operator*=(const Quaternion& other);
        Quaternion& operator*=(value_type factor);
        Quaternion& operator/=(value_type divisor);

        value_type operator[](size_type i) const;
        value_type& operator[](size_type i);

        value_type a() const;
        value_type b() const;
        value_type c() const;
        value_type d() const;

        value_type& a();
        value_type& b();
        value_type& c();
        value_type& d();

        template<typename U> requires std::convertible_to<U, typename Traits::value_type>
        void set(const U& a, const U& b, const U& c, const U& d);

        template<size_type I> requires (I < Traits::size)
        value_type get() const;

        template<size_type I> requires (I < Traits::size)
        value_type& get();

        template<size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& u);

        value_type norm() const;
        value_type norm_squared() const;

        const Quaternion& conjugate() const;
        const Quaternion& inverse() const;

        void normalize();
        void clear();

        bool unit(int ulp = ulpDefault) const;
        bool zero() const;
        bool float_zero(int ulp = ulpDefault) const;

        // traits for data extraction, Traits::data(rep) ...
        const value_type* data() const;
        value_type* data();

        constexpr size_type size() const;

        // rep should only be visible to functions that should actually be friends ...
        // rep should only be used in calls to traits functions
        const rep_type& rep() const;
        rep_type& rep();

    protected:

        rep_type m_rep;
    };

    template<typename Traits>
    Quaternion<Traits> operator+(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs);

    template<typename Traits>
    Quaternion<Traits> operator-(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs);

    template<typename Traits>
    Quaternion<Traits> operator*(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs);

    template<typename Traits>
    Quaternion<Traits> operator*(Quaternion<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    Quaternion<Traits> operator*(const typename Traits::value_type& lhs, Quaternion<Traits> rhs);

    template<typename Traits>
    Quaternion<Traits> operator/(Quaternion<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    bool operator==(const Quaternion<Traits>& lhs, Quaternion<Traits> rhs);

    template<typename Traits>
    bool operator!=(const Quaternion<Traits>& lhs, const Quaternion<Traits>& rhs);

    template<typename Traits>
    Quaternion<Traits> normalize(const Quaternion<Traits>& v);

    template<typename Traits>
    bool float_eq(const Quaternion<Traits>& lhs, Quaternion<Traits> rhs, int ulp = ulpDefault);

    template<typename Traits>
    bool float_eq0(const Quaternion<Traits>& v, int ulp = ulpDefault);

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Quaternion<Traits>& v);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Quaternion<Traits>& v);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::Quaternion() :
    m_rep()
{
    clear();
}
/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::Quaternion(value_type a, value_type b, value_type c, value_type d) :
    m_rep()
{
}
*/
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::Quaternion(std::span<value_type, size> s) :
    m_rep{s[0], s[1], s[2], s[3]}
{
}
/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::Quaternion(std::initializer_list<value_type> elements) :
    m_rep(elements)
{
}
*/
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::Quaternion(const rep_type& rep) :
    m_rep(rep)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator=(std::span<value_type, dimension> s)
{
    Traits::assign(m_rep, s);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::Quaternion<Traits>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::Quaternion<Traits>::operator-() const
{
    Traits::unary_minus(m_rep);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator+=(const Quaternion& other)
{
    Traits::assign_add(m_rep, other);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator-=(const Quaternion& other)
{
    Traits::assign_subtract(m_rep, other);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator*=(const Quaternion& other)
{
    Traits::assign_multiply(m_rep, other);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator*=(value_type factor)
{
    Traits::assign_multiply(m_rep, factor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::operator/=(value_type divisor)
{
    Traits::assign_divide(m_rep, divisor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::operator[](size_type i) const
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::operator[](size_type i)
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::a() const
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::b() const
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::c() const
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::d() const
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::a()
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::b()
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::c()
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::d()
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Quaternion<Traits>::set(const U& a, const U& b, const U& c, const U& d)
{
    Traits::assign(m_rep, {value_type(a), value_type(b), value_type(c), value_type(d)});
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_type I> requires (I < Traits::size)
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::get() const
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_type I> requires (I < Traits::size)
quetzal::math::Quaternion<Traits>::value_type& quetzal::math::Quaternion<Traits>::get()
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Quaternion<Traits>::set(const U& u)
{
    Traits::get<I>(m_rep) = value_type(u);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::norm() const
{
    return std::sqrt(norm_squared());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type quetzal::math::Quaternion<Traits>::norm_squared() const
{
    return
        get<0>(m_rep) * get<0>(m_rep) +
        get<1>(m_rep) * get<1>(m_rep) +
        get<2>(m_rep) * get<2>(m_rep) +
        get<3>(m_rep) * get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::conjugate() const
{
    return Traits::assign(m_rep, {get<0>(m_rep), -get<1>(m_rep), -get<2>(m_rep), -get<3>(m_rep)});
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::math::Quaternion<Traits>& quetzal::math::Quaternion<Traits>::inverse() const
{
    *this = conjugate() / norm();
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Quaternion<Traits>::normalize()
{
    auto length = norm();
    if (!float_eq0(length)) // assert? ...
    {
        *this /= length;
    }
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Quaternion<Traits>::clear()
{
    Traits:::clear(m_rep);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Quaternion<Traits>::unit(int ulp) const
{
    return float_eq(norm_squared(), value_type(1), ulp);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Quaternion<Traits>::zero() const
{
    return Traits::zero(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Quaternion<Traits>::float_zero(int ulp) const
{
    return Traits::float_zero(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::math::Quaternion<Traits>::value_type* data() const
{
    return m_rep.data(); // Traits::data() ...
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::value_type* quetzal::math::Quaternion<Traits>::data()
{
    return m_rep.data(); // Traits::data() ...
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Quaternion<Traits>::size_type size() const
{
    return Traits::size;
}

//------------------------------------------------------------------------------
template<typename Traits>
const quetzal::math::Quaternion<Traits>::rep_type& rep() const
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits>::rep_type& quetzal::math::Quaternion<Traits>::rep()
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator+(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator-(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator*(Quaternion<Traits> lhs, const Quaternion<Traits>& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator*(Quaternion<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator*(const typename Traits::value_type& lhs, Quaternion<Traits> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::operator/(Quaternion<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::operator==(const Quaternion<Traits>& lhs, Quaternion<Traits> rhs)
{
    return Traits::equals(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::operator!=(const Quaternion<Traits>& lhs, const Quaternion<Traits>& rhs)
{
    return !(lhs == rhs);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Quaternion<Traits> quetzal::math::normalize(const Quaternion<Traits>& q)
{
    return q / q.norm();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::float_eq(const Quaternion<Traits>& lhs, Quaternion<Traits> rhs, int ulp)
{
    return Traits::float_equals(lhs.rep(), rhs.rep(), ulp);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::float_eq0(const Quaternion<Traits>& q, int ulp)
{
    return q.float_zero(ulp);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::math::operator>>(std::istream& is, Quaternion<Traits>& q)
{
    return Traits::extract(is, q.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::math::operator<<(std::ostream& os, const Quaternion<Traits>& q)
{
    return Traits::insert(os, q.rep());
}

#endif // QUETZAL_MATH_QUATERNION_HPP



// ---------------------------------------------------------------------
// construct the quaternion that rotates one vector to another
// uses the usual trick to get the half angle
Quaternion::Quaternion( const Vector3& vFrom, const Vector3& vTo )
{
    a = 1 + Dot( vTo, vFrom );

    Vector3 vec = Cross( vFrom, vTo );
    v0 = vec[0];
    v1 = vec[1];
    v2 = vec[2];
    normalize();
}

// angle+axis, axis must be normalized
Quaternion::Quaternion( float angleRadian, const Vector3& axis )
{
    float sina = sin(angleRadian / 2.f);
    a = cos(angleRadian / 2.f);

    v0 = sina * axis[0];
    v1 = sina * axis[1];
    v2 = sina * axis[2];
}

/// Q (x,0) Q*
// non-optimized
Vector3 Quaternion::rotate( const Vector3& x ) const
{
    const Quaternion& p = *this;

    // q = P (x,0)
    Quaternion q;
    q.v0 = p.a * x[0] + p.v1 * x[2] - p.v2 * x[1];
    q.v1 = p.a * x[1] + p.v2 * x[0] - p.v0 * x[2];
    q.v2 = p.a * x[2] + p.v0 * x[1] - p.v1 * x[0];

    q.a = - p.v0 * x[0] - p.v1 * x[1] - p.v2 * x[2];

    // r = q P*
    Vector3 r;
    r[0] = q.a * -p.v0 + p.a * q.v0  -  q.v1 * p.v2 + q.v2 * p.v1;
    r[1] = q.a * -p.v1 + p.a * q.v1  -  q.v2 * p.v0 + q.v0 * p.v2;
    r[2] = q.a * -p.v2 + p.a * q.v2  -  q.v0 * p.v1 + q.v1 * p.v0;

    return r;
}

// rotate a rotor by another
inline Quaternion Quaternion::rotate( const Quaternion& r ) const
{
    // should unwrap this for efficiency
    return (*this) * r * (*this).conjugate();
}

// convert to matrix
// non-optimized
inline Matrix3 Quaternion::toMatrix3() const
{
    Vector3 v0 = rotate( Vector3(1,0,0) );
    Vector3 v1 = rotate( Vector3(0,1,0) );
    Vector3 v2 = rotate( Vector3(0,0,1) );
    return Matrix3( v0, v1, v2 );
}
