//------------------------------------------------------------------------------
// stratum_format
// SettingsDocument.cpp
//------------------------------------------------------------------------------

#include "SettingsDocument.hpp"
#include "checked_values.hpp"
#include "json_util.hpp"
#include <array>
#include <format>
#include <ostream>
#include <string_view>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    constexpr std::array<std::string_view, 4> file_keys {"format", "version", "settings", "groups"};

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

    if (const raw_values_type* p = member(v, "groups"); p != nullptr)
    {
        snapshot.groups = text_values(*p, "groups");
    }

    return snapshot;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::add_snapshot_sections(std::vector<std::string>& sections, const settings::SettingsSnapshot& snapshot)
{
    sections.push_back(std::format("{}\"settings\": {}", indentation(section_indent), text_values_text(snapshot.settings)));
    sections.push_back(std::format("{}\"groups\": {}", indentation(section_indent), text_values_text(snapshot.groups)));
    return;
}
