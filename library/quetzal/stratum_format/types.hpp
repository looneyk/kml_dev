#if !defined(QUETZAL_STRATUM_FORMAT_TYPES_HPP)
#define QUETZAL_STRATUM_FORMAT_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_format
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/stratum_core/types.hpp"

// nlohmann/json 3.12.0's binary_reader has code after an if constexpr that returns, which MSVC reports as C4702 whatever
// the external warning level, since the warning comes from code generation
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
#include <nlohmann/json.hpp>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <array>
#include <string_view>
#include <cstdint>

namespace quetzal::stratum_format
{

    // The category of stratum_format's entries in the process-wide log
    inline constexpr std::string_view log_category = "stratum_format";

    using stratum_core::value_type;
    using stratum_core::traits_type;
    using stratum_core::point_type;
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

    // An entity id in a zone file: a local id in a zone template, a persistent id in a saved zone
    using stored_entity_id_type = std::uint64_t;

    // Values the format does not interpret, passed back to the caller as written and in their original order
    using raw_values_type = nlohmann::ordered_json;

    // A position on the surface of a wall or a flat cell
    using surface_point_type = geometry::Point<math::CartesianTraits<value_type, 2>>;

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_TYPES_HPP
