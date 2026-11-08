#if !defined(QUETZAL_STRATUM_SIM_GENERATION_HPP)
#define QUETZAL_STRATUM_SIM_GENERATION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Generation.hpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "WorldDefinition.hpp"
#include "ZoneSpec.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace quetzal::stratum_format
{
    struct Document;
}

namespace quetzal::stratum_sim
{

    // The RegionSpec key of the pattern of its zones' names, in which each {...} is replaced: {region_name} by the region's
    // name, and any other by the value of an integer expression in the zone's coordinates x, y, and z, such as {abs(z)}
    inline constexpr std::string_view zone_name_pattern_key = "zone_name_pattern";

    // The prefix of a RegionSpec's keys that are the ZoneSpec of each of its zones
    inline constexpr std::string_view zone_prefix = "zone.";

    // RegionSpec keys: the registered region spec it is based on, and the name of its parent region in a region spec tree
    inline constexpr std::string_view region_spec_key = "region_spec";
    inline constexpr std::string_view parent_key = "parent";

    // ZoneSpec and RegionSpec keys
    inline constexpr std::string_view name_key = "name";
    inline constexpr std::string_view seed_key = "seed";

    // The ZoneSpec and RegionSpec keys of a position's coordinates. A region's position has no z
    inline constexpr std::string_view position_x_key = "position.x";
    inline constexpr std::string_view position_y_key = "position.y";
    inline constexpr std::string_view position_z_key = "position.z";
    inline constexpr std::string_view parameters_prefix = "parameters.";

    // The prefix of a top-level RegionSpec's keys for its worldspace, and the keys under it: the extent on each axis, an
    // interval written as "-10 .. -1", and a grid worldspace's zone size
    inline constexpr std::string_view worldspace_prefix = "worldspace.";
    inline constexpr std::string_view extent_x_key = "extent.x";
    inline constexpr std::string_view extent_y_key = "extent.y";
    inline constexpr std::string_view extent_z_key = "extent.z";
    inline constexpr std::string_view zone_size_x_key = "zone_size.x";
    inline constexpr std::string_view zone_size_y_key = "zone_size.y";

    //--------------------------------------------------------------------------
    // The registries that World creates regions and zones from
    struct Generation
    {
    public:

        using zone_generator_type = std::function<stratum_format::Document(const ZoneSpec& zone_spec)>;

        std::map<stratum_format::Notation, zone_generator_type> m_zone_generators;
        zone_generator_type m_zone_generator;
        std::map<std::string, RegionSpec> m_region_specs;

        WorldDefinition m_world_definition;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATION_HPP
