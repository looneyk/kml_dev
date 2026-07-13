#if !defined(QUETZAL_GEOMETRY_POINT_HPP)
#define QUETZAL_GEOMETRY_POINT_HPP
//------------------------------------------------------------------------------
// geometry
// Point.hpp
//------------------------------------------------------------------------------

#include "Vector.hpp"
#include "quetzal/math/floating_point.hpp"
#include <concepts>
#include <iosfwd>
#include <format>
//#include <ranges>
#include <span>
#include <sstream>
//#include <string_view>
#include <cmath>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Point
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using size_type = Traits::size_type;
        static constexpr size_type dimension = Traits::dimension;
        static constexpr size_type component_count = Traits::size;
        using rep_type = Traits::rep_type;

        using vector_type = Vector<Traits>;

        Point();
        Point(const vector_type& v);

        template<size_t M> requires Traits::template accepts<M>
        Point(std::span<const value_type, M> values);

        Point(std::initializer_list<const value_type> elements); // waiting for the fix ...
        Point(const rep_type& rep);

        template<typename U> requires Traits::template accepts<1> && std::convertible_to<U, typename Traits::value_type>
        Point(const U& x);

        template<typename U> requires Traits::template accepts<2> && std::convertible_to<U, typename Traits::value_type>
        Point(const U& x, const U& y);

        template<typename U> requires Traits::template accepts<3> && std::convertible_to<U, typename Traits::value_type>
        Point(const U& x, const U& y, const U& z);

        template<typename U> requires Traits::template accepts<4> && std::convertible_to<U, typename Traits::value_type>
        Point(const U& x, const U& y, const U& z, const U& w);

        Point(const Point&) = default;
        Point(Point&&) = default;
        ~Point() = default;

        Point& operator=(const Point&) = default;
        Point& operator=(Point&&) = default;

        template<size_t M> requires Traits::template accepts<M>
        Point& operator=(std::span<value_type, M> values);

        Point& operator+=(const vector_type& other); // Translation shorthand
        Point& operator-=(const vector_type& other); // Translation shorthand
        Point& operator*=(value_type factor); // Scaling shorthand
        Point& operator/=(value_type divisor); // Scaling shorthand

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

        template<typename U> requires Traits::template accepts<1> && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x);

        template<typename U> requires Traits::template accepts<2> && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y);

        template<typename U> requires Traits::template accepts<3> && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y, const U& z);

        template<typename U> requires Traits::template accepts<4> && std::convertible_to<U, typename Traits::value_type>
        void set(const U& x, const U& y, const U& z, const U& w);

        template<size_type I> requires (I < Traits::size)
        value_type get() const;

        template<size_type I> requires (I < Traits::size)
        value_type& get();

        template<size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
        void set(const U& u);

        Point position() const; // For compatibility with Positional concept

        void clear();

        vector_type vector() const; // vector defined by origin to point (now with * and / scaling, see where we can get rid of this ...)

        bool valid() const;
        bool zero() const;
        bool float_zero(int tolerance = math::tolerance_default) const;

        // denoise, abs, min, max are componentwise
        void denoise0(int tolerance = math::tolerance_default);
        void denoise01(int tolerance = math::tolerance_default);
        void denoise011(int tolerance = math::tolerance_default);
        void abs();

        constexpr static Point origin();

        // Set all components to values from numeric_limits
        constexpr static Point lowest();
        constexpr static Point highest();

        constexpr const value_type* data() const noexcept;
        constexpr value_type* data() noexcept;

        constexpr size_type size() const;

        // rep should only be visible to functions that should actually be friends ...
        // rep should only be used in calls to traits functions
        const rep_type& rep() const;
        rep_type& rep();

        // reduce, projective_reduce, ...
        Point& projective_divide() requires (Traits::size == Traits::dimension + 1);

    private:

        rep_type m_rep;
    };

    template<typename Traits>
    Point<Traits> operator+(Point<Traits> lhs, const typename Point<Traits>::vector_type& rhs);

    template<typename Traits>
    Point<Traits> operator+(const typename Point<Traits>::vector_type& lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> operator-(Point<Traits> lhs, const typename Point<Traits>::vector_type& rhs);

    template<typename Traits>
    typename Point<Traits>::vector_type operator-(Point<Traits> lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> operator*(Point<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    Point<Traits> operator*(const typename Traits::value_type& lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> operator/(Point<Traits> lhs, const typename Traits::value_type& rhs);

    template<typename Traits>
    bool operator==(const Point<Traits>& lhs, Point<Traits> rhs);

    // Ordering is done using Traits::priority_less
    // points don't have length, but needed for sorting when used in map keys in wavefront_obj ...

    template<typename Traits>
    bool operator<(const Point<Traits>& lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> lerp(const Point<Traits>& a, const Point<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const Point<Traits>& lhs, Point<Traits> rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    bool float_eq0(const Point<Traits>& point, int tolerance = math::tolerance_default);

    template<typename Traits>
    bool epsilon_eq(const Point<Traits>& lhs, Point<Traits> rhs, typename Traits::value_type epsilon = typename Traits::value_type(0.001));

    template<typename Traits>
    bool ne_or_exact(const Point<Traits>& lhs, Point<Traits> rhs, int tolerance = math::tolerance_default);

    template<typename Traits> requires (Traits::dimension == 2)
    bool colinear(const Point<Traits>& a, const Point<Traits>& b, const Point<Traits>& c);

    template<typename Traits> requires (Traits::dimension == 3)
    bool colinear(const Point<Traits>& a, const Point<Traits>& b, const Point<Traits>& c);

    template<typename Traits>
    typename Point<Traits>::vector_type direction(const Point<Traits>& point);

    template<typename Traits>
    typename Point<Traits>::vector_type to_vector(const Point<Traits>& point);

    template<typename Traits>
    Point<Traits> to_point(const Vector<Traits>& v);

    // denoise, abs, min, max are componentwise

    template<typename Traits>
    Point<Traits> denoise0(Point<Traits> point, int tolerance = math::tolerance_default);

    template<typename Traits>
    Point<Traits> denoise01(Point<Traits> point, int tolerance = math::tolerance_default);

    template<typename Traits>
    Point<Traits> denoise011(Point<Traits> point, int tolerance = math::tolerance_default);

    template<typename Traits>
    Point<Traits> abs(Point<Traits> point);

    template<typename Traits>
    Point<Traits> min(const Point<Traits>& lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> max(const Point<Traits>& lhs, Point<Traits> rhs);

    template<typename Traits>
    Point<Traits> projective_divide(Point<Traits> point) requires (Traits::size == Traits::dimension + 1);

    template<typename Traits>
    bool priority_less(const Point<Traits>& lhs, const Point<Traits>& rhs);

    template<typename Traits>
    bool all_less(const Point<Traits>& lhs, const Point<Traits>& rhs);

    template<typename Traits>
    bool any_less(const Point<Traits>& lhs, const Point<Traits>& rhs);

    template<typename Traits>
    bool float_priority_less(const Point<Traits>& lhs, const Point<Traits>& rhs, int tolerance);

    template<typename Traits>
    std::string to_string(const Point<Traits>& point);

    // These two operators are not symmetrical. extrace parses numeric values separated by whitespace, and insert outputs a formatted point

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Point<Traits>& point);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Point<Traits>& point);

    // Matrix operations

    template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::cols)
    Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> operator*(const M& lhs, const Point<Traits>& rhs);

    template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
    Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> operator*(const Point<Traits>& lhs, const M& rhs);

    template<typename Traits, typename M> requires (M::square && std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
    Point<Traits>& operator*=(Point<Traits>& lhs, const M& rhs);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>::Point() :
    m_rep()
{
    clear();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>::Point(const vector_type& v) :
    m_rep(v.rep())
{
    Traits::conclude<dimension>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_t M> requires Traits::template accepts<M>
quetzal::geometry::Point<Traits>::Point(std::span<const value_type, M> values) :
    m_rep()
{
    Traits::assign(m_rep, values);
    Traits::conclude<M>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>::Point(std::initializer_list<const value_type> elements) :
    m_rep{}
{
    assert(elements.size() == Traits::size || elements.size() == Traits::dimension);

    auto k = elements.begin();
    for (size_t i = 0; i < Traits::dimension; ++i)
    {
        m_rep[i] = *k++;
    }

//    Traits::assign(m_rep, elements);
    Traits::conclude<Traits::dimension>(m_rep, value_type(1)); // any w is discarded ...
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>::Point(const rep_type& rep) :
    m_rep(rep)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<1> && std::convertible_to<U, typename Traits::value_type>
quetzal::geometry::Point<Traits>::Point(const U& x) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x)});
    Traits::conclude<1>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<2> && std::convertible_to<U, typename Traits::value_type>
quetzal::geometry::Point<Traits>::Point(const U& x, const U& y) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y)});
    Traits::conclude<2>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<3> && std::convertible_to<U, typename Traits::value_type>
quetzal::geometry::Point<Traits>::Point(const U& x, const U& y, const U& z) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z)});
    Traits::conclude<3>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<4> && std::convertible_to<U, typename Traits::value_type>
quetzal::geometry::Point<Traits>::Point(const U& x, const U& y, const U& z, const U& w) :
    m_rep()
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z), value_type(w)});
    Traits::conclude<4>(m_rep, value_type(1));
}

//------------------------------------------------------------------------------
template<typename Traits>
template<size_t M> requires Traits::template accepts<M>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::operator=(std::span<value_type, M> values)
{
    Traits::assign(m_rep, values);
    Traits::conclude<M>(m_rep, value_type(1));
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::operator+=(const vector_type& other)
{
    Traits::prepare(m_rep);
    Traits::assign_add(m_rep, other.rep());
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::operator-=(const vector_type& other)
{
    Traits::prepare(m_rep);
    Traits::assign_subtract(m_rep, other.rep());
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::operator*=(value_type factor)
{
    Traits::assign_multiply(m_rep, factor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::operator/=(value_type divisor)
{
    Traits::assign_divide(m_rep, divisor);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::geometry::Point<Traits>::operator[](size_type i) const
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::geometry::Point<Traits>::operator[](size_type i)
{
    assert(i < Traits::size);
    return Traits::get(m_rep, i);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::geometry::Point<Traits>::x() const requires (Traits::size >= 1)
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::geometry::Point<Traits>::y() const requires (Traits::size >= 2)
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::geometry::Point<Traits>::z() const requires (Traits::size >= 3)
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type quetzal::geometry::Point<Traits>::w() const requires (Traits::size >= 4)
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::geometry::Point<Traits>::x() requires (Traits::size >= 1)
{
    return Traits::get<0>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::geometry::Point<Traits>::y() requires (Traits::size >= 2)
{
    return Traits::get<1>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::geometry::Point<Traits>::z() requires (Traits::size >= 3)
{
    return Traits::get<2>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::value_type& quetzal::geometry::Point<Traits>::w() requires (Traits::size >= 4)
{
    return Traits::get<3>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 1) && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set_x(const U& x)
{
    Traits::get<0>(m_rep) = x;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 2) && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set_y(const U& y)
{
    Traits::get<1>(m_rep) = y;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 3) && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set_z(const U& z)
{
    Traits::get<2>(m_rep) = z;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires (Traits::size >= 4) && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set_w(const U& w)
{
    Traits::get<3>(m_rep) = w;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<1> && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set(const U& x)
{
    Traits::assign(m_rep, {value_type(x)});
    Traits::conclude<1>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<2> && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set(const U& x, const U& y)
{
    Traits::assign(m_rep, {value_type(x), value_type(y)});
    Traits::conclude<2>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<3> && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set(const U& x, const U& y, const U& z)
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z)});
    Traits::conclude<3>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename U> requires Traits::template accepts<4> && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set(const U& x, const U& y, const U& z, const U& w)
{
    Traits::assign(m_rep, {value_type(x), value_type(y), value_type(z), value_type(w)});
    Traits::conclude<4>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I> requires (I < Traits::size)
typename Traits::value_type quetzal::geometry::Point<Traits>::get() const
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I> requires (I < Traits::size)
typename Traits::value_type& quetzal::geometry::Point<Traits>::get()
{
    return Traits::get<I>(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
template<Traits::size_type I, typename U> requires (I < Traits::size) && std::convertible_to<U, typename Traits::value_type>
void quetzal::geometry::Point<Traits>::set(const U& u)
{
    Traits::get<I>(m_rep) = value_type(u);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::Point<Traits>::position() const
{
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Point<Traits>::clear()
{
    Traits::clear(m_rep);
    Traits::conclude<dimension>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Point<Traits>::vector_type quetzal::geometry::Point<Traits>::vector() const
{
    return to_vector(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Point<Traits>::valid() const
{
    return Traits::valid(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Point<Traits>::zero() const
{
    return Traits::zero(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Point<Traits>::float_zero(int tolerance) const
{
    return Traits::float_zero(m_rep, tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Point<Traits>::denoise0(int tolerance)
{
    Traits::denoise0(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Point<Traits>::denoise01(int tolerance)
{
    Traits::denoise01(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Point<Traits>::denoise011(int tolerance)
{
    Traits::denoise011(m_rep, tolerance);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Point<Traits>::abs()
{
// prepare, conclude? just apply abs to all size components ...
    Traits::prepare(m_rep);
    Traits::set_abs(m_rep);
    Traits::conclude<dimension>(m_rep, value_type(1));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::geometry::Point<Traits> quetzal::geometry::Point<Traits>::origin()
{
    return {};
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::geometry::Point<Traits> quetzal::geometry::Point<Traits>::lowest()
{
    Point<Traits> point;
    Traits::set_lowest(point.rep());
    Traits::conclude<dimension>(point.rep(), value_type(1));
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::geometry::Point<Traits> quetzal::geometry::Point<Traits>::highest()
{
    Point<Traits> point;
    Traits::set_highest(point.rep());
    Traits::conclude<dimension>(point.rep(), value_type(1));
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr const quetzal::geometry::Point<Traits>::value_type* quetzal::geometry::Point<Traits>::data() const noexcept
{
    return Traits::data(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr quetzal::geometry::Point<Traits>::value_type* quetzal::geometry::Point<Traits>::data() noexcept
{
    return Traits::data(m_rep);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr Traits::size_type quetzal::geometry::Point<Traits>::size() const
{
    return Traits::size;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename Traits::rep_type& quetzal::geometry::Point<Traits>::rep() const
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::rep_type& quetzal::geometry::Point<Traits>::rep()
{
    return m_rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits>& quetzal::geometry::Point<Traits>::projective_divide() requires (Traits::size == Traits::dimension + 1)
{
    Traits::prepare(m_rep);
    return *this;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator+(Point<Traits> lhs, const typename Point<Traits>::vector_type& rhs)
{
    return lhs += rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator+(const typename Point<Traits>::vector_type& lhs, Point<Traits> rhs)
{
    return rhs += lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator-(Point<Traits> lhs, const typename Point<Traits>::vector_type& rhs)
{
    return lhs -= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Point<Traits>::vector_type quetzal::geometry::operator-(Point<Traits> lhs, Point<Traits> rhs)
{
    Traits::prepare(lhs.rep());
    Traits::prepare(rhs.rep());
    Traits::assign_subtract(lhs.rep(), rhs.rep());
    return lhs.rep();
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator*(Point<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs *= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator*(const typename Traits::value_type& lhs, Point<Traits> rhs)
{
    return rhs *= lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::operator/(Point<Traits> lhs, const typename Traits::value_type& rhs)
{
    return lhs /= rhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::operator==(const Point<Traits>& lhs, Point<Traits> rhs)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return Traits::equals(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::operator<(const Point<Traits>& lhs, Point<Traits> rhs)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return Traits::priority_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::lerp(const Point<Traits>& a, const Point<Traits>& b, typename Traits::value_type t)
{
    if (t == Traits::value_type(1))
    {
        return b;
    }

    return a + t * (b - a);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const Point<Traits>& lhs, Point<Traits> rhs, int tolerance)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return Traits::float_equals(lhs.rep(), rhs.rep(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq0(const Point<Traits>& point, int tolerance)
{
    return point.float_zero(tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::epsilon_eq(const Point<Traits>& lhs, Point<Traits> rhs, typename Traits::value_type epsilon)
{
    // Traits based if this is useful ...

    for (size_t i = 0; i < Traits::size; ++i)
    {
        if (std::abs(lhs[i] - rhs[i]) > epsilon)
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::ne_or_exact(const Point<Traits>& lhs, Point<Traits> rhs, int tolerance)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return !Traits::float_equals(lhs.rep(), rhs.rep(), tolerance) || Traits::equals(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 2)
bool quetzal::geometry::colinear(const Point<Traits>& a, const Point<Traits>& b, const Point<Traits>& c)
{
    return math::float_eq0(math::dot_perp(b - a, c - a));
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
bool quetzal::geometry::colinear(const Point<Traits>& a, const Point<Traits>& b, const Point<Traits>& c)
{
    return math::float_eq0(math::cross(b - a, c - a).norm_squared());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Point<Traits>::vector_type quetzal::geometry::direction(const Point<Traits>& point)
{
    return normalize(denoise0(to_vector(point)));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Point<Traits>::vector_type quetzal::geometry::to_vector(const Point<Traits>& point)
{
    typename Traits::rep_type rep = point.rep();
    Traits::prepare(rep);
    Traits::conclude<Traits::dimension>(rep, Traits::value_type(0));
    return rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::to_point(const Vector<Traits>& v)
{
    typename Traits::rep_type rep = v.rep();
    Traits::conclude<Traits::dimension>(rep, Traits::value_type(1));
    return rep;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::denoise0(Point<Traits> point, int tolerance)
{
    point.denoise0(tolerance);
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::denoise01(Point<Traits> point, int tolerance)
{
    point.denoise01(tolerance);
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::denoise011(Point<Traits> point, int tolerance)
{
    point.denoise011(tolerance);
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::abs(Point<Traits> point)
{
    point.abs();
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::min(const Point<Traits>& lhs, Point<Traits> rhs)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return Traits::min(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::max(const Point<Traits>& lhs, Point<Traits> rhs)
{
    Traits::prepare(lhs.rep(), rhs.rep());
    return Traits::max(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Point<Traits> quetzal::geometry::projective_divide(Point<Traits> point) requires (Traits::size == Traits::dimension + 1)
{
    Traits::prepare(point.rep());
    return point;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::priority_less(const Point<Traits>& lhs, const Point<Traits>& rhs)
{
    return Traits::priority_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::all_less(const Point<Traits>& lhs, const Point<Traits>& rhs)
{
    return Traits::all_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::any_less(const Point<Traits>& lhs, const Point<Traits>& rhs)
{
    return Traits::any_less(lhs.rep(), rhs.rep());
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_priority_less(const Point<Traits>& lhs, const Point<Traits>& rhs, int tolerance)
{
    return Traits::float_priority_less(lhs.rep(), rhs.rep(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::geometry::to_string(const Point<Traits>& point)
{
    // insert/extract now handle typical formatting scenarios
    std::ostringstream oss;
    oss << point;
    return oss.str();
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Point<Traits>& point)
{
    return Traits::extract(is, point.rep()); // add provision for specific format, as is should infer format ...
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Point<Traits>& point)
{
    return Traits::insert(os, point.rep()); // add provision for specific format ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::cols)
quetzal::geometry::Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> quetzal::geometry::operator*(const M& lhs, const Point<Traits>& rhs)
{
    Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::rows>>> point;
    Traits::conclude<Traits::dimension>(point.rep(), Traits::value_type(0)); // ? ...

    const typename M::value_type* plhs = lhs.data();
    for (size_t i = 0; i < M::rows; ++i)
    {
        for (size_t j = 0; j < M::cols; ++j)
        {
            point[i] += plhs[j] * rhs[j];
        }

        plhs += M::cols;
    }

    return point;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
quetzal::geometry::Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> quetzal::geometry::operator*(const Point<Traits>& lhs, const M& rhs)
{
    Point<typename Traits::template resized_traits<Traits::template effective_dimension<M::cols>>> point;
    Traits::conclude<Traits::dimension>(point.rep(), Traits::value_type(0)); // ? ...

    const typename M::value_type* prhs = rhs.data();
    for (size_t i = 0; i < M::rows; ++i)
    {
        for (size_t j = 0; j < M::cols; ++j)
        {
            point[j] += lhs[i] * prhs[j];
        }

        prhs += M::cols;
    }

    return point;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M> requires (M::square && std::same_as<typename M::value_type, typename Traits::value_type> && Traits::size == M::rows)
quetzal::geometry::Point<Traits>& quetzal::geometry::operator*=(Point<Traits>& lhs, const M& rhs)
{
    Point<Traits> point;
    Traits::conclude<Traits::dimension>(point.rep(), Traits::value_type(0)); // ? ...

    for (size_t j = 0; j < Traits::size; ++j)
    {
        for (size_t i = 0; i < Traits::size; ++i)
        {
            point[j] += lhs[i] * rhs(i, j);
        }
    }

    lhs = point;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Point<Traits>> : std::formatter<std::string>
{
    auto format(const quetzal::geometry::Point<Traits>& point, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(to_string(point), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_POINT_HPP
