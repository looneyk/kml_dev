#if !defined(QUETZAL_STRATUM_SIM_GENERATION_HPP)
#define QUETZAL_STRATUM_SIM_GENERATION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Generation.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <flecs.h>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_format
{
    struct Document;
}

namespace quetzal::stratum_sim
{

    class World;

    using RegionSpec = KeyValueStore; // type, terrain, competency level ...
    using ZoneSpec = KeyValueStore;

    // The RegionSpec key of the region's generator selector
    inline constexpr std::string_view generator_selector_key = "generator_selector";

    // The RegionSpec key of the access point type and level offset at which a new game's character enters the starting
    // region's first zone, written as the type, a comma, and the offset: "staircase,1"
    inline constexpr std::string_view entry_access_key = "entry_access";

    // The prefix of a RegionSpec's keys that are the ZoneSpec of each of its zones
    inline constexpr std::string_view zone_prefix = "zone.";

    // ZoneSpec keys
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

        // Exactly one of generator_name, zone_template and region_spec_name is set
        struct GeneratorSelection
        {
            std::optional<std::string> generator_name;   // A generator creates the zone
            std::optional<std::string> zone_template;    // A pre-built zone, by its tag in the pregenerated content index
            std::optional<std::string> region_spec_name; // The zone is the first of a new region, created from this region spec
            ZoneSpec zone_spec;                          // The request, completed with the generator's parameters
        };

        using generator_selector_type = std::function<GeneratorSelection(const World& world, flecs::entity region, const ZoneSpec& zone_spec)>;
        using generator_type = std::function<stratum_format::Document(const ZoneSpec& zone_spec)>;

        std::map<std::string, generator_selector_type> m_generator_selectors;
        std::map<std::string, generator_type> m_generators;
        std::map<std::string, RegionSpec> m_region_specs;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATION_HPP
