//------------------------------------------------------------------------------
// stratum_sim
// Config.cpp
//------------------------------------------------------------------------------

#include "Config.hpp"
#include "quetzal/common/system_path.hpp"
#include "quetzal/stratum_format/config_format.hpp"
#include "quetzal/stratum_format/read_config.hpp"
#include <optional>
#include <string>
#include <string_view>

namespace
{

    constexpr std::string_view organization_name = "KML";
    constexpr std::string_view program_name = "Stratum";

    //--------------------------------------------------------------------------
    // Returns the location, or the default if there is none, joined to the directory. An absolute location replaces the
    // directory, as operator/ joins them
    std::filesystem::path located(const std::filesystem::path& directory, const std::optional<std::string>& location,
        const std::filesystem::path& default_location)
    {
        return directory / (location ? std::filesystem::path(*location) : default_location);
    }

} // namespace

//------------------------------------------------------------------------------
std::filesystem::path quetzal::stratum_sim::default_user_directory()
{
    return application_data_directory() / organization_name / program_name;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Config quetzal::stratum_sim::load_config(const std::filesystem::path& program_directory)
{
    const std::filesystem::path filepath = program_directory / stratum_format::config_filename;

    stratum_format::ConfigDocument document;
    if (std::filesystem::exists(filepath))
    {
        document = stratum_format::read_config(filepath);
    }

    Config config;
    config.assets_directory = located(program_directory, document.assets_directory, "assets");
    config.user_directory = document.user_directory ? program_directory / *document.user_directory : default_user_directory();
    config.saves_directory = located(config.user_directory, document.saves_directory, "saves");
    config.working_directory = located(config.user_directory, document.working_directory, "working");
    config.logs_directory = located(config.user_directory, document.logs_directory, "logs");
    config.cache_directory = located(config.user_directory, document.cache_directory, "cache");
    config.mods_directory = located(config.user_directory, document.mods_directory, "mods");
    config.user_settings_filepath = located(config.user_directory, document.user_settings_file, "settings.json");
    config.zone_count_max = document.zone_count_max.value_or(0);
    config.log_file_count_max = document.log_file_count_max.value_or(10);
    return config;
}
