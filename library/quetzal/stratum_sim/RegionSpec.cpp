//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.cpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "Generation.hpp"
#include "stratum_sim_util.hpp"
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
quetzal::stratum_sim::RegionSpec quetzal::stratum_sim::composed_region_spec(const RegionSpec& region_spec,
    const std::map<std::string, RegionSpec>& region_specs)
{
    RegionSpec composed;
    if (region_spec.region_spec.has_value())
    {
        composed = value_at(region_specs, region_spec.region_spec.value(), "region spec");
    }

    auto layer = []<typename T>(std::optional<T>& member, const std::optional<T>& over)
    {
        if (over.has_value())
        {
            member = over;
        }

        return;
    };

    layer(composed.name, region_spec.name);
    layer(composed.display_name, region_spec.display_name);
    layer(composed.seed, region_spec.seed);
    layer(composed.extent, region_spec.extent);
    layer(composed.zone_size, region_spec.zone_size);
    composed.store |= region_spec.store;
    composed.region_spec.reset();
    composed.parent.reset();
    return composed;
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
