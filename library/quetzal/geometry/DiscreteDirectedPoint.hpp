#if !defined(QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// DiscreteDirectedPoint.hpp
//------------------------------------------------------------------------------

#include "DiscretePoint.hpp"
#include "quetzal/common/string_util.hpp"
#include <format>
#include <iostream>

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
    os << to_string(directed_point, os);
    return os;
}

//------------------------------------------------------------------------------
template<std::signed_integral T, size_t N> requires (N > 0)
struct std::formatter<quetzal::geometry::DiscreteDirectedPoint<T, N>>
{
    // Parses the whole format spec, which the nested formatters share
    std::formatter<quetzal::geometry::DiscretePoint<T, N>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::DiscreteDirectedPoint<T, N>& directed_point, FormatContext& ctx) const
    {
        std::formatter<typename quetzal::geometry::DiscretePoint<T, N>::vector_type> vector_formatter;
        vector_formatter.m_object_delimiters = m_point_formatter.m_object_delimiters;
        vector_formatter.m_element_delimiter = m_point_formatter.m_element_delimiter;
        vector_formatter.m_element_formatter = m_point_formatter.m_element_formatter;

        auto out = m_point_formatter.format(directed_point.position(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return vector_formatter.format(directed_point.direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_DISCRETEDIRECTEDPOINT_HPP
