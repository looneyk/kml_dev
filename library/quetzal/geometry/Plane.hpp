#if !defined(QUETZAL_GEOMETRY_PLANE_HPP)
#define QUETZAL_GEOMETRY_PLANE_HPP
//------------------------------------------------------------------------------
// geometry
// Plane.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include "quetzal/common/string_util.hpp"
#include <charconv>
#include <format>
#include <istream>
#include <ostream>
#include <sstream>
#include <string_view>
#include <system_error>
#include <cassert>

namespace quetzal::geometry
{

    // Client responsibility to ensure that the normal vector is unit length

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Plane : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        Plane() requires (Traits::dimension == 2); // xy-plane
        Plane() requires (Traits::dimension >= 3); // xy-plane
        Plane(const point_type& point, const vector_type& normal) requires (Traits::dimension >= 3);
        Plane(const Plane&) = default;
        Plane(Plane&&) = default;
        ~Plane() = default;

        Plane& operator=(const Plane&) = default;
        Plane& operator=(Plane&&) = default;

        point_type point() const requires (Traits::dimension >= 3);
        void set_point(const point_type& point) requires (Traits::dimension >= 3);

        vector_type normal() const requires (Traits::dimension >= 3);
        void set_normal(const vector_type& normal) requires (Traits::dimension >= 3);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        // Results here are either interior or exterior, plane is not considered to have or be a boundary
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

    private:

        point_type m_point;
        vector_type m_normal;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Plane<Traits> plane);

    template<typename Traits>
    std::istream& operator>>(std::istream& is, Plane<Traits>& plane);

    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Plane<Traits>& plane, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Plane<Traits>& plane, const FormatSpec& spec = {});

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<Traits>::Plane() requires (Traits::dimension == 2) :
    PointRelation<Traits>(),
    m_point(),
    m_normal()
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<Traits>::Plane() requires (Traits::dimension >= 3) :
    Plane({}, vector_type::z_direction())
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<Traits>::Plane(const point_type& point, const vector_type& normal) requires (Traits::dimension >= 3) :
    PointRelation<Traits>(),
    m_point(point),
    m_normal(normal)
{
    assert(normal.unit());
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Plane<Traits>::point_type quetzal::geometry::Plane<Traits>::point() const requires (Traits::dimension >= 3)
{
    return m_point;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Plane<Traits>::set_point(const point_type& point) requires (Traits::dimension >= 3)
{
    m_point = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Plane<Traits>::vector_type quetzal::geometry::Plane<Traits>::normal() const requires (Traits::dimension >= 3)
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Plane<Traits>::set_normal(const vector_type& normal) requires (Traits::dimension >= 3)
{
    assert(normal.unit());
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Plane<Traits>::compare(const point_type& point, int tolerance) const
{
    if constexpr(Traits::dimension == 2)
    {
        return std::partial_ordering::less;
    }
    else if constexpr(Traits::dimension >= 3)
    {
        auto a = dot(m_normal, point.vector());
        auto b = dot(m_normal, m_point.vector());
        return math::float_eq(a, b, tolerance) ? std::partial_ordering::less : std::partial_ordering::greater;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Plane<Traits> plane)
{
    os << quetzal::to_string(plane, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Plane<Traits>& plane)
{
    typename Plane<Traits>::point_type point;
    typename Plane<Traits>::vector_type normal;
    if (quetzal::read_components(is,point, normal))
    {
        plane = Plane<Traits>(point, normal);
    }

    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Plane<Traits>& plane, const FormatSpec& spec)
{
    typename Plane<Traits>::point_type point;
    typename Plane<Traits>::vector_type normal;
    auto result = quetzal::components_from_chars(first, last, spec,point, normal);
    if (result.ec == std::errc {})
    {
        plane = Plane<Traits>(point, normal);
    }

    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Plane<Traits>& plane, const FormatSpec& spec)
{
    return quetzal::components_to_chars(first, last, spec,plane.point(), plane.normal());
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Plane<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Plane<Traits>& plane, FormatContext& ctx) const
    {
        ctx.advance_to(this->m_element_formatter.format(plane.point(), ctx));
        this->format_series_delimiter(ctx);
        return quetzal::matching_formatter<quetzal::math::Vector<Traits>>(this->m_element_formatter).format(plane.normal(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_PLANE_HPP
