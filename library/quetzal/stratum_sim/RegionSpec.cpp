//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "Generation.hpp"
#include "quetzal/stratum_format/RegionSpecsDocument.hpp"
#include <string>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionPosition quetzal::stratum_sim::RegionSpec::position() const
{
    return {keys.value_or<coord_type>(position_x_key, 0), keys.value_or<coord_type>(position_y_key, 0)};
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::RegionSpec::set_position(const RegionPosition& position)
{
    keys.insert(std::string(position_x_key), std::to_string(position.x));
    keys.insert(std::string(position_y_key), std::to_string(position.y));
    return;
}

//------------------------------------------------------------------------------
std::map<std::string, quetzal::stratum_sim::RegionSpec> quetzal::stratum_sim::load_region_specs(const std::filesystem::path& assets_directory)
{
    stratum_format::RegionSpecsDocument document = stratum_format::read_region_specs(assets_directory / stratum_format::RegionSpecsDocument::filepath);

    std::map<std::string, RegionSpec> region_specs;
    for (auto& [name, keys] : document.region_specs)
    {
        region_specs.emplace(name, RegionSpec {std::move(keys)});
    }

    return region_specs;
}
