#if !defined(QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// WorldDefinition.hpp
//------------------------------------------------------------------------------

#include "RandomStream.hpp"
#include "RegionSpec.hpp"
#include "RegionSpecNode.hpp"
#include "ScriptLimits.hpp"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The region spec trees that a new game's world is created from
    struct WorldDefinition
    {
        // The region specs of one tree, the root spec first and each spec before its subregions
        using region_spec_tree_type = std::vector<RegionSpecNode>;

        // <root region name, tree>
        std::map<std::string, region_spec_tree_type> region_spec_trees;

        // The name of the root region of the tree a new game starts in
        std::string starting_region;
    };

    // Returns the world definition from the world definition file in the assets directory
    // Throws std::runtime_error if the file cannot be read or the reader rejects it, a tree does not have exactly one root
    // spec, a spec has no name, a parent is not the name of exactly one spec of the tree, two specs with the same parent have
    // the same name, a spec is not below the root, a root spec has the name of another tree's root, or the starting region
    // is not a tree's root
    WorldDefinition load_world_definition(const std::filesystem::path& assets_directory);

    // Returns the tree with each spec composed over its parent's resolved spec and its patterns resolved, parents first
    // Throws std::runtime_error if a spec is based on a region spec that is not registered, or for a pattern that does not
    // compile, fails, or has a table or a function as its value
    WorldDefinition::region_spec_tree_type resolve_region_spec_tree(const WorldDefinition::region_spec_tree_type& tree,
        const std::map<std::string, RegionSpec>& region_specs, RandomStream::seed_type world_seed, const ScriptLimits& script_limits);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP
