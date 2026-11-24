#if !defined(QUETZAL_CATAGEN_TYPES_HPP)
#define QUETZAL_CATAGEN_TYPES_HPP
//------------------------------------------------------------------------------
// catagen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/geometry/CardinalDirection.hpp"
#include "quetzal/stratum_core/types.hpp"
#include <compare>

namespace quetzal::catagen
{

    using stratum_core::coord_type;
    using stratum_core::discrete_vector2_type;
    using stratum_core::discrete_point2_type;
    using stratum_core::discrete_vector3_type;
    using stratum_core::discrete_point3_type;
    using stratum_core::discrete_box2_type;
    using stratum_core::discrete_box3_type;
    using stratum_core::discrete_directed_point2_type;
    using stratum_core::discrete_directed_point3_type;
    using stratum_core::random_type;
    using stratum_core::seed_type;

    using geometry::CardinalDirection;

    inline const auto& cardinal_directions = geometry::cardinal_directions<coord_type, 2>;

    inline const discrete_vector2_type& cardinal_direction(CardinalDirection direction)
    {
        return geometry::cardinal_direction<coord_type, 2>(direction);
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
