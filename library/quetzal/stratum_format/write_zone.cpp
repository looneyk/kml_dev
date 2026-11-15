//------------------------------------------------------------------------------
// stratum_format
// write_zone.cpp
//------------------------------------------------------------------------------

#include "write_zone.hpp"
#include "json_util.hpp"
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

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
    EntryMember entries_member(std::string_view key, std::vector<entry_type> entries)
    {
        return {std::string(key), {}, std::move(entries)};
    }

    //--------------------------------------------------------------------------
    void add_components(entry_type& entry, const raw_values_type& components)
    {
        if (!components.is_null())
        {
            entry.push_back({"components", inline_text(components)});
        }

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
    std::string generator_section(const Generator& generator)
    {
        return object_section("generator", {std::format("\"name\": {}", string_text(generator.name)),
            std::format("\"parameters\": {}", text_values_text(generator.parameters))});
    }

    //--------------------------------------------------------------------------
    std::string cells_section(const Cells& cells)
    {
        std::vector<std::string> members
        {
            std::format("\"notation\": {}", string_text(name(cells.notation))),
            std::format("\"extent\": {:/s, /b[]}", cells.extent),
            std::format("\"origin\": {:/s, /b[]}", cells.origin),
            std::format("\"scale\": {}", cells.scale)
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
        entry_type entry {{"wall", wall_placement_text(wall.placement)}};
        add_components(entry, wall.components);
        return entry;
    }

    //--------------------------------------------------------------------------
    void add_flat_properties(entry_type& entry, const FlatProperties& properties)
    {
        if (properties.height.has_value())
        {
            entry.push_back({"height", std::format("{}", *properties.height)});
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
    // Sections in the order of the format's key table, omitting empty ones
    std::string document_text(const ZoneDocument& document)
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

        if (document.generator.has_value())
        {
            sections.push_back(generator_section(*document.generator));
        }

        if (document.zone_spec.has_value())
        {
            sections.push_back(std::format("{}\"zone_spec\": {}", indentation(section_indent), text_values_text(document.zone_spec.value())));
        }

        // access_points is required in a zone with cells
        std::vector<std::string> components;
        if (document.cells.has_value())
        {
            std::vector<std::string> access_points;
            for (stored_entity_id_type id : document.access_points)
            {
                access_points.push_back(reference_text(id));
            }

            components.push_back(std::format("\"access_points\": [ {} ]", joined(access_points)));
        }
        if (document.position.has_value())
        {
            components.push_back(std::format("\"position\": {:/s, /b[]}", *document.position));
        }

        if (document.is_unpopulated)
        {
            components.push_back("\"unpopulated\": true");
        }

        if (document.components.is_object())
        {
            for (const auto& [key, value] : document.components.items())
            {
                components.push_back(std::format("{}: {}", string_text(key), inline_text(value)));
            }
        }

        sections.push_back(object_section("layout", {std::format("\"components\": {}", object_text(components))}));

        if (document.cells.has_value())
        {
            sections.push_back(cells_section(document.cells.value()));
        }

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

        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const ZoneDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const ZoneDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}
