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
    std::string region_text(const WorldRegion& region, size_t indent)
    {
        std::vector<std::string> opening {std::format("\"id\": {}", region.id)};
        if (!region.components.is_null())
        {
            opening.push_back(std::format("\"components\": {}", inline_text(region.components)));
        }

        size_t continuation_indent = indent + 2;
        std::string text = std::format("{}{{ {},\n", indentation(indent), joined(opening));
        text += std::format("{}\"spec\": {},\n", indentation(continuation_indent), object_or_empty_text(region.spec));
        text += std::format("{}\"zones\": [", indentation(continuation_indent));

        if (region.zones.empty())
        {
            return text + "] }";
        }

        text += "\n";

        for (size_t i = 0; i < region.zones.size(); ++i)
        {
            const ZoneReference& zone = region.zones[i];
            text += std::format("{}{}{}\n", indentation(continuation_indent + 2), object_text({std::format("\"id\": {}", zone.id),
                std::format("\"file\": {}", string_text(zone.file))}), i + 1 == region.zones.size() ? "" : ",");
        }

        return text + indentation(continuation_indent) + "] }";
    }

    //--------------------------------------------------------------------------
    std::string document_text(const WorldDocument& document)
    {
        std::string regions = "\"regions\": [";

        if (document.regions.empty())
        {
            regions += "]";
        }
        else
        {
            regions += "\n";

            for (size_t i = 0; i < document.regions.size(); ++i)
            {
                regions += std::format("{}{}\n", region_text(document.regions[i], entry_indent + 2), i + 1 == document.regions.size() ? "" : ",");
            }

            regions += indentation(entry_indent) + "]";
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
        sections.push_back(object_section("world", {regions, entity_zones}));

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
