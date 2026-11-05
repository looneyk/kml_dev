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
#include "quetzal/math/Interval.hpp"
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Inclusive interval extents, separately on each axis, may be unspecified to indicate no bounds on that axis
    struct WorldspaceExtent
    {
        bool contains(const ZonePosition& position) const;

        std::optional<math::Interval<coord_type>> x;
        std::optional<math::Interval<coord_type>> y;
        std::optional<math::Interval<coord_type>> z;
    };

    //--------------------------------------------------------------------------
    // The coordinate system of its zones, and the tree of regions rooted in it
    struct Worldspace
    {
        // Returns a worldspace with the extent and zone size in a top-level region spec's worldspace keys, the top-level
        // region, and no zones. Throws std::runtime_error if an extent is not an interval of integers, or the zone size has
        // one coordinate without the other
        static Worldspace create(PersistentId worldspace_id, std::string_view name, const RegionSpec& region_spec, Region root_region);

        PersistentId worldspace_id;
        std::string name;
        WorldspaceExtent extent;
        std::optional<discrete_vector_type> zone_size; // A grid worldspace's
        std::map<ZonePosition, PersistentId> zone_ids; // <position, zone_id>, loaded or saved
        RegionTree regions;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLDSPACE_HPP
