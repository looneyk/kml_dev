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
