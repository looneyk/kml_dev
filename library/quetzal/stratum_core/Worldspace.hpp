#if !defined(QUETZAL_STRATUM_CORE_WORLDSPACE_HPP)
#define QUETZAL_STRATUM_CORE_WORLDSPACE_HPP
//------------------------------------------------------------------------------
// stratum_core
// Worldspace.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "PersistentId.hpp"
#include "Region.hpp"
#include "RegionSpec.hpp"
#include "RegionTree.hpp"
#include "types.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    struct Worldspace
    {
        using extent_type = geometry::DiscreteBox<coord_type, 3>;

        // Returns an empty initialized worldspace
        // Throws std::runtime_error if the spec does not have an extent appropriate for a worldspace (fully specified, single rectangular area)
        static Worldspace create(PersistentId worldspace_id, std::string_view name, seed_type seed, const RegionSpec& region_spec, Region root_region);

        bool contains(const zone_position_type& position) const;

        PersistentId worldspace_id;
        std::string name;
        seed_type seed;
        extent_type extent;
        std::optional<discrete_vector_type> zone_size;
        RegionTree regions;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_WORLDSPACE_HPP
