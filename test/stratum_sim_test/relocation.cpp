//------------------------------------------------------------------------------
// stratum_sim_test
// relocation.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Intent.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscretePlacementIndex.hpp"
#include "quetzal/stratum_sim/Name.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/seeds.hpp"
#include "quetzal/stratum_sim/specs.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/RelocatorTarget.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("relocation");

    //--------------------------------------------------------------------------
    TEST_CASE("specs")
    {
        const stratum_sim::RegionSpec spec {quetzal::KeyValueStore {{"generator_selector", "common"}, {"name", "The Downs"}, {"zone.position.z", "-1"},
            {"zone.parameters.denizens", "few"}, {"zones", "3"}}};

        CHECK_CASE("no unknown keys when every key is known", stratum_sim::find_unknown_keys(spec.keys,
            vector<string_view> {"generator_selector", "name", "zone.position.z", "zone.parameters.denizens", "zones", "difficulty"}).empty());
        CHECK_CASE("the unknown keys, in key order", stratum_sim::find_unknown_keys(spec.keys, vector<string_view> {"name"})
            == vector<string> {"generator_selector", "zone.parameters.denizens", "zone.position.z", "zones"});

        const KeyValueStore zone_keys = stratum_sim::select_prefixed(spec.keys, "zone.");
        CHECK_CASE("the entries under a prefix, with the prefix removed, and not the key zones under zone.",
            zone_keys.value("position.z") == "-1" && zone_keys.value("parameters.denizens") == "few" && !zone_keys.contains("s")
            && zone_keys.size() == 2);
        const KeyValueStore none = stratum_sim::select_prefixed(spec.keys, "region.");
        CHECK_CASE("nothing under a prefix that no key has", none.empty());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "worldspaces and regions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        stratum_sim::PersistentId surface = world.create_worldspace("surface", test_region_spec("staircase"));
        stratum_sim::PersistentId crypt_worldspace = world.create_worldspace("crypt", test_region_spec("staircase"));
        stratum_sim::PersistentId crypt = top_level_region(world, crypt_worldspace);

        CHECK_CASE("a worldspace has a persistent id and its name", world.worldspace(surface).worldspace_id == surface
            && world.worldspace(surface).name == "surface");
        CHECK_CASE("the world has its worldspaces in the order they were created", world.worldspaces().size() == 3
            && world.worldspaces()[1].worldspace_id == surface && world.worldspaces()[2].worldspace_id == crypt_worldspace);
        CHECK_CASE("a worldspace's region tree is rooted in its top-level region, with a persistent id and the region spec it was created from",
            world.worldspace(crypt_worldspace).regions.regions().size() == 1 && crypt != crypt_worldspace
            && world.region(crypt).region_spec.keys.value(stratum_sim::generator_selector_key) == "staircase");

        flecs::entity field = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity chapel = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel, stratum_format::read_zone("data/corridor.zone"));

        CHECK_CASE("a zone is in its worldspace and its region", stratum_sim::worldspace_of(chapel) == crypt_worldspace
            && stratum_sim::region_of(chapel) == crypt && stratum_sim::region_of(field) == world.region_id);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity monk = world.create_entity(chapel,
            stratum_sim::placement_type(zone_layout(world, chapel).position(stratum_sim::discrete_point_type {1, 1}), east));
        stratum_sim::EntityReference monk_reference = world.make_reference(monk);

        CHECK_CASE("an entity in a zone is in that zone, and a zone is in none", stratum_sim::zone_of(monk) == chapel
            && !stratum_sim::zone_of(chapel));

        flecs::entity chapel_below = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel_below, stratum_format::read_zone("data/room.zone"));
        stratum_sim::PersistentId chapel_id = chapel.get<stratum_sim::PersistentId>();
        CHECK_CASE_EQUAL("a world counts the zones of every worldspace", world.zone_count(), 3);

        world.unload_worldspace(crypt_worldspace);

        CHECK_CASE("unloading a worldspace removes it and its regions, and deletes its zones",
            !ranges::contains(world.worldspaces(), crypt_worldspace, &stratum_sim::Worldspace::worldspace_id) && !chapel.is_alive()
            && !chapel_below.is_alive() && !monk.is_alive());
        CHECK_CASE("and leaves the other worldspaces and their zones", world.worldspaces().size() == 2 && world.worldspaces()[1].worldspace_id == surface
            && field.is_alive() && world.zone_count() == 1);
        CHECK_CASE("the world index keeps the zone of an entity something refers to, as an unloaded zone's",
            world.zone_of(monk_reference.persistent_id) == chapel_id && !world.entity(monk_reference.persistent_id));

        // A zone's position, from its file or from the region that makes it
        stratum_format::Document leveled_document = stratum_format::read_zone("data/door.zone");
        leveled_document.position = stratum_format::zone_position_type {2, -5, -3};
        flecs::entity leveled = world.create_zone(world.region_id, world.next_id());
        stratum_sim::load_layout(leveled, leveled_document);

        const auto* leveled_position = leveled.try_get<stratum_sim::ZonePosition>();
        const map<stratum_sim::ZonePosition, stratum_sim::PersistentId>& positioned_zone_ids =
            world.worldspace(stratum_sim::worldspace_of(leveled)).zone_ids;
        CHECK_CASE("a zone has the position its file states", leveled_position != nullptr && leveled_position->x == 2
            && leveled_position->y == -5 && leveled_position->z == -3);
        CHECK_CASE("and is listed in its worldspace at that position", positioned_zone_ids.size() == 1
            && positioned_zone_ids.begin()->first == stratum_sim::ZonePosition {2, -5, -3}
            && positioned_zone_ids.begin()->second == leveled.get<stratum_sim::PersistentId>());
        CHECK_CASE("and none where its file states none", field.try_get<stratum_sim::ZonePosition>() == nullptr);

        stratum_format::Document named_document = stratum_format::read_zone("data/door.zone");
        named_document.components["name"] = "The Narrow Door";
        bool named_buildable = !stratum_sim::check_buildable(world.entities(), named_document).has_value();
        flecs::entity named = world.create_zone(world.region_id, world.next_id());
        stratum_sim::load_layout(named, named_document);

        CHECK_CASE("a zone has the name its file states", named_buildable
            && named.try_get<stratum_sim::Name>() != nullptr && named.get<stratum_sim::Name>().name == "The Narrow Door");
        CHECK_CASE("and the display name derived from it", named.try_get<stratum_sim::DisplayName>() != nullptr
            && named.get<stratum_sim::DisplayName>().name == "THE NARROW DOOR");

        stratum_sim::PersistentId dungeon = create_test_region(world, "door");
        flecs::entity made = world.create_zone(dungeon, test_zone_spec(-1));

        CHECK_CASE("a region makes a zone at a level with the generator its selector chooses", stratum_sim::region_of(made) == dungeon
            && level_of(made) == -1 && made.has<stratum_sim::ZoneLayout>());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("region trees")
    {
        auto make_region = [](stratum_sim::PersistentId::type id, string name)
        {
            stratum_sim::Region region;
            region.region_id = stratum_sim::PersistentId {id};
            region.name = std::move(name);
            return region;
        };

        stratum_sim::RegionTree regions(make_region(1, "dungeon"));
        regions.add_subregion(stratum_sim::PersistentId {1}, make_region(2, "crypt"));
        regions.add_subregion(stratum_sim::PersistentId {2}, make_region(3, "ossuary"));
        regions.add_subregion(stratum_sim::PersistentId {1}, make_region(4, "mine"));

        vector<stratum_sim::PersistentId::type> region_ids;
        for (const stratum_sim::Region& region : regions.regions())
        {
            region_ids.push_back(region.region_id.id);
        }

        CHECK_CASE("a region tree is rooted in its top-level region, which has no parent",
            regions.root().region_id == stratum_sim::PersistentId {1} && !regions.root().parent_id.has_value());
        CHECK_CASE("a subregion has its parent, and its parent lists it", regions.region(stratum_sim::PersistentId {3}).parent_id
            == stratum_sim::PersistentId {2} && regions.region(stratum_sim::PersistentId {1}).subregion_ids
            == vector<stratum_sim::PersistentId> {stratum_sim::PersistentId {2}, stratum_sim::PersistentId {4}});
        CHECK_CASE("the regions are in the order they were added, each after its parent",
            region_ids == vector<stratum_sim::PersistentId::type> {1, 2, 3, 4});
        CHECK_CASE("a tree contains its own regions",
            regions.contains(stratum_sim::PersistentId {3}) && !regions.contains(stratum_sim::PersistentId {5}));
        CHECK_CASE("a region's extended name is the name of each region from the top-level region down to it, joined by '/'",
            regions.extended_name(stratum_sim::PersistentId {3}) == "dungeon/crypt/ossuary"
            && regions.extended_name(stratum_sim::PersistentId {1}) == "dungeon");

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "names")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        auto name_of = [](flecs::entity e) { return e.get<stratum_sim::Name>().name; };

        CHECK_CASE("a zone name pattern has each coordinate of the zone's position in place of {x}, {y} and {z}",
            stratum_sim::zone_name("Level {z} at {x}, {y}", stratum_sim::ZonePosition {3, -4, -2}) == "Level -2 at 3, -4");

        stratum_sim::PersistentId dungeon_worldspace = world.create_worldspace("dungeon", test_region_spec("door"));
        stratum_sim::PersistentId dungeon = top_level_region(world, dungeon_worldspace);
        stratum_sim::PersistentId second_worldspace = world.create_worldspace("dungeon", test_region_spec("door"));
        stratum_sim::PersistentId third_worldspace = world.create_worldspace("dungeon", test_region_spec("door"));

        CHECK_CASE("a worldspace has the name it is created with, and so does its top-level region",
            world.worldspace(dungeon_worldspace).name == "dungeon" && world.region(dungeon).name == "dungeon");
        CHECK_CASE("and a worldspace whose name another has gets a suffix from the world's counter",
            world.worldspace(second_worldspace).name == "dungeon_1" && world.region(top_level_region(world, second_worldspace)).name == "dungeon_1"
            && world.worldspace(third_worldspace).name == "dungeon_2");

        flecs::entity made = world.create_zone(dungeon, test_zone_spec(-1));
        stratum_sim::ZoneSpec patterned_spec = test_zone_spec(-2);
        patterned_spec.keys.insert(string(stratum_sim::name_key), "Level {z}");
        flecs::entity patterned = world.create_zone(dungeon, patterned_spec);

        CHECK_CASE("a zone made with no name pattern has its region's name and its coordinates", name_of(made) == "dungeon_0_0_-1");
        CHECK_CASE("and one made with a pattern has the pattern's name", name_of(patterned) == "Level -2");

        stratum_sim::RegionSpec grid_spec = test_region_spec("door");
        grid_spec.keys.insert("worldspace.zone_size.x", "40");
        grid_spec.keys.insert("worldspace.zone_size.y", "20");
        stratum_sim::PersistentId surface = top_level_region(world, world.create_worldspace("surface", grid_spec));
        stratum_sim::ZoneSpec grid_zone_spec;
        grid_zone_spec.set_position(stratum_sim::ZonePosition {1, 2, 0});
        CHECK_CASE("and a zone of a grid worldspace has its region's name and its x and y",
            name_of(world.create_zone(surface, grid_zone_spec)) == "surface_1_2");

        // data/door.zone has no name and no position
        flecs::entity first_loaded = world.load_zone(dungeon, "data/door.zone");
        flecs::entity second_loaded = world.load_zone(dungeon, "data/door.zone");
        const stratum_sim::Region& dungeon_data = world.region(dungeon);

        CHECK_CASE("a zone with no name and no position has its region's name", name_of(first_loaded) == "dungeon");
        CHECK_CASE("and a zone whose name another zone of the region has gets a suffix from the region's counter",
            name_of(second_loaded) == "dungeon_1" && dungeon_data.name_counter == 1);
        CHECK_CASE("a region lists its zones by name", dungeon_data.named_zone_ids.size() == 4
            && dungeon_data.named_zone_ids.at("dungeon_1") == second_loaded.get<stratum_sim::PersistentId>());

        CHECK_CASE("a zone's extended name is its worldspace's name and its name", world.extended_name(patterned) == "dungeon/Level -2");

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "positions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        stratum_sim::PersistentId second = create_test_region(world, "door");
        stratum_sim::PersistentId third = create_test_region(world, "door");
        stratum_sim::RegionSpec placed_spec = test_region_spec("door");
        placed_spec.set_position(stratum_sim::RegionPosition {-4, 7});
        stratum_sim::PersistentId placed = top_level_region(world, world.create_worldspace("door", placed_spec));
        stratum_sim::PersistentId fourth = create_test_region(world, "door");

        auto region_at = [&](stratum_sim::PersistentId region, stratum_sim::coord_type x, stratum_sim::coord_type y)
        {
            const stratum_sim::RegionPosition position = world.region(region).region_spec.position();
            return position.x == x && position.y == y;
        };

        CHECK_CASE("the first region's seed position is the origin, and each new one's one past the largest x, at y 0",
            region_at(world.region_id, 0, 0) && region_at(second, 1, 0) && region_at(third, 2, 0));
        CHECK_CASE("a region whose spec has a seed position keeps it", region_at(placed, -4, 7) && region_at(fourth, 3, 0));

        flecs::entity made = world.create_zone(second, test_zone_spec(-3));
        const stratum_sim::ZonePosition& made_position = made.get<stratum_sim::ZonePosition>();
        CHECK_CASE("a zone is at the position its spec has", made_position.x == 0 && made_position.y == 0 && made_position.z == -3);

        const stratum_format::Document made_document = stratum_sim::make_document(made);
        CHECK_CASE("and a saved zone keeps its position", made_document.position == stratum_format::zone_position_type {0, 0, -3});

        CHECK_CASE("a coordinate a spec does not have is 0", stratum_sim::ZoneSpec {KeyValueStore {{string(stratum_sim::position_z_key), "5"}}}.position().x == 0
            && stratum_sim::ZoneSpec {}.position().z == 0);
        const stratum_sim::ZoneSpec compass_spec {KeyValueStore {{string(stratum_sim::position_x_key), "east"}}};
        CHECK_THROWS_AS(compass_spec.position(), runtime_error);
        stratum_sim::PersistentId second_worldspace = stratum_sim::worldspace_of(made);

        // A worldspace's coordinate system
        stratum_sim::RegionSpec barrow_spec = test_region_spec("door");
        barrow_spec.keys.insert("worldspace.extent.z", "-10 .. -1");
        barrow_spec.keys.insert("worldspace.zone_size.x", "64");
        barrow_spec.keys.insert("worldspace.zone_size.y", "32");
        stratum_sim::PersistentId barrow_worldspace = world.create_worldspace("barrow", barrow_spec);
        const stratum_sim::Worldspace barrow = world.worldspace(barrow_worldspace);

        CHECK_CASE("a worldspace has the extent and zone size its region spec states, and is unbounded on an axis it does not state",
            !barrow.extent.x.has_value() && !barrow.extent.y.has_value() && barrow.extent.z.has_value() && barrow.extent.z->lower() == -10
            && barrow.extent.z->upper() == -1 && barrow.zone_size == stratum_sim::discrete_vector_type(64, 32));

        auto creation_throws = [&](string_view key, string_view value)
        {
            stratum_sim::RegionSpec spec = test_region_spec("door");
            spec.keys.insert(string(key), string(value));
            try
            {
                world.create_worldspace("malformed", spec);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        CHECK_CASE("an extent that is not an interval of integers is an error", creation_throws("worldspace.extent.x", "east"));
        CHECK_CASE("and one whose lower bound is above its upper bound", creation_throws("worldspace.extent.x", "1 .. -1"));
        CHECK_CASE("and a zone size with one coordinate", creation_throws("worldspace.zone_size.x", "64"));

        stratum_sim::PersistentId barrow_region = top_level_region(world, barrow_worldspace);
        flecs::entity deep = world.create_zone(barrow_region, test_zone_spec(-4));
        auto barrow_zone_ids = [&]() { return world.worldspace(barrow_worldspace).zone_ids; };
        CHECK_CASE("a zone made at a position is listed in its worldspace at it", barrow_zone_ids().size() == 1
            && barrow_zone_ids().at(stratum_sim::ZonePosition {0, 0, -4}) == deep.get<stratum_sim::PersistentId>());

        size_t zone_count = world.zone_count();
        CHECK_THROWS_AS(world.create_zone(barrow_region, test_zone_spec(-4)), runtime_error);
        CHECK_THROWS_AS(world.create_zone(barrow_region, test_zone_spec(0)), runtime_error);
        CHECK_CASE("a zone at another zone's position, or outside the extent, is an error, and nothing is built",
            world.zone_count() == zone_count && barrow_zone_ids().size() == 1);

        flecs::entity same_position = world.create_zone(second, test_zone_spec(-4));
        CHECK_CASE("a zone of another worldspace may be at the same position", level_of(same_position) == -4
            && stratum_sim::worldspace_of(same_position) == second_worldspace);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "seeds")
    {
        const array<stratum_sim::coord_type, 3> coordinates {1, -2, 3};
        const stratum_sim::RandomStream::seed_type derived = stratum_sim::derived_seed(7, coordinates);
        CHECK_CASE("a derived seed is the same for the same seed and coordinates", stratum_sim::derived_seed(7, coordinates) == derived);
        CHECK_CASE("and differs by seed, by coordinate, and by coordinate order", stratum_sim::derived_seed(8, coordinates) != derived
            && stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 3> {1, -2, 4}) != derived
            && stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 3> {-2, 1, 3}) != derived);

        stratum_sim::RegionSpec region_spec = test_region_spec("door");
        region_spec.set_position(stratum_sim::RegionPosition {4, -5});
        CHECK_CASE("a region's seed derives from the world seed and the region's position",
            stratum_sim::region_seed(7, region_spec) == stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 2> {4, -5}));
        stratum_sim::ZoneSpec zone_spec = test_zone_spec(-3);
        CHECK_CASE("a zone's seed derives from the region's seed and the zone's position",
            stratum_sim::zone_seed(7, zone_spec) == stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 3> {0, 0, -3}));

        stratum_sim::RegionSpec seeded_region_spec = region_spec;
        seeded_region_spec.keys.insert(string(stratum_sim::seed_key), "11");
        stratum_sim::ZoneSpec seeded_zone_spec = zone_spec;
        seeded_zone_spec.keys.insert(string(stratum_sim::seed_key), "12");
        CHECK_CASE("a spec's seed takes precedence", stratum_sim::region_seed(7, seeded_region_spec) == 11
            && stratum_sim::zone_seed(7, seeded_zone_spec) == 12);

        vector<stratum_sim::RandomStream::seed_type> generated_seeds;
        stratum_sim::Generation generation = test_generation();
        generation.m_generators["door"] = [&generated_seeds](const stratum_sim::ZoneSpec& requested_zone_spec)
        {
            generated_seeds.push_back(*requested_zone_spec.keys.value<stratum_sim::RandomStream::seed_type>(stratum_sim::seed_key));
            stratum_format::Document document = stratum_format::read_zone("data/door.zone");
            const stratum_sim::ZonePosition position = requested_zone_spec.position();
            document.position = stratum_format::zone_position_type {position.x, position.y, position.z};
            document.is_unpopulated = false;
            return document;
        };

        auto generate_zone = [&generation, &region_spec](stratum_sim::RandomStream::seed_type world_seed, const stratum_sim::ZoneSpec& requested_zone_spec)
        {
            stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
            stratum_sim::World world(id_allocator, generation, test_zone_count_max);
            stratum_sim::register_game_types(world);
            world.set_world_seed(world_seed);
            world.create_zone(top_level_region(world, world.create_worldspace("door", region_spec)), requested_zone_spec);
            return;
        };

        generate_zone(7, zone_spec);
        generate_zone(7, zone_spec);
        generate_zone(8, zone_spec);
        generate_zone(7, seeded_zone_spec);
        CHECK_CASE("a world's zone has the seed derived from the world seed through its region's",
            generated_seeds.size() == 4 && generated_seeds[0] == stratum_sim::zone_seed(stratum_sim::region_seed(7, region_spec), zone_spec));
        CHECK_CASE("the same for the same world seed, and differing for another", generated_seeds.size() == 4
            && generated_seeds[1] == generated_seeds[0] && generated_seeds[2] != generated_seeds[0]);
        CHECK_CASE("and the seed its spec has, if it has one", generated_seeds.size() == 4 && generated_seeds[3] == 12);
        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "level targets")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        auto up_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, 1); };
        auto down_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, -1); };
        auto retarget = [](flecs::entity e, string_view text)
        {
            e.get_mut<stratum_sim::AccessPoint>().target = *stratum_format::parse_relocator_target(text);
        };

        // data/walls.zone has one staircase leading up and one leading down
        auto load_at_level = [&](stratum_sim::PersistentId region, stratum_sim::level_type level)
        {
            stratum_format::Document document = stratum_format::read_zone("data/walls.zone");
            document.position = document_position_at_level(level);
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        auto zone_arrived_in = [](flecs::entity arrival) { return arrival ? stratum_sim::zone_of(arrival) : flecs::entity(); };

        stratum_sim::PersistentId dungeon = create_test_region(world, "staircase");

        flecs::entity field = load_at_level(world.region_id, 0);
        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);
        flecs::entity first_up = up_of(first);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);

        // Resolving does not bind, so the test case binds an arrival to keep it from being found again
        auto bind = [&](flecs::entity arrival)
        {
            if (arrival)
            {
                arrival.get_mut<stratum_sim::AccessPoint>().target = world.make_reference(first_up);
            }
        };

        CHECK_CASE("moving down finds the unbound staircase that leads back, in the zone of its worldspace at the level below",
            world.resolve_target(first_down) == second_up);

        retarget(first_down, "=-2");
        CHECK_CASE("an absolute target finds a zone at its level", world.resolve_target(first_down) == second_up);
        retarget(first_down, "-1");
        retarget(second_up, "=-1");
        CHECK_CASE("a staircase that leads back with an absolute target is found", world.resolve_target(first_down) == second_up);
        retarget(second_up, "+1");

        retarget(first_up, "staircase/+1");
        flecs::entity arrival = world.resolve_target(first_up);
        flecs::entity made = zone_arrived_in(arrival);
        stratum_sim::PersistentId region_made = made ? stratum_sim::region_of(made) : stratum_sim::PersistentId {};
        CHECK_CASE("a target with a region makes a zone in a new worldspace created from the region spec of that name, and named for it, though a zone at the level has a staircase that leads back", made
            && made != field && region_made != dungeon && region_made != world.region_id
            && world.region(region_made).region_spec.keys.value(stratum_sim::generator_selector_key) == "staircase"
            && world.region(region_made).zone_ids.size() == 1 && level_of(made) == 0
            && world.worldspace(stratum_sim::worldspace_of(made)).name == "staircase_2" && arrival == down_of(made));

        retarget(first_up, "unregistered/+1");
        bool region_spec_unregistered = false;

        try
        {
            world.resolve_target(first_up);
        }
        catch (const runtime_error&)
        {
            region_spec_unregistered = true;
        }

        CHECK_CASE("a region spec that is not registered is an error", region_spec_unregistered);
        retarget(first_up, "+1");

        arrival = world.resolve_target(down_of(second));
        made = zone_arrived_in(arrival);
        CHECK_CASE("finding none, it makes a zone at the level in its own worldspace and arrives at its entry point", made
            && stratum_sim::region_of(made) == dungeon && level_of(made) == -3 && arrival == up_of(made));
        CHECK_CASE("the placeholder places the made zone at x 0 and y 0, at its level",
            made && made.get<stratum_sim::ZonePosition>() == stratum_sim::ZonePosition {0, 0, -3});

        arrival = world.resolve_target(first_up);
        made = zone_arrived_in(arrival);
        CHECK_CASE("moving up looks only in its own worldspace, so a zone at level 0 in another is not found and one is made", made
            && made != field && stratum_sim::region_of(made) == dungeon && level_of(made) == 0
            && arrival == down_of(made));
        bind(arrival);

        auto throws = [&](flecs::entity access_point)
        {
            try
            {
                world.resolve_target(access_point);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        world.unload_zone(second);
        CHECK_CASE("the zone at the target's position is read back if it has unloaded, which a world with no game directory cannot do",
            throws(first_down));

        // A zone made from data/door.zone above a staircase leading up has no staircase leading back down
        size_t worldspace_count = world.worldspaces().size();
        size_t zone_count = world.zone_count();
        retarget(first_up, "door/+1");
        CHECK_CASE("a zone made in a new worldspace with no access point that leads back is an error, and the worldspace is discarded",
            throws(first_up) && world.worldspaces().size() == worldspace_count && world.zone_count() == zone_count);
        retarget(first_up, "+1");

        stratum_sim::PersistentId doors = create_test_region(world, "door");
        flecs::entity below_doors = load_at_level(doors, -1);
        worldspace_count = world.worldspaces().size();
        zone_count = world.zone_count();
        CHECK_CASE("a zone made in its own worldspace with no access point that leads back is an error, and only that zone is discarded",
            throws(up_of(below_doors)) && world.worldspaces().size() == worldspace_count && below_doors.is_alive()
            && world.zone_count() == zone_count && world.worldspace(stratum_sim::worldspace_of(below_doors)).zone_ids.size() == 1
            && world.region(doors).zone_ids.empty());

        stratum_format::Document door_document = stratum_format::read_zone("data/door.zone");
        door_document.position = document_position_at_level(0);
        flecs::entity above_doors = world.create_zone(doors, world.next_id());
        stratum_sim::load_layout(above_doors, door_document);
        CHECK_CASE("a zone at the target's position with no access point that leads back is an error, and is kept",
            throws(up_of(below_doors)) && above_doors.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "access points")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        // Two rooms, a staircase leading up in one and one leading down in the other
        flecs::entity zone = load_zone_unpopulated(world, "data/walls.zone");
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        flecs::entity portal_type = stratum_sim::access_type(world.entities(), "portal");

        CHECK_CASE("the world registers the access point types staircase, door, portal and waypoint", staircase_type && portal_type
            && stratum_sim::access_type(world.entities(), "door") && stratum_sim::access_type(world.entities(), "waypoint")
            && !stratum_sim::access_type(world.entities(), "up") && !stratum_sim::access_type(world.entities(), "chute"));
        CHECK_CASE("each arriving at its own type", staircase_type.get<stratum_sim::Arrival>().access_type == staircase_type
            && portal_type.get<stratum_sim::Arrival>().access_type == portal_type);

        flecs::entity up = stratum_sim::entry_point(zone, staircase_type, 1);
        flecs::entity down = stratum_sim::entry_point(zone, staircase_type, -1);

        CHECK_CASE("the loader builds each access point of the file, of its type", up && down && up != down);
        CHECK_CASE("each sited on the wall the file sites it on", stratum_sim::wall_id_of(up.get<stratum_sim::Site>())
            == layout.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::West)));
        CHECK_CASE("with the order the file states", up.get<stratum_sim::AccessPoint>().order == 0
            && down.get<stratum_sim::AccessPoint>().order == 1);
        CHECK_CASE("an actor arrives in the cell of the access point's wall, facing away from the wall",
            layout.cell_position_at(stratum_sim::arrival_placement(up).position()) == stratum_sim::discrete_point_type {2, 1}
            && stratum_sim::arrival_placement(up).direction().x() > 0.0f);
        CHECK_CASE("an entry point's target is the level at the offset", !stratum_sim::entry_point(zone, staircase_type, -2)
            && stratum_sim::targets_level(up, 1) && !stratum_sim::targets_level(up, -1));

        // Of the access points whose targets are the offset, the lowest-ordered is the entry point
        stratum_format::RelocatorTarget up_target {nullopt, stratum_format::RelativeZone {1}, nullopt};
        flecs::entity up_2 = stratum_sim::create_access_point(zone, staircase_type, up.get<stratum_sim::Site>(), 2, up_target);
        CHECK_CASE("the entry point is the access point of the type with the lowest order", stratum_sim::entry_point(zone, staircase_type, 1) == up
            && up_2.is_alive());

        // An absolute target's offset is the difference between its level and the zone's
        up_2.get_mut<stratum_sim::AccessPoint>().target = stratum_format::RelocatorTarget {nullopt, stratum_format::AbsoluteZone {-1}, nullopt};
        bool unleveled = !stratum_sim::targets_level(up_2, 1);
        zone.set<stratum_sim::ZonePosition>(zone_position_at_level(-2));
        CHECK_CASE("an absolute target's offset is its difference from the zone's level, and a zone with no level has none",
            unleveled && stratum_sim::targets_level(up_2, 1) && !stratum_sim::targets_level(up_2, -1));

        // A portal on the floor, in the middle of a room
        stratum_sim::placement_type north(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(0.0f, 1.0f, 0.0f));
        flecs::entity portal = stratum_sim::create_access_point(zone, portal_type,
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {0, 1}}, north},
            0, stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {0}, nullopt});
        CHECK_CASE("an access point of any type, on any surface, is an entry point", stratum_sim::entry_point(zone, portal_type, 0) == portal);
        CHECK_CASE("an actor arrives on the floor of the access point's flat cell, facing in the site's orientation",
            stratum_sim::arrival_placement(portal).position() == layout.position(stratum_sim::discrete_point_type {0, 1})
            && stratum_sim::arrival_placement(portal).direction().y() > 0.0f);

        // A wall id decodes back to the cell it is on and the direction it faces, for all four
        bool decoded = true;
        for (const auto& direction : stratum_sim::cardinal_directions)
        {
            id_type wall_id = layout.wall_toward(stratum_sim::discrete_point_type {0, 0}, direction);
            if (wall_id != nullid)
            {
                decoded = decoded && layout.wall_cell_position(wall_id) == stratum_sim::discrete_point_type {0, 0}
                    && layout.wall_direction(wall_id) == direction;
            }
        }

        CHECK_CASE("a wall id decodes to its cell and its direction", decoded);

        // Entering places the character in front of up staircase 0, facing away from it
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();
        enter_zone_file(game, "data/corridor.zone");

        const stratum_sim::placement_type& start = stratum_sim::placement_of(session.character().entity());
        CHECK_CASE("the character enters on the floor in front of the staircase leading up",
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(start.position()) == stratum_sim::discrete_point_type {1, 1});
        CHECK_CASE("facing away from it", start.direction().y() < 0.0f && start.direction().x() == 0.0f);

        // A zone with a portal alone is entered at the portal, and not by default
        flecs::entity portal_only = game.world().create_zone(stratum_sim::region_of(session.zone()), make_unique<stratum_sim::DiscreteLayout>(layout.grid().origin, 1.0f,
            layout.grid().extent));
        stratum_sim::DiscreteLayout& portal_only_layout = zone_layout(game.world(), portal_only);
        portal_only_layout.set_cell_open(stratum_sim::discrete_point_type {0, 0}, true);
        portal_only_layout.set_cell_walls(stratum_sim::discrete_point_type {0, 0}, stratum_sim::DirectionMask::West);

        flecs::entity game_portal_type = stratum_sim::access_type(game.world().entities(), "portal");
        stratum_sim::create_access_point(portal_only, game_portal_type,
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {portal_only_layout.floor_id(), stratum_sim::discrete_point_type {0, 0}},
            stratum_sim::placement_type(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f))}, 0,
            stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {0}, nullopt});

        bool thrown = false;

        try
        {
            session.enter(portal_only, test_entry_type(session), test_entry_offset);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        CHECK_CASE("entering a zone with no staircase leading up throws", thrown && session.zone() != portal_only);

        session.enter(portal_only, game_portal_type, 0);
        CHECK_CASE("and entering it at its portal places the character at the portal's placement", session.zone() == portal_only
            && portal_only_layout.cell_position_at(stratum_sim::placement_of(session.character().entity()).position()) == stratum_sim::discrete_point_type {0, 0});

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "relocating through an access point")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        auto up_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, 1); };
        auto down_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, -1); };
        auto target_of = [](flecs::entity e) { return *e.get<stratum_sim::AccessPoint>().target; };
        auto refers_to = [&](flecs::entity e, flecs::entity to)
        {
            const auto* reference = get_if<stratum_sim::EntityReference>(&*e.get<stratum_sim::AccessPoint>().target);
            return reference != nullptr && reference->persistent_id == to.get<stratum_sim::PersistentId>();
        };

        // data/walls.zone has one staircase leading up and one leading down
        auto load_at_level = [&](stratum_sim::PersistentId region, stratum_sim::level_type level)
        {
            stratum_format::Document document = stratum_format::read_zone("data/walls.zone");
            document.position = document_position_at_level(level);
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        stratum_sim::PersistentId dungeon = create_test_region(world, "staircase");

        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);
        stratum_sim::placement_type second_up_placement = stratum_sim::arrival_placement(second_up);

        flecs::entity actor = world.create_entity(first, stratum_sim::arrival_placement(first_down));

        flecs::entity arrival = world.relocate_through(actor, first_down);
        CHECK_CASE("an actor goes through an unbound access point to the access point that leads back", arrival == second_up
            && stratum_sim::zone_of(actor) == second);
        CHECK_CASE("and is placed at its arrival placement", stratum_sim::placement_of(actor).position() == second_up_placement.position()
            && stratum_sim::placement_of(actor).direction() == second_up_placement.direction());
        CHECK_CASE("both ends are bound to each other", refers_to(first_down, second_up) && refers_to(second_up, first_down));
        CHECK_CASE("and a bound access point is no longer an entry point", !up_of(second));

        size_t zone_count = world.zone_count();
        arrival = world.relocate_through(actor, second_up);
        CHECK_CASE("going back through the bound access point arrives at the access point gone through", arrival == first_down
            && stratum_sim::zone_of(actor) == first && world.zone_count() == zone_count);

        arrival = world.relocate_through(actor, first_down);
        flecs::entity second_down = down_of(second);
        arrival = world.relocate_through(actor, second_down);
        flecs::entity made = arrival ? stratum_sim::zone_of(arrival) : flecs::entity();
        CHECK_CASE("finding none, it goes to a zone made at the target level and binds its entry point", made
            && level_of(made) == -3 && stratum_sim::zone_of(actor) == made && refers_to(second_down, arrival)
            && refers_to(arrival, second_down));

        auto throws = [&](flecs::entity access_point)
        {
            try
            {
                world.relocate_through(actor, access_point);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        flecs::entity named = up_of(first);
        named.get_mut<stratum_sim::AccessPoint>().target = *stratum_format::parse_relocator_target("town/market@fountain");
        stratum_format::RelocatorTarget named_target = get<stratum_format::RelocatorTarget>(target_of(named));
        CHECK_CASE("a named zone is not resolved, which is an error, and nothing moves or binds", throws(named)
            && stratum_sim::zone_of(actor) == made && get<stratum_format::RelocatorTarget>(target_of(named)) == named_target);

        world.unload_zone(first);
        CHECK_CASE("a bound target whose zone has unloaded, in a world with no game directory to read it back from, is an error, and nothing moves",
            throws(second_up) && stratum_sim::zone_of(actor) == made);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "one-way access points")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        // A chute leads down to a landing, which is arrival only
        flecs::entity chute_type = stratum_sim::create_access_type(world.entities(), "chute");
        flecs::entity landing_type = stratum_sim::create_access_type(world.entities(), "landing");
        chute_type.set<stratum_sim::Arrival>(stratum_sim::Arrival {landing_type});
        landing_type.set<stratum_sim::Arrival>(stratum_sim::Arrival {landing_type});

        auto load_at_level = [&](stratum_sim::PersistentId region, stratum_sim::level_type level)
        {
            stratum_format::Document document = stratum_format::read_zone("data/walls.zone");
            document.position = document_position_at_level(level);
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        auto on_floor = [&](flecs::entity zone, stratum_sim::discrete_point_type cell_position)
        {
            const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
            return stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {layout.floor_id(), cell_position},
                stratum_sim::placement_type(stratum_sim::point_type(0.0f, 0.0f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f))};
        };

        stratum_sim::PersistentId dungeon = create_test_region(world, "staircase");
        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);

        flecs::entity chute = stratum_sim::create_access_point(first, chute_type, on_floor(first, {2, 1}), 0,
            *stratum_format::parse_relocator_target("-1"));
        flecs::entity landing = stratum_sim::create_access_point(second, landing_type, on_floor(second, {2, 1}), 0);
        stratum_sim::create_access_point(second, landing_type, on_floor(second, {0, 1}), 1);

        flecs::entity actor = world.create_entity(first, stratum_sim::arrival_placement(chute));
        flecs::entity arrival = world.relocate_through(actor, chute);
        const auto* chute_reference = get_if<stratum_sim::EntityReference>(&*chute.get<stratum_sim::AccessPoint>().target);

        CHECK_CASE("an actor goes through an access point to the lowest-ordered arrival-only access point of the arrival type",
            arrival == landing && stratum_sim::zone_of(actor) == second);
        CHECK_CASE("which binds only the access point gone through", chute_reference != nullptr
            && chute_reference->persistent_id == landing.get<stratum_sim::PersistentId>() && !landing.get<stratum_sim::AccessPoint>().target.has_value());
        CHECK_CASE("and cannot be entered", !stratum_sim::can_enter(landing, actor));

        flecs::entity another = world.create_entity(first, stratum_sim::arrival_placement(chute));
        CHECK_CASE("the same arrival-only access point receives every actor that comes through", world.relocate_through(another, chute) == landing
            && stratum_sim::zone_of(another) == second);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "entering an access point")
    {
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();
        stratum_sim::World& world = game.world();
        game.generation() = test_generation();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        flecs::entity character = session.character().entity();
        flecs::entity first = session.zone();
        first.set<stratum_sim::ZonePosition>(zone_position_at_level(-1));

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        flecs::entity first_down = stratum_sim::entry_point(first, staircase_type, -1);

        const stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        const stratum_sim::vector_type west(-1.0f, 0.0f, 0.0f);

        auto step = [&](const stratum_sim::vector_type& direction, stratum_sim::EventQueue& events)
        {
            session.character().set_intent(stratum_sim::Intent {direction, 1.0f, direction});
            session.advance(events);
        };

        auto interact = [&](stratum_sim::EventQueue& events)
        {
            session.character().submit(stratum_sim::Interact {});
            session.advance(events);
        };

        auto relocated = [](const stratum_sim::EventQueue& events)
        {
            return ranges::any_of(events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); });
        };

        stratum_sim::EventQueue moved_events;
        step(east, moved_events);
        CHECK_CASE("moving into a wall with an access point on it is blocked", !relocated(moved_events) && session.zone() == first
            && ranges::any_of(moved_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Blocked>(); }));

        // A closed access point is not entered
        first_down.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Closed});
        stratum_sim::EventQueue closed_events;
        interact(closed_events);
        CHECK_CASE("interacting with a closed access point opens it rather than entering it", !relocated(closed_events)
            && session.zone() == first && stratum_sim::access(first_down) == stratum_sim::AccessState::Open);
        first_down.remove<stratum_sim::Access>();

        stratum_sim::EventQueue down_events;
        interact(down_events);
        flecs::entity second = session.zone();
        CHECK_CASE("interacting with an access point on the wall ahead enters the access point", relocated(down_events) && second != first
            && stratum_sim::zone_of(character) == second && level_of(second) == -2);
        CHECK_CASE("and the session plays the zone arrived in, with the character in its turn order", session.scheduler().contains(character.id()));

        stratum_sim::EventQueue up_events;
        step(west, up_events);
        interact(up_events);
        CHECK_CASE("going back up arrives at the staircase gone down", relocated(up_events) && session.zone() == first
            && zone_layout(world, first).cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1}
            && stratum_sim::placement_of(character).direction().x() < 0.0f);

        // An access point on the floor of the character's cell, arrival only at first, then bound to the staircase leading down
        // in the second zone
        stratum_sim::DiscreteLayout& layout = zone_layout(world, first);
        flecs::entity hatch = stratum_sim::create_access_point(first, stratum_sim::access_type(world.entities(), "portal"),
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {2, 1}}, {}}, 0);

        session.character().submit(stratum_sim::Enter {});
        stratum_sim::EventQueue arrival_only_events;
        session.advance(arrival_only_events);
        CHECK_CASE("an access point with no target is not entered", !relocated(arrival_only_events) && session.zone() == first);

        flecs::entity second_down = stratum_sim::entry_point(second, staircase_type, -1);
        hatch.get_mut<stratum_sim::AccessPoint>().target = world.make_reference(second_down);

        session.character().submit(stratum_sim::Enter {});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        CHECK_CASE("entering the access point on the floor of the character's cell relocates the character", relocated(enter_events)
            && session.zone() == second && zone_layout(world, second).cell_position_at(stratum_sim::placement_of(character).position())
                == zone_layout(world, second).cell_position_at(stratum_sim::arrival_placement(second_down).position()));

        // A denizen facing the staircase leading down, in the zone the character left
        flecs::entity denizen = world.create_entity(first, stratum_sim::arrival_placement(first_down));
        stratum_sim::EventQueue denizen_events;
        stratum_sim::apply(world, stratum_sim::Command(static_cast<id_type>(denizen.id()), stratum_sim::Interact {}), denizen_events);
        CHECK_CASE("an actor other than the controlled actor does not enter an access point", !relocated(denizen_events)
            && stratum_sim::zone_of(denizen) == first);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "interacting with an access point")
    {
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        session.zone().set<stratum_sim::ZonePosition>(zone_position_at_level(-1));
        flecs::entity staircase = stratum_sim::entry_point(session.zone(), stratum_sim::access_type(game.world().entities(), "staircase"), -1);

        game.generation() = test_generation();
        flecs::entity first = session.zone();
        session.character().submit(stratum_sim::Interact {});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        CHECK_CASE("interacting with an access point goes through it", stratum_sim::access_type_name(staircase) == "staircase"
            && session.zone() != first
            && ranges::any_of(enter_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); }));

        // A named zone is not resolved. A game whose advance throws is quit, so this game is used only for this case
        stratum_sim::Game failing(g_test_config, g_test_manifest);
        stratum_sim::Session& failing_session = failing.session();
        enter_zone_file(failing, "data/walls.zone");
        failing_session.zone().set<stratum_sim::ZonePosition>(zone_position_at_level(-1));
        flecs::entity named = stratum_sim::entry_point(failing_session.zone(), stratum_sim::access_type(failing.world().entities(), "staircase"), -1);
        named.get_mut<stratum_sim::AccessPoint>().target = *stratum_format::parse_relocator_target("town/market");

        flecs::entity unmoved = failing_session.zone();
        failing_session.character().submit(stratum_sim::Interact {});
        bool threw = false;

        try
        {
            stratum_sim::EventQueue events;
            failing_session.advance(events);
        }
        catch (const runtime_error&)
        {
            threw = true;
        }

        CHECK_CASE("interacting with an access point whose target is not resolved is an error that passes out of advance, and nothing moves",
            threw && stratum_sim::zone_of(failing_session.character().entity()) == unmoved);

        return;
    }

    TEST_SUITE_END();

} // namespace
