//------------------------------------------------------------------------------
// stratum_sim
// Config.cpp
//------------------------------------------------------------------------------

#include "Config.hpp"
#include "quetzal/stratum_format/config_format.hpp"
#include "quetzal/stratum_format/read_config.hpp"
#include "quetzal/stratum_format/write_config.hpp"
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
    quetzal::stratum_format::ConfigDocument default_config_document()
    {
        quetzal::stratum_format::ConfigDocument document;
        document.version = std::string(quetzal::stratum_format::config_format_version);
        document.saves_directory = "saves";
        document.working_directory = "working";
        document.logs_directory = "logs";
        document.cache_directory = "cache";
        document.user_settings_file = "settings.json";
        document.zone_count_max = 8;
        return document;
    }

} // namespace

//------------------------------------------------------------------------------
std::filesystem::path quetzal::stratum_sim::data_directory()
{
    return application_data_directory() / organization_name / program_name;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Config quetzal::stratum_sim::load_config(const std::filesystem::path& directory)
{
    const std::filesystem::path filepath = directory / stratum_format::config_filename;

    if (!std::filesystem::exists(filepath))
    {
        std::filesystem::create_directories(directory);
        stratum_format::write(filepath, default_config_document());
    }

    stratum_format::ConfigDocument document = stratum_format::read_config(filepath);

    // An absolute location replaces the directory, as operator/ joins them
    auto located = [&directory](const std::string& location)
    {
        return directory / std::filesystem::path(location);
    };

    return Config {located(document.saves_directory), located(document.working_directory), document.zone_count_max,
        located(document.logs_directory), located(document.cache_directory), located(document.user_settings_file)};
}
