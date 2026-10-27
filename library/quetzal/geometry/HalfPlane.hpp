#if !defined(QUETZAL_GEOMETRY_HALFPLANE_HPP)
#define QUETZAL_GEOMETRY_HALFPLANE_HPP
//------------------------------------------------------------------------------
// geometry
// HalfPlane.hpp
//------------------------------------------------------------------------------

#include "Line.hpp"
#include "Plane.hpp"
#include "Point.hpp"
#include "PointRelation.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <string_view>
#include <system_error>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits> requires (Traits::dimension >= 2)
    class HalfPlane : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using line_type = Line<Traits>;
        using plane_type = Plane<Traits>;

        HalfPlane() = default;
        HalfPlane(const line_type& line, const vector_type& exterior_direction);
        HalfPlane(const HalfPlane&) = default;
        HalfPlane(HalfPlane&&) = default;
        ~HalfPlane() = default;

        HalfPlane& operator=(const HalfPlane&) = default;
        HalfPlane& operator=(HalfPlane&&) = default;

        const line_type& line() const;
        const vector_type& exterior_direction() const;

        plane_type plane() const requires (Traits::dimension >= 3);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

    private:

        line_type m_line;
        vector_type m_exterior_direction;
        mutable std::optional<plane_type> m_plane;
    };

    template<typename Traits> requires (Traits::dimension >= 2)
    std::ostream& operator<<(std::ostream& os, HalfPlane<Traits> halfplane);

    template<typename Traits> requires (Traits::dimension >= 2)
    std::istream& operator>>(std::istream& is, HalfPlane<Traits>& halfplane);

    template<typename Traits> requires (Traits::dimension >= 2)
    std::from_chars_result from_chars(const char* first, const char* last, HalfPlane<Traits>& halfplane, const FormatSpec& spec = {});

    template<typename Traits> requires (Traits::dimension >= 2)
    std::to_chars_result to_chars(char* first, char* last, const HalfPlane<Traits>& halfplane, const FormatSpec& spec = {});

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
quetzal::geometry::HalfPlane<Traits>::HalfPlane(const line_type& line, const vector_type& exterior_direction) :
    PointRelation<Traits>(),
    m_line(line),
    m_exterior_direction(exterior_direction),
    m_plane(std::nullopt)
{
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
const typename quetzal::geometry::HalfPlane<Traits>::line_type& quetzal::geometry::HalfPlane<Traits>::line() const
{
    return m_line;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
const typename quetzal::geometry::HalfPlane<Traits>::vector_type& quetzal::geometry::HalfPlane<Traits>::exterior_direction() const
{
    return m_exterior_direction;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
quetzal::geometry::HalfPlane<Traits>::plane_type quetzal::geometry::HalfPlane<Traits>::plane() const requires (Traits::dimension >= 3)
{
    if (!m_plane)
    {
        m_plane.emplace(m_line.point(), math::normalize(math::cross(m_line.direction(), m_exterior_direction)));
    }

    return *m_plane;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::partial_ordering quetzal::geometry::HalfPlane<Traits>::compare(const point_type& point, int tolerance) const
{
    if constexpr (Traits::dimension >= 3)
    {
        if (!plane().contains(point, tolerance))
        {
            return std::partial_ordering::greater; // exterior
        }
    }

    auto d = dot(m_exterior_direction, point - m_line.point());
    return math::float_compare0(d, tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, HalfPlane<Traits> halfplane)
{
    os << quetzal::to_string(halfplane, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::istream& quetzal::geometry::operator>>(std::istream& is, HalfPlane<Traits>& halfplane)
{
    typename HalfPlane<Traits>::line_type line;
    typename HalfPlane<Traits>::vector_type exterior_direction;
    if (quetzal::read_components(is,line, exterior_direction))
    {
        halfplane = HalfPlane<Traits>(line, exterior_direction);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, HalfPlane<Traits>& halfplane, const FormatSpec& spec)
{
    typename HalfPlane<Traits>::line_type line;
    typename HalfPlane<Traits>::vector_type exterior_direction;
    auto result = quetzal::components_from_chars(first, last, spec,line, exterior_direction);
    if (result.ec == std::errc {})
    {
        halfplane = HalfPlane<Traits>(line, exterior_direction);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const HalfPlane<Traits>& halfplane, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,halfplane.line(), halfplane.exterior_direction());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::HalfPlane<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::HalfPlane<Traits>& halfplane, FormatContext& ctx) const
    {
        std::formatter<quetzal::geometry::Line<Traits>> line_formatter;
        line_formatter.m_element_formatter = this->m_element_formatter;
        line_formatter.set_format_spec(this->m_format_spec);

        ctx.advance_to(line_formatter.format(halfplane.line(), ctx));
        this->format_series_delimiter(ctx);
        return quetzal::matching_formatter<quetzal::math::Vector<Traits>>(this->m_element_formatter).format(halfplane.exterior_direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_HALFPLANE_HPP
