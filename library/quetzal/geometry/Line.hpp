#if !defined(QUETZAL_GEOMETRY_LINE_HPP)
#define QUETZAL_GEOMETRY_LINE_HPP
//------------------------------------------------------------------------------
// geometry
// Line.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/math/floating_point.hpp"
#include <array>
#include <format>
#include <ostream>
#include <cassert>

namespace quetzal::geometry
{

    // Client responsibility to ensure that the direction vector is unit length

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Line : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        Line() = default;
        Line(const point_type& point, const vector_type& direction);
        Line(const Line&) = default;
        Line(Line&&) = default;
        ~Line() = default;

        Line& operator=(const Line&) = default;
        Line& operator=(Line&&) = default;

        point_type point() const;
        void set_point(const point_type& point);

        vector_type direction() const;
        void set_direction(const vector_type& direction);

        point_type point(value_type t) const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        // Results are either interior or exterior, line is not considered to have (or be) a boundary
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        point_type projection(const point_type& point) const;
        value_type projection_parameter(const point_type& point) const;

    private:

        point_type m_point;
        vector_type m_direction;
    };

    // Compute the line parameters of the two closest points on lines A and B
    template<typename Traits>
    std::array<typename Traits::value_type, 2> closest_point_parameters(const Line<Traits>& lineA, const Line<Traits>& lineB);

    // For the case where one or both are actually segments which require vector instead of direction ...
    template<typename P>
    std::array<typename P::value_type, 2> closest_point_parameters(const P& pointA, const typename P::vector_type& vectorA, const P& pointB, const typename P::vector_type& vectorB);

    template<typename Traits>
    std::string to_string(const Line<Traits>& line);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Line<Traits> line);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Line<Traits>::Line(const point_type& point, const vector_type& direction) :
    PointRelation<Traits>(),
    m_point(point),
    m_direction(direction)
{
    assert(direction.unit());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Line<Traits>::point_type quetzal::geometry::Line<Traits>::point() const
{
    return m_point;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Line<Traits>::set_point(const point_type& point)
{
    m_point = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Line<Traits>::vector_type quetzal::geometry::Line<Traits>::direction() const
{
    return m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Line<Traits>::set_direction(const vector_type& direction)
{
    assert(direction.unit());

    m_direction = direction;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Line<Traits>::point_type quetzal::geometry::Line<Traits>::point(value_type t) const
{
    return m_point + t * m_direction;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Line<Traits>::compare(const point_type& point, int tolerance) const
{
    return float_eq(point, projection(point), tolerance) ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Line<Traits>::point_type quetzal::geometry::Line<Traits>::projection(const Point<Traits>& point) const
{
    return this->point(projection_parameter(point));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Line<Traits>::value_type quetzal::geometry::Line<Traits>::projection_parameter(const Point<Traits>& point) const
{
    return math::dot(point - m_point, m_direction);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::array<typename Traits::value_type, 2> quetzal::geometry::closest_point_parameters(const Line<Traits>& lineA, const Line<Traits>& lineB)
{
    return closest_point_parameters(lineA.point(), lineA.direction(), lineB.point(), lineB.direction());
/*
    using T = Traits::value_type;
    using V = typename Line<Traits>::vector_type;

    V u = lineA.direction();
    V v = lineB.direction();
    V w = lineA.point() - lineB.point();

    T uv = dot(u, v);
    T uw = dot(u, w);
    T vw = dot(v, w);

// with these Line's, uu and vv should be 1 ...

    // In the common case where directions are unit vectors, avoid potential precision problems
    T uu = dot(u, u); // >= 0
    if (math::float_eq(uu, T(1))) uu = T(1);
    T vv = dot(v, v); // >= 0
    if (math::float_eq(vv, T(1))) vv = T(1);

    T denom = uu * vv - uv * uv; // >= 0
    if (math::float_eq0(denom)) // Parallel
    {
        return {T(0), (uv > vv ? uw / uv : vw / vv)}; // Use the largest denominator
    }

    T sn = (uv * vw - vv * uw);
    T sd = denom;
    T s = math::float_eq0(sn) ? T(0) : sn / sd;

    T tn = (uu * vw - uv * uw);
    T td = denom;
    T t = math::float_eq0(tn) ? T(0) : tn / td;

    return {s, t};
*/
}

//------------------------------------------------------------------------------
template<typename P>
std::array<typename P::value_type, 2> quetzal::geometry::closest_point_parameters(const P& pointA, const typename P::vector_type& vectorA, const P& pointB, const typename P::vector_type& vectorB)
{
    using T = P::value_type;
    using V = P::vector_type;

    V u = vectorA;
    V v = vectorB;
    V w = pointA - pointB;

    T uv = dot(u, v);
    T uw = dot(u, w);
    T vw = dot(v, w);

    // In the common case where directions are unit vectors, avoid potential precision problems
    T uu = dot(u, u); // >= 0
    if (math::float_eq(uu, T(1))) uu = T(1);
    T vv = dot(v, v); // >= 0
    if (math::float_eq(vv, T(1))) vv = T(1);

    T denom = uu * vv - uv * uv; // >= 0
    if (math::float_eq0(denom)) // Parallel
    {
        return {T(0), (uv > vv ? uw / uv : vw / vv)}; // Use the largest denominator
    }

    T sn = (uv * vw - vv * uw);
    T sd = denom;
    T s = math::float_eq0(sn) ? T(0) : sn / sd;

    T tn = (uu * vw - uv * uw);
    T td = denom;
    T t = math::float_eq0(tn) ? T(0) : tn / td;

    return {s, t};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::geometry::to_string(const Line<Traits>& line)
{
    std::ostringstream oss;
    oss << to_string(line.point()) << ", " << to_string(line.direction());
    return oss.str();
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Line<Traits> line)
{
    os << to_string(line);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Line<Traits>>
{
    std::formatter<typename Traits::value_type> m_element_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_element_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::Line<Traits>& line, std::format_context& ctx) const
    {
        std::formatter<quetzal::geometry::Point<Traits>> point_formatter;
        point_formatter.m_element_formatter = m_element_formatter;

        std::formatter<quetzal::math::Vector<Traits>> vector_formatter;
        vector_formatter.m_element_formatter = m_element_formatter;

        auto out = point_formatter.format(line.point(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return vector_formatter.format(line.direction(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_LINE_HPP
