#if !defined(QUETZAL_STRATUM_SIM_DISPLAYNAME_HPP)
#define QUETZAL_STRATUM_SIM_DISPLAYNAME_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DisplayName.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <string>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // What an entity is called for a player to read. It identifies nothing. The file component name decodes into it
    struct DisplayName
    {
        std::string name;
    };

    // Registers DisplayName with its reflection as a string, so a file's "name": "..." decodes into it
    void register_display_name(flecs::world& entities);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DISPLAYNAME_HPP
