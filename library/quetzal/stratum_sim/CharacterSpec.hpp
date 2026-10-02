#if !defined(QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP)
#define QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// CharacterSpec.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <map>
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A request to create a character: the choices a new game makes
    struct CharacterSpec
    {
        struct InventoryLocation
        {
            std::string relation; // "in", "worn", ...
            std::string place;    // "backpack", "legs", ...
        };

        struct InventoryItem
        {
            std::string type; // Used to find the template for this item
            InventoryLocation location;
            KeyValueStore properties; // name, quantity, size, weight; values that differ from the type's default values
        };

        std::string name;
        std::map<std::string, int> stats;
        std::map<std::string, int> skills;
        std::map<std::string, int> spells;
        std::vector<InventoryItem> inventory;

        // The game's own choices, which data defines: race, class_role, background, appearance, level, ...
        KeyValueStore properties;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP
