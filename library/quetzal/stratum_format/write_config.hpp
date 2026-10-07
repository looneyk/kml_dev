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

    void write(const std::filesystem::path& filepath, const ConfigDocument& document);
    void write(std::ostream& os, const ConfigDocument& document);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_WRITE_CONFIG_HPP
