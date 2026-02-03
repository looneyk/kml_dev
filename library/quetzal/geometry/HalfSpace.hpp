#if !defined(QUETZAL_GEOMETRY_HALFSPACE_HPP)
#define QUETZAL_GEOMETRY_HALFSPACE_HPP
//------------------------------------------------------------------------------
// geometry
// HalfSpace.hpp
//------------------------------------------------------------------------------

#include "PointRelation.hpp"
#include "Plane.hpp"
#include "Point.hpp"
#include "quetzal/math/floating_point.hpp"
#include <ostream>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits> requires (Traits::dimension >= 3)
    class HalfSpace : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using plane_type = Plane<Traits>;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        HalfSpace() = default;
        HalfSpace(const plane_type& plane);
        HalfSpace(const point_type& point, const vector_type& normal);
        HalfSpace(const HalfSpace&) = default;
        HalfSpace(HalfSpace&&) = default;
        ~HalfSpace() = default;

        HalfSpace& operator=(const HalfSpace&) = default;
        HalfSpace& operator=(HalfSpace&&) = default;

        plane_type plane() const;
        void set_plane(const plane_type& plane);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        // Exterior is defined by direction of plane normal
        std::partial_ordering compare(const point_type& point) const override;

    private:

        plane_type m_plane;
    };

    template<typename Traits> requires (Traits::dimension >= 3)
    std::ostream& operator<<(std::ostream& os, HalfSpace<Traits> plane);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::HalfSpace<Traits>::HalfSpace(const plane_type& plane) :
    PointRelation<Traits>(),
    m_plane(plane)
{
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
quetzal::geometry::HalfSpace<Traits>::HalfSpace(const point_type& point, const vector_type& normal) :
    PointRelation<Traits>(),
    m_plane(point, normal)
{
    assert(normal.unit());
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
typename quetzal::geometry::HalfSpace<Traits>::plane_type quetzal::geometry::HalfSpace<Traits>::plane() const
{
    return m_plane;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
void quetzal::geometry::HalfSpace<Traits>::set_plane(const plane_type& plane)
{
    m_plane = plane;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
std::partial_ordering quetzal::geometry::HalfSpace<Traits>::compare(const point_type& point) const
{
    static_assert(Traits::dimension >= 3);

    auto d = dot(plane().normal(), (point - plane().point()));
    return math::float_compare0(d);
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension >= 3)
std::ostream& quetzal::geometry::operator<<(std::ostream& os, HalfSpace<Traits> halfspace)
{
    os << halfspace.plane();
    return os;
}

#endif // QUETZAL_GEOMETRY_HALFSPACE_HPP
