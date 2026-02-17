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
#include "quetzal/math/floating_point.hpp"
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
        std::partial_ordering compare(const point_type& point) const override;

    private:

        line_type m_line;
        vector_type m_exterior_direction;
    };

    template<typename Traits> requires (Traits::dimension >= 2)
    std::ostream& operator<<(std::ostream& os, HalfPlane<Traits> halfplane);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
quetzal::geometry::HalfPlane<Traits>::HalfPlane(const line_type& line, const vector_type& exterior_direction) :
    PointRelation<Traits>(),
    m_line(line),
    m_exterior_direction(exterior_direction)
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
    return {m_line.point(), normalize(cross(m_line.direction(), m_exterior_direction))};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::partial_ordering quetzal::geometry::HalfPlane<Traits>::compare(const point_type& point) const
{
    if constexpr (Traits::dimension >= 3)
    {
        if (!plane().contains(point))
        {
            return std::partial_ordering::greater; // exterior
        }
    }

    auto d = dot(m_exterior_direction, point - m_line.point());
    return math::float_compare0(d);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, HalfPlane<Traits> halfplane)
{
    os << halfplane.line() << ' ' << halfplane.exterior_direction();
    return os;
}

#endif // QUETZAL_GEOMETRY_HALFPLANE_HPP
