#if !defined(QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// GameDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <map>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of a game file: the game's own state and its settings
    struct GameDocument
    {
        std::string version;
        stored_entity_id_type next_id = 0;
        std::string seed;
        std::map<std::string, std::string> engines; // <stream, state>
        raw_values_type settings;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_GAMEDOCUMENT_HPP
