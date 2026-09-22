#if !defined(QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// DirectedPoint.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Vector.hpp"
#include <format>
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class DirectedPoint
    {
    public:

		static_assert(Traits::dimension >= 3);

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using direction_type = point_type::vector_type;

        DirectedPoint() = default;
        DirectedPoint(const point_type& position, const direction_type& direction);
        DirectedPoint(const DirectedPoint&) = default;
        ~DirectedPoint() = default;

        DirectedPoint& operator=(const DirectedPoint&) = default;

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

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, DirectedPoint<Traits> directed_point);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::DirectedPoint<Traits>::DirectedPoint(const point_type& position, const direction_type& direction) :
    m_position(position),
    m_direction(direction)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::DirectedPoint<Traits>::point_type quetzal::geometry::DirectedPoint<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::DirectedPoint<Traits>::point_type& quetzal::geometry::DirectedPoint<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::DirectedPoint<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::DirectedPoint<Traits>::direction_type quetzal::geometry::DirectedPoint<Traits>::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::DirectedPoint<Traits>::direction_type& quetzal::geometry::DirectedPoint<Traits>::direction()
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::DirectedPoint<Traits>::set_direction(const direction_type& direction)
{
    m_direction = direction;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, DirectedPoint<Traits> directed_point)
{
    os << to_string(directed_point, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::DirectedPoint<Traits>>
{
    // Parses the whole format spec, which the nested formatters share
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::DirectedPoint<Traits>& directed_point, FormatContext& ctx) const
    {
        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
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

#endif // QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP
