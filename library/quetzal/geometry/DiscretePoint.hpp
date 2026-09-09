#if !defined(QUETZAL_GEOMETRY_DISCRETEPOINT_HPP)
#define QUETZAL_GEOMETRY_DISCRETEPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// DiscretePoint.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "quetzal/math/DiscreteVector.hpp"
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
#include <cstdlib>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    class DiscretePoint
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_type dimension = N;
        using rep_type = std::array<value_type, dimension>;

        using vector_type = math::DiscreteVector<T, N>;

        DiscretePoint();
        DiscretePoint(const vector_type& v);

        DiscretePoint(std::span<value_type, dimension> s);
        DiscretePoint(const rep_type& rep);

        explicit DiscretePoint(value_type x) requires (N == 1);
        DiscretePoint(value_type x, value_type y) requires (N == 2);
        DiscretePoint(value_type x, value_type y, value_type z) requires (N == 3);
        DiscretePoint(value_type x, value_type y, value_type z, value_type w) requires (N == 4);

        DiscretePoint(const DiscretePoint&) = default;
        DiscretePoint(DiscretePoint&&) = default;
        ~DiscretePoint() = default;

        DiscretePoint& operator=(const DiscretePoint&) = default;
        DiscretePoint& operator=(DiscretePoint&&) = default;
        DiscretePoint& operator=(const value_type(&components)[dimension]); // span? ...

        DiscretePoint& operator+=(const vector_type& other);
        DiscretePoint& operator-=(const vector_type& other);
        DiscretePoint& operator*=(value_type f);
        DiscretePoint& operator/=(value_type f);

        value_type operator[](size_type i) const;
        value_type& operator[](size_type i);

        value_type x() const requires (N >= 1);
        value_type y() const requires (N >= 2);
        value_type z() const requires (N >= 3);
        value_type w() const requires (N >= 4);

        value_type& x() requires (N >= 1);
        value_type& y() requires (N >= 2);
        value_type& z() requires (N >= 3);
        value_type& w() requires (N >= 4);

        // The same components under the grid names, for code that indexes rather than measures
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

        template<size_type I> requires (I < N)
        value_type get() const;

        template<size_type I> requires (I < N)
        value_type& get();

        template<size_type I> requires (I < N)
        void set(value_type u);

        DiscretePoint position() const; // For compatibility with Positional concept

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
    DiscretePoint<T, N> operator+(DiscretePoint<T, N> lhs, const typename DiscretePoint<T, N>::vector_type& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscretePoint<T, N> operator+(const typename DiscretePoint<T, N>::vector_type& lhs, DiscretePoint<T, N> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscretePoint<T, N> operator-(DiscretePoint<T, N> lhs, const typename DiscretePoint<T, N>::vector_type& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    typename DiscretePoint<T, N>::vector_type operator-(DiscretePoint<T, N> lhs, const DiscretePoint<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscretePoint<T, N> operator*(DiscretePoint<T, N> lhs, std::type_identity_t<T> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscretePoint<T, N> operator*(std::type_identity_t<T> lhs, DiscretePoint<T, N> rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscretePoint<T, N> operator/(DiscretePoint<T, N> lhs, std::type_identity_t<T> rhs);

    // A total order over the lattice, supplying <, >, <= and >=. The highest axis decides
    // first and the lowest last, as the discrete vector orders
    template<std::signed_integral T, size_t N> requires (N > 0)
    std::strong_ordering operator<=>(const DiscretePoint<T, N>& lhs, const DiscretePoint<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    bool operator==(const DiscretePoint<T, N>& lhs, const DiscretePoint<T, N>& rhs);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscretePoint<T, N>& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, const DiscretePoint<T, N>& v);

    // Whitespace separated components, as operator<< writes them. A component this cannot read
    // leaves the ones before it assigned and reports where it stopped, which is what
    // std::from_chars does with a single value
    template<std::signed_integral T, size_t N> requires (N > 0)
    std::from_chars_result from_chars(const char* first, const char* last, DiscretePoint<T, N>& point);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::to_chars_result to_chars(char* first, char* last, const DiscretePoint<T, N>& point);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint() :
    m_components{}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(const vector_type& v) :
    m_components(v.rep())
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(std::span<value_type, dimension> s) :
    m_components(s)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(const rep_type& rep) :
    m_components(rep)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(value_type x) requires (N == 1) :
    m_components{x}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(value_type x, value_type y) requires (N == 2) :
    m_components{x, y}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(value_type x, value_type y, value_type z) requires (N == 3) :
    m_components{x, y, z}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>::DiscretePoint(value_type x, value_type y, value_type z, value_type w) requires (N == 4) :
    m_components{x, y, z, w}
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>& quetzal::geometry::DiscretePoint<T, N>::operator=(const value_type(&components)[dimension])
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] = components[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>& quetzal::geometry::DiscretePoint<T, N>::operator+=(const vector_type& other)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] += other[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>& quetzal::geometry::DiscretePoint<T, N>::operator-=(const vector_type& other)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] -= other[i];
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>& quetzal::geometry::DiscretePoint<T, N>::operator*=(value_type f)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] *= f;
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N>& quetzal::geometry::DiscretePoint<T, N>::operator/=(value_type f)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_components[i] /= f;
    }

    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::operator[](size_type i) const
{
    assert(i < dimension);
    return m_components[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::operator[](size_type i)
{
    assert(i < dimension);
    return m_components[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::x() const requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::y() const requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::z() const requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::w() const requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::x() requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::y() requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::z() requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::w() requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::i() const requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::j() const requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::k() const requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::l() const requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::i() requires (N >= 1)
{
    return m_components[0];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::j() requires (N >= 2)
{
    return m_components[1];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::k() requires (N >= 3)
{
    return m_components[2];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::l() requires (N >= 4)
{
    return m_components[3];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set_x(value_type x) requires (N >= 1)
{
    m_components[0] = x;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set_y(value_type y) requires (N >= 2)
{
    m_components[1] = y;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set_z(value_type z) requires (N >= 3)
{
    m_components[2] = z;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set_w(value_type w) requires (N >= 4)
{
    m_components[3] = w;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set(value_type x) requires (N == 1)
{
    m_components[0] = x;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set(value_type x, value_type y) requires (N == 2)
{
    m_components[0] = x;
    m_components[1] = y;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set(value_type x, value_type y, value_type z) requires (N == 3)
{
    m_components[0] = x;
    m_components[1] = y;
    m_components[2] = z;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::set(value_type x, value_type y, value_type z, value_type w) requires (N == 4)
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
typename quetzal::geometry::DiscretePoint<T, N>::value_type quetzal::geometry::DiscretePoint<T, N>::get() const
{
    return m_components[I];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
template<size_t I> requires (I < N)
typename quetzal::geometry::DiscretePoint<T, N>::value_type& quetzal::geometry::DiscretePoint<T, N>::get()
{
    return m_components[I];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
template<size_t I> requires (I < N)
void quetzal::geometry::DiscretePoint<T, N>::set(value_type u)
{
    m_components[I] = u;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::DiscretePoint<T, N>::position() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscretePoint<T, N>::clear()
{
    m_components.fill(value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::geometry::DiscretePoint<T, N>::zero() const
{
    return std::all_of(m_components.begin(), m_components.end(), [](value_type v) { return v == value_type(0); });
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr const typename quetzal::geometry::DiscretePoint<T, N>::value_type* quetzal::geometry::DiscretePoint<T, N>::data() const noexcept
{
    return m_components.data();
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr typename quetzal::geometry::DiscretePoint<T, N>::value_type* quetzal::geometry::DiscretePoint<T, N>::data() noexcept
{
    return m_components.data();
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
constexpr typename quetzal::geometry::DiscretePoint<T, N>::size_type quetzal::geometry::DiscretePoint<T, N>::size() const
{
    return dimension;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
const typename quetzal::geometry::DiscretePoint<T, N>::rep_type& quetzal::geometry::DiscretePoint<T, N>::rep() const
{
    return m_components;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::rep_type& quetzal::geometry::DiscretePoint<T, N>::rep()
{
    return m_components;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator+(DiscretePoint<T, N> lhs, const typename DiscretePoint<T, N>::vector_type& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator+(const typename DiscretePoint<T, N>::vector_type& lhs, DiscretePoint<T, N> rhs)
{
    return rhs += lhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator-(DiscretePoint<T, N> lhs, const typename DiscretePoint<T, N>::vector_type& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
// The displacement from rhs to lhs, which is what separates a position from one
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscretePoint<T, N>::vector_type quetzal::geometry::operator-(DiscretePoint<T, N> lhs, const DiscretePoint<T, N>& rhs)
{
    typename DiscretePoint<T, N>::vector_type v;

    for (size_t i = 0; i < N; ++i)
    {
        v[i] = lhs[i] - rhs[i];
    }

    return v;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator*(DiscretePoint<T, N> lhs, std::type_identity_t<T> rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator*(std::type_identity_t<T> lhs, DiscretePoint<T, N> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscretePoint<T, N> quetzal::geometry::operator/(DiscretePoint<T, N> lhs, std::type_identity_t<T> rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::strong_ordering quetzal::geometry::operator<=>(const DiscretePoint<T, N>& lhs, const DiscretePoint<T, N>& rhs)
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
bool quetzal::geometry::operator==(const DiscretePoint<T, N>& lhs, const DiscretePoint<T, N>& rhs)
{
    return lhs.rep() == rhs.rep();
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::istream& quetzal::geometry::operator>>(std::istream& is, DiscretePoint<T, N>& v)
{
    for (size_t i = 0; i < N; ++i)
    {
        is >> v[i];
    }

    return is;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const DiscretePoint<T, N>& v)
{
    for (size_t i = 0; i < N; ++i)
    {
        os << (i > 0 ? " " : "") << v[i];
    }

    return os;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, DiscretePoint<T, N>& point)
{
    return array_from_chars(first, last, point);
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const DiscretePoint<T, N>& point)
{
    return array_to_chars(first, last, point);
}

// Structured bindings

//------------------------------------------------------------------------------
template<typename T, size_t N>
struct std::tuple_size<quetzal::geometry::DiscretePoint<T, N>> : std::integral_constant<size_t, N> {};

//------------------------------------------------------------------------------
template<size_t I, typename T, size_t N>
struct std::tuple_element<I, quetzal::geometry::DiscretePoint<T, N>> { using type = T; };

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
struct std::formatter<quetzal::geometry::DiscretePoint<T, N>> : quetzal::array_formatter<T>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::DiscretePoint<T, N>& point, FormatContext& ctx) const
    {
        return this->format_elements(point, ctx);
    }
};

#endif // QUETZAL_GEOMETRY_DISCRETEPOINT_HPP
