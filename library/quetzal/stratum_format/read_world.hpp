#if !defined(QUETZAL_STRATUM_FORMAT_READ_WORLD_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_WORLD_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_world.hpp
//------------------------------------------------------------------------------

#include "WorldDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads and validates a world file. Region ids are unique, zone ids are unique across the world, and each entry of the
    // world index is in a zone that a region lists. Throws std::runtime_error stating what was wrong and where.
    // stream_name identifies the stream in error messages
    WorldDocument read_world(const std::filesystem::path& filepath);
    WorldDocument read_world(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_WORLD_HPP
