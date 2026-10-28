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
#include "quetzal/common/string_util.hpp"
//#include "Circle.hpp"
//#include "HalfSpace.hpp"
//#include "Polyhedron.hpp"
//#include "Polyline.hpp"
//#include "Sphere.hpp"
#include <algorithm>
#include <charconv>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Intersection
    {
    public:

        using value_type = Traits::value_type;
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
    std::ostream& operator<<(std::ostream& os, const Intersection<Traits>& ix);

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Intersection<Traits>& ix);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Intersection<Traits>& ix, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Intersection<Traits>& ix, const FormatSpec& spec = {});

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
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Intersection<Traits>& ix)
{
    os << quetzal::to_string(ix, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Intersection<Traits>& ix)
{
    Locus locus = Locus::Empty;
    if (!(is >> locus))
    {
        return is;
    }

    const std::string series_delimiter(quetzal::resolved_series_delimiter(quetzal::stream_format_spec(is), quetzal::series_delimiter_default));
    auto read_value = [&]<typename V>(V value) -> std::istream&
    {
        if (quetzal::skip_pattern(is, series_delimiter) >> value)
        {
            ix = Intersection<Traits>(value);
        }

        return is;
    };

    switch (locus)
    {
        case Locus::Empty:
            ix = Intersection<Traits>();
            return is;

        case Locus::Point:
            return read_value(typename Intersection<Traits>::point_type());

        case Locus::Line:
            return read_value(typename Intersection<Traits>::line_type());

        case Locus::Ray:
            return read_value(typename Intersection<Traits>::ray_type());

        case Locus::Segment:
            return read_value(typename Intersection<Traits>::segment_type());

        case Locus::Plane:
            if constexpr (Traits::dimension >= 3)
            {
                return read_value(typename Intersection<Traits>::plane_type());
            }
            else
            {
                is.setstate(std::ios::failbit);
                return is;
            }

        case Locus::Polygon:
            return read_value(typename Intersection<Traits>::polygon_type());

        default:
            is.setstate(std::ios::failbit);
            return is;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Intersection<Traits>& ix, const FormatSpec& spec)
{
    const std::string_view series_delimiter = quetzal::resolved_series_delimiter(spec, quetzal::series_delimiter_default);

    Locus locus = Locus::Empty;
    auto result = from_chars(first, last, locus);
    if (result.ec != std::errc {})
    {
        return result;
    }

    auto read_value = [&]<typename V>(V value) -> std::from_chars_result
    {
        const char* position = quetzal::skip_pattern(result.ptr, last, series_delimiter);
        if (position == nullptr)
        {
            return {result.ptr, std::errc::invalid_argument};
        }

        auto value_result = quetzal::component_from_chars(position, last, value, spec);
        if (value_result.ec == std::errc {})
        {
            ix = Intersection<Traits>(value);
        }

        return value_result;
    };

    switch (locus)
    {
        case Locus::Empty:
            ix = Intersection<Traits>();
            return result;

        case Locus::Point:
            return read_value(typename Intersection<Traits>::point_type());

        case Locus::Line:
            return read_value(typename Intersection<Traits>::line_type());

        case Locus::Ray:
            return read_value(typename Intersection<Traits>::ray_type());

        case Locus::Segment:
            return read_value(typename Intersection<Traits>::segment_type());

        case Locus::Plane:
            if constexpr (Traits::dimension >= 3)
            {
                return read_value(typename Intersection<Traits>::plane_type());
            }
            else
            {
                return {first, std::errc::invalid_argument};
            }

        case Locus::Polygon:
            return read_value(typename Intersection<Traits>::polygon_type());

        default:
            return {first, std::errc::invalid_argument};
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Intersection<Traits>& ix, const FormatSpec& spec)
{
    const std::string_view series_delimiter = quetzal::resolved_series_delimiter(spec, quetzal::series_delimiter_default);

    auto result = to_chars(first, last, ix.locus());
    if (result.ec != std::errc {})
    {
        return result;
    }

    auto write_value = [&](const auto& value)
    {
        result = quetzal::write_pattern(result.ptr, last, series_delimiter);
        if (result.ec != std::errc {})
        {
            return result;
        }

        return quetzal::component_to_chars(result.ptr, last, value, spec);
    };

    switch (ix.locus())
    {
        case Locus::Point:
            return write_value(ix.point());

        case Locus::Line:
            return write_value(ix.line());

        case Locus::Ray:
            return write_value(ix.ray());

        case Locus::Segment:
            return write_value(ix.segment());

        case Locus::Plane:
            if constexpr (Traits::dimension >= 3)
            {
                return write_value(ix.plane());
            }
            else
            {
                return result;
            }

        case Locus::Polygon:
            return write_value(ix.polygon());

        default:
            return result;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Intersection<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Intersection<Traits>& ix, FormatContext& ctx) const
    {
        ctx.advance_to(std::ranges::copy(quetzal::geometry::to_string(ix.locus()), ctx.out()).out);

        switch (ix.locus())
        {
            case quetzal::geometry::Locus::Point:
                this->format_series_delimiter(ctx);
                return this->m_element_formatter.format(ix.point(), ctx);

            case quetzal::geometry::Locus::Line:
                return format_value(ix.line(), ctx);

            case quetzal::geometry::Locus::Ray:
                return format_value(ix.ray(), ctx);

            case quetzal::geometry::Locus::Segment:
                return format_value(ix.segment(), ctx);

            case quetzal::geometry::Locus::Plane:
                if constexpr (Traits::dimension >= 3)
                {
                    return format_value(ix.plane(), ctx);
                }
                else
                {
                    return ctx.out();
                }

            case quetzal::geometry::Locus::Polygon:
                return format_value(ix.polygon(), ctx);

            default:
                return ctx.out();
        }
    }

    // Writes the series delimiter and then the value, with this formatter's delimiters and element format
    template<typename V, typename FormatContext>
    auto format_value(const V& value, FormatContext& ctx) const
    {
        std::formatter<V> value_formatter;
        value_formatter.m_element_formatter = this->m_element_formatter;
        value_formatter.set_format_spec(this->m_format_spec);

        this->format_series_delimiter(ctx);
        return value_formatter.format(value, ctx);
    }
};

#endif // QUETZAL_GEOMETRY_INTERSECTION_HPP
