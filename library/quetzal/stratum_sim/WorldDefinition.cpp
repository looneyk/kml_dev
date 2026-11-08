//------------------------------------------------------------------------------
// stratum_sim
// WorldDefinition.cpp
//------------------------------------------------------------------------------

#include "WorldDefinition.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/stratum_format/WorldDefinitionDocument.hpp"
#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

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
        stratum_format::WorldDefinitionDocument::region_spec_tree_type& document_tree = document.region_specs[i];
        if (std::ranges::count_if(document_tree, unparented) != 1)
        {
            fail(std::format("region spec tree {} does not have exactly one root spec, a spec with no parent", i));
        }

        auto root = std::ranges::find_if(document_tree, unparented);
        if (!root->name.has_value())
        {
            fail(std::format("the root spec of region spec tree {} has no name", i));
        }

        const std::string root_name = *root->name;
        if (definition.region_spec_trees.contains(root_name))
        {
            fail(std::format("two region spec trees have the root {}", root_name));
        }

        WorldDefinition::region_spec_tree_type& tree = definition.region_spec_trees[root_name];
        tree.push_back(std::move(*root));
        for (auto j = document_tree.begin(); j != document_tree.end(); ++j)
        {
            if (j != root)
            {
                tree.push_back(std::move(*j));
            }
        }
    }

    if (!definition.region_spec_trees.contains(document.starting_region))
    {
        fail(std::format("the starting region {} is not the root of a region spec tree", document.starting_region));
    }

    definition.starting_region = std::move(document.starting_region);
    return definition;
}
