#if !defined(QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// OrientedPoint.hpp
//------------------------------------------------------------------------------

#include "Orientation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <system_error>

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
    std::ostream& operator<<(std::ostream& os, OrientedPoint<Traits> oriented_point);

    template<typename Traits>
    std::istream& operator>>(std::istream& is, OrientedPoint<Traits>& oriented_point);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, OrientedPoint<Traits>& oriented_point, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const OrientedPoint<Traits>& oriented_point, const FormatSpec& spec = {});

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
    os << quetzal::to_string(op, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, OrientedPoint<Traits>& oriented_point)
{
    typename OrientedPoint<Traits>::point_type position;
    typename OrientedPoint<Traits>::orientation_type orientation;
    if (quetzal::read_components(is,position, orientation))
    {
        oriented_point = OrientedPoint<Traits>(position, orientation);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, OrientedPoint<Traits>& oriented_point, const FormatSpec& spec)
{
    typename OrientedPoint<Traits>::point_type position;
    typename OrientedPoint<Traits>::orientation_type orientation;
    auto result = quetzal::components_from_chars(first, last, spec,position, orientation);
    if (result.ec == std::errc {})
    {
        oriented_point = OrientedPoint<Traits>(position, orientation);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const OrientedPoint<Traits>& oriented_point, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,oriented_point.position(), oriented_point.orientation());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::OrientedPoint<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::OrientedPoint<Traits>& oriented_point, FormatContext& ctx) const
    {
        std::formatter<quetzal::geometry::Orientation<Traits>> orientation_formatter;
        orientation_formatter.m_element_formatter = quetzal::matching_formatter<quetzal::math::Vector<Traits>>(this->m_element_formatter);
        orientation_formatter.set_format_spec(this->m_format_spec);

        ctx.advance_to(this->m_element_formatter.format(oriented_point.position(), ctx));
        this->format_series_delimiter(ctx);
        return orientation_formatter.format(oriented_point.orientation(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_ORIENTEDPOINT_HPP
