#if !defined(QUETZAL_CATAGEN_TYPES_HPP)
#define QUETZAL_CATAGEN_TYPES_HPP
//------------------------------------------------------------------------------
// catagen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/geometry/DiscreteDirectedPoint.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <compare>
#include <cstdint>

namespace quetzal::catagen
{

    using coord_type = std::int32_t;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using point_type = geometry::DiscretePoint<coord_type, 2>;
    using box_type = geometry::DiscreteBox<coord_type, 2>;
    using directed_point_type = geometry::DiscreteDirectedPoint<coord_type, 2>;

    // A zone's position in its region
    using zone_position_type = geometry::DiscretePoint<coord_type, 3>;

    using math::CardinalDirection;

    inline const auto& cardinal_directions = math::cardinal_directions<coord_type, 2>;

    inline const vector_type& cardinal_direction(CardinalDirection direction)
    {
        return math::cardinal_direction<coord_type, 2>(direction);
    }

    //--------------------------------------------------------------------------
    enum class CellType
    {
        Solid,
        Open,
        Staircase
    };

    //--------------------------------------------------------------------------
    enum class WallType
    {
        Open,
        Solid,
        Door
    };

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_TYPES_HPP
