//------------------------------------------------------------------------------
// stratum_format
// write_world.cpp
//------------------------------------------------------------------------------

#include "write_world.hpp"
#include "json_util.hpp"
#include <format>
#include <ostream>
#include <string>
#include <vector>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    std::string zone_text(const WorldDocument::Zone& zone)
    {
        std::vector<std::string> members {std::format("\"id\": {}", zone.id)};
        if (zone.name.has_value())
        {
            members.push_back(std::format("\"name\": {}", string_text(*zone.name)));
        }

        members.push_back(std::format("\"file\": {}", string_text(zone.file)));
        if (zone.position.has_value())
        {
            members.push_back(std::format("\"position\": [{}, {}, {}]", zone.position->x(), zone.position->y(), zone.position->z()));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    // The first line has no indentation, as it follows the member name
    std::string region_text(const WorldDocument::Region& region, size_t indent)
    {
        std::vector<std::string> opening {std::format("\"id\": {}", region.id)};
        if (region.parent_id.has_value())
        {
            opening.push_back(std::format("\"parent\": {}", region.parent_id.value()));
        }

        opening.push_back(std::format("\"name\": {}", string_text(region.name)));
        if (region.display_name.has_value())
        {
            opening.push_back(std::format("\"display_name\": {}", string_text(region.display_name.value())));
        }

        opening.push_back(std::format("\"name_counter\": {}", region.name_counter));
        if (!region.components.is_null())
        {
            opening.push_back(std::format("\"components\": {}", inline_text(region.components)));
        }

        size_t continuation_indent = indent + 2;
        std::string text = std::format("{{ {},\n", joined(opening));
        text += std::format("{}\"spec\": {},\n", indentation(continuation_indent), region_spec_text(region.spec));
        text += std::format("{}\"zones\": [", indentation(continuation_indent));

        if (region.zones.empty())
        {
            return text + "] }";
        }

        text += "\n";

        for (size_t i = 0; i < region.zones.size(); ++i)
        {
            text += std::format("{}{}{}\n", indentation(continuation_indent + 2), zone_text(region.zones[i]), i + 1 == region.zones.size() ? "" : ",");
        }

        return text + indentation(continuation_indent) + "] }";
    }

    //--------------------------------------------------------------------------
    std::string worldspace_text(const WorldDocument::Worldspace& worldspace, size_t indent)
    {
        size_t continuation_indent = indent + 2;
        std::string text = std::format("{}{{ \"id\": {}, \"name\": {}, \"seed\": {},\n", indentation(indent), worldspace.id, string_text(worldspace.name),
            string_text(std::to_string(worldspace.seed)));
        text += std::format("{}\"regions\": [\n", indentation(continuation_indent));

        for (size_t i = 0; i < worldspace.regions.size(); ++i)
        {
            text += std::format("{}{}{}\n", indentation(continuation_indent + 2), region_text(worldspace.regions[i], continuation_indent + 2),
                i + 1 == worldspace.regions.size() ? "" : ",");
        }

        return text + indentation(continuation_indent) + "] }";
    }

    //--------------------------------------------------------------------------
    std::string document_text(const WorldDocument& document)
    {
        std::string worldspaces = "\"worldspaces\": [";

        if (document.worldspaces.empty())
        {
            worldspaces += "]";
        }
        else
        {
            worldspaces += "\n";

            for (size_t i = 0; i < document.worldspaces.size(); ++i)
            {
                worldspaces += std::format("{}{}\n", worldspace_text(document.worldspaces[i], entry_indent + 2),
                    i + 1 == document.worldspaces.size() ? "" : ",");
            }

            worldspaces += indentation(entry_indent) + "]";
        }

        std::string entity_zones = "\"entity_zones\": [";

        if (document.entity_zones.empty())
        {
            entity_zones += "]";
        }
        else
        {
            entity_zones += "\n";

            size_t i = 0;
            for (const auto& [entity_id, zone_id] : document.entity_zones)
            {
                entity_zones += std::format("{}[{}, {}]{}\n", indentation(entry_indent + 2), entity_id, zone_id, ++i == document.entity_zones.size() ? "" : ",");
            }

            entity_zones += indentation(entry_indent) + "]";
        }

        std::vector<std::string> sections = file_header_sections();
        sections.push_back(object_section("world", {std::format("\"name_counter\": {}", document.name_counter), worldspaces, entity_zones}));

        return top_level_object_text(sections);
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(std::ostream& os, const WorldDocument& document)
{
    os << document_text(document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_format::write(const std::filesystem::path& filepath, const WorldDocument& document)
{
    write_file(filepath, document_text(document));
    return;
}
