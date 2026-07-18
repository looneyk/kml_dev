#if !defined(QUETZAL_GEOMETRY_CIRCLE_HPP)
#define QUETZAL_GEOMETRY_CIRCLE_HPP
//------------------------------------------------------------------------------
// geometry
// Circle.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/floating_point.hpp"
#include <format>
#include <iostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Circle : public PointRelation<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;

        Circle() = default;
        Circle(const point_type& center, const value_type& radius);
        Circle(const Circle&) = default;
        Circle(Circle&&) = default;
        ~Circle() = default;

        Circle& operator=(const Circle&) = default;
        Circle& operator=(Circle&&) = default;

        point_type center() const;
        void set_center(const point_type& center);

        value_type radius() const;
        void set_radius(const value_type& radius);

        point_type point(value_type phi) const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

    private:

        point_type m_center;
        value_type m_radius;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Circle<Traits>& circle);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Circle<Traits>::Circle(const point_type& center, const value_type& radius) :
    PointRelation<Traits>(),
    m_center(center),
    m_radius(radius)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::point_type quetzal::geometry::Circle<Traits>::center() const
{
    return m_center;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Circle<Traits>::set_center(const point_type& center)
{
    m_center = center;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::value_type quetzal::geometry::Circle<Traits>::radius() const
{
    return m_radius;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Circle<Traits>::set_radius(const value_type& radius)
{
    m_radius = radius;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Circle<Traits>::point_type quetzal::geometry::Circle<Traits>::point(value_type phi) const
{
    if constexpr (Traits::dimension == 3)
    {
        return {m_radius * cos(phi), m_radius * sin(phi), value_type(0)};
    }
    else if constexpr (Traits::dimension == 2)
    {
        return {m_radius * cos(phi), m_radius * sin(phi)};
    }
    else
    {
        static_assert(false, "Dimension not supported.");
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Circle<Traits>::compare(const point_type& point, int tolerance) const
{
    value_type d2 = (point - m_center).norm_squared();
    value_type r2 = m_radius * m_radius;

    if (math::float_eq(d2, r2, tolerance))
    {
        return std::partial_ordering::equivalent;
    }

    return d2 < r2 ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Circle<Traits>& circle)
{
    os << to_array_string(circle, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Circle<Traits>>
{
    // Circle has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, reused for the center, with its element formatter reused
    // directly for the bare scalar radius (no Vector-style wrapper needed for a single value).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    auto format(const quetzal::geometry::Circle<Traits>& circle, std::format_context& ctx) const
    {
        auto out = m_point_formatter.format(circle.center(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return m_point_formatter.m_element_formatter.format(circle.radius(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_CIRCLE_HPP
