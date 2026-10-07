#if !defined(QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// ConfigDocument.hpp
//------------------------------------------------------------------------------

#include <cstddef>
#include <optional>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of the config file which overrides the program's defaults if it exists
    struct ConfigDocument
    {
        std::string version;
        std::optional<std::string> assets_directory;
        std::optional<std::string> user_directory;
        std::optional<std::string> saves_directory;
        std::optional<std::string> working_directory;
        std::optional<std::string> logs_directory;
        std::optional<std::string> cache_directory;
        std::optional<std::string> mods_directory;
        std::optional<std::string> user_settings_file;
        std::optional<std::size_t> zone_count_max;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP
