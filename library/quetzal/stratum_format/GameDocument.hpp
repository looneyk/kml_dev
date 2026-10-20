#if !defined(QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// GameDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
#include <map>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of a game file: the game's own state and its game settings
    struct GameDocument
    {
        std::string version;
        stored_entity_id_type next_id = 0;
        std::string seed;
        std::map<std::string, std::string> random_engines; // <stream, state>
        settings::SettingsSnapshot settings_snapshot; // The game scope's
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP
