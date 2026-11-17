#if !defined(QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP)
#define QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP
//------------------------------------------------------------------------------
// geometry
// DirectedPoint.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Vector.hpp"
#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <system_error>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class DirectedPoint
    {
    public:

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

    template<typename Traits>
    std::istream& operator>>(std::istream& is, DirectedPoint<Traits>& directed_point);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, DirectedPoint<Traits>& directed_point, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const DirectedPoint<Traits>& directed_point, const FormatSpec& spec = {});

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
    os << quetzal::to_string(directed_point, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, DirectedPoint<Traits>& directed_point)
{
    typename DirectedPoint<Traits>::point_type position;
    typename DirectedPoint<Traits>::direction_type direction;
    if (quetzal::read_components(is,position, direction))
    {
        directed_point = DirectedPoint<Traits>(position, direction);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, DirectedPoint<Traits>& directed_point, const FormatSpec& spec)
{
    typename DirectedPoint<Traits>::point_type position;
    typename DirectedPoint<Traits>::direction_type direction;
    auto result = quetzal::components_from_chars(first, last, spec,position, direction);
    if (result.ec == std::errc{})
    {
        directed_point = DirectedPoint<Traits>(position, direction);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const DirectedPoint<Traits>& directed_point, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,directed_point.position(), directed_point.direction());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::DirectedPoint<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::DirectedPoint<Traits>& directed_point, FormatContext& ctx) const
    {
        ctx.advance_to(this->m_element_formatter.format(directed_point.position(), ctx));
        this->format_series_delimiter(ctx);
        return quetzal::matching_formatter<quetzal::math::Vector<Traits>>(this->m_element_formatter).format(directed_point.direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_DIRECTEDPOINT_HPP
