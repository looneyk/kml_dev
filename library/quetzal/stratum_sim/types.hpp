#if !defined(QUETZAL_STRATUM_SIM_TYPES_HPP)
#define QUETZAL_STRATUM_SIM_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;
    using vector_type = point_type::vector_type;
    using points_type = std::vector<point_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    // Where a mover is and which way it faces. Relative movement passes its own direction;
    // absolute movement passes one the view chose and leaves the mover's alone
    using placement_type = geometry::DirectedPoint<traits_type>;

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_TYPES_HPP
