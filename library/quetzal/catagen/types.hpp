#if !defined(QUETZAL_CATAGEN_TYPES_HPP)
#define QUETZAL_CATAGEN_TYPES_HPP
//------------------------------------------------------------------------------
// catagen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include <compare>

namespace quetzal::catagen
{

    using coord_type = long;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using point_type = geometry::DiscretePoint<coord_type, 2>;
    using box_type = geometry::DiscreteBox<coord_type, 2>;

    //--------------------------------------------------------------------------
    enum class CellType
    {
        Solid,
        Open,
        StairsDown,
        StairsUp
    };

    //--------------------------------------------------------------------------
    enum class WallType
    {
        Open,
        Solid,
        Door
    };

    //--------------------------------------------------------------------------
    enum class Direction
    {
        East,
        North,
        West,
        South
    };

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_TYPES_HPP
