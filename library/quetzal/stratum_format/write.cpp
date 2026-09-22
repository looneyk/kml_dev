//------------------------------------------------------------------------------
// stratum_format
// write.cpp
//------------------------------------------------------------------------------

#include "write.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cassert>
#include <cmath>

namespace
{

    using namespace quetzal::stratum_format;

    constexpr size_t section_indent = 2;
    constexpr size_t entry_indent = 4;

    //--------------------------------------------------------------------------
    std::string indentation(size_t indent)
    {
        return std::string(indent, ' ');
    }

    //--------------------------------------------------------------------------
    std::string string_text(std::string_view s)
    {
        return values_type(std::string(s)).dump();
    }

    //--------------------------------------------------------------------------
    // The shortest digits that read back as the same value, in fixed notation, and with a fraction always shown
    template<std::floating_point T>
    std::string number_text(T value)
    {
        assert(std::isfinite(value));

        std::array<char, 512> buffer;
        auto [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
        assert(ec == std::errc());

        std::string text(buffer.data(), end);
        if (text.find('.') == std::string::npos)
        {
            text += ".0";
        }

        return text;
    }

    //--------------------------------------------------------------------------
    // Arrays of objects are spaced inside their brackets, as objects are inside their braces; arrays of anything else are not
    std::string inline_text(const values_type& v)
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
            return number_text(v.get<double>());
        }

        return v.dump();
    }

    //--------------------------------------------------------------------------
    // Joins the members of an object or the elements of an array with separators
    std::string joined(const std::vector<std::string>& parts)
    {
        std::string text;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            text += std::format("{}{}", i == 0 ? "" : ", ", parts[i]);
        }

        return text;
    }

    //--------------------------------------------------------------------------
    std::string object_text(const std::vector<std::string>& members)
    {
        return members.empty() ? "{ }" : std::format("{{ {} }}", joined(members));
    }

    //--------------------------------------------------------------------------
    std::string cell_position_text(const discrete_point_type& cell_position)
    {
        return std::format("[{}, {}]", cell_position.i(), cell_position.j());
    }

    //--------------------------------------------------------------------------
    std::string direction_text(const discrete_vector_type& direction)
    {
        return std::format("[{}, {}]", direction.x(), direction.y());
    }

    //--------------------------------------------------------------------------
    std::string address_text(const WallAddress& address)
    {
        return object_text({std::format("\"cell_position\": {}", cell_position_text(address.cell_position)),
            std::format("\"normal\": {}", direction_text(address.normal))});
    }

    //--------------------------------------------------------------------------
    std::string reference_text(stored_entity_id_type id)
    {
        return object_text({std::format("\"entity\": {}", id)});
    }

    //--------------------------------------------------------------------------
    std::string barrier_text(const Barrier& barrier)
    {
        std::vector<std::string> members {std::format("\"cell_position\": {}", cell_position_text(barrier.cell_position))};

        if (barrier.normal.has_value())
        {
            members.push_back(std::format("\"normal\": {}", direction_text(*barrier.normal)));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    std::string barriers_text(const std::vector<Barrier>& barriers)
    {
        std::vector<std::string> texts;
        for (const auto& barrier : barriers)
        {
            texts.push_back(barrier_text(barrier));
        }

        return barriers.empty() ? "[]" : std::format("[ {} ]", joined(texts));
    }

    //--------------------------------------------------------------------------
    // A run of three or more is written with a count, which is shorter only then
    std::string compressed_row(const std::string& row)
    {
        std::string text;

        for (size_t i = 0; i < row.size(); )
        {
            size_t j = i;
            while (j < row.size() && row[j] == row[i])
            {
                ++j;
            }

            size_t run = j - i;
            if (run >= 3)
            {
                text += std::format("{}{}", run, row[i]);
            }
            else
            {
                text.append(run, row[i]);
            }

            i = j;
        }

        return text;
    }

    //--------------------------------------------------------------------------
    // One member of an entry. A member with entries is an array of entries, written an entry per line
    struct EntryMember
    {
        std::string key;
        std::string value;
        std::vector<std::vector<EntryMember>> entries;
    };

    using entry_type = std::vector<EntryMember>;

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
    EntryMember entries_member(std::string_view key, std::vector<entry_type> entries)
    {
        return {std::string(key), {}, std::move(entries)};
    }

    //--------------------------------------------------------------------------
    void add_components(entry_type& entry, const values_type& components)
    {
        if (!components.is_null())
        {
            entry.push_back({"components", inline_text(components)});
        }

        return;
    }

    //--------------------------------------------------------------------------
    // On one line, unless it has members with entries: each of those begins a line of its own, aligned inside the brace
    void write_entry(std::string& text, const entry_type& entry, size_t indent, std::string_view suffix)
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

    //--------------------------------------------------------------------------
    std::string entries_section(std::string_view key, const std::vector<entry_type>& entries)
    {
        std::string text = std::format("{}{}: [\n", indentation(section_indent), string_text(key));

        for (size_t i = 0; i < entries.size(); ++i)
        {
            write_entry(text, entries[i], entry_indent, i + 1 == entries.size() ? "" : ",");
        }

        return text + indentation(section_indent) + "]";
    }

    //--------------------------------------------------------------------------
    // A member per line, each value inline
    std::string object_section(std::string_view key, const std::vector<std::string>& members)
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

    //--------------------------------------------------------------------------
    std::string content_section(std::string_view key, const values_type& values)
    {
        if (!values.is_object())
        {
            return std::format("{}{}: {}", indentation(section_indent), string_text(key), inline_text(values));
        }

        std::vector<std::string> members;
        for (const auto& [member_key, value] : values.items())
        {
            members.push_back(std::format("{}: {}", string_text(member_key), inline_text(value)));
        }

        return object_section(key, members);
    }

    //--------------------------------------------------------------------------
    std::string cells_section(const Cells& cells)
    {
        std::vector<std::string> members
        {
            std::format("\"notation\": {}", string_text(name(cells.notation))),
            std::format("\"extent\": [{}, {}]", cells.extent.x(), cells.extent.y()),
            std::format("\"origin\": [{}, {}, {}]", number_text(cells.origin[0]), number_text(cells.origin[1]), number_text(cells.origin[2])),
            std::format("\"scale\": {}", number_text(cells.scale))
        };

        if (cells.encoding != Encoding::Characters)
        {
            members.push_back(std::format("\"encoding\": {}", string_text(name(cells.encoding))));
        }

        std::string rows = "\"rows\": [\n";
        for (size_t i = 0; i < cells.rows.size(); ++i)
        {
            std::string row = cells.encoding == Encoding::Runs ? compressed_row(cells.rows[i]) : cells.rows[i];
            rows += std::format("{}{}{}\n", indentation(entry_indent + 2), string_text(row), i + 1 == cells.rows.size() ? "" : ",");
        }

        members.push_back(rows + indentation(entry_indent) + "]");
        return object_section("cells", members);
    }

    //--------------------------------------------------------------------------
    entry_type room_entry(const Room& room)
    {
        entry_type entry;

        if (const Perimeter* perimeter = std::get_if<Perimeter>(&room.identification); perimeter != nullptr)
        {
            std::vector<std::string> junctions;
            for (const auto& junction : perimeter->junctions)
            {
                junctions.push_back(cell_position_text(junction));
            }

            entry.push_back({"perimeter", std::format("[{}]", joined(junctions))});
        }
        else
        {
            const RoomLocator& locator = std::get<RoomLocator>(room.identification);
            entry.push_back({"cell_position", cell_position_text(locator.cell_position)});

            if (!locator.barriers.empty())
            {
                entry.push_back({"barriers", barriers_text(locator.barriers)});
            }
        }

        add_components(entry, room.components);
        return entry;
    }

    //--------------------------------------------------------------------------
    entry_type wall_entry(const Wall& wall)
    {
        entry_type entry {{"wall", address_text(wall.address)}};
        add_components(entry, wall.components);
        return entry;
    }

    //--------------------------------------------------------------------------
    void add_flat_properties(entry_type& entry, const FlatProperties& properties)
    {
        if (properties.height.has_value())
        {
            entry.push_back({"height", number_text(*properties.height)});
        }

        add_components(entry, properties.components);
        return;
    }

    //--------------------------------------------------------------------------
    entry_type flat_entry(const Flat& flat)
    {
        entry_type entry {{"role", string_text(name(flat.role))}};
        add_flat_properties(entry, flat.properties);

        if (!flat.rooms.empty())
        {
            std::vector<entry_type> rooms;
            for (const auto& [room_id, properties] : flat.rooms)
            {
                entry_type room {{"room", std::format("{}", room_id)}};
                add_flat_properties(room, properties);
                rooms.push_back(std::move(room));
            }

            entry.push_back(entries_member("rooms", std::move(rooms)));
        }

        if (!flat.cells.empty())
        {
            std::vector<entry_type> cells;
            for (const auto& [cell_position, properties] : flat.cells)
            {
                entry_type cell {{"cell_position", cell_position_text(cell_position)}};
                add_flat_properties(cell, properties);
                cells.push_back(std::move(cell));
            }

            entry.push_back(entries_member("cells", std::move(cells)));
        }

        return entry;
    }

    //--------------------------------------------------------------------------
    std::string site_text(const Site& site)
    {
        std::vector<std::string> members {std::format("\"relation\": {}", string_text(name(site.relation)))};

        if (const WallAddress* address = site.address.has_value() ? std::get_if<WallAddress>(&*site.address) : nullptr; address != nullptr)
        {
            members.push_back(std::format("\"wall\": {}", address_text(*address)));
        }
        else if (site.address.has_value())
        {
            const FlatCell& flat_cell = std::get<FlatCell>(*site.address);
            members.push_back(std::format("\"flat\": {}", string_text(name(flat_cell.role))));
            members.push_back(std::format("\"cell_position\": {}", cell_position_text(flat_cell.cell_position)));
        }

        if (site.position.has_value())
        {
            members.push_back(std::format("\"position\": [{}, {}]", number_text(site.position->x()), number_text(site.position->y())));
        }

        if (site.orientation.has_value())
        {
            members.push_back(std::format("\"orientation\": {}", direction_text(*site.orientation)));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    // site and parent are written into components, ahead of the rest
    entry_type entity_entry(const Entity& entity)
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

    //--------------------------------------------------------------------------
    // Sections in the order of the format's key table, omitting empty ones
    std::string document_text(const Document& document)
    {
        std::vector<std::string> sections
        {
            std::format("{}\"format\": {}", indentation(section_indent), string_text(format_name)),
            std::format("{}\"version\": {}", indentation(section_indent), string_text(format_version))
        };

        if (document.zone_id.has_value())
        {
            sections.push_back(std::format("{}\"zone\": {}", indentation(section_indent),
                object_text({std::format("\"id\": {}", *document.zone_id)})));
        }

        if (!document.generator.is_null())
        {
            sections.push_back(content_section("generator", document.generator));
        }

        if (!document.components.is_null())
        {
            sections.push_back(object_section("layout", {std::format("\"components\": {}", inline_text(document.components))}));
        }

        sections.push_back(cells_section(document.cells));

        if (!document.barriers.empty())
        {
            sections.push_back(std::format("{}\"barriers\": {}", indentation(section_indent), barriers_text(document.barriers)));
        }

        auto add_entries = [&sections](std::string_view key, const auto& values, const auto& to_entry)
        {
            if (values.empty())
            {
                return;
            }

            std::vector<entry_type> entries;
            for (const auto& value : values)
            {
                entries.push_back(to_entry(value));
            }

            sections.push_back(entries_section(key, entries));
        };

        add_entries("rooms", document.rooms, room_entry);
        add_entries("walls", document.walls, wall_entry);
        add_entries("flats", document.flats, flat_entry);
        add_entries("entities", document.entities, entity_entry);

        std::string text = "{\n";
        for (size_t i = 0; i < sections.size(); ++i)
        {
            text += std::format("{}{}\n", sections[i], i + 1 == sections.size() ? "" : ",");
        }

        return text + "}\n";
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const Document& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const Document& document)
{
    std::ofstream os(filepath, std::ios::binary);
    if (!os)
    {
        throw std::runtime_error(std::format("{}: cannot be opened for writing", filepath.string()));
    }

    write(os, document);

    if (!os)
    {
        throw std::runtime_error(std::format("{}: cannot be written", filepath.string()));
    }

    return;
}
