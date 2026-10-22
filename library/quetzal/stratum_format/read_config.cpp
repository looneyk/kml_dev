//------------------------------------------------------------------------------
// stratum_format
// read_config.cpp
//------------------------------------------------------------------------------

#include "read_config.hpp"
#include "checked_values.hpp"
#include "config_format.hpp"
#include <array>
#include <algorithm>
#include <format>
#include <optional>
#include <string_view>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    constexpr std::array<std::string_view, 12> config_keys {"format", "version", "assets_directory", "user_directory", "saves_directory",
        "working_directory", "logs_directory", "cache_directory", "mods_directory", "user_settings_file", "zone_count_max",
        "log_file_count_max"};

    //--------------------------------------------------------------------------
    std::optional<std::string> optional_string(const raw_values_type& v, std::string_view key)
    {
        const raw_values_type* p = member(v, key);
        return p != nullptr ? std::make_optional(string_value(*p, std::string(key))) : std::nullopt;
    }

    //--------------------------------------------------------------------------
    ConfigDocument read_config_document(const raw_values_type& v)
    {
        check_file_header(v, config_format_name, config_format_version);

        for (const auto& [key, value] : v.items())
        {
            if (std::ranges::find(config_keys, key) == config_keys.end())
            {
                fail(key, "not a key of the config file");
            }
        }

        ConfigDocument document;
        document.version = std::string(config_format_version);
        document.assets_directory = optional_string(v, "assets_directory");
        document.user_directory = optional_string(v, "user_directory");
        document.saves_directory = optional_string(v, "saves_directory");
        document.working_directory = optional_string(v, "working_directory");
        document.logs_directory = optional_string(v, "logs_directory");
        document.cache_directory = optional_string(v, "cache_directory");
        document.mods_directory = optional_string(v, "mods_directory");
        document.user_settings_file = optional_string(v, "user_settings_file");

        if (const raw_values_type* p = member(v, "zone_count_max"); p != nullptr)
        {
            document.zone_count_max = count_value(*p, "zone_count_max");
        }

        if (const raw_values_type* p = member(v, "log_file_count_max"); p != nullptr)
        {
            document.log_file_count_max = count_value(*p, "log_file_count_max");
        }

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
