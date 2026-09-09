#if !defined(QUETZAL_MATH_DISCRETEVECTOR_HPP)
#define QUETZAL_MATH_DISCRETEVECTOR_HPP
//------------------------------------------------------------------------------
// math
// DiscreteVector.hpp
//------------------------------------------------------------------------------

#include "Vector.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <compare>
#include <concepts>
#include <iosfwd>
#include <istream>
#include <ostream>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <cstdlib>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    class DiscreteVector
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_type dimension = N;
        using rep_type = std::array<value_type, dimension>;

        DiscreteVector();
        DiscreteVector(std::span<value_type, dimension> s);
        DiscreteVector(const rep_type& rep);

        explicit DiscreteVector(value_type x) requires (N == 1);
        DiscreteVector(value_type x, value_type y) requires (N == 2);
        DiscreteVector(value_type x, value_type y, value_type z) requires (N == 3);
        DiscreteVector(value_type x, value_type y, value_type z, value_type w) requires (N == 4);

        DiscreteVector(const DiscreteVector&) = default;
        DiscreteVector(DiscreteVector&&) = default;
        ~DiscreteVector() = default;

        DiscreteVector& operator=(const DiscreteVector&) = default;
        DiscreteVector& operator=(DiscreteVector&&) = default;
        DiscreteVector& operator=(const value_type(&components)[dimension]); // span ...

        DiscreteVector operator+() const;
        DiscreteVector operator-() const;
        DiscreteVector& operator+=(const DiscreteVector& other);
        DiscreteVector& operator-=(const DiscreteVector& other);
        DiscreteVector& operator*=(value_type f);
        DiscreteVector& operator/=(value_type f);

        value_type operator[](size_type i) const;
        value_type& operator[](size_type i);

        friend bool operator==(const DiscreteVector&, const DiscreteVector&) = default;

        value_type x() const requires (N >= 1);
        value_type y() const requires (N >= 2);
        value_type z() const requires (N >= 3);
        value_type w() const requires (N >= 4);

        value_type& x() requires (N >= 1);
        value_type& y() requires (N >= 2);
        value_type& z() requires (N >= 3);
        value_type& w() requires (N >= 4);

        // Aliases to support alternative nomenclature
        value_type i() const requires (N >= 1);
        value_type j() const requires (N >= 2);
        value_type k() const requires (N >= 3);
        value_type l() const requires (N >= 4);

        value_type& i() requires (N >= 1);
        value_type& j() requires (N >= 2);
        value_type& k() requires (N >= 3);
        value_type& l() requires (N >= 4);

        void set_x(value_type x) requires (N >= 1);
        void set_y(value_type y) requires (N >= 2);
        void set_z(value_type z) requires (N >= 3);
        void set_w(value_type w) requires (N >= 4);

        void set(value_type x) requires (N == 1);
        void set(value_type x, value_type y) requires (N == 2);
        void set(value_type x, value_type y, value_type z) requires (N == 3);
        void set(value_type x, value_type y, value_type z, value_type w) requires (N == 4);

        // The member form of the tuple protocol, which is what structured bindings use
        template<size_type I> requires (I < N)
        value_type get() const;

        template<size_type I> requires (I < N)
        value_type& get();

        template<size_type I> requires (I < N)
        void set(value_type u);

        // The unit vector oriented along the axis of the longest component, ties going to the higher order axis
        // Undefined for null vectors
        DiscreteVector direction() const;

        void clear();

        bool zero() const;

        constexpr const value_type* data() const noexcept;
        constexpr value_type* data() noexcept;

        constexpr size_type size() const;

        const rep_type& rep() const;
        rep_type& rep();

    private:

        rep_type m_components;
    };

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator+(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator-(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator*(std::type_identity_t<T> lhs, DiscreteVector<T, N> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> operator/(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::strong_ordering operator<=>(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscreteVector<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, const DiscreteVector<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::from_chars_result from_chars(const char* first, const char* last, DiscreteVector<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::to_chars_result to_chars(char* first, char* last, const DiscreteVector<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteVector<T, N> direction(const DiscreteVector<T, N>& v);

    // The unit vector oriented along the axis of the longest component, ties going to the higher order axis
    // Quantization of floating point vectors
    // Undefined for null vectors
    template<typename V, typename Traits> requires (Traits::dimension == V::dimension)
    V direction(const Vector<Traits>& v);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector() :
    m_components{}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(std::span<value_type, dimension> s) :
    m_components(s)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(const rep_type& rep) :
    m_components(rep)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(value_type x) requires (N == 1) :
    m_components{x}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(value_type x, value_type y) requires (N == 2) :
    m_components{x, y}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(value_type x, value_type y, value_type z) requires (N == 3) :
    m_components{x, y, z}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>::DiscreteVector(value_type x, value_type y, value_type z, value_type w) requires (N == 4) :
    m_components{x, y, z, w}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>& quetzal::math::DiscreteVector<T, N>::operator=(const value_type(&components)[dimension])
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] = components[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::DiscreteVector<T, N>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::DiscreteVector<T, N>::operator-() const
{
    DiscreteVector v;

    for (size_type i = 0; i < dimension; ++i)
    {
        v[i] = -m_components[i];
    }

    return v;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>& quetzal::math::DiscreteVector<T, N>::operator+=(const DiscreteVector& other)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] += other.m_components[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>& quetzal::math::DiscreteVector<T, N>::operator-=(const DiscreteVector& other)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] -= other.m_components[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>& quetzal::math::DiscreteVector<T, N>::operator*=(value_type f)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] *= f;
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N>& quetzal::math::DiscreteVector<T, N>::operator/=(value_type f)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] /= f;
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::operator[](size_type i) const
{
    assert(i < dimension);
    return m_components[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::operator[](size_type i)
{
    assert(i < dimension);
    return m_components[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::x() const requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::y() const requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::z() const requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::w() const requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::x() requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::y() requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::z() requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::w() requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::i() const requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::j() const requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::k() const requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::l() const requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::i() requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::j() requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::k() requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::l() requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set_x(value_type x) requires (N >= 1)
{
    m_components[0] = x;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set_y(value_type y) requires (N >= 2)
{
    m_components[1] = y;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set_z(value_type z) requires (N >= 3)
{
    m_components[2] = z;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set_w(value_type w) requires (N >= 4)
{
    m_components[3] = w;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set(value_type x) requires (N == 1)
{
    m_components[0] = x;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set(value_type x, value_type y) requires (N == 2)
{
    m_components[0] = x;
    m_components[1] = y;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set(value_type x, value_type y, value_type z) requires (N == 3)
{
    m_components[0] = x;
    m_components[1] = y;
    m_components[2] = z;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::set(value_type x, value_type y, value_type z, value_type w) requires (N == 4)
{
    m_components[0] = x;
    m_components[1] = y;
    m_components[2] = z;
    m_components[3] = w;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
template<size_t I> requires (I < N)
typename quetzal::math::DiscreteVector<T, N>::value_type quetzal::math::DiscreteVector<T, N>::get() const
{
    return m_components[I];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
template<size_t I> requires (I < N)
typename quetzal::math::DiscreteVector<T, N>::value_type& quetzal::math::DiscreteVector<T, N>::get()
{
    return m_components[I];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
template<size_t I> requires (I < N)
void quetzal::math::DiscreteVector<T, N>::set(value_type u)
{
    m_components[I] = u;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::DiscreteVector<T, N>::direction() const
{
    size_type axis = 0;
    value_type largest = value_type(0);

    // Ties go to the later axis, so the comparison is not strict
    for (size_type i = 0; i < dimension; ++i)
    {
        value_type magnitude = std::abs(m_components[i]);
        if (magnitude > value_type(0) && magnitude >= largest)
        {
            largest = magnitude;
            axis = i;
        }
    }

    if (largest == value_type(0))
    {
        return {};
    }

    DiscreteVector result;
    result[axis] = m_components[axis] > value_type(0) ? value_type(1) : value_type(-1);

    return result;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::math::DiscreteVector<T, N>::clear()
{
    m_components.fill(value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::math::DiscreteVector<T, N>::zero() const
{
    return std::all_of(m_components.begin(), m_components.end(), [](value_type v) { return v == value_type(0); });
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr const typename quetzal::math::DiscreteVector<T, N>::value_type* quetzal::math::DiscreteVector<T, N>::data() const noexcept
{
    return m_components.data();
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr typename quetzal::math::DiscreteVector<T, N>::value_type* quetzal::math::DiscreteVector<T, N>::data() noexcept
{
    return m_components.data();
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr typename quetzal::math::DiscreteVector<T, N>::size_type quetzal::math::DiscreteVector<T, N>::size() const
{
    return dimension;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
const typename quetzal::math::DiscreteVector<T, N>::rep_type& quetzal::math::DiscreteVector<T, N>::rep() const
{
    return m_components;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::math::DiscreteVector<T, N>::rep_type& quetzal::math::DiscreteVector<T, N>::rep()
{
    return m_components;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::operator+(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::operator-(DiscreteVector<T, N> lhs, const DiscreteVector<T, N>& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::operator*(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::operator*(std::type_identity_t<T> lhs, DiscreteVector<T, N> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::operator/(DiscreteVector<T, N> lhs, std::type_identity_t<T> rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::strong_ordering quetzal::math::operator<=>(const DiscreteVector<T, N>& lhs, const DiscreteVector<T, N>& rhs)
{
    for (size_t i = N; i > 0; --i)
    {
        std::strong_ordering result = lhs[i - 1] <=> rhs[i - 1];
        if (result != 0)
        {
            return result;
        }
    }

    return std::strong_ordering::equal;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::istream& quetzal::math::operator>>(std::istream& is, DiscreteVector<T, N>& v)
{
    for (size_t i = 0; i < N; ++i)
    {
        is >> v[i];
    }

    return is;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::ostream& quetzal::math::operator<<(std::ostream& os, const DiscreteVector<T, N>& v)
{
    for (size_t i = 0; i < N; ++i)
    {
        os << (i > 0 ? " " : "") << v[i];
    }

    return os;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::from_chars_result quetzal::math::from_chars(const char* first, const char* last, DiscreteVector<T, N>& v)
{
    return array_from_chars(first, last, v);
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::to_chars_result quetzal::math::to_chars(char* first, char* last, const DiscreteVector<T, N>& v)
{
    return array_to_chars(first, last, v);
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::math::DiscreteVector<T, N> quetzal::math::direction(const DiscreteVector<T, N>& v)
{
    return v.direction();
}

//------------------------------------------------------------------------------
template<typename V, typename Traits> requires (Traits::dimension == V::dimension)
V quetzal::math::direction(const Vector<Traits>& v)
{
    using component_type = typename Traits::value_type;
    using value_type = typename V::value_type;

    size_t axis = 0;
    component_type largest = component_type(0);

    // Ties go to the later axis, so the comparison is not strict
    for (size_t i = 0; i < V::dimension; ++i)
    {
        component_type magnitude = std::abs(v[i]);
        if (!float_eq0(magnitude) && magnitude >= largest)
        {
            largest = magnitude;
            axis = i;
        }
    }

    if (float_eq0(largest))
    {
        return {};
    }

    V result;
    result[axis] = v[axis] > component_type(0) ? value_type(1) : value_type(-1);

    return result;
}

// Structured bindings

//------------------------------------------------------------------------------
template<typename T, size_t N>
struct std::tuple_size<quetzal::math::DiscreteVector<T, N>> : std::integral_constant<size_t, N> {};

//------------------------------------------------------------------------------
template<size_t I, typename T, size_t N>
struct std::tuple_element<I, quetzal::math::DiscreteVector<T, N>> { using type = T; };

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
struct std::formatter<quetzal::math::DiscreteVector<T, N>> : quetzal::array_formatter<T>
{
    template<typename FormatContext>
    auto format(const quetzal::math::DiscreteVector<T, N>& v, FormatContext& ctx) const
    {
        return this->format_elements(v, ctx);
    }
};

#endif // QUETZAL_MATH_DISCRETEVECTOR_HPP
