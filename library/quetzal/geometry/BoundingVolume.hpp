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

        using point_type = Point<Traits>;

        BoundingVolume() = default;
        BoundingVolume(const BoundingVolume&) = default;
        BoundingVolume(BoundingVolume&&) = default;
        ~BoundingVolume() = default;

        BoundingVolume& operator=(const BoundingVolume&) = default;
        BoundingVolume& operator=(BoundingVolume&&) = default;

        virtual point_type centroid() const = 0;

        virtual void clear() = 0;

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        virtual std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const = 0;

        virtual bool empty() const = 0;

        virtual void print(std::ostream& os) const = 0;
    };

    // The bounding volumes do not intersect
//    template<typename Traits, typename B> requires (std::is_base_of_v<quetzal::geometry::BoundingVolume<Traits>, B>)
//    bool disjoint(const B& a, const B& b);

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
