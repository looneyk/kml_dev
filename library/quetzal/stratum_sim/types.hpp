#if !defined(QUETZAL_STRATUM_SIM_TYPES_HPP)
#define QUETZAL_STRATUM_SIM_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/MinProjection.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <vector>
#include <cstdint>

namespace quetzal::stratum_sim
{

    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;
    using vector_type = point_type::vector_type;
    using points_type = std::vector<point_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    // Position and facing direction
    using placement_type = geometry::DirectedPoint<traits_type>;

    // Projection along the z axis onto the xy plane
    using point_projection_type = geometry::MinProjection<point_type>;
    using vector_projection_type = geometry::MinProjection<vector_type>;

    using coord_type = std::int32_t;
    using discrete_vector_type = math::DiscreteVector<coord_type, 2>;
    using discrete_point_type = geometry::DiscretePoint<coord_type, 2>;

    using math::CardinalDirection;

    inline const auto& cardinal_directions = math::cardinal_directions<coord_type, 2>;

    inline const discrete_vector_type& cardinal_direction(CardinalDirection direction)
    {
        return math::cardinal_direction<coord_type, 2>(direction);
    }

    // The cardinal direction nearest the given direction
    inline discrete_vector_type grid_step(const vector_type& direction)
    {
        static const vector_projection_type project;
        return math::direction<discrete_vector_type>(project(direction));
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_TYPES_HPP
