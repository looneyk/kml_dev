#if !defined(QUETZAL_STRATUM_FORMAT_READ_CONFIG_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_CONFIG_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_config.hpp
//------------------------------------------------------------------------------

#include "ConfigDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Reads and validates a config file. Throws std::runtime_error stating what was wrong and where. stream_name identifies the stream in error messages
    ConfigDocument read_config(const std::filesystem::path& filepath);
    ConfigDocument read_config(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_CONFIG_HPP
