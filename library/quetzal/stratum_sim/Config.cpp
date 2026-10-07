//------------------------------------------------------------------------------
// stratum_sim
// Config.cpp
//------------------------------------------------------------------------------

#include "Config.hpp"
#include "quetzal/stratum_format/config_format.hpp"
#include "quetzal/stratum_format/read_config.hpp"
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{

    constexpr std::string_view organization_name = "KML";
    constexpr std::string_view program_name = "Stratum";

    //--------------------------------------------------------------------------
    // Returns std::nullopt if the variable is not set or is empty
    std::optional<std::filesystem::path> environment_path(const char* name)
    {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // MSVC deprecates the standard getenv in favor of its own _dupenv_s
#endif
        const char* value = std::getenv(name);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
        if (value == nullptr || *value == '\0')
        {
            return std::nullopt;
        }

        return std::filesystem::path(value);
    }

    //--------------------------------------------------------------------------
    std::filesystem::path required_environment_path(const char* name)
    {
        std::optional<std::filesystem::path> path = environment_path(name);
        if (!path)
        {
            throw std::runtime_error(std::string("the environment variable ") + name + " is not set");
        }

        return *path;
    }

    //--------------------------------------------------------------------------
    // Where the platform stores user application data on this machine
    std::filesystem::path application_data_directory()
    {
#if defined(_WIN32)
        return required_environment_path("LOCALAPPDATA");
#elif defined(__APPLE__)
        return required_environment_path("HOME") / "Library/Application Support";
#else
        std::optional<std::filesystem::path> data_home = environment_path("XDG_DATA_HOME");
        return data_home ? *data_home : required_environment_path("HOME") / ".local/share";
#endif
    }

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
    config.zone_count_max = document.zone_count_max.value_or(8);
    return config;
}
