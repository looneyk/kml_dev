#if !defined(QUETZAL_STRATUM_FORMAT_READ_SETTING_DEFINITIONS_HPP)
#define QUETZAL_STRATUM_FORMAT_READ_SETTING_DEFINITIONS_HPP
//------------------------------------------------------------------------------
// stratum_format
// read_setting_definitions.hpp
//------------------------------------------------------------------------------

#include "SettingDefinitionsDocument.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>

namespace quetzal::stratum_format
{

    // Throws std::runtime_error on error or an unsupported key, stream_name used in error messages
    SettingDefinitionsDocument read_setting_definitions(const std::filesystem::path& filepath);
    SettingDefinitionsDocument read_setting_definitions(std::istream& is, const std::string& stream_name);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_READ_SETTING_DEFINITIONS_HPP
