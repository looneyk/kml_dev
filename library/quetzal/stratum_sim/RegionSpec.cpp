//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "Generation.hpp"
#include "quetzal/stratum_format/RegionSpecsDocument.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace
{

    constexpr std::string_view display_name_key = "display_name";

} // namespace

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
quetzal::stratum_sim::RegionSpec quetzal::stratum_sim::resolve_region_spec(const RegionSpec& region_spec, PatternResolver& resolver)
{
    auto resolve_member = [&resolver](std::string_view key, const std::optional<std::string>& value)
    {
        return value.has_value() ? resolver(key, value.value()) : std::nullopt;
    };

    RegionSpec resolved = region_spec;
    resolved.name = resolve_member(name_key, region_spec.name);
    resolved.display_name = resolve_member(display_name_key, region_spec.display_name);

    resolved.store.clear();
    for (const auto& [key, value] : region_spec.store)
    {
        if (key.starts_with(zone_prefix))
        {
            resolved.store.insert(key, value);
        }
        else if (std::optional<std::string> resolved_value = resolver(key, value); resolved_value.has_value())
        {
            resolved.store.insert(key, std::move(resolved_value.value()));
        }
    }

    return resolved;
}

//------------------------------------------------------------------------------
std::map<std::string, quetzal::stratum_sim::RegionSpec> quetzal::stratum_sim::load_region_specs(const std::filesystem::path& assets_directory)
{
    stratum_format::RegionSpecsDocument document = stratum_format::read_region_specs(assets_directory / stratum_format::RegionSpecsDocument::filepath);
    return std::move(document.region_specs);
}
