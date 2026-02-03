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
        using line_type = Line<Traits>;
        using plane_type = Plane<Traits>;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        HalfPlane() = default;
        HalfPlane(const line_type& line, const vector_type& exterior_direction_direction);
        HalfPlane(const HalfPlane&) = default;
        HalfPlane(HalfPlane&&) = default;
        ~HalfPlane() = default;

        HalfPlane& operator=(const HalfPlane&) = default;
        HalfPlane& operator=(HalfPlane&&) = default;

        line_type line() const;
        void set_line(const line_type& line);

        vector_type exterior_direction() const;
        void set_exterior_direction(const vector_type& exterior_direction);

        plane_type plane() const requires (Traits::dimension >= 3);

        // Returns -1, 0, 1: interior, boundary, exterior
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
typename quetzal::geometry::HalfPlane<Traits>::line_type quetzal::geometry::HalfPlane<Traits>::line() const
{
    return m_line;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
void quetzal::geometry::HalfPlane<Traits>::set_line(const line_type& line)
{
    m_line = line;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
typename quetzal::geometry::HalfPlane<Traits>::vector_type quetzal::geometry::HalfPlane<Traits>::exterior_direction() const
{
    return m_exterior_direction;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 2)
void quetzal::geometry::HalfPlane<Traits>::set_exterior_direction(const vector_type& exterior_direction)
{
    m_exterior_direction = exterior_direction;
    return;
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
    os << halfplane.line();
    if constexpr (Traits::dimension >= 3)
    {
        os << ' ' << halfplane.exterior_direction();
    }
    return os;
}

#endif // QUETZAL_GEOMETRY_HALFPLANE_HPP
