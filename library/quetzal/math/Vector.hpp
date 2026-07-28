#if !defined(QUETZAL_MATH_VECTOR_HPP)
#define QUETZAL_MATH_VECTOR_HPP
//------------------------------------------------------------------------------
// math
// Vector.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include "quetzal/common/string_util.hpp"
#include <charconv>
#include <concepts>
#include <iosfwd>
#include <format>
#include <optional>
#include <span>
#include <sstream>
#include <utility>
#include <cmath>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Vector
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using size_type = Traits::size_type;
        static constexpr size_type dimension = Traits::dimension;
        static constexpr size_type component_count = Traits::size;
        using rep_type = Traits::rep_type;

        Vector();
        Vector(std::span<value_type, dimension> s);
//        Vector(std::initializer_list<value_type> elements); // waiting for the fix ...
        Vector(const rep_type& rep);

        template<typename U> requires (Traits::dimension == 1) && std::convertible_to<U, typename Traits::value_type>
        Vector(const U& x);

        template<typename U> requires (Traits::dimension == 2) && std::convertible_to<U, typename Traits::value_type>
        Vector(const U& x, const U& y);

        template<typename U> requires (Traits::dimension == 3) && std::convertible_to<U, typename Traits::value_type>
        Vector(const U& x, const U& y, const U& z);

        template<typename U> requires (Traits::dimension == 4) && std::convertible_to<U, typename Traits::value_type>
        Vector(const U& x, const U& y, const U& z, const U& w);

        Vector(const Vector&) = default;
        Vector(Vector&&) = default;
        ~Vector() = default; // why is virtual destructor here causing errors? ...

        Vector& operator=(const Vector&) = default;
        Vector& operator=(Vector&&) = default;
        Vector& operator=(std::span<value_type, dimension> s);

        Vector operator+() const;
        Vector operator-() const;
        Vector& operator+=(const Vector& other);
        Vector& operator-=(const Vector& other);
        Vector& operator*=(const Vector& other); // Componentwise
        Vector& operator*=(value_type factor);
        Vector& operator/=(value_type divisor);

        value_type operator[](size_type i) const;
        value_type& operator[](size_type i);

        value_type x() const requires (Traits::size >= 1);
        value_type y() const requires (Traits::size >= 2);
        value_type z() const requires (Traits::size >= 3);
        value_type w() const requires (Traits::size >= 4);

        value_type& x() requires (Traits::size >= 1);
        value_type& y() requires (Traits::size >= 2);
        value_type& z() requires (Traits::size >= 3);
        value_type& w() requires (Traits::size >= 4);

        template<typename U> requires (Traits::size >= 1) && std::convertible_to<U, typename Traits::value_type>
        void set_x(const U& x);

        template<typename U> requires (Traits::size >= 2) && std::convertible_to<U, typename Traits::value_type>
        void set_y(const U& y);

        template<typename U> requires (Traits::size >= 3) && std::convertible_to<U, typename Traits::value_type>
        void set_z(const U& z);

        template<typename U> requires (Traits::size >= 4) && std::convertible_to<U, typename Traits::value_type>
        void set_w(const U& w);

        template<typename U> requires (Traits::dimension == 1) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x);

        template<typename U> requires (Traits::dimension == 2) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y);

        template<typename U> requires (Traits::dimension == 3) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y, const U& z);

        template<typename U> requires (Traits::dimension == 4) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y, const U& z, const U& w);

        template<size_type I> requires (I < Traits::size)
        value_type get() const;

        template<size_type I> requires (I < Traits::size)
        value_type& get();

        template<size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& u);

        value_type norm() const;
        value_type norm_squared() const;

        Vector direction() const;

        void normalize();
        void clear();

        bool valid() const;
        bool unit(int tolerance = tolerance_default) const;
        bool zero() const;
        bool float_zero(int tolerance = tolerance_default) const;

        // denoise is componentwise
        void denoise0(int tolerance = tolerance_default);
        void denoise01(int tolerance = tolerance_default);
        void denoise011(int tolerance = tolerance_default);

        size_type longest_component() const; // special operation, should probably be external, in application; prefers the rightmost component in case of tie ...

        constexpr const value_type* data() const noexcept;
        constexpr value_type* data() noexcept;

        constexpr size_type size() const;

        // rep should only be visible to functions that should actually be friends ...
        // rep should only be used in calls to traits functions
        const rep_type& rep() const;
        rep_type& rep();

        template<size_t I> requires (I < Traits::dimension)
        constexpr static Vector direction();

        constexpr static Vector direction(size_t i);

        constexpr static Vector x_direction() requires (Traits::dimension >= 1);
        constexpr static Vector y_direction() requires (Traits::dimension >= 2);
        constexpr static Vector z_direction() requires (Traits::dimension >= 3);

    private:

        rep_type m_rep;
    };

    template<typename Traits>
    Vector<Traits> operator+(Vector<Traits> lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    Vector<Traits> operator-(Vector<Traits> lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    Vector<Traits> operator*(Vector<Traits> lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    Vector<Traits> operator*(Vector<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    Vector<Traits> operator*(const typename Traits::value_type& lhs, Vector<Traits> rhs);

    template<typename Traits>
    Vector<Traits> operator/(Vector<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    bool operator==(const Vector<Traits>& lhs, Vector<Traits> rhs);

    template<typename Traits>
    typename Traits::value_type dot(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits> requires (Traits::dimension == 3)
    Vector<Traits> cross(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits> requires (Traits::dimension == 2)
    Vector<Traits> perp(const Vector<Traits>& v);

    // dot(perp(lhs), rhs)
    template<typename Traits> requires (Traits::dimension == 2)
    typename Traits::value_type dot_perp(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    Vector<Traits> normalize(const Vector<Traits>& v);

    template<typename Traits>
    bool parallel_unit(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool parallel(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool perpendicular(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    std::partial_ordering magnitude_compare(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    // greater: vectors are in the same direction (angle between them less than pi / 2
    // equivalent: vectors are perpendicular (angle between them is pi / 2
    // less: vectors are in the opposite direction (angle between them greater than pi / 2
    template<typename Traits>
    std::partial_ordering direction_compare(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool float_eq(const Vector<Traits>& lhs, Vector<Traits> rhs, int tolerance = tolerance_default);

    template<typename Traits>
    bool float_eq0(const Vector<Traits>& v, int tolerance = tolerance_default);

    template<typename Traits>
    bool ne_or_exact(const Vector<Traits>& lhs, Vector<Traits> rhs, int tolerance = tolerance_default);

    // denoise0 is componentwise
    template<typename Traits>
    Vector<Traits> denoise0(Vector<Traits> v, int tolerance = tolerance_default);

    // denoise01 is componentwise
    template<typename Traits>
    Vector<Traits> denoise01(Vector<Traits> v, int tolerance = tolerance_default);

    // denoise011 is componentwise
    template<typename Traits>
    Vector<Traits> denoise011(Vector<Traits> v, int tolerance = tolerance_default);

    // abs is componentwise
    template<typename Traits>
    Vector<Traits> abs(Vector<Traits> v);

    template<typename Traits>
    bool priority_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool all_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool any_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs);

    template<typename Traits>
    bool float_priority_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs, int tolerance);

    // These two operators are not symmetrical. extract parses numeric values separated by whitespace, and insert outputs a formatted point

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Vector<Traits>& v);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Vector<Traits>& v);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Vector<Traits>& v);

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Vector<Traits>& v);

    // Matrix operations

    template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::cols)
    Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> operator*(const M& lhs, const Vector<Traits>& rhs);

    template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
    Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> operator*(const Vector<Traits>& lhs, const M& rhs);

    template<typename Traits, typename M> requires (M::square && std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
    Vector<Traits>& operator*=(Vector<Traits>& lhs, const M& rhs);

    //--------------------------------------------------------------------------
    template<typename T>
    struct PriorityLess
    {
        bool operator()(const T& lhs, const T& rhs) const
        {
            return priority_less(lhs, rhs);
        }
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct AllLess
    {
        bool operator()(const T& lhs, const T& rhs) const
        {
            return all_less(lhs, rhs);
        }
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct AnyLess
    {
        bool operator()(const T& lhs, const T& rhs) const
        {
            return any_less(lhs, rhs);
        }
    };

    //--------------------------------------------------------------------------
    template<typename T>
    struct FloatPriorityLess
    {
        FloatPriorityLess() :
            FloatPriorityLess(2)
        {}

        FloatPriorityLess(int tolerance) :
            m_ulp(tolerance)
        {}

        bool operator()(const T& lhs, const T& rhs) const
        {
            return float_priority_less(lhs, rhs, m_ulp);
        }

        int m_ulp;
    };

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>::Vector() :
    m_rep()
{
    clear();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>::Vector(std::span<value_type, dimension> s) :
    m_rep()
{
    Traits::assign(m_rep, s);
    Traits::conclude<dimension>(m_rep, value_type(0));
}
/*
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>::Vector(std::initializer_list<value_type> elements) :
    m_rep{}
{
    assert(elements.size() == m_rep.size());

    auto k = elements.begin();
    for (size_t i = 0; i < m_rep.size(); ++i)
    {
        m_rep[i] = *k++;
    }
}
*/
//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>::Vector(const rep_type& rep) :
    m_rep(rep)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 1) && std::convertible_to<U, typename Traits::value_type>
quetzal::math::Vector<Traits>::Vector(const U& x) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x)});
    Traits::conclude<dimension>(m_rep, value_type(0));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 2) && std::convertible_to<U, typename Traits::value_type>
quetzal::math::Vector<Traits>::Vector(const U& x, const U& y) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y)});
    Traits::conclude<dimension>(m_rep, value_type(0));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 3) && std::convertible_to<U, typename Traits::value_type>
quetzal::math::Vector<Traits>::Vector(const U& x, const U& y, const U& z) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z)});
    Traits::conclude<dimension>(m_rep, value_type(0));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 4) && std::convertible_to<U, typename Traits::value_type>
quetzal::math::Vector<Traits>::Vector(const U& x, const U& y, const U& z, const U& w) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z), value_type(w)});
    Traits::conclude<dimension>(m_rep, value_type(0));
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator=(std::span<value_type, dimension> s)
{
    Traits::assign(m_rep, s);
    Traits::conclude<dimension>(m_rep, value_type(0));
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::operator+() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::operator-() const
{
    return Traits::unary_minus(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator+=(const Vector& other)
{
    Traits::assign_add(m_rep, other.m_rep);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator-=(const Vector& other)
{
    Traits::assign_subtract(m_rep, other.m_rep);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator*=(const Vector& other)
{
    Traits::assign_multiply(m_rep, other.m_rep);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator*=(value_type factor)
{
    Traits::assign_multiply(m_rep, factor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits>& quetzal::math::Vector<Traits>::operator/=(value_type divisor)
{
    Traits::assign_divide(m_rep, divisor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::operator[](size_type i) const
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::math::Vector<Traits>::operator[](size_type i)
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::x() const requires (Traits::size >= 1)
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::y() const requires (Traits::size >= 2)
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::z() const requires (Traits::size >= 3)
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::w() const requires (Traits::size >= 4)
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::math::Vector<Traits>::x() requires (Traits::size >= 1)
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::math::Vector<Traits>::y() requires (Traits::size >= 2)
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::math::Vector<Traits>::z() requires (Traits::size >= 3)
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::math::Vector<Traits>::w() requires (Traits::size >= 4)
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 1) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set_x(const U& x)
{
    Traits::get<0>(m_rep) = x;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 2) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set_y(const U& y)
{
    Traits::get<1>(m_rep) = y;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 3) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set_z(const U& z)
{
    Traits::get<2>(m_rep) = z;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 4) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set_w(const U& w)
{
    Traits::get<3>(m_rep) = w;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 1) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set(const U& x)
{
    Traits::assign(m_rep, {value_type(x)});
    Traits::conclude<dimension>(m_rep, value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 2) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set(const U& x, const U& y)
{
    Traits::assign(m_rep, {value_type(x), value_type(y)});
    Traits::conclude<dimension>(m_rep, value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 3) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set(const U& x, const U& y, const U& z)
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z)});
    Traits::conclude<dimension>(m_rep, value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::dimension == 4) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set(const U& x, const U& y, const U& z, const U& w)
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z), value_type(w)});
    Traits::conclude<dimension>(m_rep, value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I> requires (I < Traits::size)
typename Traits::value_type quetzal::math::Vector<Traits>::get() const
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I> requires (I < Traits::size)
typename Traits::value_type& quetzal::math::Vector<Traits>::get()
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
void quetzal::math::Vector<Traits>::set(const U& u)
{
    Traits::get<I>(m_rep) = value_type(u);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::norm() const
{
    return std::sqrt(norm_squared());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::Vector<Traits>::norm_squared() const
{
    return Traits::dot_product(this->rep(), this->rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::direction() const
{
    return math::normalize(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Vector<Traits>::normalize()
{
    *this /= norm();
    /* periodically verify ...
    auto denominator = norm();
    assert(!float_eq0(denominator));
    *this /= denominator;
    */
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Vector<Traits>::clear()
{
    Traits::clear(m_rep);
    Traits::conclude<dimension>(m_rep, value_type(0));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Vector<Traits>::valid() const
{
    return Traits::valid(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Vector<Traits>::unit(int tolerance) const
{
    return float_eq(norm_squared(), value_type(1), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Vector<Traits>::zero() const
{
    return Traits::zero(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::Vector<Traits>::float_zero(int tolerance) const
{
    return Traits::float_zero(m_rep, tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Vector<Traits>::denoise0(int tolerance)
{
    Traits::denoise0(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Vector<Traits>::denoise01(int tolerance)
{
    Traits::denoise01(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::math::Vector<Traits>::denoise011(int tolerance)
{
    Traits::denoise011(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
Traits::size_type quetzal::math::Vector<Traits>::longest_component() const
{
    value_type s = std::abs((*this)[0]);
    size_type nComponentMax = 0;

    for (size_type nComponent = 1; nComponent < Traits::dimension; ++nComponent)
    {
        value_type t = std::abs((*this)[nComponent]);
        if (t >= s)
        {
            s = t;
            nComponentMax = nComponent;
        }
    }

    return nComponentMax;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr const quetzal::math::Vector<Traits>::value_type* quetzal::math::Vector<Traits>::data() const noexcept
{
    return Traits::data(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Vector<Traits>::value_type* quetzal::math::Vector<Traits>::data() noexcept
{
    return Traits::data(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr Traits::size_type quetzal::math::Vector<Traits>::size() const
{
    return Traits::size;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename Traits::rep_type& quetzal::math::Vector<Traits>::rep() const
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::rep_type& quetzal::math::Vector<Traits>::rep()
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_t I> requires (I < Traits::dimension)
constexpr quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::direction()
{
    quetzal::math::Vector<Traits> v{};
    v[I] = value_type(1);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::direction(size_t i)
{
    assert(i < Traits::dimension);
    quetzal::math::Vector<Traits> v{};
    v[i] = value_type(1);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::x_direction() requires (Traits::dimension >= 1)
{
    Vector<Traits> v;
    v[0] = value_type(1);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::y_direction() requires (Traits::dimension >= 2)
{
    Vector<Traits> v;
    v[1] = value_type(1);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::math::Vector<Traits> quetzal::math::Vector<Traits>::z_direction() requires (Traits::dimension >= 3)
{
    Vector<Traits> v;
    v[2] = value_type(1);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator+(Vector<Traits> lhs, const Vector<Traits>& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator-(Vector<Traits> lhs, const Vector<Traits>& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator*(Vector<Traits> lhs, const Vector<Traits>& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator*(Vector<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator*(const typename Traits::value_type& lhs, Vector<Traits> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::operator/(Vector<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::operator==(const Vector<Traits>& lhs, Vector<Traits> rhs)
{
    return Traits::equals(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::math::dot(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return Traits::dot_product(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::math::Vector<Traits> quetzal::math::cross(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return {lhs.y() * rhs.z() - lhs.z() * rhs.y(), lhs.z() * rhs.x() - lhs.x() * rhs.z(), lhs.x() * rhs.y() - lhs.y() * rhs.x()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 2)
quetzal::math::Vector<Traits> quetzal::math::perp(const Vector<Traits>& v)
{
    return {-v.y(), v.x()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 2)
typename Traits::value_type quetzal::math::dot_perp(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return lhs.y() * rhs.x() - lhs.x() * rhs.y();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::normalize(const Vector<Traits>& v)
{
    return v / v.norm();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::parallel_unit(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    assert(lhs.unit());
    assert(rhs.unit());
    return float_eq(lhs, rhs) || float_eq(lhs, -rhs);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::parallel(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return float_eq0(cross(lhs, rhs));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::perpendicular(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return float_eq0(dot(lhs, rhs));
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::math::magnitude_compare(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return float_compare(lhs.norm_squared(), rhs.norm_squared());
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::math::direction_compare(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    assert(float_gt0(lhs.norm_squared()));
    assert(float_gt0(rhs.norm_squared()));
    return float_compare0(dot(lhs, rhs));
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::float_eq(const Vector<Traits>& lhs, Vector<Traits> rhs, int tolerance)
{
    return Traits::float_equals(lhs.rep(), rhs.rep(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::float_eq0(const Vector<Traits>& v, int tolerance)
{
    return v.float_zero(tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::ne_or_exact(const Vector<Traits>& lhs, Vector<Traits> rhs, int tolerance)
{
    return !float_eq(lhs, rhs, tolerance) || lhs == rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::denoise0(Vector<Traits> v, int tolerance)
{
    v.denoise0(tolerance);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::denoise01(Vector<Traits> v, int tolerance)
{
    v.denoise01(tolerance);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::denoise011(Vector<Traits> v, int tolerance)
{
    v.denoise011(tolerance);
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::math::Vector<Traits> quetzal::math::abs(Vector<Traits> v)
{
    Traits::set_abs(v.rep());
    return v;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::priority_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return Traits::priority_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::all_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return Traits::all_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::any_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs)
{
    return Traits::any_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::math::float_priority_less(const Vector<Traits>& lhs, const Vector<Traits>& rhs, int tolerance)
{
    return Traits::float_priority_less(lhs.rep(), rhs.rep(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::math::operator>>(std::istream& is, quetzal::math::Vector<Traits>& v)
{
    return Traits::extract(is, v.rep()); // add provision for specific format, as is should infer format ...
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::math::operator<<(std::ostream& os, const quetzal::math::Vector<Traits>& v)
{
    os << to_string(v, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::math::from_chars(const char* first, const char* last, Vector<Traits>& v)
{
    auto result = Traits::from_chars(first, last, v.rep());
//    if (result) ...
    if (result.ec == std::errc{})
    {
        Traits::conclude<Traits::dimension>(v.rep(), typename Traits::value_type(0));
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::math::to_chars(char* first, char* last, const Vector<Traits>& v)
{
    return Traits::to_chars(first, last, v.rep());
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::cols)
quetzal::math::Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> quetzal::math::operator*(const M& lhs, const Vector<Traits>& rhs)
{
    Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> v;

    const typename M::value_type* plhs = lhs.data();
    for (size_t i = 0; i < M::rows; ++i)
    {
        for (size_t j = 0; j < M::cols; ++j)
        {
            v[i] += plhs[j] * rhs[j];
//            v[i] = fma(plhs[j], rhs[j], v[i]);
        }

        plhs += M::cols;
    }

    return v;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
quetzal::math::Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> quetzal::math::operator*(const Vector<Traits>& lhs, const M& rhs)
{
    Vector<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> v;

    const typename M::value_type* prhs = rhs.data();
    for (size_t i = 0; i < M::rows; ++i)
    {
        for (size_t j = 0; j < M::cols; ++j)
        {
            v[j] += lhs[i] * prhs[j];
//            v[j] = fma(lhs[i], prhs[j], v[j]);
        }

        prhs += M::cols;
    }

    return v;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (M::square && std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
quetzal::math::Vector<Traits>& quetzal::math::operator*=(Vector<Traits>& lhs, const M& rhs)
{
    Vector<Traits> v;

    for (size_t j = 0; j < Traits::size; ++j)
    {
        for (size_t i = 0; i < Traits::size; ++i)
        {
            v[j] += lhs[i] * rhs(i, j);
//            v[j] = fma(lhs[i], rhs(i, j), v[j]);
        }
    }

    lhs = v;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::math::Vector<Traits>>
{
    std::formatter<typename Traits::value_type> m_element_formatter;

    // Leading format-spec characters: an optional '[' or '(' selects a bracket pair to wrap the elements
    // ('[' -> "[ ]", '(' -> "( )"), followed by an optional run of characters drawn from ",;: _" that
    // becomes the literal element separator verbatim, with '_' standing in for a space (so the separator
    // can be spelled out unambiguously in a spec string without relying on trailing/embedded raw spaces,
    // e.g. "[,_" -> bracket '[' + separator ", "; "_:_"  -> separator " : "). Neither of these leading
    // sections appears in the output otherwise. Whatever remains of the spec after them is handed to the
    // element type's own formatter (precision, etc.).
    std::optional<std::pair<char, char>> m_object_delimiters;
    std::string m_element_delimiter = " ";

    constexpr auto parse(std::format_parse_context& ctx)
    {
        auto it = ctx.begin();

        if (it != ctx.end() && (*it == '[' || *it == '('))
        {
            m_object_delimiters = (*it == '[') ? std::make_pair('[', ']') : std::make_pair('(', ')');
            ++it;
        }

        std::string delimiter;
        bool has_delimiter = false;
        while (it != ctx.end() && (*it == ',' || *it == ';' || *it == ':' || *it == ' ' || *it == '_'))
        {
            delimiter += (*it == '_') ? ' ' : *it;
            has_delimiter = true;
            ++it;
        }

        if (has_delimiter)
        {
            m_element_delimiter = std::move(delimiter);
        }

        ctx.advance_to(it);
        return m_element_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::math::Vector<Traits>& vector, FormatContext& ctx) const
    {
        return Traits::format_to(ctx, m_element_formatter, vector.rep(), m_element_delimiter, m_object_delimiters);
    }
};

#endif // QUETZAL_MATH_VECTOR_HPP
