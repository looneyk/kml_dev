#if !defined(QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// OrientedPoint.hpp
//------------------------------------------------------------------------------

#include "Orientation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include <format>
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class OrientedPoint
    {
    public:

		static_assert(Traits::dimension >= 3);

        using value_type = Traits::value_type;
        using orientation_type = Orientation<Traits>;
        using vector_type = orientation_type::vector_type;
        using point_type = Point<Traits>;

        OrientedPoint() = default;
        OrientedPoint(const point_type& position, const orientation_type& orientation);
        OrientedPoint(const OrientedPoint&) = default;
        ~OrientedPoint() = default;

        OrientedPoint& operator=(const OrientedPoint&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        orientation_type orientation() const;
        orientation_type& orientation();
        void set_orientation(const orientation_type& orientation);

    private:

        point_type m_position;
        orientation_type m_orientation;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, OrientedPoint<Traits> plane);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::OrientedPoint<Traits>::OrientedPoint(const point_type& position, const orientation_type& orientation) :
    m_position(position),
    m_orientation(orientation)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::point_type quetzal::geometry::OrientedPoint<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::point_type& quetzal::geometry::OrientedPoint<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::OrientedPoint<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::orientation_type quetzal::geometry::OrientedPoint<Traits>::orientation() const
{
    return m_orientation;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::OrientedPoint<Traits>::orientation_type& quetzal::geometry::OrientedPoint<Traits>::orientation()
{
    return m_orientation;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::OrientedPoint<Traits>::set_orientation(const orientation_type& orientation)
{
    m_orientation = orientation;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, OrientedPoint<Traits> op)
{
    os << to_string(op, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::OrientedPoint<Traits>>
{
    // OrientedPoint has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, whose parsed state is shared with a Vector formatter (used
    // to build the nested Orientation formatter, itself composed of two Vectors).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::OrientedPoint<Traits>& op, FormatContext& ctx) const
    {
        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
        vector_formatter.m_object_delimiters = m_point_formatter.m_object_delimiters;
        vector_formatter.m_element_delimiter = m_point_formatter.m_element_delimiter;
        vector_formatter.m_element_formatter = m_point_formatter.m_element_formatter;

        std::formatter<quetzal::geometry::Orientation<Traits>> orientation_formatter;
        orientation_formatter.m_vector_formatter = vector_formatter;

        auto out = m_point_formatter.format(op.position(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return orientation_formatter.format(op.orientation(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
