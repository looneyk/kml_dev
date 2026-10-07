#if !defined(QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// ConfigDocument.hpp
//------------------------------------------------------------------------------

#include <cstddef>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of the config file. Each location is as written, relative to the config file's directory
    // unless it is absolute
    struct ConfigDocument
    {
        std::string version;
        std::string saves_directory;
        std::string working_directory;
        std::string logs_directory;
        std::string cache_directory;
        std::string user_settings_file;
        std::size_t zone_count_max = 0;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_CONFIGDOCUMENT_HPP
