#if !defined(QUETZAL_STRATUM_FORMAT_WRITE_WORLD_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITE_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_format
// write_world.hpp
//------------------------------------------------------------------------------

#include "WorldDocument.hpp"
#include <filesystem>
#include <iosfwd>

namespace quetzal::stratum_format
{

    // Writes the world file in the written form that game_file_spec.md specifies: a region's id and components open it, its
    // spec is on the next line, and each zone reference has a line of its own, as each entry of the world index does
    void write(const std::filesystem::path& filepath, const WorldDocument& document);
    void write(std::ostream& os, const WorldDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_WORLD_HPP
