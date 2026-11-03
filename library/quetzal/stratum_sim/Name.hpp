#if !defined(QUETZAL_STRATUM_SIM_NAME_HPP)
#define QUETZAL_STRATUM_SIM_NAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Name.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include <flecs.h>
#include <concepts>
#include <cstddef>
#include <format>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The name of an entity. A zone's is unique in its region. The file component name decodes into it
    struct Name
    {
        std::string name;
    };

    // The name of Name's file component
    constexpr std::string_view name_component = "name";

    //--------------------------------------------------------------------------
    // The name of an entity for a player to read. The file component display_name decodes into it, and an entity that
    // has a Name and none has a placeholder: the name in capitals
    struct DisplayName
    {
        std::string name;
    };

    // The name of DisplayName's file component
    constexpr std::string_view display_name_component = "display_name";

    // Registers Name and DisplayName with their reflection as strings, so a file's "name": "..." and "display_name": "..."
    // decode into them, and an observer that gives an entity whose Name is set the placeholder DisplayName if it has none
    void register_names(flecs::world& entities);

    // Returns the pattern with each "{x}", "{y}" and "{z}" replaced by that coordinate of the position
    std::string zone_name(std::string_view pattern, const ZonePosition& position);

    // Returns the name if it is not taken, and otherwise the name with the suffix _<counter> for the first counter after
    // the given one whose name is not taken, which becomes the counter
    template<std::predicate<const std::string&> Taken>
    std::string untaken_name(const std::string& name, std::size_t& counter, Taken taken);

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<std::predicate<const std::string&> Taken>
std::string quetzal::stratum_sim::untaken_name(const std::string& name, std::size_t& counter, Taken taken)
{
    std::string untaken = name;

    while (taken(untaken))
    {
        untaken = std::format("{}_{}", name, ++counter);
    }

    return untaken;
}

#endif // QUETZAL_STRATUM_SIM_NAME_HPP
