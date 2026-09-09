#if !defined(QUETZAL_GEOMETRY_DISCRETEBOX_HPP)
#define QUETZAL_GEOMETRY_DISCRETEBOX_HPP
//------------------------------------------------------------------------------
// geometry
// DiscreteBox.hpp
//------------------------------------------------------------------------------

#include "DiscretePoint.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <algorithm>
#include <charconv>
#include <concepts>
#include <iosfwd>
#include <istream>
#include <ostream>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    // A half open range on the lattice: lower is included, upper is not, so the extent is the
    // count of positions along each axis and a zero component makes the box empty
    template<std::signed_integral T, size_t N> requires (N > 0)
    class DiscreteBox
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_type dimension = N;

        using point_type = DiscretePoint<T, N>;
        using vector_type = math::DiscreteVector<T, N>;

        DiscreteBox();
        DiscreteBox(const point_type& position, const vector_type& extent);
        DiscreteBox(const point_type& lower, const point_type& upper);
        DiscreteBox(const DiscreteBox&) = default;
        DiscreteBox(DiscreteBox&&) = default;
        ~DiscreteBox() = default;

        DiscreteBox& operator=(const DiscreteBox&) = default;
        DiscreteBox& operator=(DiscreteBox&&) = default;

        DiscreteBox& operator+=(const vector_type& v);
        DiscreteBox& operator-=(const vector_type& v);

        friend bool operator==(const DiscreteBox&, const DiscreteBox&) = default;

        const point_type& position() const;
        point_type& position();
        void set_position(const point_type& position);

        const vector_type& extent() const;
        vector_type& extent();
        void set_extent(const vector_type& extent);

        void set_bounds(const point_type& lower, const point_type& upper);

        const point_type& lower() const;
        point_type upper() const;

        value_type lower(size_type i) const;
        value_type upper(size_type i) const;
        value_type extent(size_type i) const;

        point_type centroid() const;
        value_type volume() const;
        bool empty() const;

        void clear();

        bool contains(const point_type& point) const;
        bool contains(const DiscreteBox& other) const;

        // Moves the box without changing its extent
        void translate(const vector_type& v);

        // Adds margin to each side, lower and upper of each dimension
        void expand(value_type margin);

    private:

        point_type m_position;
        vector_type m_extent;
    };

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteBox<T, N> operator+(DiscreteBox<T, N> box, const typename DiscreteBox<T, N>::vector_type& v);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteBox<T, N> operator+(const typename DiscreteBox<T, N>::vector_type& v, DiscreteBox<T, N> box);

    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteBox<T, N> operator-(DiscreteBox<T, N> box, const typename DiscreteBox<T, N>::vector_type& v);

    // The boxes do not intersect
    template<std::signed_integral T, size_t N> requires (N > 0)
    bool disjoint(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b);

    // The minimum box that contains both
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteBox<T, N> op_union(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b);

    // Empty where the two do not overlap
    template<std::signed_integral T, size_t N> requires (N > 0)
    DiscreteBox<T, N> op_intersection(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscreteBox<T, N>& box);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, const DiscreteBox<T, N>& box);

    // The position followed by the extent, whitespace separated, as operator<< writes them
    template<std::signed_integral T, size_t N> requires (N > 0)
    std::from_chars_result from_chars(const char* first, const char* last, DiscreteBox<T, N>& box);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::to_chars_result to_chars(char* first, char* last, const DiscreteBox<T, N>& box);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N>::DiscreteBox() :
    m_position(),
    m_extent()
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N>::DiscreteBox(const point_type& position, const vector_type& extent) :
    m_position(position),
    m_extent(extent)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N>::DiscreteBox(const point_type& lower, const point_type& upper) :
    m_position(lower),
    m_extent(upper - lower)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N>& quetzal::geometry::DiscreteBox<T, N>::operator+=(const vector_type& v)
{
    m_position += v;
    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N>& quetzal::geometry::DiscreteBox<T, N>::operator-=(const vector_type& v)
{
    m_position -= v;
    return *this;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
const typename quetzal::geometry::DiscreteBox<T, N>::point_type& quetzal::geometry::DiscreteBox<T, N>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::point_type& quetzal::geometry::DiscreteBox<T, N>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
const typename quetzal::geometry::DiscreteBox<T, N>::vector_type& quetzal::geometry::DiscreteBox<T, N>::extent() const
{
    return m_extent;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::vector_type& quetzal::geometry::DiscreteBox<T, N>::extent()
{
    return m_extent;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::set_extent(const vector_type& extent)
{
    m_extent = extent;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::set_bounds(const point_type& lower, const point_type& upper)
{
    m_position = lower;
    m_extent = upper - lower;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
const typename quetzal::geometry::DiscreteBox<T, N>::point_type& quetzal::geometry::DiscreteBox<T, N>::lower() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::point_type quetzal::geometry::DiscreteBox<T, N>::upper() const
{
    return m_position + m_extent;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::value_type quetzal::geometry::DiscreteBox<T, N>::lower(size_type i) const
{
    assert(i < dimension);
    return m_position[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::value_type quetzal::geometry::DiscreteBox<T, N>::upper(size_type i) const
{
    assert(i < dimension);
    return m_position[i] + m_extent[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::value_type quetzal::geometry::DiscreteBox<T, N>::extent(size_type i) const
{
    assert(i < dimension);
    return m_extent[i];
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::point_type quetzal::geometry::DiscreteBox<T, N>::centroid() const
{
    point_type point;

    for (size_type i = 0; i < dimension; ++i)
    {
        point[i] = m_position[i] + m_extent[i] / 2;
    }

    return point;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteBox<T, N>::value_type quetzal::geometry::DiscreteBox<T, N>::volume() const
{
    value_type result = value_type(1);

    for (size_type i = 0; i < dimension; ++i)
    {
        result *= m_extent[i];
    }

    return result;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::geometry::DiscreteBox<T, N>::empty() const
{
    for (size_type i = 0; i < dimension; ++i)
    {
        if (m_extent[i] <= value_type(0))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::clear()
{
    m_position.clear();
    m_extent.clear();
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::geometry::DiscreteBox<T, N>::contains(const point_type& point) const
{
    for (size_type i = 0; i < dimension; ++i)
    {
        if (point[i] < lower(i) || point[i] >= upper(i))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::geometry::DiscreteBox<T, N>::contains(const DiscreteBox& other) const
{
    if (other.empty())
    {
        return true;
    }

    if (empty())
    {
        return false;
    }

    for (size_type i = 0; i < dimension; ++i)
    {
        if (other.lower(i) < lower(i) || other.upper(i) > upper(i))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::translate(const vector_type& v)
{
    m_position += v;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteBox<T, N>::expand(value_type margin)
{
    for (size_type i = 0; i < dimension; ++i)
    {
        m_position[i] -= margin;
        m_extent[i] += 2 * margin;
    }

    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N> quetzal::geometry::operator+(DiscreteBox<T, N> box, const typename DiscreteBox<T, N>::vector_type& v)
{
    return box += v;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N> quetzal::geometry::operator+(const typename DiscreteBox<T, N>::vector_type& v, DiscreteBox<T, N> box)
{
    return box += v;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N> quetzal::geometry::operator-(DiscreteBox<T, N> box, const typename DiscreteBox<T, N>::vector_type& v)
{
    return box -= v;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
bool quetzal::geometry::disjoint(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b)
{
    if (a.empty() || b.empty())
    {
        return true;
    }

    for (size_t i = 0; i < N; ++i)
    {
        if (a.upper(i) <= b.lower(i) || b.upper(i) <= a.lower(i))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N> quetzal::geometry::op_union(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b)
{
    if (a.empty())
    {
        return b;
    }

    if (b.empty())
    {
        return a;
    }

    typename DiscreteBox<T, N>::point_type lower;
    typename DiscreteBox<T, N>::point_type upper;

    for (size_t i = 0; i < N; ++i)
    {
        lower[i] = std::min(a.lower(i), b.lower(i));
        upper[i] = std::max(a.upper(i), b.upper(i));
    }

    return {lower, upper};
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteBox<T, N> quetzal::geometry::op_intersection(const DiscreteBox<T, N>& a, const DiscreteBox<T, N>& b)
{
    if (disjoint(a, b))
    {
        return {};
    }

    typename DiscreteBox<T, N>::point_type lower;
    typename DiscreteBox<T, N>::point_type upper;

    for (size_t i = 0; i < N; ++i)
    {
        lower[i] = std::max(a.lower(i), b.lower(i));
        upper[i] = std::min(a.upper(i), b.upper(i));
    }

    return {lower, upper};
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::istream& quetzal::geometry::operator>>(std::istream& is, DiscreteBox<T, N>& box)
{
    is >> box.position() >> box.extent();
    return is;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const DiscreteBox<T, N>& box)
{
    os << box.position() << " " << box.extent();
    return os;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, DiscreteBox<T, N>& box)
{
    auto result = from_chars(first, last, box.position());
    if (result.ec != std::errc {})
    {
        return result;
    }

    return math::from_chars(result.ptr, last, box.extent());
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const DiscreteBox<T, N>& box)
{
    auto result = to_chars(first, last, box.position());
    if (result.ec != std::errc {})
    {
        return result;
    }

    if (result.ptr == last)
    {
        return {result.ptr, std::errc::value_too_large};
    }

    *result.ptr++ = ' ';

    return math::to_chars(result.ptr, last, box.extent());
}

//------------------------------------------------------------------------------
// The position and then the extent, separated by the element delimiter, as operator<< writes them
template<std::signed_integral T, size_t N> requires (N > 0)
struct std::formatter<quetzal::geometry::DiscreteBox<T, N>> : quetzal::array_formatter<T>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::DiscreteBox<T, N>& box, FormatContext& ctx) const
    {
        auto out = this->format_elements(box.position(), ctx);
        ctx.advance_to(out);

        for (char c : this->m_element_delimiter)
        {
            *out++ = c;
        }

        ctx.advance_to(out);
        return this->format_elements(box.extent(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_DISCRETEBOX_HPP
