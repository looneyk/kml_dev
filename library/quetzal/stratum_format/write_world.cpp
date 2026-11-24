//------------------------------------------------------------------------------
// stratum_format
// write_world.cpp
//------------------------------------------------------------------------------

#include "write_world.hpp"
#include "RegionSpec.hpp"
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
    std::string zone_text(const quetzal::stratum_core::Zone& zone)
    {
        std::vector<std::string> members{std::format("\"id\": {}", zone.zone_id.id)};
        if (zone.name.has_value())
        {
            members.push_back(std::format("\"name\": {}", string_text(zone.name.value())));
        }

        if (zone.position.has_value())
        {
            members.push_back(std::format("\"position\": [{}, {}, {}]", zone.position->x(), zone.position->y(), zone.position->z()));
        }

        return object_text(members);
    }

    //--------------------------------------------------------------------------
    std::string region_text(const quetzal::stratum_core::Region& region, size_t indent)
    {
        std::vector<std::string> opening{std::format("\"id\": {}", region.region_id.id)};
        if (region.parent_id.has_value())
        {
            opening.push_back(std::format("\"parent\": {}", region.parent_id.value().id));
        }

        opening.push_back(std::format("\"name\": {}", string_text(region.name)));
        if (region.display_name.has_value())
        {
            opening.push_back(std::format("\"display_name\": {}", string_text(region.display_name.value())));
        }

        if (!region.components.empty())
        {
            std::vector<std::string> components;
            for (const auto& [key, value] : region.components)
            {
                components.push_back(std::format("{}: {}", string_text(key), string_text(value)));
            }

            opening.push_back(std::format("\"components\": {}", object_text(components)));
        }

        std::vector<std::string> zone_ids;
        for (quetzal::stratum_core::PersistentId zone_id : region.zone_ids)
        {
            zone_ids.push_back(std::to_string(zone_id.id));
        }

        size_t continuation_indent = indent + 2;
        std::string text = std::format("{}{{ {},\n", indentation(indent), joined(opening));
        text += std::format("{}\"spec\": {},\n", indentation(continuation_indent), region_spec_text(region.region_spec));
        return text + std::format("{}\"zone_ids\": [{}] }}", indentation(continuation_indent), joined(zone_ids));
    }

    //--------------------------------------------------------------------------
    std::string worldspace_text(const quetzal::stratum_core::Worldspace& worldspace, size_t indent)
    {
        size_t continuation_indent = indent + 2;
        std::string text = std::format("{}{{ \"id\": {}, \"name\": {}, \"seed\": {},\n", indentation(indent), worldspace.worldspace_id.id,
            string_text(worldspace.name), string_text(std::to_string(worldspace.seed)));
        text += std::format("{}\"regions\": [\n", indentation(continuation_indent));

        const quetzal::stratum_core::RegionTree::regions_type& regions = worldspace.regions.regions();
        for (size_t i = 0; i < regions.size(); ++i)
        {
            text += std::format("{}{}\n", region_text(regions[i], continuation_indent + 2), i + 1 == regions.size() ? "" : ",");
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

        std::string zones = "\"zones\": [";

        if (document.zones.empty())
        {
            zones += "]";
        }
        else
        {
            zones += "\n";

            size_t i = 0;
            for (const auto& [zone_id, zone] : document.zones)
            {
                zones += std::format("{}{}{}\n", indentation(entry_indent + 2), zone_text(zone), ++i == document.zones.size() ? "" : ",");
            }

            zones += indentation(entry_indent) + "]";
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
        sections.push_back(object_section("world", {worldspaces, zones, entity_zones}));

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
