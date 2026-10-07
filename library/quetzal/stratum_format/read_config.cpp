//------------------------------------------------------------------------------
// stratum_format
// read_config.cpp
//------------------------------------------------------------------------------

#include "read_config.hpp"
#include "checked_values.hpp"
#include "config_format.hpp"

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    std::string required_string(const raw_values_type& v, std::string_view key)
    {
        return string_value(required_member(v, "", key), std::string(key));
    }

    //--------------------------------------------------------------------------
    ConfigDocument read_config_document(const raw_values_type& v)
    {
        check_file_header(v, config_format_name, config_format_version);

        ConfigDocument document;
        document.version = std::string(config_format_version);
        document.saves_directory = required_string(v, "saves_directory");
        document.working_directory = required_string(v, "working_directory");
        document.logs_directory = required_string(v, "logs_directory");
        document.cache_directory = required_string(v, "cache_directory");
        document.user_settings_file = required_string(v, "user_settings_file");
        document.zone_count_max = count_value(required_member(v, "", "zone_count_max"), "zone_count_max");

        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::ConfigDocument quetzal::stratum_format::read_config(std::istream& is, const std::string& stream_name)
{
    return read_document<ConfigDocument>(is, stream_name, read_config_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::ConfigDocument quetzal::stratum_format::read_config(const std::filesystem::path& filepath)
{
    return read_document<ConfigDocument>(filepath, read_config_document);
}
