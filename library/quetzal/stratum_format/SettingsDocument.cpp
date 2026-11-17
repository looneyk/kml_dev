//------------------------------------------------------------------------------
// stratum_format
// SettingsDocument.cpp
//------------------------------------------------------------------------------

#include "SettingsDocument.hpp"
#include "checked_values.hpp"
#include "json_util.hpp"
#include <array>
#include <format>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    constexpr std::array<std::string_view, 4> file_keys{"format", "version", "settings", "presets"};

    //--------------------------------------------------------------------------
    SettingsDocument read_settings_document(const raw_values_type& v)
    {
        check_file_header(v, SettingsDocument::format_name, SettingsDocument::format_version);
        check_keys(v, "", file_keys, "settings file");

        SettingsDocument document;
        document.version = std::string(SettingsDocument::format_version);
        document.snapshot = read_snapshot(v);
        return document;
    }

    //--------------------------------------------------------------------------
    std::string document_text(const SettingsDocument& document)
    {
        std::vector<std::string> sections = file_header_sections(SettingsDocument::format_name, SettingsDocument::format_version);
        add_snapshot_sections(sections, document.snapshot);
        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::SettingsDocument quetzal::stratum_format::read_settings(const std::filesystem::path& filepath)
{
    return read_document<SettingsDocument>(filepath, read_settings_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::SettingsDocument quetzal::stratum_format::read_settings(std::istream& is, const std::string& stream_name)
{
    return read_document<SettingsDocument>(is, stream_name, read_settings_document);
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const SettingsDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const SettingsDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
quetzal::settings::SettingsSnapshot quetzal::stratum_format::detail::read_snapshot(const raw_values_type& v)
{
    settings::SettingsSnapshot snapshot;

    if (const raw_values_type* p = member(v, "settings"); p != nullptr)
    {
        snapshot.settings = text_values(*p, "settings");
    }

    if (const raw_values_type* p = member(v, "presets"); p != nullptr)
    {
        for (const auto& [group_key, group_presets] : object(*p, "presets").items())
        {
            const std::string group_path = member_path("presets", group_key);
            for (const auto& [preset_name, values] : object(group_presets, group_path).items())
            {
                snapshot.presets.emplace(settings::Settings::preset_key_type{group_key, preset_name}, text_values(values,
                    member_path(group_path, preset_name)));
            }
        }
    }

    return snapshot;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::add_snapshot_sections(std::vector<std::string>& sections, const settings::SettingsSnapshot& snapshot)
{
    std::map<std::string, std::vector<std::string>> group_presets; // <group key, preset members>
    for (const auto& [key, values] : snapshot.presets)
    {
        group_presets[key.first].push_back(std::format("{}: {}", string_text(key.second), text_values_text(values)));
    }

    std::vector<std::string> groups;
    for (const auto& [group_key, preset_members] : group_presets)
    {
        groups.push_back(std::format("{}: {}", string_text(group_key), object_text(preset_members)));
    }

    sections.push_back(std::format("{}\"settings\": {}", indentation(section_indent), text_values_text(snapshot.settings)));
    sections.push_back(std::format("{}\"presets\": {}", indentation(section_indent), object_text(groups)));
    return;
}
