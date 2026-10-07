//------------------------------------------------------------------------------
// stratum_format
// write_config.cpp
//------------------------------------------------------------------------------

#include "write_config.hpp"
#include "config_format.hpp"
#include "written_form.hpp"
#include <format>
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

        auto add_section = [&sections](std::string_view key, const std::string& value_text)
        {
            sections.push_back(std::format("{}{}: {}", indentation(section_indent), string_text(key), value_text));
        };

        add_section("saves_directory", string_text(document.saves_directory));
        add_section("working_directory", string_text(document.working_directory));
        add_section("logs_directory", string_text(document.logs_directory));
        add_section("cache_directory", string_text(document.cache_directory));
        add_section("user_settings_file", string_text(document.user_settings_file));
        add_section("zone_count_max", std::to_string(document.zone_count_max));

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
