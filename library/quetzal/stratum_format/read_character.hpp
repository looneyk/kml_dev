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

    // Reads and validates a character file. Every entity of the inventory has a unique id and a parent, the character or
    // another entity of the inventory, with no cycle, and is sited on its parent if sited at all. Throws std::runtime_error
    // stating what was wrong and where. name identifies the stream in error messages
    CharacterDocument read_character(const std::filesystem::path& filepath);
    CharacterDocument read_character(std::istream& is, const std::string& name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_CHARACTER_HPP
