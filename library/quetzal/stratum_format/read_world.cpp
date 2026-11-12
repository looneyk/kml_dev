//------------------------------------------------------------------------------
// stratum_format
// read_world.cpp
//------------------------------------------------------------------------------

#include "read_world.hpp"
#include "checked_values.hpp"
#include "saved_game_format.hpp"
#include "quetzal/log/Log.hpp"
#include <array>
#include <set>
#include <utility>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    //--------------------------------------------------------------------------
    // Adds id to entity_ids, and an id already there is an error
    void add_entity_id(std::set<stored_entity_id_type>& entity_ids, stored_entity_id_type id, const std::string& path)
    {
        if (!entity_ids.insert(id).second)
        {
            const std::string what = std::format("a second entity with the id {}", id);
            quetzal::log::logger().error(log_category, "{}: {}", path, what);
            fail(path, what);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Reads a region and its list of zones. entity_ids has the id of every worldspace, region and zone read so far in the
    // file, zone_ids the id of every zone, and zone_positions the position of every zone read so far in this worldspace.
    // The region and its zones are added to them, and a zone with an existing position or name is an error
    WorldDocument::Region read_region(const raw_values_type& v, const std::string& path, std::set<stored_entity_id_type>& entity_ids,
        std::set<stored_entity_id_type>& zone_ids, std::set<std::array<coord_type, 3>>& zone_positions)
    {
        object(v, path);

        WorldDocument::Region region;
        region.id = id_value(required_member(v, path, "id"), member_path(path, "id"));
        add_entity_id(entity_ids, region.id, member_path(path, "id"));

        if (const raw_values_type* p = member(v, "parent"); p != nullptr)
        {
            region.parent_id = id_value(*p, member_path(path, "parent"));
        }

        region.name = string_value(required_member(v, path, "name"), member_path(path, "name"));

        if (const raw_values_type* p = member(v, "display_name"); p != nullptr)
        {
            region.display_name = string_value(*p, member_path(path, "display_name"));
        }

        region.name_counter = count_value(required_member(v, path, "name_counter"), member_path(path, "name_counter"));
        region.components = read_components(v, path);
        std::set<std::string> zone_names;

        region.spec = read_region_spec(required_member(v, path, "spec"), member_path(path, "spec"));

        std::string zones_path = member_path(path, "zones");
        const raw_values_type& zones = array(required_member(v, path, "zones"), zones_path);

        for (size_t i = 0; i < zones.size(); ++i)
        {
            std::string zone_path = element_path(zones_path, i);
            object(zones[i], zone_path);

            WorldDocument::Zone zone {id_value(required_member(zones[i], zone_path, "id"), member_path(zone_path, "id")),
                string_value(required_member(zones[i], zone_path, "file"), member_path(zone_path, "file"))};

            add_entity_id(entity_ids, zone.id, member_path(zone_path, "id"));
            zone_ids.insert(zone.id);

            if (const raw_values_type* p = member(zones[i], "position"); p != nullptr)
            {
                std::string position_path = member_path(zone_path, "position");
                const raw_values_type& a = fixed_array(*p, position_path, 3);
                const std::array<coord_type, 3> position {coord_value(a[0], element_path(position_path, 0)),
                    coord_value(a[1], element_path(position_path, 1)), coord_value(a[2], element_path(position_path, 2))};

                if (!zone_positions.insert(position).second)
                {
                    const std::string what = std::format("a second zone of the worldspace at ({}, {}, {})", position[0], position[1], position[2]);
                    quetzal::log::logger().error(log_category, "{}: {}", position_path, what);
                    fail(position_path, what);
                }

                zone.position = zone_position_type {position[0], position[1], position[2]};
            }

            if (const raw_values_type* p = member(zones[i], "name"); p != nullptr)
            {
                std::string name_path = member_path(zone_path, "name");
                zone.name = string_value(*p, name_path);

                if (!zone_names.insert(*zone.name).second)
                {
                    const std::string what = std::format("a second zone of the region named {}", *zone.name);
                    quetzal::log::logger().error(log_category, "{}: {}", name_path, what);
                    fail(name_path, what);
                }
            }

            region.zones.push_back(std::move(zone));
        }

        return region;
    }

    //--------------------------------------------------------------------------
    // Reads a worldspace and its regions. entity_ids has the id of every worldspace, region and zone read so far in the file,
    // and zone_ids the id of every zone. The worldspace, its regions and their zones are added to them. A worldspace without a
    // region, a top-level region with a parent, or a later region whose parent is not an earlier region of the worldspace is
    // an error
    WorldDocument::Worldspace read_worldspace(const raw_values_type& v, const std::string& path, std::set<stored_entity_id_type>& entity_ids,
        std::set<stored_entity_id_type>& zone_ids)
    {
        object(v, path);

        WorldDocument::Worldspace worldspace;
        worldspace.id = id_value(required_member(v, path, "id"), member_path(path, "id"));
        add_entity_id(entity_ids, worldspace.id, member_path(path, "id"));
        worldspace.name = string_value(required_member(v, path, "name"), member_path(path, "name"));

        const std::string regions_path = member_path(path, "regions");
        const raw_values_type& regions = array(required_member(v, path, "regions"), regions_path);
        if (regions.empty())
        {
            fail(regions_path, "a worldspace with no regions");
        }

        std::set<std::array<coord_type, 3>> zone_positions;
        std::set<stored_entity_id_type> region_ids;
        for (size_t i = 0; i < regions.size(); ++i)
        {
            const std::string region_path = element_path(regions_path, i);
            WorldDocument::Region region = read_region(regions[i], region_path, entity_ids, zone_ids, zone_positions);

            if (i == 0 && region.parent_id.has_value())
            {
                fail(member_path(region_path, "parent"), "a top-level region with a parent");
            }

            if (i != 0 && (!region.parent_id.has_value() || !region_ids.contains(region.parent_id.value())))
            {
                fail(region_path, "a subregion whose parent is not an earlier region of the worldspace");
            }

            region_ids.insert(region.id);
            worldspace.regions.push_back(std::move(region));
        }

        return worldspace;
    }

    //--------------------------------------------------------------------------
    WorldDocument read_world_document(const raw_values_type& v)
    {
        check_file_header(v, saved_game_format_name, saved_game_format_version);

        WorldDocument document;
        document.version = std::string(saved_game_format_version);

        const raw_values_type& world = object(required_member(v, "", "world"), "world");
        document.name_counter = count_value(required_member(world, "world", "name_counter"), "world.name_counter");

        const raw_values_type& worldspaces = array(required_member(world, "world", "worldspaces"), "world.worldspaces");
        std::set<stored_entity_id_type> entity_ids;
        std::set<stored_entity_id_type> zone_ids;

        for (size_t i = 0; i < worldspaces.size(); ++i)
        {
            document.worldspaces.push_back(read_worldspace(worldspaces[i], element_path("world.worldspaces", i), entity_ids, zone_ids));
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
quetzal::stratum_format::WorldDocument quetzal::stratum_format::read_world(std::istream& is, const std::string& stream_name)
{
    return read_document<WorldDocument>(is, stream_name, read_world_document);
}

//------------------------------------------------------------------------------
quetzal::stratum_format::WorldDocument quetzal::stratum_format::read_world(const std::filesystem::path& filepath)
{
    return read_document<WorldDocument>(filepath, read_world_document);
}
