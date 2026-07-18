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
#include <format>
#include <ostream>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits> requires (Traits::dimension >= 2)
    class HalfPlane : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
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
    os << to_string(halfplane, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::HalfPlane<Traits>>
{
    // HalfPlane has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, shared with a Line formatter (for the line) and a Vector
    // formatter (for the exterior direction).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::HalfPlane<Traits>& halfplane, std::format_context& ctx) const
    {
        std::formatter<quetzal::geometry::Line<Traits>> line_formatter;
        line_formatter.m_point_formatter = m_point_formatter;

        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
        vector_formatter.m_object_delimiters = m_point_formatter.m_object_delimiters;
        vector_formatter.m_element_delimiter = m_point_formatter.m_element_delimiter;
        vector_formatter.m_element_formatter = m_point_formatter.m_element_formatter;

        auto out = line_formatter.format(halfplane.line(), ctx);
        ctx.advance_to(out);
        *out++ = ' ';
        ctx.advance_to(out);

        return vector_formatter.format(halfplane.exterior_direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_HALFPLANE_HPP
