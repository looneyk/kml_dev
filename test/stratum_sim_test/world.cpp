//------------------------------------------------------------------------------
// stratum_sim_test
// world.cpp
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
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/Name.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/actors.hpp"
#include "quetzal/stratum_sim/ai.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("world");

    //--------------------------------------------------------------------------
    // Returns true if two layouts have the same grid and the same open cells and walls
    bool same_geometry(const stratum_sim::DiscreteLayout& a, const stratum_sim::DiscreteLayout& b)
    {
        if (a.grid().extent != b.grid().extent)
        {
            return false;
        }

        for (stratum_sim::coord_type j = 0; j < a.grid().extent.y(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < a.grid().extent.x(); ++i)
            {
                stratum_sim::discrete_point_type cell_position {i, j};
                if (a.open(cell_position) != b.open(cell_position) || a.walls(cell_position) != b.walls(cell_position))
                {
                    return false;
                }
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "world")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);

        CHECK_CASE("a new world has only the empty zones of its worldspace", world.zone_count() == test_extent_zone_count
            && filled_zone_count(world) == 0);

        flecs::entity crypt = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity moor = world.create_zone(world.region_id, make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            1.0f, stratum_sim::discrete_vector_type(4, 4)));

        CHECK_CASE_EQUAL("zones are created", filled_zone_count(world), 2);
        CHECK_CASE("a zone is found by its persistent id", world.entity(crypt.get<stratum_sim::PersistentId>()) == crypt);
        CHECK_CASE("each zone has its own layout", &world.layout(crypt) != &world.layout(moor));

        CHECK_CASE("a zone has the layout it was created with", zone_layout(world, crypt).grid().extent == stratum_sim::discrete_vector_type(6, 5));
        CHECK_CASE("the other stays empty", zone_layout(world, moor).flat_store_count() == 0);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::placement_type where(zone_layout(world, crypt).position(stratum_sim::discrete_point_type {2, 2}), east);

        flecs::entity monster = world.create_entity(crypt, where);
        CHECK_CASE("an entity is placed in its zone", stratum_sim::zone_of(monster) == crypt);
        CHECK_CASE("and has its placement", monster.try_get<stratum_sim::Site>() != nullptr);

        // Migration keeps the id, which is what quests and leashes depend on
        flecs::entity_t id_before = monster.id();
        world.relocate(monster, moor, where);

        CHECK_CASE("relocating moves it to the other zone", stratum_sim::zone_of(monster) == moor);
        CHECK_CASE("its id is unchanged", monster.id() == id_before);
        CHECK_CASE("it is no longer in the old zone", !monster.has(flecs::ChildOf, crypt));

        // Reparenting moves an entity's children with it
        flecs::entity coin = world.entities().entity();
        stratum_sim::reparent(coin, monster, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});

        CHECK_CASE("taking something makes it a child of what holds it", coin.parent() == monster);
        CHECK_CASE("through Parent, where a zone contains its entities through ChildOf", coin.has<flecs::Parent>() && !monster.has<flecs::Parent>());
        CHECK_CASE("and it has a persistent id, being in a zone", coin.has<stratum_sim::PersistentId>());
        CHECK_CASE("and sites it there", coin.get<stratum_sim::Site>().relation == stratum_sim::Relation::In
            && !coin.get<stratum_sim::Site>().address.has_value());

        world.relocate(monster, crypt, where);
        CHECK_CASE("what it carries travels with it", stratum_sim::zone_of(coin) == crypt && coin.parent() == monster);

        stratum_sim::reparent(coin, crypt, stratum_sim::Site {stratum_sim::Relation::In, {}, where});
        CHECK_CASE("putting it down is the same operation at the zone", coin.parent() == crypt
            && stratum_sim::placement_of(coin).position() == where.position());
        CHECK_CASE("and it is in the zone through ChildOf again", !coin.has<flecs::Parent>());

        // An entity with no parent is the game's
        flecs::entity wanderer = world.entities().entity();
        world.relocate(wanderer, moor, where);

        CHECK_CASE("relocating an entity with no parent keeps it the game's, in the zone without being its child",
            !wanderer.parent() && stratum_sim::zone_of(wanderer) == moor && wanderer.has<stratum_sim::PersistentId>());
        CHECK_CASE("and among the zone's entities", world.entities_at(moor, where) == stratum_sim::World::entities_type {wanderer});

        world.relocate(wanderer, crypt, where);
        CHECK_CASE("relocating it again moves it to the other zone, still the game's", !wanderer.parent()
            && stratum_sim::zone_of(wanderer) == crypt && world.entities_at(moor, where).empty());

        flecs::entity pouch = world.entities().entity();
        stratum_sim::reparent(pouch, wanderer, stratum_sim::Site {stratum_sim::Relation::On, {}, {}});
        stratum_sim::EntityReference pouch_reference = world.make_reference(pouch);

        world.relocate(wanderer, moor, where);
        CHECK_CASE("what the game's entity carries moves with it, and its entry in the world index follows",
            stratum_sim::zone_of(pouch) == moor && world.zone_of(pouch_reference.persistent_id) == moor.get<stratum_sim::PersistentId>());

        world.relocate(wanderer, crypt, where);
        world.unload_zone(crypt);

        CHECK_CASE("unloading a zone destroys its contents", !monster.is_alive());
        CHECK_CASE_EQUAL("and drops its layout", filled_zone_count(world), 1);
        CHECK_CASE("the other zone survives", moor.is_alive());
        CHECK_CASE("the game's entity in it, and what it carries, are left out of it and in no zone", wanderer.is_alive()
            && pouch.is_alive() && !stratum_sim::zone_of(wanderer) && !stratum_sim::zone_of(pouch));

        // Zones the world makes whole, with their doors and their denizens
        auto denizens_in = [&](flecs::entity zone)
        {
            stratum_sim::World::entities_type denizens;

            world.entities().query_builder<const stratum_sim::Speed>()
                .with(flecs::ChildOf, zone)
                .build()
                .each([&](flecs::entity e, const stratum_sim::Speed&) { denizens.push_back(e); });

            return denizens;
        };

        auto door_east_of_start = [&](flecs::entity zone)
        {
            return stratum_sim::find_access_attachment(zone, zone_layout(world, zone).wall_toward(stratum_sim::discrete_point_type {1, 1},
                cardinal_direction(CardinalDirection::East)));
        };

        flecs::entity generated = world.load_zone_at(world.create_worldspace("door", {{.region_spec = test_region_spec("door")}}, test_worldspace_seed), stratum_sim::ZonePosition {});
        stratum_sim::World::entities_type generated_denizens = denizens_in(generated);

        CHECK_CASE("a generated zone has its doors", door_east_of_start(generated).is_alive());
        CHECK_CASE("and is populated, its denizen scheduled by speed, blocking, and pursuing no one",
            generated_denizens.size() == 1 && generated_denizens.front().has<stratum_sim::Blocking>()
            && !generated_denizens.front().has<stratum_sim::Pursue>());

        flecs::entity loaded = world.load_zone(world.region_id, "data/door.zone");

        auto door_type_east_of_start = [&](flecs::entity zone)
        {
            return stratum_sim::access_entity(door_east_of_start(zone)).has(flecs::IsA, stratum_sim::entity_type(world.entities(), "door"));
        };

        CHECK_CASE("its door is of the door type, as a loaded one is", door_type_east_of_start(generated));
        CHECK_CASE("a loaded zone agrees with it", same_geometry(zone_layout(world, loaded), zone_layout(world, generated))
            && door_type_east_of_start(loaded) && denizens_in(loaded).size() == 1);
        CHECK_CASE_EQUAL("creating and loading unload nothing", filled_zone_count(world), 3);

        CHECK_CASE("a zone loaded unpopulated is written populated", !stratum_sim::make_document(loaded).is_unpopulated);

        stratum_format::Document prepopulated_document = stratum_format::read_zone("data/door.zone");
        prepopulated_document.is_unpopulated = false;
        filesystem::create_directories("out");
        {
            ofstream prepopulated_file("out/prepopulated.zone", ios::binary);
            stratum_format::write(prepopulated_file, prepopulated_document);
        }

        flecs::entity prepopulated = world.load_zone(create_test_region(world, "door"), "out/prepopulated.zone");
        CHECK_CASE("a zone whose file is not unpopulated is not populated", denizens_in(prepopulated).empty()
            && door_east_of_start(prepopulated).is_alive());
        world.unload_worldspace(stratum_sim::worldspace_of(prepopulated));

        stratum_sim::PersistentId empty_worldspace = world.create_worldspace("door", {{.region_spec = test_region_spec("door")}}, test_worldspace_seed);
        flecs::entity empty = world.entity(world.worldspace(empty_worldspace).zone_ids.at(zone_position_at_level(-2)));
        const stratum_sim::PersistentId empty_id = empty.get<stratum_sim::PersistentId>();
        const string empty_name = empty.get<stratum_sim::Name>().name;
        {
            ofstream empty_file("out/empty.zone", ios::binary);
            stratum_format::write(empty_file, stratum_sim::make_document(empty));
        }

        world.unload_zone(empty);
        flecs::entity read_back = world.load_zone(top_level_region(world, empty_worldspace), "out/empty.zone");
        CHECK_CASE("an empty zone is written as an empty zone's file, and read back as an empty zone with its id, position and name",
            read_back && stratum_sim::empty_zone(read_back) && read_back.get<stratum_sim::PersistentId>() == empty_id
            && level_of(read_back) == -2 && read_back.get<stratum_sim::Name>().name == empty_name);

        flecs::entity generated_empty = world.load_zone_at(empty_worldspace, zone_position_at_level(-2));
        CHECK_CASE("and its content is generated from its zone spec when the zone at its position is loaded, keeping its name",
            generated_empty == read_back && door_east_of_start(generated_empty).is_alive()
            && generated_empty.get<stratum_sim::Name>().name == empty_name);
        world.unload_worldspace(empty_worldspace);

        // Zones created through the registries
        stratum_sim::PersistentId counted_worldspace = world.create_worldspace("staircase", {{.region_spec = test_region_spec("staircase")}}, test_worldspace_seed);
        stratum_sim::PersistentId counted = top_level_region(world, counted_worldspace);
        flecs::entity counted_generated = world.load_zone_at(counted_worldspace, zone_position_at_level(-1));
        flecs::entity counted_loaded = world.load_zone(counted, "data/door.zone");
        const vector<stratum_sim::PersistentId>& counted_zone_ids = world.region(counted).zone_ids;
        CHECK_CASE("a region lists the zones created in it in order, the empty zones of its extent in worldspace position order first",
            counted_zone_ids.size() == test_extent_zone_count + 1 && counted_zone_ids[9] == counted_generated.get<stratum_sim::PersistentId>()
            && counted_zone_ids.back() == counted_loaded.get<stratum_sim::PersistentId>());

        // data/saved.zone is a saved zone, listed when it was created
        flecs::entity saved = world.load_zone(counted, "data/saved.zone");
        world.unload_zone(saved);
        flecs::entity saved_again = world.load_zone(counted, "data/saved.zone");
        bool saved_unpopulated = true;
        world.entities().each([&](flecs::entity e, const stratum_sim::Speed&) { saved_unpopulated = saved_unpopulated && stratum_sim::zone_of(e) != saved_again; });
        CHECK_CASE("a saved zone read back is not listed again, however often it is loaded after unloading", saved_again.is_alive()
            && saved_again.get<stratum_sim::PersistentId>() == stratum_sim::PersistentId {900}
            && world.region(counted).zone_ids.size() == test_extent_zone_count + 1);
        CHECK_CASE("and is not populated", saved_unpopulated);
        world.unload_zone(saved_again);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "persistent ids")
    {
        stratum_sim::CountingIdAllocator counting(stratum_sim::PersistentId {});
        set<stratum_sim::PersistentId> allocated {counting.next(), counting.next()};
        stratum_sim::CountingIdAllocator restored(counting.next());
        allocated.insert(restored.next());
        allocated.insert(restored.next());
        CHECK_CASE_EQUAL("an allocator restored from an id it returned repeats no id", allocated.size(), 4);

        auto persistent_id = [](flecs::entity e) { return e.get<stratum_sim::PersistentId>(); };

        TestWorld world(counting);
        flecs::entity crypt = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::placement_type where(zone_layout(world, crypt).position(stratum_sim::discrete_point_type {2, 2}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f));
        flecs::entity monster = world.create_entity(crypt, where);
        flecs::entity chest = world.create_entity(crypt, where);

        CHECK_CASE("zones and entities have persistent ids of their own",
            set {persistent_id(crypt), persistent_id(moor), persistent_id(monster), persistent_id(chest)}.size() == 4);
        CHECK_CASE("an entity is found by its persistent id",
            world.entity(persistent_id(monster)) == monster && world.entity(persistent_id(chest)) == chest);

        stratum_sim::PersistentId chest_id = persistent_id(chest);
        chest.destruct();
        CHECK_CASE("a destroyed entity is not found", !world.entity(chest_id));

        stratum_sim::EntityReference reference = world.make_reference(monster);
        CHECK_CASE("a reference resolves to its entity", world.resolve(reference) == monster);
        CHECK_CASE("the world index has the zone it is in", world.zone_of(reference.persistent_id) == persistent_id(crypt));

        world.relocate(monster, moor, where);
        CHECK_CASE("relocating it changes its zone in the world index", world.zone_of(reference.persistent_id) == persistent_id(moor));

        flecs::entity coin = world.create_entity(moor, where);
        stratum_sim::EntityReference coin_reference = world.make_reference(coin);
        stratum_sim::reparent(coin, monster, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        world.relocate(monster, crypt, where);
        CHECK_CASE("what it carries changes zone with it", world.zone_of(coin_reference.persistent_id) == persistent_id(crypt));

        stratum_sim::PersistentId crypt_id = persistent_id(crypt);
        world.unload_zone(crypt);

        CHECK_CASE("unloading its zone leaves a reference unresolved",
            !world.resolve(reference) && !world.entity(reference.persistent_id));
        CHECK_CASE("and the world index keeps its zone", world.zone_of(reference.persistent_id) == crypt_id);

        // Loaded again with the id it was saved with, as a saved zone restores it
        flecs::entity crypt_again = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity monster_again = world.entities().entity()
            .child_of(crypt_again)
            .set<stratum_sim::Site>(stratum_sim::Site {stratum_sim::Relation::In, {}, where})
            .set<stratum_sim::PersistentId>(reference.persistent_id);

        CHECK_CASE("a reference re-resolves once its entity is loaded again", world.resolve(reference) == monster_again);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "residency")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator, 2);

        auto corridor = [&]() { return load_zone_unpopulated(world, "data/corridor.zone"); };

        flecs::entity a = corridor();
        flecs::entity b = corridor();
        flecs::entity c = corridor();

        CHECK_CASE_EQUAL("creating zones unloads none, past the maximum or not", filled_zone_count(world), 3);

        world.trim_zones(a);
        CHECK_CASE("past the maximum, the zone entered and those never entered are not unloaded, and empty zones are not counted",
            filled_zone_count(world) == 3 && world.zone_count() == 3 + test_extent_zone_count);

        world.trim_zones(b);
        CHECK_CASE("the zone entered before is", !a.is_alive() && filled_zone_count(world) == 2);

        world.trim_zones(c);
        CHECK_CASE("at the maximum, nothing is", b.is_alive() && filled_zone_count(world) == 2);

        flecs::entity d = corridor();
        world.trim_zones(d);
        CHECK_CASE("the zone entered longest ago is unloaded first", !b.is_alive() && c.is_alive() && d.is_alive());

        // Entering c again makes it the zone entered most recently
        world.trim_zones(c);
        flecs::entity e = corridor();
        world.set_zone_count_max(1);
        world.trim_zones(e);

        CHECK_CASE("as many are unloaded as the maximum requires, never the zone entered",
            !d.is_alive() && !c.is_alive() && e.is_alive() && filled_zone_count(world) == 1);

        flecs::entity f = corridor();
        flecs::entity g = corridor();

        flecs::entity kept = world.create_entity(e, stratum_sim::placement_type(
            zone_layout(world, e).position(stratum_sim::discrete_point_type {1, 1}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        stratum_sim::EntityReference kept_reference = world.make_reference(kept);
        stratum_sim::PersistentId e_id = e.get<stratum_sim::PersistentId>();

        world.trim_zones(f);
        world.trim_zones(g);
        CHECK_CASE("two trims in a row unload the two zones entered before and no more",
            !e.is_alive() && !f.is_alive() && g.is_alive() && filled_zone_count(world) == 1);
        CHECK_CASE("and an entity referred to in one keeps its zone in the world index",
            world.zone_of(kept_reference.persistent_id) == e_id && !world.entity(kept_reference.persistent_id));

        // In readonly mode, as inside a system, a stage queues the deletion, and the unload finishes when the stage is merged
        flecs::entity staged = corridor();
        const std::size_t zone_count = world.zone_count();
        world.entities().readonly_begin();
        world.unload_zone(world.entities().get_stage(0).entity(staged.id()));
        const bool pending = staged.is_alive() && world.zone_count() == zone_count - 1;
        world.entities().readonly_end();
        CHECK_CASE("a zone whose deletion is queued stays until the stage is merged, and is not counted", pending);
        CHECK_CASE("then it is unloaded", !staged.is_alive() && world.zone_count() == zone_count - 1);

        world.set_zone_count_max(0);
        flecs::entity h = corridor();
        world.trim_zones(g);
        world.trim_zones(h);
        CHECK_CASE("with no maximum, nothing is unloaded", g.is_alive() && h.is_alive() && filled_zone_count(world) == 2);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "saved zone documents")
    {
        // Writes the zone as a saved zone, reads it into a second world, and writes it again
        struct RoundTrip
        {
            string written;
            string rewritten;
            bool same_geometry = false;
        };

        auto round_trip = [](flecs::entity zone, auto inspect)
        {
            ostringstream written;
            stratum_format::write(written, stratum_sim::make_document(zone));

            istringstream is(written.str());
            stratum_format::Document read_back = stratum_format::read_zone(is, "a saved zone");

            stratum_sim::CountingIdAllocator other_ids(stratum_sim::PersistentId {100000});
            TestWorld other(other_ids);
            flecs::entity copy = other.create_zone(other.region_id, stratum_sim::PersistentId {*read_back.zone_id});
            stratum_sim::load_layout(copy, read_back);
            inspect(other, copy);

            ostringstream rewritten;
            stratum_format::write(rewritten, stratum_sim::make_document(copy));
            return RoundTrip {written.str(), rewritten.str(), same_geometry(zone_layout(other, copy), zone_layout(other, zone))};
        };

        for (const string& filepath : {"data/catagen.zone", "data/corridor.zone", "data/door.zone", "data/loop.zone", "data/room.zone",
            "data/saved.zone", "data/walls.zone"})
        {
            stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
            TestWorld world(id_allocator);
            RoundTrip trip = round_trip(world.load_zone(world.region_id, filepath), [](TestWorld&, flecs::entity) {});

            CHECK_CASE(format("{} is written as a saved zone, read back with its geometry, and written again identically", filepath),
                trip.same_geometry && !trip.written.empty() && trip.written == trip.rewritten);
        }

        // data/door.zone: a door on the doorway east of cell (1, 1), and a denizen that populating places, given a behavior
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = world.load_zone(world.region_id, "data/door.zone");

        flecs::entity door = stratum_sim::access_entity(stratum_sim::find_access_attachment(zone,
            zone_layout(world, zone).wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East))));
        door.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Open});
        stratum_sim::PersistentId door_id = door.get<stratum_sim::PersistentId>();

        flecs::entity pursuer;
        world.entities().each([&](flecs::entity e, const stratum_sim::Speed&) { pursuer = e; });
        stratum_sim::PersistentId pursuer_id = pursuer.get<stratum_sim::PersistentId>();
        stratum_sim::discrete_point_type pursuer_cell = zone_layout(world, zone).cell_position_at(stratum_sim::placement_of(pursuer).position());
        pursuer.set<stratum_sim::Pursue>(stratum_sim::Pursue {world.make_reference(door), zone_layout(world, zone).position(stratum_sim::discrete_point_type {1, 1}), true});

        bool door_open = false;
        bool pursuer_kept = false;
        bool character_left_out = true;

        round_trip(zone, [&](TestWorld& other, flecs::entity copy)
        {
            flecs::entity door_copy = other.entity(door_id);
            door_open = door_copy && stratum_sim::access(door_copy) == stratum_sim::AccessState::Open;

            flecs::entity pursuer_copy = other.entity(pursuer_id);
            const stratum_sim::Pursue* pursue = pursuer_copy ? pursuer_copy.try_get<stratum_sim::Pursue>() : nullptr;
            pursuer_kept = pursue != nullptr && pursuer_copy.has<stratum_sim::Blocking>() && pursuer_copy.has<stratum_sim::Speed>()
                && pursue->seen && pursue->target.has_value() && pursue->target->persistent_id == door_id
                && zone_layout(other, copy).cell_position_at(stratum_sim::placement_of(pursuer_copy).position()) == pursuer_cell
                && !pursuer_copy.get<stratum_sim::Site>().address.has_value();
        });

        CHECK_CASE("a door's state is written and read back", door_open);
        CHECK_CASE("and a denizen's tags, speed and behavior, its cell, and a reference in its behavior", pursuer_kept);

        // The character, which the game owns, is in the zone but in none of its subtree
        stratum_sim::Game game(g_test_config, g_test_manifest);
        enter_zone_file(game, "data/corridor.zone");
        stratum_sim::PersistentId character_id = game.character().entity().get<stratum_sim::PersistentId>();
        stratum_format::Document with_character = stratum_sim::make_document(game.session().zone());
        character_left_out = std::ranges::none_of(with_character.entities, [&](const stratum_format::Entity& e) { return e.id == character_id.id; });
        CHECK_CASE("the character is not written with the zone it is in", character_left_out);

        return;
    }

    //--------------------------------------------------------------------------
    // Every actor but the controlled one decides for itself
    TEST_CASE_FIXTURE(WorldFixture, "deciding for itself")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type west(-1.0f, 0.0f, 0.0f);
        auto at = [&](int i) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, 1}), east); };
        auto cell_of = [&](flecs::entity e) { return layout.cell_position_at(stratum_sim::placement_of(e).position()); };

        flecs::entity character = world.create_entity(zone, at(1));
        character.add<stratum_sim::Blocking>();

        flecs::entity monster = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {5, 1}), west));
        monster.add<stratum_sim::Blocking>();
        monster.set<stratum_sim::Pursue>(stratum_sim::Pursue {world.make_reference(character), stratum_sim::point_type(), false});

        id_type controlled = static_cast<id_type>(character.id());

        // Down one corridor, so it is seen and its position stored
        stratum_sim::think(world, controlled);

        CHECK_CASE("a pursuer that can see its target stores where", monster.get<stratum_sim::Pursue>().seen);
        CHECK_CASE("and intends to move toward it", monster.get<stratum_sim::Intent>().direction.x() < 0.0f);
        CHECK_CASE("the controlled entity is not decided for", character.try_get<stratum_sim::Intent>() == nullptr);

        // Running it closes the distance
        stratum_sim::Scheduler scheduler;
        scheduler.add(character.id(), 100);
        scheduler.add(monster.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;

        // The controlled entity wins ties and has no Intent, so its turn is spent to let the others act
        auto round = [&]()
        {
            stratum_sim::think(world, controlled);
            id_type up = clock.advance(events, controlled);
            CHECK_CASE_EQUAL("the pass stops at the controlled entity", up, controlled);
            scheduler.spend(up);
            clock.run(events, controlled);
        };

        for (int n = 0; n < 3; ++n)
        {
            round();
        }

        CHECK_CASE("it closed the distance", cell_of(monster) == stratum_sim::discrete_point_type(2, 1));

        // It does not walk into what it is chasing, occupancy blocking it as a wall would
        for (int n = 0; n < 3; ++n)
        {
            round();
        }

        CHECK_CASE("and stops where its target stands", cell_of(monster) == stratum_sim::discrete_point_type(2, 1));
        CHECK_CASE("which is the target's own cell it did not take", cell_of(character) == stratum_sim::discrete_point_type(1, 1));

        // A target gone unseen is sought where it was last seen
        flecs::entity gone = world.create_entity(zone, at(4));
        stratum_sim::EntityReference gone_reference = world.make_reference(gone);
        gone.destruct();

        flecs::entity blind = world.create_entity(zone, at(5));
        blind.set<stratum_sim::Pursue>(stratum_sim::Pursue {gone_reference, layout.position(stratum_sim::discrete_point_type {3, 1}), true});

        stratum_sim::think(world, controlled);
        CHECK_CASE("a pursuer whose target is gone still goes where it last looked",
            blind.get<stratum_sim::Intent>().direction.x() < 0.0f);
        CHECK_CASE("and keeps the position last seen", blind.get<stratum_sim::Pursue>().seen);

        // Arriving there with the target out of sight, it stops
        stratum_sim::set_placement(blind, at(3));
        stratum_sim::think(world, controlled);

        CHECK_CASE("arriving there with nothing to find, it stops going", !blind.get<stratum_sim::Pursue>().seen);

        // With no target, it pursues the controlled actor
        flecs::entity seeker = world.create_entity(zone, at(4));
        seeker.set<stratum_sim::Pursue>(stratum_sim::Pursue {});
        stratum_sim::think(world, controlled);

        const stratum_sim::Pursue& seeking = seeker.get<stratum_sim::Pursue>();
        CHECK_CASE("a pursuer with no target pursues the controlled actor",
            seeking.seen && layout.cell_position_at(seeking.position_last_seen) == cell_of(character));

        // Something with no behavior at all is left alone
        flecs::entity inert = world.create_entity(zone, at(4));
        stratum_sim::think(world, controlled);
        CHECK_CASE("an entity that decides nothing is not decided for", inert.try_get<stratum_sim::Intent>() == nullptr);

        // A zone generated rather than read
        {
            stratum_sim::Game game_created(g_test_config, g_test_manifest);
            stratum_sim::Session& created = game_created.session();
            game_created.generation() = test_generation();
            game_created.character().create(stratum_sim::CharacterSpec {});
            created.enter(game_created.world().load_zone_at(game_created.world().create_worldspace("door", {{.region_spec = test_region_spec("door")}}, test_worldspace_seed),
                stratum_sim::ZonePosition {}), test_entry_type(created), test_entry_offset);

            CHECK_CASE("a constructed zone is loaded", created.loaded());
            CHECK_CASE("with a character in it", created.character().entity().is_alive());

            // A door on the doorway shows the reported walls were built
            CHECK_CASE("its doors are attached to the walls the generator reported",
                stratum_sim::find_access_attachment(created.zone(), stratum_sim::layout_as<stratum_sim::DiscreteLayout>(
                    created.layout()).wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East))).is_alive());

            // Reading the same geometry from a file produces the same zone
            stratum_sim::Game game_loaded(g_test_config, g_test_manifest);
            stratum_sim::Session& loaded = game_loaded.session();
            enter_zone_file(game_loaded, "data/door.zone");
            CHECK_CASE("and a zone read whole agrees with it",
                same_geometry(stratum_sim::layout_as<stratum_sim::DiscreteLayout>(loaded.layout()),
                    stratum_sim::layout_as<stratum_sim::DiscreteLayout>(created.layout())));
        }

        // Through a Session
        stratum_sim::Game game(g_test_config, g_test_manifest);
        stratum_sim::Session& session = game.session();
        enter_zone_file(game, "data/corridor.zone");

        flecs::entity hunter;
        session.world().entities().query_builder<const stratum_sim::Speed>()
            .with(flecs::ChildOf, session.zone())
            .build()
            .each([&](flecs::entity e, const stratum_sim::Speed&) { hunter = e; });

        CHECK_CASE("a session places a denizen", hunter.is_alive());
        hunter.set<stratum_sim::Pursue>(stratum_sim::Pursue {});

        // Off the staircase, into the denizen's sight along the corridor
        game.world().relocate(session.character().entity(), session.zone(), stratum_sim::placement_type(
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).position(stratum_sim::discrete_point_type {1, 1}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::EventQueue session_events;
        stratum_sim::discrete_point_type before = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(stratum_sim::placement_of(hunter).position());

        // The controlled entity waits
        for (int n = 0; n < 3; ++n)
        {
            session.scheduler().spend(session.advance(session_events));
        }

        stratum_sim::discrete_point_type after = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(stratum_sim::placement_of(hunter).position());
        CHECK_CASE("and given a pursuit, it closes on the controlled entity while the host only waits", after.i() < before.i());
        CHECK_CASE("the character is the game's, in the zone without being its child",
            !session.character().entity().parent() && session.character().entity().target<stratum_sim::InZone>() == session.zone());

        flecs::entity session_character = session.character().entity();
        flecs::entity zone_left = session.zone();
        enter_zone_file(game, "data/room.zone");

        CHECK_CASE("loading another zone moves the same character into it", &session.character() == &game.character()
            && session.character().entity() == session_character && stratum_sim::zone_of(session_character) == session.zone());
        CHECK_CASE("and leaves the zone it left loaded", zone_left.is_alive() && filled_zone_count(game.world()) == 2);

        flecs::entity zone_entered = session.zone();
        flecs::entity closed = game.world().create_zone(stratum_sim::region_of(session.zone()), make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            1.0f, stratum_sim::discrete_vector_type(2, 2)));

        bool thrown = false;

        try
        {
            session.enter(closed, test_entry_type(session), test_entry_offset);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        CHECK_CASE("entering a zone with no open cell throws, the character staying where it was",
            thrown && session.zone() == zone_entered && stratum_sim::zone_of(session_character) == zone_entered);

        // The zone never entered is no candidate
        session.world().set_zone_count_max(1);
        enter_zone_file(game, "data/corridor.zone");

        CHECK_CASE("past the maximum, loading another unloads the zones the character entered before",
            !zone_left.is_alive() && !zone_entered.is_alive() && closed.is_alive() && stratum_sim::zone_of(session_character) == session.zone());

        return;
    }

    TEST_SUITE_END();

} // namespace
