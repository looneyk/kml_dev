//------------------------------------------------------------------------------
// stratum_sim_test
// occupancy.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscretePlacementIndex.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/actors.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;

namespace
{

    TEST_SUITE_BEGIN("occupancy");

    //--------------------------------------------------------------------------
    TEST_CASE("placement index")
    {
        using entity_ids_type = stratum_sim::PlacementIndex::entity_ids_type;

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};
        unique_ptr<stratum_sim::PlacementIndex> index = layout.create_placement_index();
        auto* discrete_index = dynamic_cast<stratum_sim::DiscretePlacementIndex*>(index.get());

        CHECK_CASE("a discrete layout creates an empty discrete placement index", discrete_index != nullptr && index->entity_count() == 0);
        if (discrete_index == nullptr)
        {
            return;
        }

        auto at = [&](stratum_sim::coord_type i, stratum_sim::coord_type j)
        {
            return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f));
        };

        index->place(7, at(2, 3));
        index->place(3, at(2, 3));
        index->place(5, at(4, 3));
        CHECK_CASE("the entities in a cell are in ascending id order", discrete_index->entities_at(stratum_sim::discrete_point_type {2, 3})
            == entity_ids_type {3, 7});
        CHECK_CASE("and are found by a placement in the cell", index->entities_at(at(2, 3)) == entity_ids_type {3, 7});
        CHECK_CASE("a cell with nothing in it has no entities", index->entities_at(at(0, 0)).empty());

        index->place(7, at(2, 3));
        CHECK_CASE("placing an entity again in its cell does not add it twice", index->entity_count() == 3
            && index->entities_at(at(2, 3)) == entity_ids_type {3, 7});

        index->place(7, at(4, 3));
        CHECK_CASE("placing it in another cell moves it", index->entities_at(at(2, 3)) == entity_ids_type {3}
            && index->entities_at(at(4, 3)) == entity_ids_type {5, 7});

        index->remove(3);
        CHECK_CASE("a removed entity is in no cell", !index->contains(3) && index->entities_at(at(2, 3)).empty() && index->entity_count() == 2);

        // Enough entities that small ranges visit cells and large ones visit entities
        vector<pair<id_type, stratum_sim::placement_type>> placed {{5, at(4, 3)}, {7, at(4, 3)}};
        for (id_type entity_id = 10; entity_id < 30; ++entity_id)
        {
            stratum_sim::placement_type placement = at(static_cast<stratum_sim::coord_type>(entity_id % 8), static_cast<stratum_sim::coord_type>(entity_id / 4));
            index->place(entity_id, placement);
            placed.push_back({entity_id, placement});
        }

        auto scanned_within = [&](const stratum_sim::placement_type& placement, stratum_sim::value_type range)
        {
            entity_ids_type entity_ids;
            for (const auto& [entity_id, other] : placed)
            {
                stratum_sim::value_type dx = other.position().x() - placement.position().x();
                stratum_sim::value_type dy = other.position().y() - placement.position().y();
                stratum_sim::value_type dz = other.position().z() - placement.position().z();
                if (dx * dx + dy * dy + dz * dz <= range * range)
                {
                    entity_ids.push_back(entity_id);
                }
            }

            sort(entity_ids.begin(), entity_ids.end());
            return entity_ids;
        };

        bool within_as_scanned = true;
        for (stratum_sim::placement_type origin : {at(2, 3), at(0, 0), at(7, 7)})
        {
            for (stratum_sim::value_type range : {0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 12.0f})
            {
                within_as_scanned = within_as_scanned && index->entities_within(origin, range) == scanned_within(origin, range);
            }
        }

        CHECK_CASE("the entities within a range are those a scan finds, over cells or over entities", within_as_scanned);
        CHECK_CASE("a range reaches a cell center at exactly its distance", index->entities_within(at(2, 3), 2.0f) == scanned_within(at(2, 3), 2.0f)
            && !index->entities_within(at(2, 3), 2.0f).empty());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "occupancy")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        flecs::entity a = world.create_entity(zone, place(1, 1));
        flecs::entity b = world.create_entity(zone, place(1, 1));
        flecs::entity c = world.create_entity(zone, place(4, 1));

        CHECK_CASE_EQUAL("two entities share a cell", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 2);
        CHECK_CASE("and are returned in id order", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).front() == a);
        CHECK_CASE_EQUAL("an empty cell contains none", world.entities_at(zone, stratum_sim::discrete_point_type {3, 1}).size(), 0);
        CHECK_CASE_EQUAL("a distant one is elsewhere", world.entities_at(zone, stratum_sim::discrete_point_type {4, 1}).size(), 1);

        CHECK_CASE_EQUAL("range reaches its own cell", world.entities_within(zone, place(1, 1), 0.5f).size(), 2);
        CHECK_CASE_EQUAL("a wider range reaches further", world.entities_within(zone, place(1, 1), 4.0f).size(), 3);
        CHECK_CASE("the far entity is the third", world.entities_within(zone, place(1, 1), 4.0f).back() == c);

        // Occupancy is per zone
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");
        world.create_entity(moor, place(1, 1));

        CHECK_CASE_EQUAL("another zone's entities do not appear", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 2);
        CHECK_CASE_EQUAL("they appear in their own", world.entities_at(moor, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        b.destruct();
        CHECK_CASE_EQUAL("a destroyed entity stops occupying", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "zone placement index")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity hall = load_zone_unpopulated(world, "data/corridor.zone");
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, hall);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        // The index has exactly the entities whose sites have no address, found by a scan of the zone, each at its
        // placement
        auto index_agrees = [&](flecs::entity zone)
        {
            size_t scanned_count = 0;
            bool found = true;

            stratum_sim::with_in_zone(world.entities().query_builder<const stratum_sim::Site>(), zone)
                .build()
                .each([&](flecs::entity e, const stratum_sim::Site& site)
                {
                    if (site.address.has_value())
                    {
                        return;
                    }

                    ++scanned_count;
                    stratum_sim::World::entities_type at = world.entities_at(zone, site.placement);
                    found = found && find(at.begin(), at.end(), e) != at.end();
                });

            return found && scanned_count == world.placement_index(zone).entity_count();
        };

        CHECK_CASE("a zone loaded from a file has an empty index", world.placement_index(hall).entity_count() == 0 && index_agrees(hall));

        flecs::entity rat = world.create_entity(hall, place(1, 1));
        flecs::entity bat = world.create_entity(hall, place(2, 1));
        CHECK_CASE("a created entity is indexed", world.placement_index(hall).entity_count() == 2 && index_agrees(hall));

        stratum_sim::set_placement(rat, place(3, 1));
        CHECK_CASE("a moved entity is indexed where it moved", world.entities_at(hall, stratum_sim::discrete_point_type {3, 1})
            == stratum_sim::World::entities_type {rat} && world.entities_at(hall, stratum_sim::discrete_point_type {1, 1}).empty()
            && index_agrees(hall));

        stratum_sim::EventQueue events;
        stratum_sim::apply(world, stratum_sim::Command(rat.id(), stratum_sim::Step {east, 1.0f}), events);
        CHECK_CASE("and so is one moved by a command", world.entities_at(hall, stratum_sim::discrete_point_type {4, 1})
            == stratum_sim::World::entities_type {rat} && index_agrees(hall));

        flecs::entity torch = world.create_entity(hall, place(2, 1));
        torch.set<stratum_sim::Site>(stratum_sim::Site {stratum_sim::Relation::On,
            stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {2, 1}}, {}});
        CHECK_CASE("an entity sited on an address leaves the index", !world.placement_index(hall).contains(torch.id())
            && world.entities_at(hall, stratum_sim::discrete_point_type {2, 1}) == stratum_sim::World::entities_type {bat} && index_agrees(hall));

        flecs::entity chest = world.create_entity(hall, place(5, 1));
        flecs::entity coin = world.create_entity(hall, place(5, 1));
        stratum_sim::reparent(coin, chest, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        CHECK_CASE("an entity put in another leaves the index", !world.placement_index(hall).contains(coin.id())
            && world.entities_at(hall, stratum_sim::discrete_point_type {5, 1}) == stratum_sim::World::entities_type {chest} && index_agrees(hall));

        world.relocate(chest, moor, place(1, 1));
        CHECK_CASE("a relocated entity leaves its zone's index for the other's", !world.placement_index(hall).contains(chest.id())
            && world.entities_at(moor, stratum_sim::discrete_point_type {1, 1}) == stratum_sim::World::entities_type {chest}
            && !world.placement_index(moor).contains(coin.id()) && index_agrees(hall) && index_agrees(moor));

        flecs::entity wanderer = world.entities().entity();
        world.relocate(wanderer, hall, place(5, 1));
        CHECK_CASE("an entity owned by the game is indexed in the zone it is in", world.entities_at(hall, stratum_sim::discrete_point_type {5, 1})
            == stratum_sim::World::entities_type {wanderer} && index_agrees(hall));

        world.relocate(wanderer, moor, place(5, 1));
        CHECK_CASE("and moves to the index of the zone it is relocated to", !world.placement_index(hall).contains(wanderer.id())
            && world.placement_index(moor).contains(wanderer.id()) && index_agrees(hall) && index_agrees(moor));

        bat.destruct();
        CHECK_CASE("a deleted entity leaves the index", world.placement_index(hall).entity_count() == 1 && index_agrees(hall));

        world.unload_zone(moor);
        CHECK_CASE("unloading a zone leaves the game's entity in no zone", wanderer.is_alive() && !stratum_sim::zone_of(wanderer));

        world.relocate(wanderer, hall, place(5, 1));
        CHECK_CASE("from which it is relocated into another zone's index", world.entities_at(hall, stratum_sim::discrete_point_type {5, 1})
            == stratum_sim::World::entities_type {wanderer} && index_agrees(hall));

        stratum_format::Document saved = stratum_sim::make_document(hall);
        bool saved_without_wanderer = none_of(saved.entities.begin(), saved.entities.end(),
            [&](const stratum_format::Entity& entity) { return entity.id == wanderer.get<stratum_sim::PersistentId>().id; });
        CHECK_CASE("and is left out of the zone's saved document", saved_without_wanderer);

        flecs::entity loaded = world.load_zone(world.region_id, "data/door.zone");
        CHECK_CASE("a populated zone indexes its denizen and not its staircase", world.placement_index(loaded).entity_count() == 1
            && index_agrees(loaded));

        return;
    }

    //--------------------------------------------------------------------------
    // World composes topology and occupancy
    TEST_CASE_FIXTURE(WorldFixture, "world movement")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };
        auto cell_of = [&](flecs::entity e) { return layout.cell_position_at(stratum_sim::placement_of(e).position()); };

        flecs::entity mover = world.create_entity(zone, place(1, 1));
        mover.add<stratum_sim::Blocking>();

        // Nothing in the way, so the world returns what the layout alone would
        stratum_sim::MoveResult clear = world.move(mover, east, 2.0f);
        CHECK_CASE("a move with nothing in the way reaches its distance", layout.cell_position_at(clear.placement.position()) == stratum_sim::discrete_point_type(3, 1));
        CHECK_CASE("and reports no blocker", clear.blocking_wall_id == nullid && clear.blocking_entity_id == nullid);

        // A blocking entity stops it where a wall would
        flecs::entity boulder = world.create_entity(zone, place(3, 1));
        boulder.add<stratum_sim::Blocking>();

        stratum_sim::MoveResult stopped = world.move(mover, east, 4.0f);
        CHECK_CASE("a blocking entity stops the mover short", layout.cell_position_at(stopped.placement.position()) == stratum_sim::discrete_point_type(2, 1));
        CHECK_CASE("and is named as the blocker", stopped.blocking_entity_id == static_cast<id_type>(boulder.id()));
        CHECK_CASE("rather than as a wall", stopped.blocking_wall_id == nullid);

        // Only a Blocking entity blocks
        boulder.remove<stratum_sim::Blocking>();
        stratum_sim::MoveResult past = world.move(mover, east, 4.0f);
        CHECK_CASE("an entity that does not block is moved through", layout.cell_position_at(past.placement.position()).i() > 3);
        CHECK_CASE("and nothing is reported as a blocker", past.blocking_entity_id == nullid);

        boulder.add<stratum_sim::Blocking>();

        // An entity never blocks itself
        stratum_sim::MoveResult alone = world.move(boulder, east, 1.0f);
        CHECK_CASE("a mover is not stopped by itself", layout.cell_position_at(alone.placement.position()) == stratum_sim::discrete_point_type(4, 1));

        // Through a command, so that the event and the charge come with it
        stratum_sim::EventQueue events;
        stratum_sim::set_placement(world.entities().entity(mover.id()), place(1, 1));
        stratum_sim::set_placement(world.entities().entity(boulder.id()), place(3, 1));

        stratum_sim::Command step(mover.id(), stratum_sim::Step {east, 4.0f});
        int cost = stratum_sim::apply(world, step, events);

        CHECK_CASE("the command stopped at the blocker", cell_of(mover) == stratum_sim::discrete_point_type(2, 1));
        CHECK_CASE_EQUAL("charged for the one cell it covered",
            static_cast<size_t>(cost), static_cast<size_t>(stratum_sim::Scheduler::action_cost));
        CHECK_CASE("and reported being blocked", events.events().back().is<stratum_sim::Blocked>());
        CHECK_CASE("naming the entity rather than a wall",
            events.events().back().get<stratum_sim::Blocked>().blocking_entity_id == static_cast<id_type>(boulder.id())
            && events.events().back().get<stratum_sim::Blocked>().blocking_wall_id == nullid);

        // A layout queried directly resolves only its own geometry
        stratum_sim::MoveResult geometric = layout.move(place(1, 1), east, 4.0f);
        CHECK_CASE("the layout alone does not stop for entities", layout.cell_position_at(geometric.placement.position()).i() > 3);

        return;
    }

    TEST_SUITE_END();

} // namespace
