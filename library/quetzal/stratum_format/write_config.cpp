//------------------------------------------------------------------------------
// stratum_format
// write_config.cpp
//------------------------------------------------------------------------------

#include "write_config.hpp"
#include "config_format.hpp"
#include "json_util.hpp"
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    std::string document_text(const ConfigDocument& document)
    {
        std::vector<std::string> sections = file_header_sections(config_format_name, config_format_version);

        auto add_location = [&sections](std::string_view key, const std::optional<std::string>& location)
        {
            if (location)
            {
                sections.push_back(std::format("{}{}: {}", indentation(section_indent), string_text(key), string_text(*location)));
            }
        };

        add_location("assets_directory", document.assets_directory);
        add_location("user_directory", document.user_directory);
        add_location("saves_directory", document.saves_directory);
        add_location("working_directory", document.working_directory);
        add_location("logs_directory", document.logs_directory);
        add_location("cache_directory", document.cache_directory);
        add_location("mods_directory", document.mods_directory);
        add_location("user_settings_file", document.user_settings_file);

        if (document.zone_count_max)
        {
            sections.push_back(std::format("{}\"zone_count_max\": {}", indentation(section_indent), *document.zone_count_max));
        }

        if (document.log_file_count_max)
        {
            sections.push_back(std::format("{}\"log_file_count_max\": {}", indentation(section_indent), *document.log_file_count_max));
        }

        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const ConfigDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const ConfigDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}
