#if !defined(QUETZAL_STRATUM_FORMAT_READ_CHARACTER_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_CHARACTER_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_character.hpp
//------------------------------------------------------------------------------

#include "CharacterDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads and validates a character file. Its name is its name component's. Every entity of the inventory has a unique
    // id and a parent, the character or another entity of the inventory, with no cycle, and is sited on its parent if
    // sited at all. Throws std::runtime_error stating what was wrong and where. stream_name identifies the stream in error
    // messages
    CharacterDocument read_character(const std::filesystem::path& filepath);
    CharacterDocument read_character(std::istream& is, const std::string& stream_name);

    // Reads the character's name from the file's header, after the format and the version, and parses nothing after it,
    // so a save can be listed without reading its character. Throws std::runtime_error if the format, the version or the
    // name is absent or wrong, or the file is not JSON up to the name
    std::string read_character_name(const std::filesystem::path& filepath);
    std::string read_character_name(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_CHARACTER_HPP
