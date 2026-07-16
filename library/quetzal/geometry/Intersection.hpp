#if !defined(QUETZAL_GEOMETRY_INTERSECTION_HPP)
#define QUETZAL_GEOMETRY_INTERSECTION_HPP
//------------------------------------------------------------------------------
// geometry
// Intersection.hpp
//------------------------------------------------------------------------------

#include "Line.hpp"
#include "Locus.hpp"
#include "Plane.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "Ray.hpp"
#include "Segment.hpp"
//#include "Circle.hpp"
//#include "HalfSpace.hpp"
//#include "Polyhedron.hpp"
//#include "Polyline.hpp"
//#include "Sphere.hpp"
#include <format>
#include <iostream>
#include <string_view>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Intersection
    {
    public:

        using point_type = Point<Traits>;
        using line_type = Line<Traits>;
        using ray_type = Ray<Traits>;
        using segment_type = Segment<Traits>;
        using plane_type = Plane<Traits>;
        using polygon_type = Polygon<Traits>;

        Intersection();
        Intersection(const point_type& point);
        Intersection(const line_type& line);
        Intersection(const ray_type& ray);
        Intersection(const segment_type& segment);
        Intersection(const plane_type& plane);
        Intersection(const polygon_type& polygon);
        Intersection(const Intersection&) = default;
        Intersection(Intersection&&) = default;
        ~Intersection() = default;

        Intersection& operator=(const Intersection&) = default;
        Intersection& operator=(Intersection&&) = default;

        Locus locus() const;
        const point_type point() const;
        const line_type line() const;
        const ray_type ray() const;
        const segment_type segment() const;
        const plane_type plane() const requires (Traits::dimension >= 3);
        const polygon_type polygon() const;

        bool empty() const;

    private:

        // This is really just a variant type specific to geometry elements
        Locus m_locus;
        point_type m_point;
        line_type m_line;
        ray_type m_ray;
        segment_type m_segment;
        plane_type m_plane;
        polygon_type m_polygon;
    };

    template<typename Traits>
    std::string to_string(const Intersection<Traits>& ix);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Intersection<Traits>& ix);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection() :
    m_locus(Locus::Empty),
    m_point(),
    m_line(),
    m_ray(),
    m_segment(),
//    m_plane(),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const point_type& point) :
    m_locus(Locus::Point),
    m_point(point),
    m_line(),
    m_ray(),
    m_segment(),
//    m_plane(),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const line_type& line) :
    m_locus(Locus::Line),
    m_point(),
    m_line(line),
    m_ray(),
    m_segment(),
//    m_plane(),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const ray_type& ray) :
    m_locus(Locus::Ray),
    m_point(),
    m_line(),
    m_ray(ray),
    m_segment(),
//    m_plane(),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const segment_type& segment) :
    m_locus(Locus::Segment),
    m_point(),
    m_line(),
    m_ray(),
    m_segment(segment),
//    m_plane(),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const plane_type& plane) :
    m_locus(Locus::Plane),
    m_point(),
    m_line(),
    m_ray(),
    m_segment(),
    m_plane(plane),
    m_polygon()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Intersection<Traits>::Intersection(const polygon_type& polygon) :
    m_locus(Locus::Polygon),
    m_point(),
    m_line(),
    m_ray(),
    m_segment(),
//    m_plane(),
    m_polygon(polygon)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Locus quetzal::geometry::Intersection<Traits>::locus() const
{
    return m_locus;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Point<Traits> quetzal::geometry::Intersection<Traits>::point() const
{
    assert(m_locus == Locus::Point);
    return m_point;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Line<Traits> quetzal::geometry::Intersection<Traits>::line() const
{
    assert(m_locus == Locus::Line);
    return m_line;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Ray<Traits> quetzal::geometry::Intersection<Traits>::ray() const
{
    assert(m_locus == Locus::Ray);
    return m_ray;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Segment<Traits> quetzal::geometry::Intersection<Traits>::segment() const
{
    assert(m_locus == Locus::Segment);
    return m_segment;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Plane<Traits> quetzal::geometry::Intersection<Traits>::plane() const requires (Traits::dimension >= 3)
{
    assert(m_locus == Locus::Plane);
    return m_plane;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Polygon<Traits> quetzal::geometry::Intersection<Traits>::polygon() const
{
    assert(m_locus == Locus::Polygon);
    return m_polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Intersection<Traits>::empty() const
{
    return m_locus == Locus::Empty;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::geometry::to_string(const Intersection<Traits>& ix)
{
    std::ostringstream oss;
    switch (ix.locus())
    {
        case Locus::Empty:
            oss << "Empty";
            break;

        case Locus::Point:
            oss << "Point " << ix.point();
            break;

        case Locus::Line:
            oss << "Line " << ix.line();
            break;

        case Locus::Ray:
            oss << "Ray " << ix.ray();
            break;

        case Locus::Segment:
            oss << "Segment " << ix.segment();
            break;

/*
        case Locus::Plane:
            oss << "Plane " << ix.plane();
            break;
*/

        case Locus::Polygon:
            oss << "Polygon " << ix.polygon();
            break;
    }
    return oss.str();
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Intersection<Traits>& ix)
{
    os << to_string(ix);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Intersection<Traits>>
{
    std::formatter<typename Traits::value_type> m_element_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_element_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::Intersection<Traits>& ix, std::format_context& ctx) const
    {
        std::string label = quetzal::geometry::to_string(ix.locus());
        auto out = ctx.out();
        for (char c : label)
        {
            *out++ = c;
        }

        if (ix.locus() == quetzal::geometry::Locus::Empty)
        {
            return out;
        }

        *out++ = ' ';
        ctx.advance_to(out);

        switch (ix.locus())
        {
            case quetzal::geometry::Locus::Point:
            {
                std::formatter<quetzal::geometry::Point<Traits>> f;
                f.m_element_formatter = m_element_formatter;
                return f.format(ix.point(), ctx);
            }

            case quetzal::geometry::Locus::Line:
            {
                std::formatter<quetzal::geometry::Line<Traits>> f;
                f.m_element_formatter = m_element_formatter;
                return f.format(ix.line(), ctx);
            }

            case quetzal::geometry::Locus::Ray:
            {
                std::formatter<quetzal::geometry::Ray<Traits>> f;
                f.m_element_formatter = m_element_formatter;
                return f.format(ix.ray(), ctx);
            }

            case quetzal::geometry::Locus::Segment:
            {
                std::formatter<quetzal::geometry::Segment<Traits>> f;
                f.m_element_formatter = m_element_formatter;
                return f.format(ix.segment(), ctx);
            }

            case quetzal::geometry::Locus::Polygon:
            {
                std::formatter<quetzal::geometry::Polygon<Traits>> f;
                f.m_element_formatter = m_element_formatter;
                return f.format(ix.polygon(), ctx);
            }

            default:
                return out;
        }
    }
};

#endif // QUETZAL_GEOMETRY_INTERSECTION_HPP
