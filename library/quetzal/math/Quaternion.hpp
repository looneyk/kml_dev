#if !defined(QUETZAL_MATH_QUATERNION_HPP)
#define QUETZAL_MATH_QUATERNION_HPP
//------------------------------------------------------------------------------
// math
// Quaternion.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include <array>
#include <iostream>
#include <span>
#include <cassert>

namespace quetzal::math
{

    // This uses (x, y, z, w) component ordering to be consistent with 3D graphics
    // As much as I would like to use the (w, x, y, z) component ordering as in mathematics to be consistent with complex (real, imag)

    //--------------------------------------------------------------------------
    template<typename T>
    class alignas(16) Quaternion
    {
    public:

        using value_type = T;
        static constexpr size_t dimension = 3;
        static constexpr size_t component_count = 4;
        using rep_type = std::array<value_type, component_count>;

        Quaternion();
        Quaternion(value_type x, value_type y, value_type z, value_type w);
        Quaternion(std::span<value_type, component_count> s);
        Quaternion(const rep_type& rep);
        Quaternion(const Quaternion&) = default;
        Quaternion(Quaternion&&) = default;
        virtual ~Quaternion() = default;

        Quaternion& operator=(const Quaternion&) = default;
        Quaternion& operator=(Quaternion&& other) = default;
        Quaternion& operator=(std::span<value_type, component_count> s);

        Quaternion operator+() const;
        Quaternion operator-() const;
        Quaternion& operator+=(const Quaternion& other);
        Quaternion& operator-=(const Quaternion& other);
        Quaternion& operator*=(const Quaternion& other);
        Quaternion& operator*=(value_type factor);
        Quaternion& operator/=(value_type divisor);

        value_type operator[](size_t i) const;
        value_type& operator[](size_t i);

        value_type x() const;
        value_type y() const;
        value_type z() const;
        value_type w() const;

        value_type& x();
        value_type& y();
        value_type& z();
        value_type& w();

        void set(value_type x, value_type y, value_type z, value_type w)
        {
            m_rep = {x, y, z, w};
            return;
        }

        template<size_t I> requires (I < component_count)
        void set(value_type u)
        {
            m_rep[I] = u;
            return;
        }

        template<size_t I> requires (I < Quaternion<T>::component_count)
        value_type get() const
        {
            return m_rep[I];
        }

        template<size_t I> requires (I < Quaternion<T>::component_count)
        value_type& get()
        {
            return m_rep[I];
        }

        value_type norm() const;
        value_type norm_squared() const;

        const Quaternion& conjugate() const;
        const Quaternion& inverse() const;

        void normalize();
        void clear();

        bool unit(int tolerance = tolerance_default) const;
        bool identity(int tolerance = tolerance_default) const;
        bool zero() const;
        bool float_zero(int tolerance = tolerance_default) const;

        const value_type* data() const;
        value_type* data();

        constexpr size_t size() const;

        const rep_type& rep() const;
        rep_type& rep();

    protected:

        alignas(16) rep_type m_rep;
    };

    template<typename T>
    Quaternion<T> operator+(Quaternion<T> lhs, const Quaternion<T>& rhs);

    template<typename T>
    Quaternion<T> operator-(Quaternion<T> lhs, const Quaternion<T>& rhs);

    template<typename T>
    Quaternion<T> operator*(Quaternion<T> lhs, const Quaternion<T>& rhs);

    template<typename T>
    Quaternion<T> operator*(Quaternion<T> lhs, const T& rhs);

    template<typename T>
    Quaternion<T> operator*(const T& lhs, Quaternion<T> rhs);

    template<typename T>
    Quaternion<T> operator/(Quaternion<T> lhs, const T& rhs);

    template<typename T>
    bool operator==(const Quaternion<T>& lhs, const Quaternion<T>& rhs);

    template<typename T>
    bool operator!=(const Quaternion<T>& lhs, const Quaternion<T>& rhs);

    template<typename T>
    Quaternion<T> dot(const Quaternion<T>& lhs, const Quaternion<T>& rhs);

    template<typename T>
    Quaternion<T> normalize(const Quaternion<T>& v);

    template<typename T>
    bool float_eq(const Quaternion<T>& lhs, Quaternion<T> rhs, int tolerance = tolerance_default);

    template<typename T>
    bool float_eq0(const Quaternion<T>& v, int tolerance = tolerance_default);

    template<typename T>
    std::istream& operator>>(std::istream& is, Quaternion<T>& rhs);

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Quaternion<T>& rhs);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::Quaternion() :
    m_rep()
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::Quaternion(value_type x, value_type y, value_type z, value_type w) :
    m_rep{x, y, z, w}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::Quaternion(std::span<value_type, component_count> s) :
    m_rep{s[0], s[1], s[2], s[3]}
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::Quaternion(const rep_type& rep) :
    m_rep(rep)
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator=(std::span<value_type, component_count> s)
{
    m_rep[0] = s[0];
    m_rep[1] = s[1];
    m_rep[2] = s[2];
    m_rep[3] = s[3];
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::Quaternion<T>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::Quaternion<T>::operator-() const
{
    return {-m_rep[0], -m_rep[1], -m_rep[2], -m_rep[3]};
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator+=(const Quaternion& other)
{
    m_rep[0] += other.m_rep[0];
    m_rep[1] += other.m_rep[1];
    m_rep[2] += other.m_rep[2];
    m_rep[3] += other.m_rep[3];
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator-=(const Quaternion& other)
{
    m_rep[0] -= other.m_rep[0];
    m_rep[1] -= other.m_rep[1];
    m_rep[2] -= other.m_rep[2];
    m_rep[3] -= other.m_rep[3];
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator*=(const Quaternion& other)
{
    m_rep = 
    {
        m_rep[3] * other.m_rep[3] - m_rep[0] * other.m_rep[0] - m_rep[1] * other.m_rep[1] - m_rep[2] * other.m_rep[2],
        m_rep[3] * other.m_rep[0] + m_rep[0] * other.m_rep[3] + m_rep[1] * other.m_rep[2] - m_rep[2] * other.m_rep[1],
        m_rep[3] * other.m_rep[1] - m_rep[0] * other.m_rep[2] + m_rep[1] * other.m_rep[3] + m_rep[2] * other.m_rep[0],
        m_rep[3] * other.m_rep[2] + m_rep[0] * other.m_rep[1] - m_rep[1] * other.m_rep[0] + m_rep[2] * other.m_rep[3],
    };            
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator*=(value_type factor)
{
    m_rep[0] *= factor;
    m_rep[1] *= factor;
    m_rep[2] *= factor;
    m_rep[3] *= factor;
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::operator/=(value_type divisor)
{
    m_rep[0] /= divisor;
    m_rep[1] /= divisor;
    m_rep[2] /= divisor;
    m_rep[3] /= divisor;
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::operator[](size_t i) const
{
    assert(i < component_count);
    return m_rep[i];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type& quetzal::math::Quaternion<T>::operator[](size_t i)
{
    assert(i < component_count);
    return m_rep[i];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::x() const
{
    return m_rep[0];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::y() const
{
    return m_rep[1];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::z() const
{
    return m_rep[2];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::w() const
{
    return m_rep[3];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type& quetzal::math::Quaternion<T>::x()
{
    return m_rep[0];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type& quetzal::math::Quaternion<T>::y()
{
    return m_rep[1];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type& quetzal::math::Quaternion<T>::z()
{
    return m_rep[2];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type& quetzal::math::Quaternion<T>::w()
{
    return m_rep[3];
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::norm() const
{
    return std::sqrt(norm_squared());
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type quetzal::math::Quaternion<T>::norm_squared() const
{
    return
        m_rep[0] * m_rep[0] +
        m_rep[1] * m_rep[1] +
        m_rep[2] * m_rep[2] +
        m_rep[3] * m_rep[3];
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::conjugate() const
{
    m_rep = {m_rep[0], -m_rep[1], -m_rep[2], -m_rep[3]};
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::math::Quaternion<T>& quetzal::math::Quaternion<T>::inverse() const
{
    *this = conjugate() / norm();
    return *this;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Quaternion<T>::normalize()
{
    auto denom = norm_squared();
    assert(!float_eq0(denom));
    if (!float_eq(denom, T(1)))
    {
        *this /= sqrt(denom);
    }
    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::Quaternion<T>::clear()
{
    m_rep.clear();
    return;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Quaternion<T>::unit(int tolerance) const
{
    return float_eq(norm_squared(), T(1), tolerance);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Quaternion<T>::identity(int tolerance) const
{
    return math::float_eq0(m_rep[0]) && math::float_eq0(m_rep[1]) && math::float_eq0(m_rep[2]) && math::float_eq(m_rep[3], T(1));
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Quaternion<T>::zero() const
{
    return m_rep[0] == T(0) && m_rep[1] == T(0) && m_rep[2] == T(0) && m_rep[3] == T(0);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::Quaternion<T>::float_zero(int tolerance) const
{
    return math::float_eq0(m_rep[0]) && math::float_eq0(m_rep[1]) && math::float_eq0(m_rep[2]) && math::float_eq0(m_rep[3]);
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::math::Quaternion<T>::value_type* quetzal::math::Quaternion<T>::data() const
{
    return m_rep.data();
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::value_type* quetzal::math::Quaternion<T>::data()
{
    return m_rep.data();
}

//------------------------------------------------------------------------------
template<typename T>
constexpr size_t quetzal::math::Quaternion<T>::size() const
{
    return component_count;
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::math::Quaternion<T>::rep_type& quetzal::math::Quaternion<T>::rep() const
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T>::rep_type& quetzal::math::Quaternion<T>::rep()
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator+(Quaternion<T> lhs, const Quaternion<T>& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator-(Quaternion<T> lhs, const Quaternion<T>& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator*(Quaternion<T> lhs, const Quaternion<T>& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator*(Quaternion<T> lhs, const T& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator*(const T& lhs, Quaternion<T> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::operator/(Quaternion<T> lhs, const T& rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::operator==(const Quaternion<T>& lhs, const Quaternion<T>& rhs)
{
    return lhs.rep() == rhs.rep();
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::operator!=(const Quaternion<T>& lhs, const Quaternion<T>& rhs)
{
    return !(lhs == rhs);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::dot(const Quaternion<T>& lhs, const Quaternion<T>& rhs)
{
    return
        get<0>(lhs) * get<0>(rhs) +
        get<1>(lhs) * get<1>(rhs) +
        get<2>(lhs) * get<2>(rhs) +
        get<3>(lhs) * get<3>(rhs);
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::math::Quaternion<T> quetzal::math::normalize(const Quaternion<T>& q)
{
    return q / q.norm();
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::float_eq(const Quaternion<T>& lhs, Quaternion<T> rhs, int tolerance)
{
    return float_equals(lhs.rep(), rhs.rep(), tolerance);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::math::float_eq0(const Quaternion<T>& q, int tolerance)
{
    return q.float_zero(tolerance);
}

//------------------------------------------------------------------------------
template<typename T>
std::istream& quetzal::math::operator>>(std::istream& is, Quaternion<T>& rhs)
{
    is >> get<0>(rhs);
    is >> get<1>(rhs);
    is >> get<2>(rhs);
    is >> get<3>(rhs);
    return is;
}

//------------------------------------------------------------------------------
template<typename T>
std::ostream& quetzal::math::operator<<(std::ostream& os, const Quaternion<T>& rhs)
{
    os << (get<0>(rhs) == T(0) ? T(0) : get<0>(rhs))
        << " " << (get<1>(rhs) == T(0) ? T(0) : get<1>(rhs))
        << " " << (get<2>(rhs) == T(0) ? T(0) : get<2>(rhs))
        << " " << (get<3>(rhs) == T(0) ? T(0) : get<3>(rhs));
    return os;
}

#endif // QUETZAL_MATH_QUATERNION_HPP


/*
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



class Quaternion {
public:
    // Internal memory is XYZW for your gaming/modeling context
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    // 1. Tagged Constructor ensures WXYZ safety at the API level
    struct wxyz_t {};
    static constexpr wxyz_t wxyz{};
    
    Quaternion(wxyz_t, float w_val, float x_val, float y_val, float z_val)
        : x(x_val), y(y_val), z(z_val), w(w_val) {}

    Quaternion() = default;

    // 2. Factory method that guarantees a valid "Versor" state
    static Quaternion FromAxisAngle(float angle_rad, float axis_x, float axis_y, float axis_z) {
        float half_angle = angle_rad * 0.5f;
        float s = std::sin(half_angle);
        
        // Constructs a guaranteed unit quaternion (versor)
        return Quaternion(wxyz, std::cos(half_angle), axis_x * s, axis_y * s, axis_z * s);
    }

    // 3. Normalization is explicit, avoiding hidden performance costs
    void normalize() {
        float mag_sq = x*x + y*y + z*z + w*w;
        if (mag_sq > 0.0f) {
            float inv_mag = 1.0f / std::sqrt(mag_sq);
            x *= inv_mag; y *= inv_mag; z *= inv_mag; w *= inv_mag;
        }
    }

    // 4. The Sandwich Product lives here, but strictly asserts validity
    struct Vector3 { float x, y, z; };
    
    Vector3 rotate_vector(const Vector3& v) const {
        // Assert ensures it acts as a Versor in debug mode without slowing down release mode
        #ifndef NDEBUG
        float mag_sq = x*x + y*y + z*z + w*w;
        const float epsilon = 0.001f;
        assert(std::abs(mag_sq - 1.0f) < epsilon && "Attempted to rotate a vector with a non-unit quaternion!");
        #endif

        // Optimized sandwich product: v' = v + 2w(q_xyz x v) + 2(q_xyz x (q_xyz x v))
        float qx_v_x = y * v.z - z * v.y;
        float qx_v_y = z * v.x - x * v.z;
        float qx_v_z = x * v.y - y * v.x;

        return Vector3{
            v.x + 2.0f * (w * qx_v_x + (y * qx_v_z - z * qx_v_y)),
            v.y + 2.0f * (w * qx_v_y + (z * qx_v_x - x * qx_v_z)),
            v.z + 2.0f * (w * qx_v_z + (x * qx_v_y - y * qx_v_x))
        };
    }
};

    // column major
    Matrix4x4 to_matrix() const {
        assert(std::abs(norm_squared() - 1.0f) < 0.001f && "Cannot convert non-unit quaternion to a rotation matrix!");

        // Pre-calculate squares and cross-products to maximize CPU cache performance
        float xx = x * x; float yy = y * y; float zz = z * z;
        float xy = x * y; float xz = x * z; float yz = y * z;
        float wx = w * x; float wy = w * y; float wz = w * z;

        Matrix4x4 mat;

        // Column 0
        mat[0]  = 1.0f - 2.0f * (yy + zz);
        mat[1]  = 2.0f * (xy + wz);
        mat[2]  = 2.0f * (xz - wy);
        mat[3]  = 0.0f;

        // Column 1
        mat[4]  = 2.0f * (xy - wz);
        mat[5]  = 1.0f - 2.0f * (xx + zz);
        mat[6]  = 2.0f * (yz + wx);
        mat[7]  = 0.0f;

        // Column 2
        mat[8]  = 2.0f * (xz + wy);
        mat[9]  = 2.0f * (yz - wx);
        mat[10] = 1.0f - 2.0f * (xx + yy);
        mat[11] = 0.0f;

        // Column 3 (Translation column for a pure rotation matrix)
        mat[12] = 0.0f;
        mat[13] = 0.0f;
        mat[14] = 0.0f;
        mat[15] = 1.0f;

        return mat;
    }

    Matrix4x4 to_matrix_row_major() const {
        assert(std::abs(norm_squared() - 1.0f) < 0.001f && "Cannot convert non-unit quaternion to a rotation matrix!");

        // Pre-calculate squared terms and cross-products
        float xx = x * x; float yy = y * y; float zz = z * z;
        float xy = x * y; float xz = x * z; float yz = y * z;
        float wx = w * x; float wy = w * y; float wz = w * z;

        Matrix4x4 mat;

        // Row 0
        mat[0]  = 1.0f - 2.0f * (yy + zz);
        mat[1]  = 2.0f * (xy - wz);
        mat[2]  = 2.0f * (xz + wy);
        mat[3]  = 0.0f;

        // Row 1
        mat[4]  = 2.0f * (xy + wz);
        mat[5]  = 1.0f - 2.0f * (xx + zz);
        mat[6]  = 2.0f * (yz - wx);
        mat[7]  = 0.0f;

        // Row 2
        mat[8]  = 2.0f * (xz - wy);
        mat[9]  = 2.0f * (yz + wx);
        mat[10] = 1.0f - 2.0f * (xx + yy);
        mat[11] = 0.0f;

        // Row 3 (Translation row)
        mat[12] = 0.0f;
        mat[13] = 0.0f;
        mat[14] = 0.0f;
        mat[15] = 1.0f;

        return mat;
    }
*/
