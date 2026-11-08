#if !defined(QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP)
#define QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP
//------------------------------------------------------------------------------
// stratum_sim
// WorldDefinition.hpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
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
        // The region specs of one tree, its root spec first
        using region_spec_tree_type = std::vector<RegionSpec>;

        // <root region name, tree>
        std::map<std::string, region_spec_tree_type> region_spec_trees;

        // The name of the root region of the tree a new game starts in
        std::string starting_region;
    };

    // Returns the world definition from the world definition file in the assets directory
    // Throws std::runtime_error if the file cannot be read or the reader rejects it, a tree does not have exactly one root
    // spec, a root spec has no name or the name of another tree's root, or the starting region is not a tree's root
    WorldDefinition load_world_definition(const std::filesystem::path& assets_directory);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WORLD_DEFINITION_HPP
