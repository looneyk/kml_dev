#if !defined(QUETZAL_STRATUM_SIM_NAME_HPP)
#define QUETZAL_STRATUM_SIM_NAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Name.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The name of an entity. The file component name decodes into it
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

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_NAME_HPP
