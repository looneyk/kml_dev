#if !defined(QUETZAL_STRATUM_FORMAT_READ_GAME_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_GAME_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_game.hpp
//------------------------------------------------------------------------------

#include "GameDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads and validates a game file. Throws std::runtime_error stating what was wrong and where. stream_name identifies the stream in error messages
    GameDocument read_game(const std::filesystem::path& filepath);
    GameDocument read_game(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_GAME_HPP
