//------------------------------------------------------------------------------
// stratum_format
// read_world.cpp
//------------------------------------------------------------------------------

#include "read_world.hpp"
#include "read_values.hpp"
#include "saved_game_format.hpp"
#include <set>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    // Adds each of the region's zone ids to zone_ids, which has those of the regions read before it
    WorldRegion read_region(const raw_values_type& v, const std::string& path, std::set<stored_entity_id_type>& zone_ids)
    {
        object(v, path);

        WorldRegion region;
        region.id = id_value(required_member(v, path, "id"), member_path(path, "id"));
        region.components = read_components(v, path);

        std::string spec_path = member_path(path, "spec");
        region.spec = object(required_member(v, path, "spec"), spec_path);
        for (const auto& [key, value] : region.spec.items())
        {
            string_value(value, member_path(spec_path, key));
        }

        std::string zones_path = member_path(path, "zones");
        const raw_values_type& zones = array(required_member(v, path, "zones"), zones_path);

        for (size_t i = 0; i < zones.size(); ++i)
        {
            std::string zone_path = element_path(zones_path, i);
            object(zones[i], zone_path);

            ZoneReference zone {id_value(required_member(zones[i], zone_path, "id"), member_path(zone_path, "id")),
                string_value(required_member(zones[i], zone_path, "file"), member_path(zone_path, "file"))};

            if (!zone_ids.insert(zone.id).second)
            {
                fail(member_path(zone_path, "id"), std::format("a second zone with the id {}", zone.id));
            }

            region.zones.push_back(std::move(zone));
        }

        return region;
    }

    //--------------------------------------------------------------------------
    WorldDocument read_world_document(const raw_values_type& v)
    {
        check_format(v, saved_game_format_name, saved_game_format_version);

        WorldDocument document;
        document.version = std::string(saved_game_format_version);

        const raw_values_type& world = object(required_member(v, "", "world"), "world");

        const raw_values_type& regions = array(required_member(world, "world", "regions"), "world.regions");
        std::set<stored_entity_id_type> region_ids;
        std::set<stored_entity_id_type> zone_ids;

        for (size_t i = 0; i < regions.size(); ++i)
        {
            std::string region_path = element_path("world.regions", i);
            WorldRegion region = read_region(regions[i], region_path, zone_ids);

            if (!region_ids.insert(region.id).second)
            {
                fail(member_path(region_path, "id"), std::format("a second region with the id {}", region.id));
            }

            document.regions.push_back(std::move(region));
        }

        const raw_values_type& entity_zones = array(required_member(world, "world", "entity_zones"), "world.entity_zones");

        for (size_t i = 0; i < entity_zones.size(); ++i)
        {
            std::string entry_path = element_path("world.entity_zones", i);
            const raw_values_type& entry = fixed_array(entity_zones[i], entry_path, 2);
            stored_entity_id_type entity_id = id_value(entry[0], element_path(entry_path, 0));
            stored_entity_id_type zone_id = id_value(entry[1], element_path(entry_path, 1));

            if (!zone_ids.contains(zone_id))
            {
                fail(element_path(entry_path, 1), std::format("zone {} is in no region", zone_id));
            }

            if (!document.entity_zones.emplace(entity_id, zone_id).second)
            {
                fail(element_path(entry_path, 0), std::format("a second entry for entity {}", entity_id));
            }
        }

        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::WorldDocument quetzal::stratum_format::read_world(std::istream& is, const std::string& name)
{
    return read_document<WorldDocument>(is, name, read_world_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::WorldDocument quetzal::stratum_format::read_world(const std::filesystem::path& filepath)
{
    return read_document<WorldDocument>(filepath, read_world_document);
}
