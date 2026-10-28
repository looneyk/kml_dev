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
#include "quetzal/stratum_sim/DisplayName.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/positions.hpp"
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
        const stratum_sim::RegionSpec spec {{"generator_selector", "common"}, {"name", "The Downs"}, {"zone.position.z", "-1"},
            {"zone.parameters.denizens", "few"}, {"zones", "3"}};

        CHECK_CASE("no unknown keys when every key is known", stratum_sim::find_unknown_keys(spec,
            vector<string_view> {"generator_selector", "name", "zone.position.z", "zone.parameters.denizens", "zones", "difficulty"}).empty());
        CHECK_CASE("the unknown keys, in key order", stratum_sim::find_unknown_keys(spec, vector<string_view> {"name"})
            == vector<string> {"generator_selector", "zone.parameters.denizens", "zone.position.z", "zones"});

        const stratum_sim::ZoneSpec zone_spec = stratum_sim::select_prefixed(spec, "zone.");
        CHECK_CASE("the entries under a prefix, with the prefix removed, and not the key zones under zone.",
            zone_spec.value("position.z") == "-1" && zone_spec.value("parameters.denizens") == "few" && !zone_spec.contains("s")
            && zone_spec.size() == 2);
        const KeyValueStore none = stratum_sim::select_prefixed(spec, "region.");
        CHECK_CASE("nothing under a prefix that no key has", none.empty());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "regions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        flecs::entity surface = world.create_region(test_region_spec("staircase"));
        flecs::entity crypt = world.create_region(test_region_spec("staircase"));

        CHECK_CASE("a region has a persistent id", surface.has<stratum_sim::PersistentId>()
            && world.entity(surface.get<stratum_sim::PersistentId>()) == surface);
        CHECK_CASE("a region has no parent", !surface.parent() && !crypt.parent());

        flecs::entity field = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity chapel = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel, stratum_format::read_zone("data/corridor.zone"));

        CHECK_CASE("a zone is its region's child", stratum_sim::region_of(chapel) == crypt && chapel.parent() == crypt
            && stratum_sim::region_of(field) == world.region);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity monk = world.create_entity(chapel,
            stratum_sim::placement_type(zone_layout(world, chapel).position(stratum_sim::discrete_point_type {1, 1}), east));
        stratum_sim::EntityReference monk_reference = world.make_reference(monk);

        CHECK_CASE("the zone of an entity in it is unchanged by the region above it", stratum_sim::zone_of(monk) == chapel
            && !stratum_sim::zone_of(chapel));

        flecs::entity chapel_below = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel_below, stratum_format::read_zone("data/room.zone"));
        stratum_sim::PersistentId chapel_id = chapel.get<stratum_sim::PersistentId>();
        CHECK_CASE_EQUAL("a world counts the zones of every region", world.zone_count(), 3);

        world.unload_region(crypt);

        CHECK_CASE("unloading a region deletes it and its zones", !crypt.is_alive() && !chapel.is_alive() && !chapel_below.is_alive()
            && !monk.is_alive());
        CHECK_CASE("and leaves the other regions and their zones", surface.is_alive() && field.is_alive() && world.zone_count() == 1);
        CHECK_CASE("the world index keeps the zone of an entity something refers to, as an unloaded zone's",
            world.zone_of(monk_reference.persistent_id) == chapel_id && !world.entity(monk_reference.persistent_id));

        // A zone's position, from its file or from the region that makes it
        stratum_format::Document leveled_document = stratum_format::read_zone("data/door.zone");
        leveled_document.position = stratum_format::zone_position_type {2, -5, -3};
        flecs::entity leveled = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(leveled, leveled_document);

        const auto* leveled_position = leveled.try_get<stratum_sim::ZonePosition>();
        CHECK_CASE("a zone has the position its file states", leveled_position != nullptr && leveled_position->x == 2
            && leveled_position->y == -5 && leveled_position->z == -3);
        CHECK_CASE("and none where its file states none", field.try_get<stratum_sim::ZonePosition>() == nullptr);

        stratum_format::Document named_document = stratum_format::read_zone("data/door.zone");
        named_document.components["name"] = "The Narrow Door";
        bool named_buildable = !stratum_sim::check_buildable(world.entities(), named_document).has_value();
        flecs::entity named = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(named, named_document);

        CHECK_CASE("a zone has the name its file states, as its display name", named_buildable
            && named.try_get<stratum_sim::DisplayName>() != nullptr && named.get<stratum_sim::DisplayName>().name == "The Narrow Door");

        flecs::entity dungeon = world.create_region(test_region_spec("door"));
        flecs::entity made = world.create_zone(dungeon, test_zone_spec(-1));

        CHECK_CASE("a region makes a zone at a level with the generator its selector chooses", stratum_sim::region_of(made) == dungeon
            && level_of(made) == -1 && made.has<stratum_sim::ZoneLayout>());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "positions")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        flecs::entity second = world.create_region(test_region_spec("door"));
        flecs::entity third = world.create_region(test_region_spec("door"));
        stratum_sim::RegionSpec placed_spec = test_region_spec("door");
        stratum_sim::set_region_position(placed_spec, stratum_sim::RegionPosition {-4, 7});
        flecs::entity placed = world.create_region(placed_spec);
        flecs::entity fourth = world.create_region(test_region_spec("door"));

        auto region_at = [](flecs::entity region, stratum_sim::coord_type x, stratum_sim::coord_type y)
        {
            const stratum_sim::RegionPosition& position = region.get<stratum_sim::RegionPosition>();
            return position.x == x && position.y == y && stratum_sim::region_position(region.get<stratum_sim::Region>().region_spec).x == x;
        };

        CHECK_CASE("the first region is at the origin, and each new one one past the largest x, at y 0", region_at(world.region, 0, 0)
            && region_at(second, 1, 0) && region_at(third, 2, 0));
        CHECK_CASE("a region whose spec has a position is there, and its spec keeps it", region_at(placed, -4, 7) && region_at(fourth, 3, 0));

        flecs::entity made = world.create_zone(second, test_zone_spec(-3));
        const stratum_sim::ZonePosition& made_position = made.get<stratum_sim::ZonePosition>();
        CHECK_CASE("a zone is at the position its spec has", made_position.x == 0 && made_position.y == 0 && made_position.z == -3);

        const stratum_format::Document made_document = stratum_sim::make_document(made);
        CHECK_CASE("and a saved zone keeps its position", made_document.position == stratum_format::zone_position_type {0, 0, -3});

        CHECK_CASE("a coordinate a spec does not have is 0", stratum_sim::zone_position(stratum_sim::ZoneSpec {
            {string(stratum_sim::position_z_key), "5"}}).x == 0 && stratum_sim::zone_position(stratum_sim::ZoneSpec {}).z == 0);
        CHECK_THROWS_AS(stratum_sim::zone_position(stratum_sim::ZoneSpec {{string(stratum_sim::position_x_key), "east"}}), runtime_error);
        CHECK_CASE("the placeholder places a new zone at x 0 and y 0, at its level", stratum_sim::new_zone_position(world, second, -2).z == -2
            && stratum_sim::new_zone_position(world, second, -2).x == 0);
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
        stratum_sim::set_region_position(region_spec, stratum_sim::RegionPosition {4, -5});
        CHECK_CASE("a region's seed derives from the world seed and the region's position",
            stratum_sim::region_seed(7, region_spec) == stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 2> {4, -5}));
        stratum_sim::ZoneSpec zone_spec = test_zone_spec(-3);
        CHECK_CASE("a zone's seed derives from the region's seed and the zone's position",
            stratum_sim::zone_seed(7, zone_spec) == stratum_sim::derived_seed(7, array<stratum_sim::coord_type, 3> {0, 0, -3}));

        stratum_sim::RegionSpec seeded_region_spec = region_spec;
        seeded_region_spec.insert(string(stratum_sim::seed_key), "11");
        stratum_sim::ZoneSpec seeded_zone_spec = zone_spec;
        seeded_zone_spec.insert(string(stratum_sim::seed_key), "12");
        CHECK_CASE("a spec's seed takes precedence", stratum_sim::region_seed(7, seeded_region_spec) == 11
            && stratum_sim::zone_seed(7, seeded_zone_spec) == 12);

        vector<stratum_sim::RandomStream::seed_type> generated_seeds;
        stratum_sim::Generation generation = test_generation();
        generation.m_generators["door"] = [&generated_seeds](const stratum_sim::ZoneSpec& requested_zone_spec)
        {
            generated_seeds.push_back(*requested_zone_spec.value<stratum_sim::RandomStream::seed_type>(stratum_sim::seed_key));
            stratum_format::Document document = stratum_format::read_zone("data/door.zone");
            const stratum_sim::ZonePosition position = stratum_sim::zone_position(requested_zone_spec);
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
            flecs::entity region = world.create_region(region_spec);
            world.create_zone(region, requested_zone_spec);
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
            e.set<stratum_sim::Relocator>(stratum_sim::Relocator {*stratum_format::parse_relocator_target(text)});
        };

        // data/walls.zone has one staircase leading up and one leading down
        auto load_at_level = [&](flecs::entity region, stratum_sim::level_type level)
        {
            stratum_format::Document document = stratum_format::read_zone("data/walls.zone");
            document.position = document_position_at_level(level);
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        auto zone_arrived_in = [](flecs::entity arrival) { return arrival ? stratum_sim::zone_of(arrival) : flecs::entity(); };

        flecs::entity dungeon = world.create_region(test_region_spec("staircase"));

        flecs::entity field = load_at_level(world.region, 0);
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
                arrival.set<stratum_sim::Relocator>(stratum_sim::Relocator {world.make_reference(first_up)});
            }
        };

        CHECK_CASE("moving down finds the unbound staircase that leads back, in its region's zone at the level below",
            world.resolve_relocator_target(first_down) == second_up);

        retarget(first_down, "=-2");
        CHECK_CASE("an absolute target finds a zone at its level", world.resolve_relocator_target(first_down) == second_up);
        retarget(first_down, "-1");
        retarget(second_up, "=-1");
        CHECK_CASE("a staircase that leads back with an absolute target is found", world.resolve_relocator_target(first_down) == second_up);
        retarget(second_up, "+1");

        retarget(first_up, "staircase/+1");
        flecs::entity arrival = world.resolve_relocator_target(first_up);
        flecs::entity made = zone_arrived_in(arrival);
        flecs::entity region_made = made ? stratum_sim::region_of(made) : flecs::entity();
        CHECK_CASE("a target with a region makes a zone in a new region created from the region spec of that name, though a zone at the level has a staircase that leads back", made
            && made != field && region_made != dungeon && region_made != world.region
            && region_made.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && region_made.get<stratum_sim::Region>().zone_ids.size() == 1 && level_of(made) == 0
            && arrival == down_of(made));

        retarget(first_up, "unregistered/+1");
        bool region_spec_unregistered = false;

        try
        {
            world.resolve_relocator_target(first_up);
        }
        catch (const runtime_error&)
        {
            region_spec_unregistered = true;
        }

        CHECK_CASE("a region spec that is not registered is an error", region_spec_unregistered);
        retarget(first_up, "+1");

        arrival = world.resolve_relocator_target(down_of(second));
        made = zone_arrived_in(arrival);
        CHECK_CASE("finding none, it makes a zone at the level in its own region and arrives at its entry point", made
            && stratum_sim::region_of(made) == dungeon && level_of(made) == -3 && arrival == up_of(made));

        arrival = world.resolve_relocator_target(first_up);
        made = zone_arrived_in(arrival);
        CHECK_CASE("moving up looks only in its own region, so a zone at level 0 in another is not found and one is made", made
            && made != field && stratum_sim::region_of(made) == dungeon && level_of(made) == 0
            && arrival == down_of(made));
        bind(arrival);

        flecs::entity elsewhere = world.create_region(test_region_spec("staircase"));
        load_at_level(elsewhere, -2);
        world.unload_zone(second);
        made = zone_arrived_in(world.resolve_relocator_target(first_down));
        CHECK_CASE("an unloaded zone and a zone in an unrelated region are not searched", made
            && stratum_sim::region_of(made) == dungeon && level_of(made) == -2);

        // A zone made from data/door.zone above a staircase leading up has no staircase leading back down
        auto throws = [&](flecs::entity relocator)
        {
            try
            {
                world.resolve_relocator_target(relocator);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        int region_count = world.entities().count<stratum_sim::Region>();
        size_t zone_count = world.zone_count();
        retarget(first_up, "door/+1");
        CHECK_CASE("a zone made in a new region with no access point that leads back is an error, and the region is discarded",
            throws(first_up) && world.entities().count<stratum_sim::Region>() == region_count && world.zone_count() == zone_count);
        retarget(first_up, "+1");

        flecs::entity doors = world.create_region(test_region_spec("door"));
        flecs::entity below_doors = load_at_level(doors, -1);
        zone_count = world.zone_count();
        CHECK_CASE("a zone made in its own region with no access point that leads back is an error, and only that zone is discarded",
            throws(up_of(below_doors)) && doors.is_alive() && below_doors.is_alive() && world.zone_count() == zone_count
            && doors.get<stratum_sim::Region>().zone_ids.empty());

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
        CHECK_CASE("with the order and the placement the file states", up.get<stratum_sim::AccessPoint>().order == 0
            && down.get<stratum_sim::AccessPoint>().order == 1
            && layout.cell_position_at(up.get<stratum_sim::AccessPoint>().placement.position()) == stratum_sim::discrete_point_type {2, 1}
            && up.get<stratum_sim::AccessPoint>().placement.direction().x() > 0.0f);
        CHECK_CASE("an entry point's relocator targets the level at the offset", !stratum_sim::entry_point(zone, staircase_type, -2)
            && stratum_sim::targets_relocator_level(up, 1) && !stratum_sim::targets_relocator_level(up, -1));

        // Of the access points whose relocators target the offset, the lowest-ordered is the entry point
        stratum_format::RelocatorTarget up_target {nullopt, stratum_format::RelativeZone {1}, nullopt};
        flecs::entity up_2 = stratum_sim::create_access_point(zone, staircase_type, up.get<stratum_sim::Site>(), 2,
            up.get<stratum_sim::AccessPoint>().placement);
        up_2.set<stratum_sim::Relocator>(stratum_sim::Relocator {up_target});
        CHECK_CASE("the entry point is the access point of the type with the lowest order", stratum_sim::entry_point(zone, staircase_type, 1) == up
            && up_2.is_alive());

        // An absolute target's offset is the difference between its level and the zone's
        up_2.set<stratum_sim::Relocator>(stratum_sim::Relocator {stratum_format::RelocatorTarget {nullopt, stratum_format::AbsoluteZone {-1}, nullopt}});
        bool unleveled = !stratum_sim::targets_relocator_level(up_2, 1);
        zone.set<stratum_sim::ZonePosition>(zone_position_at_level(-2));
        CHECK_CASE("an absolute target's offset is its difference from the zone's level, and a zone with no level has none",
            unleveled && stratum_sim::targets_relocator_level(up_2, 1) && !stratum_sim::targets_relocator_level(up_2, -1));

        // A portal on the floor, in the middle of a room
        stratum_sim::placement_type portal_placement(layout.position(stratum_sim::discrete_point_type {0, 1}), stratum_sim::vector_type(0.0f, 1.0f, 0.0f));
        flecs::entity portal = stratum_sim::create_access_point(zone, portal_type,
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {0, 1}}, {}},
            0, portal_placement);
        portal.set<stratum_sim::Relocator>(stratum_sim::Relocator {stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {0}, nullopt}});
        CHECK_CASE("an access point of any type, on any surface, is an entry point", stratum_sim::entry_point(zone, portal_type, 0) == portal);

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
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {portal_only_layout.floor_id(), stratum_sim::discrete_point_type {0, 0}}, {}},
            0, stratum_sim::placement_type(portal_only_layout.position(stratum_sim::discrete_point_type {0, 0}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)))
            .set<stratum_sim::Relocator>(stratum_sim::Relocator {stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {0}, nullopt}});

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
    TEST_CASE_FIXTURE(WorldFixture, "relocating through a relocator")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        auto up_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, 1); };
        auto down_of = [&](flecs::entity zone) { return stratum_sim::entry_point(zone, staircase_type, -1); };
        auto target_of = [](flecs::entity e) { return e.get<stratum_sim::Relocator>().target; };
        auto refers_to = [&](flecs::entity e, flecs::entity to)
        {
            const auto* reference = get_if<stratum_sim::EntityReference>(&e.get<stratum_sim::Relocator>().target);
            return reference != nullptr && reference->persistent_id == to.get<stratum_sim::PersistentId>();
        };

        // data/walls.zone has one staircase leading up and one leading down
        auto load_at_level = [&](flecs::entity region, stratum_sim::level_type level)
        {
            stratum_format::Document document = stratum_format::read_zone("data/walls.zone");
            document.position = document_position_at_level(level);
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        flecs::entity dungeon = world.create_region(test_region_spec("staircase"));

        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);
        stratum_sim::placement_type second_up_placement = second_up.get<stratum_sim::AccessPoint>().placement;

        flecs::entity actor = world.create_entity(first, first_down.get<stratum_sim::AccessPoint>().placement);

        flecs::entity arrival = world.relocate_through(actor, first_down);
        CHECK_CASE("an actor goes through an unbound relocator to the access point that leads back", arrival == second_up
            && stratum_sim::zone_of(actor) == second);
        CHECK_CASE("and is placed at its placement", stratum_sim::placement_of(actor).position() == second_up_placement.position()
            && stratum_sim::placement_of(actor).direction() == second_up_placement.direction());
        CHECK_CASE("both ends are bound to each other", refers_to(first_down, second_up) && refers_to(second_up, first_down));
        CHECK_CASE("and a bound access point is no longer an entry point", !up_of(second));

        size_t zone_count = world.zone_count();
        arrival = world.relocate_through(actor, second_up);
        CHECK_CASE("going back through the bound access point arrives at the relocator", arrival == first_down
            && stratum_sim::zone_of(actor) == first && world.zone_count() == zone_count);

        arrival = world.relocate_through(actor, first_down);
        flecs::entity second_down = down_of(second);
        arrival = world.relocate_through(actor, second_down);
        flecs::entity made = arrival ? stratum_sim::zone_of(arrival) : flecs::entity();
        CHECK_CASE("finding none, it goes to a zone made at the target level and binds its entry point", made
            && level_of(made) == -3 && stratum_sim::zone_of(actor) == made && refers_to(second_down, arrival)
            && refers_to(arrival, second_down));

        auto throws = [&](flecs::entity relocator)
        {
            try
            {
                world.relocate_through(actor, relocator);
            }
            catch (const runtime_error&)
            {
                return true;
            }
            return false;
        };

        flecs::entity named = up_of(first);
        named.set<stratum_sim::Relocator>(stratum_sim::Relocator {*stratum_format::parse_relocator_target("town/market@fountain")});
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
        CHECK_CASE("moving into a wall with a relocator on it is blocked", !relocated(moved_events) && session.zone() == first
            && ranges::any_of(moved_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Blocked>(); }));

        // A closed relocator is not entered
        first_down.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Closed});
        stratum_sim::EventQueue closed_events;
        interact(closed_events);
        CHECK_CASE("interacting with a closed relocator opens it rather than entering it", !relocated(closed_events)
            && session.zone() == first && stratum_sim::access(first_down) == stratum_sim::AccessState::Open);
        first_down.remove<stratum_sim::Access>();

        stratum_sim::EventQueue down_events;
        interact(down_events);
        flecs::entity second = session.zone();
        CHECK_CASE("interacting with a relocator on the wall ahead enters the relocator", relocated(down_events) && second != first
            && stratum_sim::zone_of(character) == second && level_of(second) == -2);
        CHECK_CASE("and the session plays the zone arrived in, with the character in its turn order", session.scheduler().contains(character.id()));

        stratum_sim::EventQueue up_events;
        step(west, up_events);
        interact(up_events);
        CHECK_CASE("going back up arrives at the staircase gone down", relocated(up_events) && session.zone() == first
            && zone_layout(world, first).cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1}
            && stratum_sim::placement_of(character).direction().x() < 0.0f);

        // A relocator on the floor of the character's cell, bound to the staircase leading down in the second zone
        stratum_sim::DiscreteLayout& layout = zone_layout(world, first);
        flecs::entity hatch = world.entities().entity();
        stratum_sim::reparent(hatch, first, stratum_sim::Site {stratum_sim::Relation::On,
            stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {2, 1}}, {}});
        flecs::entity second_down = stratum_sim::entry_point(second, staircase_type, -1);
        hatch.set<stratum_sim::Relocator>(stratum_sim::Relocator {world.make_reference(second_down)});

        session.character().submit(stratum_sim::Enter {});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        CHECK_CASE("entering the relocator on the floor of the character's cell relocates the character", relocated(enter_events)
            && session.zone() == second && zone_layout(world, second).cell_position_at(stratum_sim::placement_of(character).position())
                == zone_layout(world, second).cell_position_at(second_down.get<stratum_sim::AccessPoint>().placement.position()));

        // A denizen facing the staircase leading down, in the zone the character left
        flecs::entity denizen = world.create_entity(first, first_down.get<stratum_sim::AccessPoint>().placement);
        stratum_sim::EventQueue denizen_events;
        stratum_sim::apply(world, stratum_sim::Command(static_cast<id_type>(denizen.id()), stratum_sim::Interact {}), denizen_events);
        CHECK_CASE("an actor other than the controlled actor does not enter a relocator", !relocated(denizen_events)
            && stratum_sim::zone_of(denizen) == first);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "interacting with a relocator")
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
        CHECK_CASE("interacting with a relocator goes through it", stratum_sim::access_type_name(staircase) == "staircase"
            && session.zone() != first
            && ranges::any_of(enter_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); }));

        // A named zone is not resolved. A game whose advance throws is quit, so this game is used only for this case
        stratum_sim::Game failing(g_test_config, g_test_manifest);
        stratum_sim::Session& failing_session = failing.session();
        enter_zone_file(failing, "data/walls.zone");
        failing_session.zone().set<stratum_sim::ZonePosition>(zone_position_at_level(-1));
        flecs::entity named = stratum_sim::entry_point(failing_session.zone(), stratum_sim::access_type(failing.world().entities(), "staircase"), -1);
        named.set<stratum_sim::Relocator>(stratum_sim::Relocator {*stratum_format::parse_relocator_target("town/market")});

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

        CHECK_CASE("interacting with a relocator whose target is not resolved is an error that passes out of advance, and nothing moves",
            threw && stratum_sim::zone_of(failing_session.character().entity()) == unmoved);

        return;
    }

    TEST_SUITE_END();

} // namespace
