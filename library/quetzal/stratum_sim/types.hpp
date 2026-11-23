#if !defined(QUETZAL_STRATUM_SIM_TYPES_HPP)
#define QUETZAL_STRATUM_SIM_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/CardinalDirection.hpp"
#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/MinProjection.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_core/PersistentId.hpp"
#include "quetzal/stratum_core/types.hpp"
#include <vector>
#include <cstdint>

namespace quetzal::stratum_sim
{

    using stratum_core::PersistentId;

    using stratum_core::value_type;
    using stratum_core::traits_type;
    using stratum_core::point_type;
    using stratum_core::coord_type;
    using stratum_core::discrete_vector_type;
    using stratum_core::discrete_point_type;
    using stratum_core::seed_type;
    using stratum_core::zone_position_type;

    using vector_type = point_type::vector_type;
    using points_type = std::vector<point_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    // Position and facing direction
    using placement_type = geometry::DirectedPoint<traits_type>;

    // Projection along the z axis onto the xy plane
    using point_projection_type = geometry::MinProjection<point_type>;
    using vector_projection_type = geometry::MinProjection<vector_type>;

    // A zone's position relative to the surface: 0 at the surface, negative going down and positive going up
    using level_type = std::int32_t;

    using random_type = math::Random<math::portable_random_engine_type>;

    using geometry::CardinalDirection;

    inline const auto& cardinal_directions = geometry::cardinal_directions<coord_type, 2>;

    inline const discrete_vector_type& cardinal_direction(CardinalDirection direction)
    {
        return geometry::cardinal_direction<coord_type, 2>(direction);
    }

    // The cardinal direction nearest the given direction
    inline discrete_vector_type grid_step(const vector_type& direction)
    {
        static const vector_projection_type project;
        return math::direction<discrete_vector_type>(project(direction));
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_TYPES_HPP
