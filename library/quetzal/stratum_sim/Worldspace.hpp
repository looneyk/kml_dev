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
    // The coordinate system of its zones, and the tree of regions rooted in it
    struct Worldspace
    {
        using extent_type = geometry::DiscreteBox<coord_type, 3>;

        // Returns a worldspace with the extent and zone size from a top-level region spec's worldspace keys, the top-level region, and no zones.
        // Throws std::runtime_error on an invalid extent of interval format
        static Worldspace create(PersistentId worldspace_id, std::string_view name, const RegionSpec& region_spec, Region root_region);

        bool contains(const ZonePosition& position) const;

        PersistentId worldspace_id;
        std::string name;
        extent_type extent;
        std::optional<discrete_vector_type> zone_size; // A grid worldspace's
        std::map<ZonePosition, PersistentId> zone_ids; // <position, zone_id>, loaded or saved
        RegionTree regions;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLDSPACE_HPP
