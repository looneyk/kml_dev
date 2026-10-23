#if !defined(QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP)
#define QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP
//------------------------------------------------------------------------------
// stratum_format
// saved_game_format.hpp
//------------------------------------------------------------------------------

#include <string_view>

namespace quetzal::stratum_format
{

    // The format of a saved game's game, character and world files; a zone file has its own format
    constexpr std::string_view saved_game_format_name = "quetzal.stratum_game";
    constexpr std::string_view saved_game_format_version = "0.1";

    // The files of a game directory or a save, each in that directory
    constexpr std::string_view game_filename = "game.json";
    constexpr std::string_view character_filename = "character.json";
    constexpr std::string_view world_filename = "world.json";

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_SAVED_GAME_FORMAT_HPP
