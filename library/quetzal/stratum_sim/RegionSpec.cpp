//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "Generation.hpp"
#include "quetzal/stratum_format/RegionSpecsDocument.hpp"
#include <string>
#include <utility>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::region_position(const RegionSpec& region_spec)
{
    return {region_spec.store.value_or<coord_type>(position_x_key, 0), region_spec.store.value_or<coord_type>(position_y_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_region_position(RegionSpec& region_spec, const RegionPosition& position)
{
    region_spec.store.insert(std::string(position_x_key), std::to_string(position.x));
    region_spec.store.insert(std::string(position_y_key), std::to_string(position.y));
    return;
}

//------------------------------------------------------------------------------
std::map<std::string, quetzal::stratum_sim::RegionSpec> quetzal::stratum_sim::load_region_specs(const std::filesystem::path& assets_directory)
{
    stratum_format::RegionSpecsDocument document = stratum_format::read_region_specs(assets_directory / stratum_format::RegionSpecsDocument::filepath);
    return std::move(document.region_specs);
}
