#if !defined(QUETZAL_STRATUM_FORMAT_CONFIG_FORMAT_HPP)
#define QUETZAL_STRATUM_FORMAT_CONFIG_FORMAT_HPP
//------------------------------------------------------------------------------
// stratum_format
// config_format.hpp
//------------------------------------------------------------------------------

#include <string_view>

namespace quetzal::stratum_format
{

    constexpr std::string_view config_format_name = "quetzal.stratum_config";
    constexpr std::string_view config_format_version = "0.1";

    // The config file, in the program executable's directory
    constexpr std::string_view config_filename = "config.json";

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_CONFIG_FORMAT_HPP
