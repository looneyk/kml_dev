//------------------------------------------------------------------------------
// stratum_format
// read_world.cpp
//------------------------------------------------------------------------------

#include "read_world.hpp"
#include "RegionSpec.hpp"
#include "checked_values.hpp"
#include "saved_game_format.hpp"
#include "quetzal/log/Log.hpp"
#include <optional>
#include <set>
#include <utility>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;

    using quetzal::stratum_core::PersistentId;
    using quetzal::stratum_core::Region;
    using quetzal::stratum_core::RegionSpec;
    using quetzal::stratum_core::Worldspace;
    using quetzal::stratum_core::Zone;

    //--------------------------------------------------------------------------
    // Throws if entity_ids already has the id
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
    [[noreturn]] void fail_logged(const std::string& path, const std::string& what)
    {
        quetzal::log::logger().error(log_category, "{}: {}", path, what);
        fail(path, what);
    }

    //--------------------------------------------------------------------------
    WorldDocument::zones_type read_zones(const raw_values_type& world, std::set<stored_entity_id_type>& entity_ids)
    {
        const std::string zones_path = "world.zones";
        const raw_values_type& zones = array(required_member(world, "world", "zones"), zones_path);
        WorldDocument::zones_type read;

        for (size_t i = 0; i < zones.size(); ++i)
        {
            const std::string zone_path = element_path(zones_path, i);
            object(zones[i], zone_path);

            Zone zone{PersistentId{id_value(required_member(zones[i], zone_path, "id"), member_path(zone_path, "id"))}, std::nullopt, std::nullopt};
            add_entity_id(entity_ids, zone.zone_id.id, member_path(zone_path, "id"));

            if (const raw_values_type* p = member(zones[i], "position"); p != nullptr)
            {
                const std::string position_path = member_path(zone_path, "position");
                const raw_values_type& a = fixed_array(*p, position_path, 3);
                zone.position = discrete_point3_type{coord_value(a[0], element_path(position_path, 0)), coord_value(a[1], element_path(position_path, 1)),
                    coord_value(a[2], element_path(position_path, 2))};
            }

            if (const raw_values_type* p = member(zones[i], "name"); p != nullptr)
            {
                zone.name = string_value(*p, member_path(zone_path, "name"));
            }

            const PersistentId zone_id = zone.zone_id;
            read.emplace(zone_id, std::move(zone));
        }

        return read;
    }

    //--------------------------------------------------------------------------
    Region read_region(const raw_values_type& v, const std::string& path, std::set<stored_entity_id_type>& entity_ids,
        const WorldDocument::zones_type& zones, std::set<PersistentId>& zone_ids_in_regions, std::set<discrete_point3_type>& zone_positions)
    {
        object(v, path);

        Region region;
        region.region_id = PersistentId{id_value(required_member(v, path, "id"), member_path(path, "id"))};
        add_entity_id(entity_ids, region.region_id.id, member_path(path, "id"));

        if (const raw_values_type* p = member(v, "parent"); p != nullptr)
        {
            region.parent_id = PersistentId{id_value(*p, member_path(path, "parent"))};
        }

        region.name = string_value(required_member(v, path, "name"), member_path(path, "name"));

        if (const raw_values_type* p = member(v, "display_name"); p != nullptr)
        {
            region.display_name = string_value(*p, member_path(path, "display_name"));
        }

        if (const raw_values_type* p = member(v, "components"); p != nullptr)
        {
            const std::string components_path = member_path(path, "components");
            for (const auto& [key, value] : object(*p, components_path).items())
            {
                region.components.insert(key, string_value(value, member_path(components_path, key)));
            }
        }

        region.region_spec = read_region_spec(required_member(v, path, "spec"), member_path(path, "spec"));

        const std::string zone_ids_path = member_path(path, "zone_ids");
        const raw_values_type& zone_ids = array(required_member(v, path, "zone_ids"), zone_ids_path);

        for (size_t i = 0; i < zone_ids.size(); ++i)
        {
            const std::string zone_id_path = element_path(zone_ids_path, i);
            const PersistentId zone_id{id_value(zone_ids[i], zone_id_path)};

            auto zone = zones.find(zone_id);
            if (zone == zones.end())
            {
                fail_logged(zone_id_path, std::format("zone {} is not one of the world's zones", zone_id.id));
            }

            if (!zone_ids_in_regions.insert(zone_id).second)
            {
                fail_logged(zone_id_path, std::format("zone {} is in a second region", zone_id.id));
            }

            if (const std::optional<discrete_point3_type>& position = zone->second.position; position.has_value() && !zone_positions.insert(*position).second)
            {
                fail_logged(zone_id_path, std::format("a second zone of the worldspace at ({}, {}, {})", position->x(), position->y(), position->z()));
            }

            region.zone_ids.push_back(zone_id);
        }

        return region;
    }

    //--------------------------------------------------------------------------
    Worldspace read_worldspace(const raw_values_type& v, const std::string& path, std::set<stored_entity_id_type>& entity_ids,
        const WorldDocument::zones_type& zones, std::set<PersistentId>& zone_ids_in_regions)
    {
        object(v, path);

        const PersistentId worldspace_id{id_value(required_member(v, path, "id"), member_path(path, "id"))};
        add_entity_id(entity_ids, worldspace_id.id, member_path(path, "id"));
        const std::string name = string_value(required_member(v, path, "name"), member_path(path, "name"));
        const seed_type seed = seed_value(required_member(v, path, "seed"), member_path(path, "seed"));

        const std::string regions_path = member_path(path, "regions");
        const raw_values_type& regions = array(required_member(v, path, "regions"), regions_path);
        if (regions.empty())
        {
            fail(regions_path, "a worldspace with no regions");
        }

        std::set<discrete_point3_type> zone_positions;
        const std::string root_path = element_path(regions_path, 0);
        Region root = read_region(regions[0], root_path, entity_ids, zones, zone_ids_in_regions, zone_positions);
        if (root.parent_id.has_value())
        {
            fail(member_path(root_path, "parent"), "a top-level region with a parent");
        }

        const RegionSpec root_spec = root.region_spec;
        Worldspace worldspace = Worldspace::create(worldspace_id, name, seed, root_spec, std::move(root));

        for (size_t i = 1; i < regions.size(); ++i)
        {
            const std::string region_path = element_path(regions_path, i);
            Region region = read_region(regions[i], region_path, entity_ids, zones, zone_ids_in_regions, zone_positions);

            if (!region.parent_id.has_value() || !worldspace.regions.contains(region.parent_id.value()))
            {
                fail(region_path, "a subregion whose parent is not an earlier region of the worldspace");
            }

            const PersistentId parent_id = region.parent_id.value();
            worldspace.regions.add_subregion(parent_id, std::move(region));
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
        const raw_values_type& worldspaces = array(required_member(world, "world", "worldspaces"), "world.worldspaces");
        std::set<stored_entity_id_type> entity_ids;
        document.zones = read_zones(world, entity_ids);
        std::set<PersistentId> zone_ids_in_regions;

        for (size_t i = 0; i < worldspaces.size(); ++i)
        {
            document.worldspaces.push_back(read_worldspace(worldspaces[i], element_path("world.worldspaces", i), entity_ids, document.zones, zone_ids_in_regions));
        }

        for (const auto& [zone_id, zone] : document.zones)
        {
            if (!zone_ids_in_regions.contains(zone_id))
            {
                fail_logged("world.zones", std::format("zone {} is in no region", zone_id.id));
            }
        }

        const raw_values_type& entity_zones = array(required_member(world, "world", "entity_zones"), "world.entity_zones");

        for (size_t i = 0; i < entity_zones.size(); ++i)
        {
            std::string entry_path = element_path("world.entity_zones", i);
            const raw_values_type& entry = fixed_array(entity_zones[i], entry_path, 2);
            stored_entity_id_type entity_id = id_value(entry[0], element_path(entry_path, 0));
            stored_entity_id_type zone_id = id_value(entry[1], element_path(entry_path, 1));

            if (!document.zones.contains(PersistentId{zone_id}))
            {
                fail_logged(element_path(entry_path, 1), std::format("zone {} is not one of the world's zones", zone_id));
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
