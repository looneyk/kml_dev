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

    // Reads and validates a world file
    // Throws std::runtime_error where stream_name identifies the stream in error messages
    WorldDocument read_world(const std::filesystem::path& filepath);
    WorldDocument read_world(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_WORLD_HPP
