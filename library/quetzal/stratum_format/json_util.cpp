//------------------------------------------------------------------------------
// stratum_format
// json_util.cpp
//------------------------------------------------------------------------------

#include "json_util.hpp"
#include "saved_game_format.hpp"
#include <algorithm>
#include <format>
#include <fstream>
#include <stdexcept>
#include <variant>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    std::string member_text(const EntryMember& member)
    {
        return std::format("{}: {}", string_text(member.key), member.value);
    }

    //--------------------------------------------------------------------------
    std::string entry_text(const entry_type& entry)
    {
        std::vector<std::string> members;
        for (const auto& member : entry)
        {
            members.push_back(member_text(member));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    std::string site_text(const Site& site)
    {
        std::vector<std::string> members {std::format("\"relation\": {}", string_text(name(site.relation)))};

        if (const WallAddress* address = site.address.has_value() ? std::get_if<WallAddress>(&*site.address) : nullptr; address != nullptr)
        {
            members.push_back(std::format("\"wall\": {}", address_text(*address)));
        }
        else if (const FlatCell* flat_cell = site.address.has_value() ? std::get_if<FlatCell>(&*site.address) : nullptr; flat_cell != nullptr)
        {
            members.push_back(std::format("\"flat\": {}", string_text(name(flat_cell->role))));
            members.push_back(std::format("\"cell_position\": {}", cell_position_text(flat_cell->cell_position)));
        }
        else if (site.address.has_value())
        {
            members.push_back(std::format("\"cell_position\": {}", cell_position_text(std::get<ZoneCell>(*site.address).cell_position)));
        }

        if (site.position.has_value())
        {
            members.push_back(std::format("\"position\": {:/s, /b[]}", *site.position));
        }

        if (site.orientation.has_value())
        {
            members.push_back(std::format("\"orientation\": {}", direction_text(*site.orientation)));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    std::string access_point_text(const AccessPoint& access_point)
    {
        std::vector<std::string> members {std::format("\"type\": {}", string_text(access_point.type))};

        if (access_point.target.has_value())
        {
            const auto* unbound = std::get_if<AccessPointTarget>(&*access_point.target);
            members.push_back(std::format("\"target\": {}", unbound != nullptr ? string_text(format_access_point_target(*unbound))
                : reference_text(std::get<stored_entity_id_type>(*access_point.target))));
        }

        members.push_back(std::format("\"order\": {}", access_point.order));

        if (access_point.region_spec.has_value())
        {
            members.push_back(std::format("\"region_spec\": {}", string_text(access_point.region_spec.value())));
        }

        return object_text(members);
    }

} // namespace

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::indentation(size_t indent)
{
    return std::string(indent, ' ');
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::string_text(std::string_view s)
{
    return raw_values_type(std::string(s)).dump();
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::inline_text(const raw_values_type& v)
{
    if (v.is_object())
    {
        if (v.empty())
        {
            return "{ }";
        }

        std::string text = "{ ";
        for (auto i = v.begin(); i != v.end(); ++i)
        {
            text += std::format("{}{}: {}", i == v.begin() ? "" : ", ", string_text(i.key()), inline_text(i.value()));
        }

        return text + " }";
    }

    if (v.is_array())
    {
        if (v.empty())
        {
            return "[]";
        }

        bool spaced = false;
        std::string elements;
        for (size_t i = 0; i < v.size(); ++i)
        {
            spaced = spaced || v[i].is_object();
            elements += std::format("{}{}", i == 0 ? "" : ", ", inline_text(v[i]));
        }

        return spaced ? std::format("[ {} ]", elements) : std::format("[{}]", elements);
    }

    if (v.is_number_float())
    {
        return std::format("{}", v.get<double>());
    }

    return v.dump();
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::joined(const std::vector<std::string>& parts)
{
    std::string text;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        text += std::format("{}{}", i == 0 ? "" : ", ", parts[i]);
    }

    return text;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::object_text(const std::vector<std::string>& members)
{
    return members.empty() ? "{ }" : std::format("{{ {} }}", joined(members));
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::text_values_text(const KeyValueStore& values)
{
    std::vector<std::string> members;
    for (const auto& [key, value] : values)
    {
        members.push_back(std::format("{}: {}", string_text(key), string_text(value)));
    }

    return object_text(members);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::cell_position_text(const discrete_point_type& cell_position)
{
    return std::format("{:/s, /b[]}", cell_position);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::direction_text(const discrete_vector_type& direction)
{
    return std::format("{:/s, /b[]}", direction);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::address_text(const WallAddress& address)
{
    return object_text({std::format("\"cell_position\": {}", cell_position_text(address.cell_position)),
        std::format("\"normal\": {}", direction_text(address.normal))});
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::reference_text(stored_entity_id_type id)
{
    return object_text({std::format("\"entity\": {}", id)});
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::object_section(std::string_view key, const std::vector<std::string>& members)
{
    if (members.empty())
    {
        return std::format("{}{}: {{ }}", indentation(section_indent), string_text(key));
    }

    std::string text = std::format("{}{}: {{\n", indentation(section_indent), string_text(key));

    for (size_t i = 0; i < members.size(); ++i)
    {
        text += std::format("{}{}{}\n", indentation(entry_indent), members[i], i + 1 == members.size() ? "" : ",");
    }

    return text + indentation(section_indent) + "}";
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::write_entry(std::string& text, const entry_type& entry, size_t indent, std::string_view suffix)
{
    if (std::ranges::none_of(entry, [](const EntryMember& member) { return !member.entries.empty(); }))
    {
        text += std::format("{}{}{}\n", indentation(indent), entry_text(entry), suffix);
        return;
    }

    size_t continuation_indent = indent + 2;
    std::string current = indentation(indent) + "{ ";
    bool fresh = true;
    bool after_entries = false;

    for (const auto& member : entry)
    {
        if (!fresh && (after_entries || !member.entries.empty()))
        {
            text += current + ",\n";
            current = indentation(continuation_indent);
            fresh = true;
        }

        if (member.entries.empty())
        {
            current += (fresh ? "" : ", ") + member_text(member);
            fresh = false;
            after_entries = false;
            continue;
        }

        text += std::format("{}{}: [\n", current, string_text(member.key));

        for (size_t i = 0; i < member.entries.size(); ++i)
        {
            write_entry(text, member.entries[i], continuation_indent + 2, i + 1 == member.entries.size() ? "" : ",");
        }

        current = indentation(continuation_indent) + "]";
        fresh = false;
        after_entries = true;
    }

    text += std::format("{} }}{}\n", current, suffix);
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_format::detail::entry_type quetzal::stratum_format::detail::entity_entry(const Entity& entity)
{
    entry_type entry {{"id", std::format("{}", entity.id)}, {"type", string_text(entity.type)}};

    std::vector<std::string> components;

    if (entity.parent_id.has_value())
    {
        components.push_back(std::format("\"parent\": {}", reference_text(*entity.parent_id)));
    }

    if (entity.site.has_value())
    {
        components.push_back(std::format("\"site\": {}", site_text(*entity.site)));
    }

    if (entity.access_point.has_value())
    {
        components.push_back(std::format("\"access_point\": {}", access_point_text(*entity.access_point)));
    }

    if (entity.components.is_object())
    {
        for (const auto& [key, value] : entity.components.items())
        {
            components.push_back(std::format("{}: {}", string_text(key), inline_text(value)));
        }
    }

    if (!components.empty())
    {
        entry.push_back({"components", object_text(components)});
    }

    return entry;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::object_or_empty_text(const raw_values_type& values)
{
    return values.is_null() ? "{ }" : inline_text(values);
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::stratum_format::detail::file_header_sections()
{
    return file_header_sections(saved_game_format_name, saved_game_format_version);
}

//------------------------------------------------------------------------------
std::vector<std::string> quetzal::stratum_format::detail::file_header_sections(std::string_view header_format, std::string_view header_version)
{
    return
    {
        std::format("{}\"format\": {}", indentation(section_indent), string_text(header_format)),
        std::format("{}\"version\": {}", indentation(section_indent), string_text(header_version))
    };
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_format::detail::top_level_object_text(const std::vector<std::string>& sections)
{
    std::string text = "{\n";
    for (size_t i = 0; i < sections.size(); ++i)
    {
        text += std::format("{}{}\n", sections[i], i + 1 == sections.size() ? "" : ",");
    }

    return text + "}\n";
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::detail::write_file(const std::filesystem::path& filepath, const std::string& text)
{
    std::ofstream os(filepath, std::ios::binary);
    if (!os)
    {
        throw std::runtime_error(std::format("{}: cannot be opened for writing", filepath.string()));
    }

    os << text;

    if (!os)
    {
        throw std::runtime_error(std::format("{}: cannot be written", filepath.string()));
    }

    return;
}
