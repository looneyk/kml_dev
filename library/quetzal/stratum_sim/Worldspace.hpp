#if !defined(QUETZAL_STRATUM_SIM_WORLDSPACE_HPP)
#define QUETZAL_STRATUM_SIM_WORLDSPACE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Worldspace.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "Region.hpp"
#include "RegionSpec.hpp"
#include "RegionTree.hpp"
#include "Zone.hpp"
#include "types.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    struct Worldspace
    {
        using extent_type = geometry::DiscreteBox<coord_type, 3>;

        // Returns an initialized but empty worldspace
        // Throws std::runtime_error if the spec has no extent, or one that is not ranges with a single box with every axis
        static Worldspace create(PersistentId worldspace_id, std::string_view name, seed_type seed, const RegionSpec& region_spec, Region root_region);

        bool contains(const ZonePosition& position) const;

        PersistentId worldspace_id;
        std::string name;
        seed_type seed;
        extent_type extent;
        std::optional<discrete_vector_type> zone_size; // A grid worldspace's
        std::map<ZonePosition, PersistentId> zone_ids; // <position, zone_id>
        RegionTree regions;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLDSPACE_HPP
