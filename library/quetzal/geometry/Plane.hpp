#if !defined(QUETZAL_GEOMETRY_PLANE_HPP)
#define QUETZAL_GEOMETRY_PLANE_HPP
//------------------------------------------------------------------------------
// geometry
// Plane.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Point.hpp"
#include <format>
#include <ostream>
#include <sstream>
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
        // Results are either interior or exterior, plane is not considered to have or be a boundary
        std::partial_ordering compare(const point_type& point) const override;

    private:

        point_type m_point;
        vector_type m_normal;
    };

    template<typename Traits>
    std::string to_string(const Plane<Traits>& plane);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, Plane<Traits> plane);

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
std::partial_ordering quetzal::geometry::Plane<Traits>::compare(const point_type& point) const
{
    if constexpr(Traits::dimension == 2)
    {
        return std::partial_ordering::less;
    }
    else if constexpr(Traits::dimension >= 3)
    {
        auto d = dot(m_normal, point - m_point);
        return math::float_eq0(d) ? std::partial_ordering::less : std::partial_ordering::greater;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::string quetzal::geometry::to_string(const Plane<Traits>& plane)
{
    std::ostringstream oss;
    oss << to_string(plane.point()) << ", " << to_string(plane.normal());
    return oss.str();
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, Plane<Traits> plane)
{
    os << to_string(plane);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Plane<Traits>> : std::formatter<std::string>
{
    auto format(const quetzal::geometry::Plane<Traits>& plane, std::format_context& ctx) const
    {
        return std::formatter<std::string>::format(to_string(plane), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_PLANE_HPP
