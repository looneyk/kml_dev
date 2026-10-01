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

    // The ZoneSpec key of the zone's level
    inline constexpr std::string_view level_key = "level";

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
