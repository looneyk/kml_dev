//------------------------------------------------------------------------------
// stratum_format
// SettingDefinitionsDocument.cpp
//------------------------------------------------------------------------------

#include "SettingDefinitionsDocument.hpp"
#include "checked_values.hpp"
#include <array>
#include <format>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    using document_type = SettingDefinitionsDocument;

    constexpr std::array<std::string_view, 5> file_keys {"format", "version", "enumerations", "settings", "groups"};
    constexpr std::array<std::string_view, 2> enumeration_keys {"name", "values"};
    constexpr std::array<std::string_view, 10> setting_keys {"key", "type", "default", "scope", "restart_required", "display_name", "description",
        "category", "minimum", "maximum"};
    constexpr std::array<std::string_view, 7> group_keys {"key", "scope", "default_preset", "display_name", "category", "members", "presets"};
    constexpr std::array<std::string_view, 4> preset_keys {"name", "display_name", "modifiable", "values"};

    //--------------------------------------------------------------------------
    void check_definition_keys(const raw_values_type& v, const std::string& path, std::span<const std::string_view> keys)
    {
        check_keys(v, path, keys, "setting definition file");
        return;
    }

    //--------------------------------------------------------------------------
    std::string required_string(const raw_values_type& v, const std::string& path, std::string_view key)
    {
        return string_value(required_member(v, path, key), member_path(path, key));
    }

    //--------------------------------------------------------------------------
    std::optional<std::string> optional_string(const raw_values_type& v, const std::string& path, std::string_view key)
    {
        const raw_values_type* p = member(v, key);
        return p != nullptr ? std::make_optional(string_value(*p, member_path(path, key))) : std::nullopt;
    }

    //--------------------------------------------------------------------------
    bool optional_boolean(const raw_values_type& v, const std::string& path, std::string_view key)
    {
        const raw_values_type* p = member(v, key);
        return p != nullptr && boolean_value(*p, member_path(path, key));
    }

    //--------------------------------------------------------------------------
    std::vector<std::string> required_strings(const raw_values_type& v, const std::string& path, std::string_view key)
    {
        const std::string strings_path = member_path(path, key);
        const raw_values_type& a = array(required_member(v, path, key), strings_path);

        std::vector<std::string> strings;
        for (size_t i = 0; i < a.size(); ++i)
        {
            strings.push_back(string_value(a[i], element_path(strings_path, i)));
        }

        return strings;
    }

    //--------------------------------------------------------------------------
    document_type::Scope read_scope(const raw_values_type& v, const std::string& path)
    {
        std::string scope = required_string(v, path, "scope");
        if (scope == "user")
        {
            return document_type::Scope::User;
        }

        if (scope == "game")
        {
            return document_type::Scope::Game;
        }

        fail(member_path(path, "scope"), std::format("'{}' is not a scope: user or game", scope));
    }

    //--------------------------------------------------------------------------
    template<typename T>
    std::vector<T> read_entries(const raw_values_type& v, const std::string& path, std::string_view key,
        const std::function<T(const raw_values_type&, const std::string&)>& read)
    {
        std::vector<T> entries;
        const raw_values_type* p = member(v, key);
        if (p == nullptr)
        {
            return entries;
        }

        const std::string entries_path = member_path(path, key);
        const raw_values_type& a = array(*p, entries_path);
        for (size_t i = 0; i < a.size(); ++i)
        {
            const std::string entry_path = element_path(entries_path, i);
            entries.push_back(read(object(a[i], entry_path), entry_path));
        }

        return entries;
    }

    //--------------------------------------------------------------------------
    document_type::Enumeration read_enumeration(const raw_values_type& v, const std::string& path)
    {
        check_definition_keys(v, path, enumeration_keys);

        document_type::Enumeration enumeration;
        enumeration.name = required_string(v, path, "name");
        enumeration.values = required_strings(v, path, "values");
        return enumeration;
    }

    //--------------------------------------------------------------------------
    document_type::Setting read_setting(const raw_values_type& v, const std::string& path)
    {
        check_definition_keys(v, path, setting_keys);

        document_type::Setting setting;
        setting.key = required_string(v, path, "key");
        setting.type_name = required_string(v, path, "type");
        setting.default_value = required_string(v, path, "default");
        setting.scope = read_scope(v, path);
        setting.restart_required = optional_boolean(v, path, "restart_required");
        setting.display_name = optional_string(v, path, "display_name").value_or("");
        setting.description = optional_string(v, path, "description").value_or("");
        setting.category = optional_string(v, path, "category").value_or("");
        setting.minimum = optional_string(v, path, "minimum");
        setting.maximum = optional_string(v, path, "maximum");
        return setting;
    }

    //--------------------------------------------------------------------------
    document_type::Preset read_preset(const raw_values_type& v, const std::string& path)
    {
        check_definition_keys(v, path, preset_keys);

        document_type::Preset preset;
        preset.name = required_string(v, path, "name");
        preset.display_name = optional_string(v, path, "display_name").value_or("");
        preset.modifiable = optional_boolean(v, path, "modifiable");

        if (const raw_values_type* p = member(v, "values"); p != nullptr)
        {
            preset.values = text_values(*p, member_path(path, "values"));
        }

        return preset;
    }

    //--------------------------------------------------------------------------
    document_type::Group read_group(const raw_values_type& v, const std::string& path)
    {
        check_definition_keys(v, path, group_keys);

        document_type::Group group;
        group.key = required_string(v, path, "key");
        group.scope = read_scope(v, path);
        group.default_preset_name = required_string(v, path, "default_preset");
        group.display_name = optional_string(v, path, "display_name").value_or("");
        group.category = optional_string(v, path, "category").value_or("");
        group.member_keys = required_strings(v, path, "members");

        required_member(v, path, "presets");
        group.presets = read_entries<document_type::Preset>(v, path, "presets", read_preset);
        return group;
    }

    //--------------------------------------------------------------------------
    document_type read_setting_definitions_document(const raw_values_type& v)
    {
        check_file_header(v, document_type::format_name, document_type::format_version);
        check_definition_keys(v, "", file_keys);

        document_type document;
        document.version = std::string(document_type::format_version);
        document.enumerations = read_entries<document_type::Enumeration>(v, "", "enumerations", read_enumeration);
        document.settings = read_entries<document_type::Setting>(v, "", "settings", read_setting);
        document.groups = read_entries<document_type::Group>(v, "", "groups", read_group);
        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::SettingDefinitionsDocument quetzal::stratum_format::read_setting_definitions(const std::filesystem::path& filepath)
{
    return read_document<SettingDefinitionsDocument>(filepath, read_setting_definitions_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::SettingDefinitionsDocument quetzal::stratum_format::read_setting_definitions(std::istream& is,
    const std::string& stream_name)
{
    return read_document<SettingDefinitionsDocument>(is, stream_name, read_setting_definitions_document);
}
