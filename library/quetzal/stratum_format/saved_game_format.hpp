#if !defined(QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP)
#define QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP
//------------------------------------------------------------------------------
// stratum_format
// saved_game_format.hpp
//------------------------------------------------------------------------------

#include <string_view>

namespace quetzal::stratum_format
{

    // The format of a saved game's game, character and world files; a zone file has the zone file format's own
    constexpr std::string_view saved_game_format_name = "quetzal.stratum_game";
    constexpr std::string_view saved_game_format_version = "0.1";

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP
