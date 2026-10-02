#if !defined(QUETZAL_STRATUM_FORMAT_WRITE_GAME_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITE_GAME_HPP
//------------------------------------------------------------------------------
// stratum_format
// write_game.hpp
//------------------------------------------------------------------------------

#include "GameDocument.hpp"
#include <filesystem>
#include <iosfwd>

namespace quetzal::stratum_format
{

    // Writes the game file in the written form that game_file_spec.md specifies: the game and the settings each on a line
    void write(const std::filesystem::path& filepath, const GameDocument& document);
    void write(std::ostream& os, const GameDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_GAME_HPP
