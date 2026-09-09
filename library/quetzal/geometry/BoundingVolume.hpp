#if !defined(QUETZAL_GEOMETRY_BOUNDINGVOLUME_HPP)
#define QUETZAL_GEOMETRY_BOUNDINGVOLUME_HPP
//------------------------------------------------------------------------------
// geometry
// BoundingVolume.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "PointRelation.hpp"
#include <ostream>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class BoundingVolume : public PointRelation<Traits>
    {
    public:

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;

        BoundingVolume() = default;
        BoundingVolume(const BoundingVolume&) = default;
        BoundingVolume(BoundingVolume&&) = default;
        ~BoundingVolume() = default;

        BoundingVolume& operator=(const BoundingVolume&) = default;
        BoundingVolume& operator=(BoundingVolume&&) = default;

        virtual point_type centroid() const = 0;
        virtual value_type volume() const = 0;
        virtual bool empty() const = 0;

        virtual void clear() = 0;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        virtual std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const = 0;

        virtual void print(std::ostream& os) const = 0;

        // The point on the volume's surface farthest along direction -- the GJK support mapping.
        // This is the one primitive GJK-based intersection testing needs; unlike compare()/centroid(),
        // it generalizes to volume-vs-volume tests between different concrete shapes without needing
        // to know the other shape's type, since GJK only ever calls a.support(d) and b.support(d)
        // independently of each other.
        virtual point_type support(const vector_type& direction) const = 0;
    };

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const BoundingVolume<Traits>& bv);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const BoundingVolume<Traits>& bv)
{
    bv.print(os);
    return os;
}

#endif // QUETZAL_GEOMETRY_BOUNDINGVOLUME_HPP
