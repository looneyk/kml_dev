#if !defined(QUETZAL_STRATUM_FORMAT_WRITE_CONFIG_HPP)
#define QUETZAL_STRATUM_FORMAT_WRITE_CONFIG_HPP
//------------------------------------------------------------------------------
// stratum_format
// write_config.hpp
//------------------------------------------------------------------------------

#include "ConfigDocument.hpp"
#include <filesystem>
#include <iosfwd>

namespace quetzal::stratum_format
{

    // Writes the config file in the written form that game_file_spec.md specifies: each value on a line of its own
    void write(const std::filesystem::path& filepath, const ConfigDocument& document);
    void write(std::ostream& os, const ConfigDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_CONFIG_HPP
