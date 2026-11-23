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
#include "quetzal/stratum_core/Notation.hpp"
#include <functional>
#include <map>
#include <string>

namespace quetzal::stratum_format
{
    struct ZoneDocument;
}

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The registries that World creates regions and zones from
    struct Generation
    {
    public:

        using zone_generator_type = std::function<stratum_format::ZoneDocument(const ZoneSpec& zone_spec)>;

        std::map<stratum_core::Notation, zone_generator_type> m_zone_generators;
        zone_generator_type m_zone_generator;
        std::map<std::string, RegionSpec> m_region_specs;

        WorldDefinition m_world_definition;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GENERATION_HPP
