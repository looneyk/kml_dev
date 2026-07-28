#if !defined(QUETZAL_GEOMETRY_SPHERE_HPP)
#define QUETZAL_GEOMETRY_SPHERE_HPP
//------------------------------------------------------------------------------
// geometry
// Sphere.hpp
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
    class Sphere : public PointRelation<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;

        Sphere() = default;
        Sphere(const point_type& center, const value_type& radius);
        Sphere(const Sphere&) = default;
        ~Sphere() = default;

        Sphere& operator=(const Sphere&) = default;

        point_type center() const;
        void set_center(const point_type& center);

        value_type radius() const;
        void set_radius(const value_type& radius);

        point_type point(value_type azimuth, value_type elevation) const;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

    private:

        point_type m_center;
        value_type m_radius;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Sphere<Traits>& sphere);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Sphere<Traits>::Sphere(const point_type& center, const value_type& radius) :
    PointRelation<Traits>(),
    m_center(center),
    m_radius(radius)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Sphere<Traits>::point_type quetzal::geometry::Sphere<Traits>::center() const
{
    return m_center;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Sphere<Traits>::set_center(const point_type& center)
{
    m_center = center;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Sphere<Traits>::value_type quetzal::geometry::Sphere<Traits>::radius() const
{
    return m_radius;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Sphere<Traits>::set_radius(const value_type& radius)
{
    m_radius = radius;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Sphere<Traits>::point_type quetzal::geometry::Sphere<Traits>::point(value_type azimuth, value_type elevation) const
{
    value_type cos_elevation = math::denoise011(cos(elevation));
    return {m_radius * cos_elevation * math::denoise011(cos(azimuth)), m_radius * cos_elevation * math::denoise011(sin(azimuth)), m_radius * math::denoise011(sin(elevation))};
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Sphere<Traits>::compare(const point_type& point, int tolerance) const
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
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Sphere<Traits>& sphere)
{
    os << to_string(sphere, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Sphere<Traits>>
{
    // Sphere has no formatting knobs of its own; the whole spec belongs to (and is entirely
    // consumed by) the Point formatter, reused for the center, with its element formatter reused
    // directly for the bare scalar radius (no Vector-style wrapper needed for a single value).
    std::formatter<quetzal::geometry::Point<Traits>> m_point_formatter;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return m_point_formatter.parse(ctx);
    }

    template<typename FormatContext>
    auto format(const quetzal::geometry::Sphere<Traits>& sphere, FormatContext& ctx) const
    {
        auto out = m_point_formatter.format(sphere.center(), ctx);
        ctx.advance_to(out);
        *out++ = ',';
        *out++ = ' ';
        ctx.advance_to(out);

        return m_point_formatter.m_element_formatter.format(sphere.radius(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_SPHERE_HPP
