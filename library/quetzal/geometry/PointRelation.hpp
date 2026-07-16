#if !defined(QUETZAL_GEOMETRY_POINTRELATION_HPP)
#define QUETZAL_GEOMETRY_POINTRELATION_HPP
//------------------------------------------------------------------------------
// geometry
// PointRelation.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "quetzal/math/floating_point.hpp"
#include <compare>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    template<typename Traits>
    class PointRelation
    {
    public:

        using point_type = Point<Traits>;

        PointRelation() = default;
        PointRelation(const PointRelation&) = default;
        PointRelation(PointRelation&&) = default;
        virtual ~PointRelation() = default;

        PointRelation& operator=(const PointRelation&) = default;
        PointRelation& operator=(PointRelation&&) = default;

        // disjoint, touches, contains, covers ...
        bool exterior(const point_type& point, int tolerance = math::tolerance_default) const;
        bool boundary(const point_type& point, int tolerance = math::tolerance_default) const;
        bool interior(const point_type& point, int tolerance = math::tolerance_default) const;
        bool contains(const point_type& point, int tolerance = math::tolerance_default) const; // interior || boundary

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        virtual std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const = 0;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PointRelation<Traits>::exterior(const point_type& point, int tolerance) const
{
    return compare(point, tolerance) > 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PointRelation<Traits>::boundary(const point_type& point, int tolerance) const
{
    return compare(point, tolerance) == 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PointRelation<Traits>::interior(const point_type& point, int tolerance) const
{
    return compare(point, tolerance) < 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PointRelation<Traits>::contains(const point_type& point, int tolerance) const
{
    return compare(point, tolerance) <= 0;
}

#endif // QUETZAL_GEOMETRY_POINTRELATION_HPP
