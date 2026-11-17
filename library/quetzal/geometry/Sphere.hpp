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
#include <charconv>
#include <format>
#include <iostream>
#include <string_view>
#include <system_error>

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

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Sphere<Traits>& sphere);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Sphere<Traits>& sphere, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Sphere<Traits>& sphere, const FormatSpec& spec = {});

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
    os << quetzal::to_string(sphere, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Sphere<Traits>& sphere)
{
    typename Sphere<Traits>::point_type center;
    typename Sphere<Traits>::value_type radius{};
    if (quetzal::read_components(is,center, radius))
    {
        sphere = Sphere<Traits>(center, radius);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Sphere<Traits>& sphere, const FormatSpec& spec)
{
    typename Sphere<Traits>::point_type center;
    typename Sphere<Traits>::value_type radius{};
    auto result = quetzal::components_from_chars(first, last, spec,center, radius);
    if (result.ec == std::errc{})
    {
        sphere = Sphere<Traits>(center, radius);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Sphere<Traits>& sphere, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,sphere.center(), sphere.radius());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Sphere<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Sphere<Traits>& sphere, FormatContext& ctx) const
    {
        ctx.advance_to(this->m_element_formatter.format(sphere.center(), ctx));
        this->format_series_delimiter(ctx);
        return this->m_element_formatter.m_element_formatter.format(sphere.radius(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_SPHERE_HPP
