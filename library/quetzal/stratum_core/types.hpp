#if !defined(QUETZAL_STRATUM_CORE_TYPES_HPP)
#define QUETZAL_STRATUM_CORE_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_core
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/math/Random.hpp"
#include <string_view>
#include <cstdint>

namespace quetzal::stratum_core
{

    inline constexpr std::string_view log_category = "stratum_core";

    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;

    using coord_type = std::int32_t;
    using discrete_vector_type = math::DiscreteVector<coord_type, 2>;
    using discrete_point_type = geometry::DiscretePoint<coord_type, 2>;

    // A position in its worldspace
    using zone_position_type = geometry::DiscretePoint<coord_type, 3>;

    using seed_type = math::Random<math::portable_random_engine_type>::seed_type;

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_TYPES_HPP
