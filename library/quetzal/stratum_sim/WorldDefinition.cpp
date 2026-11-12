//------------------------------------------------------------------------------
// stratum_sim
// WorldDefinition.cpp
//------------------------------------------------------------------------------

#include "WorldDefinition.hpp"
#include "Generation.hpp"
#include "PatternResolver.hpp"
#include "stratum_sim_util.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/stratum_format/WorldDefinitionDocument.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{

    //--------------------------------------------------------------------------
    // Returns the parent's resolved fields and store entries excluding zone entries
    quetzal::KeyValueStore parent_symbols(const quetzal::stratum_sim::RegionSpec& parent)
    {
        quetzal::KeyValueStore symbols;
        if (parent.name.has_value())
        {
            symbols.insert("parent.name", parent.name.value());
        }

        if (parent.display_name.has_value())
        {
            symbols.insert("parent.display_name", parent.display_name.value());
        }

        for (const auto& [key, value] : parent.store)
        {
            if (!key.starts_with(quetzal::stratum_sim::zone_prefix))
            {
                symbols.insert(std::format("parent.{}", key), value);
            }
        }

        return symbols;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::WorldDefinition quetzal::stratum_sim::load_world_definition(const std::filesystem::path& assets_directory)
{
    const std::filesystem::path filepath = assets_directory / stratum_format::WorldDefinitionDocument::filepath;
    stratum_format::WorldDefinitionDocument document = stratum_format::read_world_definition(filepath);

    auto fail = [&filepath](std::string_view problem)
    {
        const std::string message = std::format("{}: {}", filepath.generic_string(), problem);
        log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    };

    auto unparented = [](const RegionSpec& region_spec) { return !region_spec.parent.has_value(); };

    WorldDefinition definition;
    for (size_t i = 0; i < document.region_specs.size(); ++i)
    {
        const stratum_format::WorldDefinitionDocument::region_spec_tree_type& document_tree = document.region_specs[i];
        if (std::ranges::count_if(document_tree, unparented) != 1)
        {
            fail(std::format("region spec tree {} does not have exactly one root spec, a spec with no parent", i));
        }

        for (size_t j = 0; j < document_tree.size(); ++j)
        {
            if (!document_tree[j].name.has_value())
            {
                fail(std::format("region spec {} of region spec tree {} has no name", j, i));
            }
        }

        for (const RegionSpec& region_spec : document_tree)
        {
            if (region_spec.parent.has_value() && std::ranges::count(document_tree, region_spec.parent, &RegionSpec::name) != 1)
            {
                fail(std::format("the parent {} of region spec {} is not the name of exactly one spec of its tree", region_spec.parent.value(),
                    region_spec.name.value()));
            }

            auto sibling_namesake = [&region_spec](const RegionSpec& other)
            {
                return other.parent == region_spec.parent && other.name == region_spec.name;
            };

            if (std::ranges::count_if(document_tree, sibling_namesake) != 1)
            {
                fail(std::format("two region specs with the parent {} have the name {}", region_spec.parent.value(), region_spec.name.value()));
            }
        }

        const RegionSpec& root = *std::ranges::find_if(document_tree, unparented);
        const std::string& root_name = root.name.value();
        if (definition.region_spec_trees.contains(root_name))
        {
            fail(std::format("two region spec trees have the root {}", root_name));
        }

        WorldDefinition::region_spec_tree_type& tree = definition.region_spec_trees[root_name];
        tree.push_back({.region_spec = root});
        for (id_type parent_id = 0; parent_id < tree.size(); ++parent_id)
        {
            for (const RegionSpec& region_spec : document_tree)
            {
                if (region_spec.parent == tree[parent_id].region_spec.name)
                {
                    tree[parent_id].subregion_ids.push_back(tree.size());
                    tree.push_back({.region_spec = region_spec, .parent_id = parent_id});
                }
            }
        }

        if (tree.size() != document_tree.size())
        {
            fail(std::format("region spec tree {} has a spec that is not below its root", root_name));
        }
    }

    if (!definition.region_spec_trees.contains(document.starting_region))
    {
        fail(std::format("the starting region {} is not the root of a region spec tree", document.starting_region));
    }

    definition.starting_region = std::move(document.starting_region);
    return definition;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::WorldDefinition::region_spec_tree_type quetzal::stratum_sim::resolve_region_spec_tree(
    const WorldDefinition::region_spec_tree_type& tree, const std::map<std::string, RegionSpec>& region_specs, PatternResolver::random_type& random,
    seed_type world_seed, const ScriptLimits& script_limits)
{
    WorldDefinition::region_spec_tree_type resolved = tree;
    for (RegionSpecNode& node : resolved)
    {
        const seed_type seed = random.seed();
        const RegionSpec composed = composed_region_spec(node.region_spec, region_specs);
        const KeyValueStore symbols = node.parent_id != nullid ? parent_symbols(resolved[node.parent_id].region_spec) : KeyValueStore {};

        PatternResolver resolver(symbols, PatternResolver::random_type(composed.seed.value_or(seed)), world_seed, script_limits);
        RegionSpec resolved_spec = resolve_region_spec(composed, resolver);
        if (node.parent_id != nullid)
        {
            KeyValueStore inherited = resolved[node.parent_id].region_spec.store;
            for (const auto& [key, value] : composed.store)
            {
                if (!resolved_spec.store.contains(key))
                {
                    inherited.remove(key);
                }
            }

            resolved_spec.store = inherited | resolved_spec.store;
        }

        node.region_spec = std::move(resolved_spec);
    }

    return resolved;
}
