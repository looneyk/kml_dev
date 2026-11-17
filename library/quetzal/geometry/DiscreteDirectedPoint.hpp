#if !defined(QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// DiscreteDirectedPoint.hpp
//------------------------------------------------------------------------------

#include "DiscretePoint.hpp"
#include "quetzal/common/string_util.hpp"
#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <system_error>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<std::signed_integral T, size_t N> requires (N > 0)
    class DiscreteDirectedPoint
    {
    public:

        using value_type = T;
        using point_type = DiscretePoint<T, N>;
        using direction_type = point_type::vector_type;

        DiscreteDirectedPoint() = default;
        DiscreteDirectedPoint(const point_type& position, const direction_type& direction);
        DiscreteDirectedPoint(const DiscreteDirectedPoint&) = default;
        ~DiscreteDirectedPoint() = default;

        DiscreteDirectedPoint& operator=(const DiscreteDirectedPoint&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        direction_type direction() const;
        direction_type& direction();
        void set_direction(const direction_type& direction);

    private:

        point_type m_position;
        direction_type m_direction;
    };

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::ostream& operator<<(std::ostream& os, DiscreteDirectedPoint<T, N> directed_point);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::istream& operator>>(std::istream& is, DiscreteDirectedPoint<T, N>& directed_point);

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::from_chars_result from_chars(const char* first, const char* last, DiscreteDirectedPoint<T, N>& directed_point, const FormatSpec& spec = {});

    template<std::signed_integral T, size_t N> requires (N > 0)
    std::to_chars_result to_chars(char* first, char* last, const DiscreteDirectedPoint<T, N>& directed_point, const FormatSpec& spec = {});

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
quetzal::geometry::DiscreteDirectedPoint<T, N>::DiscreteDirectedPoint(const point_type& position, const direction_type& direction) :
    m_position(position),
    m_direction(direction)
{
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteDirectedPoint<T, N>::point_type quetzal::geometry::DiscreteDirectedPoint<T, N>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteDirectedPoint<T, N>::point_type& quetzal::geometry::DiscreteDirectedPoint<T, N>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteDirectedPoint<T, N>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteDirectedPoint<T, N>::direction_type quetzal::geometry::DiscreteDirectedPoint<T, N>::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
typename quetzal::geometry::DiscreteDirectedPoint<T, N>::direction_type& quetzal::geometry::DiscreteDirectedPoint<T, N>::direction()
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
void quetzal::geometry::DiscreteDirectedPoint<T, N>::set_direction(const direction_type& direction)
{
    m_direction = direction;
    return;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, DiscreteDirectedPoint<T, N> directed_point)
{
    os << quetzal::to_string(directed_point, os);
    return os;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::istream& quetzal::geometry::operator>>(std::istream& is, DiscreteDirectedPoint<T, N>& directed_point)
{
    typename DiscreteDirectedPoint<T, N>::point_type position;
    typename DiscreteDirectedPoint<T, N>::direction_type direction;
    if (quetzal::read_components(is,position, direction))
    {
        directed_point = DiscreteDirectedPoint<T, N>(position, direction);
    }

    return is;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, DiscreteDirectedPoint<T, N>& directed_point, const FormatSpec& spec)
{
    typename DiscreteDirectedPoint<T, N>::point_type position;
    typename DiscreteDirectedPoint<T, N>::direction_type direction;
    auto result = quetzal::components_from_chars(first, last, spec,position, direction);
    if (result.ec == std::errc{})
    {
        directed_point = DiscreteDirectedPoint<T, N>(position, direction);
    }

    return result;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const DiscreteDirectedPoint<T, N>& directed_point, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,directed_point.position(), directed_point.direction());
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
struct std::formatter<quetzal::geometry::DiscreteDirectedPoint<T, N>> : quetzal::series_formatter<quetzal::geometry::DiscretePoint<T, N>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::DiscreteDirectedPoint<T, N>& directed_point, FormatContext& ctx) const
    {
        using direction_type = typename quetzal::geometry::DiscretePoint<T, N>::vector_type;

        ctx.advance_to(this->m_element_formatter.format(directed_point.position(), ctx));
        this->format_series_delimiter(ctx);
        return quetzal::matching_formatter<direction_type>(this->m_element_formatter).format(directed_point.direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP
