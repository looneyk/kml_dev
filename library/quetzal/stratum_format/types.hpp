#if !defined(QUETZAL_STRATUM_FORMAT_TYPES_HPP)
#define QUETZAL_STRATUM_FORMAT_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_format
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <cstdint>

namespace quetzal::stratum_format
{

    // These match stratum_sim's, so a layout is built without conversion
    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;

    using coord_type = std::int32_t;
    using discrete_vector_type = math::DiscreteVector<coord_type, 2>;
    using discrete_point_type = geometry::DiscretePoint<coord_type, 2>;

    // An entity id in a zone file: a local id in a zone template, a persistent id in a saved zone
    using stored_entity_id_type = std::uint64_t;

    // Values passed back to the caller unprocessed, in their original order
    using values_type = nlohmann::ordered_json;

    // A position on the surface of a wall or a flat cell
    using surface_point_type = geometry::Point<math::CartesianTraits<value_type, 2>>;

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_TYPES_HPP
