#if !defined(QUETZAL_STRATUM_FORMAT_WRITE_CHARACTER_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITE_CHARACTER_HPP
//------------------------------------------------------------------------------
// stratum_format
// write_character.hpp
//------------------------------------------------------------------------------

#include "CharacterDocument.hpp"
#include <filesystem>
#include <iosfwd>

namespace quetzal::stratum_format
{

    // Writes the character file in the written form that game_file_spec.md specifies: the name third, after the format and
    // the version, each key of the character on a line, and each entity of the inventory on a line
    void write(const std::filesystem::path& filepath, const CharacterDocument& document);
    void write(std::ostream& os, const CharacterDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_CHARACTER_HPP
