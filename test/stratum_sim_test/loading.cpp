//------------------------------------------------------------------------------
// stratum_sim_test
// loading.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscreteProximityIndex.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/make_document.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/PersistentSettings.hpp"
#include "quetzal/stratum_sim/access_points.hpp"
#include "quetzal/stratum_sim/actors.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/file_components.hpp"
#include "quetzal/stratum_sim/layout_entities.hpp"
#include "quetzal/stratum_sim/properties.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/ZoneDocument.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_core/AccessPoint.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <algorithm>
#include <format>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("loading");

    //--------------------------------------------------------------------------
    // Returns the wall an entity is sited on, or none if it is sited on a flat cell or on its parent
    const stratum_format::WallPlacement* wall_placement(const stratum_format::Entity& entity)
    {
        if (!entity.site.has_value() || !entity.site->address.has_value())
        {
            return nullptr;
        }

        return get_if<stratum_format::WallPlacement>(&*entity.site->address);
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "load layout")
    {
        // A ring of eight open cells around one enclosed solid cell
        auto loop_zone = load_open("data/loop.zone");
        const stratum_sim::DiscreteLayout& loop = loop_zone->layout();

        CHECK_CASE("the grid is the file's extent", loop.grid().extent == stratum_sim::discrete_vector2_type(5, 5));
        CHECK_CASE_EQUAL("a layout has one floor and one ceiling", loop.flat_store_count(), 2);

        // Eleven sides of the ring face the solid outside it and four the solid cell it encloses, and the staircase cell above
        // the ring, which is floor, has three facing solid
        CHECK_CASE_EQUAL("a wall on every side floor meets solid", wall_counts(loop).sides, 18);
        CHECK_CASE_EQUAL("and none walled from both sides, solid having no walls of its own", wall_counts(loop).both_sides, 0);

        CHECK_CASE("an open cell reads as open", loop.open(stratum_sim::discrete_point2_type{1, 1}));
        CHECK_CASE("the enclosed solid cell does not", !loop.open(stratum_sim::discrete_point2_type{2, 2}));
        CHECK_CASE("a solid cell outside the ring does not", !loop.open(stratum_sim::discrete_point2_type{0, 0}));

        CHECK_CASE("adjacent open cells are passable", loop.crossable(stratum_sim::discrete_point2_type{1, 1}, cardinal_direction(CardinalDirection::East), loop_zone->crossable));
        CHECK_CASE("an open cell blocks against solid", !loop.crossable(stratum_sim::discrete_point2_type{1, 1}, cardinal_direction(CardinalDirection::South), loop_zone->crossable));
        CHECK_CASE("an open cell blocks against enclosed solid", !loop.crossable(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::North), loop_zone->crossable));

        // The file's first line is the highest row, so the origin is lower left
        CHECK_CASE("row order puts the origin at the lower left", loop.open(stratum_sim::discrete_point2_type{3, 1}));

        // A doorway block between two floor cells
        auto door_zone = load_open("data/door.zone");
        const stratum_sim::DiscreteLayout& door = door_zone->layout();

        CHECK_CASE("a doorway connects both ways", door.crossable(stratum_sim::discrete_point2_type{1, 1}, cardinal_direction(CardinalDirection::East), door_zone->crossable)
            && door.crossable(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::East), door_zone->crossable));

        CHECK_CASE("a block is walled sideways", !door.crossable(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::North), door_zone->crossable)
            && !door.crossable(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::South), door_zone->crossable));
        CHECK_CASE("because a wall is there", door.wall_toward(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::North)) != nullid);

        // Between a block and floor the wall is on the floor's side, so the block's own side of it is bare
        CHECK_CASE("a block sees no wall on the way out", door.wall_toward(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::East)) == nullid);
        CHECK_CASE("but the floor sees one on the way in", door.wall_toward(stratum_sim::discrete_point2_type{3, 1}, cardinal_direction(CardinalDirection::West)) != nullid);

        CHECK_CASE_EQUAL("a zone defining no rooms has none", door.room_count(), 0);

        // A real generated zone, for scale rather than for exact counts
        auto generated_zone = load_open("data/generated.zone");
        const stratum_sim::DiscreteLayout& generated = generated_zone->layout();

        CHECK_CASE("a generated zone reads", wall_counts(generated).sides > 0);
        CHECK_CASE("its grid matches the file", generated.grid().extent == stratum_sim::discrete_vector2_type(31, 31));
        CHECK_CASE_EQUAL("its rooms are the ones the file defines", generated.room_count(), 8);

        size_t room_cells = 0;
        for (stratum_sim::coord_type j = 0; j < 31; ++j)
        {
            for (stratum_sim::coord_type i = 0; i < 31; ++i)
            {
                room_cells += generated.room_at(stratum_sim::discrete_point2_type{i, j}) != nullid ? 1 : 0;
            }
        }

        // 292 of its 961 cells are room floor and 247 corridor, the rest solid, doorways and stairways
        CHECK_CASE_EQUAL("every room cell belongs to a room", room_cells, 292);
        CHECK_CASE("a corridor cell belongs to no room", generated.room_at(stratum_sim::discrete_point2_type{7, 29}) == nullid);
        CHECK_CASE("a room cell refers to the room that contains it", generated.room_at(stratum_sim::discrete_point2_type{1, 1}) != nullid);

        // Two areas never share an edge in the block notation: a block lies between them, and a wall is on the floor's
        // side of it alone. Walling an edge from both sides is what the wall notation can express and this cannot
        CHECK_CASE_EQUAL("no edge of a block zone is walled from both sides", wall_counts(generated).both_sides, 0);

        return;
    }

    //--------------------------------------------------------------------------
    size_t open_cells(const stratum_sim::DiscreteLayout& layout)
    {
        const stratum_sim::discrete_vector2_type extent = layout.grid().extent;
        size_t n = 0;

        for (stratum_sim::coord_type j = 0; j < extent.y(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < extent.x(); ++i)
            {
                n += layout.open(stratum_sim::discrete_point2_type{i, j}) ? 1 : 0;
            }
        }

        return n;
    }

    //--------------------------------------------------------------------------
    // Counts the open cells reachable from the first, crossing only the walls that crossable allows
    size_t cells_reached(const stratum_sim::DiscreteLayout& layout, const stratum_sim::LayoutBase::crossable_function_type& crossable)
    {
        const stratum_sim::discrete_vector2_type extent = layout.grid().extent;
        auto slot = [&extent](const stratum_sim::discrete_point2_type& cell_position) { return static_cast<size_t>(cell_position.y()) * extent.x() + cell_position.x(); };

        vector<bool> seen(static_cast<size_t>(extent.x()) * extent.y(), false);
        vector<stratum_sim::discrete_point2_type> pending;

        for (stratum_sim::coord_type j = 0; j < extent.y() && pending.empty(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < extent.x() && pending.empty(); ++i)
            {
                if (layout.open(stratum_sim::discrete_point2_type{i, j}))
                {
                    pending.push_back(stratum_sim::discrete_point2_type{i, j});
                    seen[slot(pending.front())] = true;
                }
            }
        }

        size_t count = 0;

        while (!pending.empty())
        {
            stratum_sim::discrete_point2_type cell_position = pending.back();
            pending.pop_back();
            ++count;

            const stratum_sim::discrete_vector2_type steps[4] {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (const stratum_sim::discrete_vector2_type& step : steps)
            {
                stratum_sim::discrete_point2_type t = cell_position + step;

                if (!layout.grid().contains(t) || seen[slot(t)] || !layout.open(t) || !layout.crossable(cell_position, step, crossable))
                {
                    continue;
                }

                seen[slot(t)] = true;
                pending.push_back(t);
            }
        }

        return count;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "load wall layout")
    {
        // Two rooms of two by three cells sharing one wall, with a door in the middle of it
        auto walls_zone = load_open("data/walls.zone");
        const stratum_sim::DiscreteLayout& walls = walls_zone->layout();
        const stratum_sim::LayoutBase::crossable_function_type& crossable = walls_zone->crossable;

        size_t door_count = 0;
        walls_zone->world.entities().query_builder<const stratum_sim::Access>()
            .with(flecs::ChildOf, walls_zone->zone)
            .build()
            .each([&](flecs::entity, const stratum_sim::Access&) { ++door_count; });

        CHECK_CASE("the grid is the file's extent", walls.grid().extent == stratum_sim::discrete_vector2_type(4, 3));
        CHECK_CASE_EQUAL("each room is walled on every side it presents", wall_counts(walls).sides, 24);
        CHECK_CASE_EQUAL("a zone defining no rooms has none", walls.room_count(), 0);

        // A wall with a room behind it instead of a solid cell, which the block notation cannot express
        CHECK_CASE_EQUAL("the shared wall is walled from both sides wherever both are floor", wall_counts(walls).both_sides, 4);
        CHECK_CASE_EQUAL("one pair of it is the door", door_count, 1);

        // At this seed the door is in the top row of the shared wall, and the staircases in the middle row of each room
        CHECK_CASE("a mover crosses at the door", walls.crossable(stratum_sim::discrete_point2_type{1, 2}, cardinal_direction(CardinalDirection::East), crossable));
        CHECK_CASE("and back through it", walls.crossable(stratum_sim::discrete_point2_type{2, 2}, cardinal_direction(CardinalDirection::West), crossable));
        CHECK_CASE("but not below it", !walls.crossable(stratum_sim::discrete_point2_type{2, 1}, cardinal_direction(CardinalDirection::West), crossable));
        CHECK_CASE("nor two below", !walls.crossable(stratum_sim::discrete_point2_type{1, 0}, cardinal_direction(CardinalDirection::East), crossable));
        CHECK_CASE("and never within one room", walls.crossable(stratum_sim::discrete_point2_type{0, 0}, cardinal_direction(CardinalDirection::East), crossable));

        CHECK_CASE_EQUAL("every open cell is reachable through the door", cells_reached(walls, crossable), open_cells(walls));

        // A generated zone, for scale and for the properties the generator promises
        auto generated_zone = load_open("data/catagen.zone");
        const stratum_sim::DiscreteLayout& generated = generated_zone->layout();

        CHECK_CASE("a generated zone reads", wall_counts(generated).sides > 0);
        CHECK_CASE("its grid is the file's extent", generated.grid().extent == stratum_sim::discrete_vector2_type(24, 14));

        size_t solid = 0;
        for (int j = 0; j < 14; ++j)
        {
            for (int i = 0; i < 24; ++i)
            {
                solid += generated.open(stratum_sim::discrete_point2_type{i, j}) ? 0 : 1;
            }
        }

        CHECK_CASE("a zone below full density leaves solid cells", solid > 0, format("{} solid cells", solid));

        CHECK_CASE("an edge walled from both sides counts once from each", wall_counts(generated).both_sides % 2 == 0);

        CHECK_CASE_EQUAL("every open cell is reachable from the first", cells_reached(generated, generated_zone->crossable),
            open_cells(generated));

        // At full density with no spacing every cell is room, so every solid cell here is spacing, and only corridors join
        // rooms
        auto spaced_zone = load_open("data/catagen_spaced.zone");
        const stratum_sim::DiscreteLayout& spaced = spaced_zone->layout();

        CHECK_CASE("a spaced zone reads", wall_counts(spaced).sides > 0);
        CHECK_CASE("its grid is the file's extent", spaced.grid().extent == stratum_sim::discrete_vector2_type(30, 18));

        size_t spaced_solid = 0;
        for (int j = 0; j < 18; ++j)
        {
            for (int i = 0; i < 30; ++i)
            {
                spaced_solid += spaced.open(stratum_sim::discrete_point2_type{i, j}) ? 0 : 1;
            }
        }

        CHECK_CASE("spacing leaves solid cells where full density would leave none", spaced_solid > 0,
            format("{} solid cells", spaced_solid));

        CHECK_CASE_EQUAL("every open cell is still reachable from the first", cells_reached(spaced, spaced_zone->crossable),
            open_cells(spaced));

        return;
    }

    //--------------------------------------------------------------------------
    // The door entities that a zone file's doors load as
    TEST_CASE_FIXTURE(WorldFixture, "doors")
    {
        // Two rooms sharing one wall, with a single door in it
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/walls.zone");

        // A door is an entity in the zone with access, and its door facings are its children
        flecs::entity door;
        size_t door_count = 0;
        world.entities().query_builder<const stratum_sim::Access>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](flecs::entity e, const stratum_sim::Access&) { door = e; ++door_count; });

        CHECK_CASE_EQUAL("one door for the one the file names", door_count, 1);

        CHECK_CASE("the door is a child of the zone", door && door.parent() == zone);
        CHECK_CASE("it is closed until something opens it", stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        stratum_sim::World::entities_type facings;
        door.children([&](flecs::entity e) { facings.push_back(e); });

        CHECK_CASE_EQUAL("it has two facings", facings.size(), 2);
        CHECK_CASE("each is sited on a wall", facings.size() == 2
            && stratum_sim::wall_id_of(facings[0].get<stratum_sim::Site>()).has_value()
            && stratum_sim::wall_id_of(facings[1].get<stratum_sim::Site>()).has_value());

        // The two walls are the two sides of one edge, which is what makes them one door
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        id_type wall_a = *stratum_sim::wall_id_of(facings[0].get<stratum_sim::Site>());
        id_type wall_b = *stratum_sim::wall_id_of(facings[1].get<stratum_sim::Site>());

        std::set<id_type> edge_sides{layout.wall_toward(stratum_sim::discrete_point2_type{1, 2}, cardinal_direction(CardinalDirection::East)),
            layout.wall_toward(stratum_sim::discrete_point2_type{2, 2}, cardinal_direction(CardinalDirection::West))};
        CHECK_CASE("on the two sides of one edge", wall_a != wall_b && edge_sides.contains(wall_a) && edge_sides.contains(wall_b));
        std::set<id_type> file_door_walls;
        vector<vector<stratum_format::WallPlacement>> file_doors = stratum_format::door_walls(stratum_format::read_zone("data/walls.zone").cells.value());

        for (const vector<stratum_format::WallPlacement>& placements : file_doors)
        {
            for (const stratum_format::WallPlacement& placement : placements)
            {
                file_door_walls.insert(layout.wall_toward(placement.cell_position, -placement.normal));
            }
        }

        CHECK_CASE("and the walls the file called a door", file_doors.size() == 1 && file_door_walls.contains(wall_a)
            && file_door_walls.contains(wall_b));

        // The queries for the entity on a wall and the entity whose access applies to it
        CHECK_CASE("a wall's way through is the facing on it", stratum_sim::find_access_attachment(zone, wall_a) == facings[0]
            || stratum_sim::find_access_attachment(zone, wall_a) == facings[1]);
        CHECK_CASE("a wall with nothing on it has none",
            !stratum_sim::find_access_attachment(zone, layout.wall_toward(stratum_sim::discrete_point2_type{1, 2},
                cardinal_direction(CardinalDirection::North))));
        CHECK_CASE("a facing has its door's access", stratum_sim::access_entity(facings[0]) == door
            && stratum_sim::access_entity(facings[1]) == door);

        // Cell (1, 2) faces east across the shared wall, which is where the door is
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point2_type{1, 2}), east));
        flecs::entity other = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point2_type{1, 1}), east));

        // Crossing the wall, which depends on the actor
        CHECK_CASE("a closed door is not crossable", !stratum_sim::crossable(zone, wall_a, actor));

        door.set<stratum_sim::Access>(stratum_sim::Access{stratum_sim::AccessState::Open});
        CHECK_CASE("an open one is, from either side", stratum_sim::crossable(zone, wall_a, actor) && stratum_sim::crossable(zone, wall_b, actor));

        facings[0].set<stratum_sim::Detection>(stratum_sim::Detection{stratum_sim::DetectionState::Concealed, {}});
        CHECK_CASE("a concealed side is a wall", !stratum_sim::crossable(zone, wall_a, actor) && !stratum_sim::crossable(zone, wall_a, other));
        CHECK_CASE("and the other side of it is not concealed", stratum_sim::crossable(zone, wall_b, actor));

        facings[0].get_mut<stratum_sim::Detection>().actor_states.push_back(
            stratum_sim::ActorDetection{actor.get<stratum_core::PersistentId>(), stratum_sim::DetectionState::Revealed});
        CHECK_CASE("an actor who has found it crosses", stratum_sim::crossable(zone, wall_a, actor));
        CHECK_CASE("and one who has not does not", !stratum_sim::crossable(zone, wall_a, other));

        facings[0].remove<stratum_sim::Detection>();
        door.set<stratum_sim::Access>(stratum_sim::Access{stratum_sim::AccessState::Locked});
        CHECK_CASE("a locked door is not crossable", !stratum_sim::crossable(zone, wall_a, actor));

        // Interacting with the door an actor faces opens and shuts it
        stratum_sim::EventQueue door_events;
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact{}), door_events);
        CHECK_CASE("interacting leaves a locked door locked", stratum_sim::access(door) == stratum_sim::AccessState::Locked);

        door.set<stratum_sim::Access>(stratum_sim::Access{stratum_sim::AccessState::Closed});
        door_events.clear();
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact{}), door_events);

        CHECK_CASE("interacting with the door opens it", stratum_sim::access(door) == stratum_sim::AccessState::Open);

        door_events.clear();
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact{}), door_events);

        CHECK_CASE("and again shuts it", stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        actor.destruct();
        other.destruct();

        // Deleting the door deletes its door facings
        door.destruct();
        CHECK_CASE("deleting a door takes its facings", !facings[0].is_alive() && !facings[1].is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    // One door entity per door glyph, its door facings on the walls of that doorway, and no door facing anywhere else
    bool doors_have_entities(const stratum_format::ZoneDocument& document, string& detail)
    {
        vector<vector<stratum_format::WallPlacement>> doors = stratum_format::door_walls(document.cells.value());
        map<stratum_format::stored_entity_id_type, vector<stratum_format::WallPlacement>> walls_by_door;
        size_t door_count = 0;

        for (const stratum_format::Entity& entity : document.entities)
        {
            if (entity.type == "door")
            {
                ++door_count;
                continue;
            }

            if (entity.type != "door_facing")
            {
                continue;
            }

            const auto* placement = wall_placement(entity);
            if (placement == nullptr || !entity.parent_id.has_value())
            {
                detail = format("a door facing with id {} that is not a child attached to a wall", entity.id);
                return false;
            }

            walls_by_door[*entity.parent_id].push_back(*placement);
        }

        if (door_count != doors.size() || walls_by_door.size() != doors.size())
        {
            detail = format("{} door glyphs, {} door entities and {} of them with door facings", doors.size(), door_count, walls_by_door.size());
            return false;
        }

        for (const vector<stratum_format::WallPlacement>& walls : doors)
        {
            if (none_of(walls_by_door.begin(), walls_by_door.end(), [&walls](const auto& entry)
                { return is_permutation(entry.second.begin(), entry.second.end(), walls.begin(), walls.end()); }))
            {
                const stratum_format::WallPlacement& wall = walls.front();
                detail = format("no door with a door facing on each wall of the doorway at [{}, {}] normal [{}, {}]",
                    wall.cell_position.x(), wall.cell_position.y(), wall.normal.x(), wall.normal.y());
                return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "door entities")
    {
        for (const string filepath : {"data/catagen.zone", "data/catagen_spaced.zone", "data/corridor.zone", "data/door.zone", "data/generated.zone",
            "data/loop.zone", "data/room.zone", "data/walls.zone"})
        {
            string detail;
            CHECK_CASE(format("{} has a door entity at each door", filepath), doors_have_entities(stratum_format::read_zone(filepath), detail), detail);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A component whose fields inherit, with a struct member whose fields inherit in turn
    struct TestTint
    {
        optional<float> red;
        optional<float> blue;
    };

    struct TestFinish
    {
        optional<int> grain;
        optional<float> gloss;
        TestTint tint;
    };

    //--------------------------------------------------------------------------
    // A component referring to another entity
    struct TestLever
    {
        stratum_sim::EntityReference opens;
    };

    //--------------------------------------------------------------------------
    // Registers each as the component that the file component with its name decodes into
    void register_test_components(flecs::world& entities)
    {
        stratum_sim::register_optional<int>(entities);
        stratum_sim::register_optional<float>(entities);
        entities.component<TestTint>()
            .member(&TestTint::red, "red")
            .member(&TestTint::blue, "blue");
        entities.component<TestFinish>()
            .member(&TestFinish::grain, "grain")
            .member(&TestFinish::gloss, "gloss")
            .member(&TestFinish::tint, "tint");
        entities.component<TestLever>()
            .member(&TestLever::opens, "opens");

        stratum_sim::register_file_component<TestFinish>(entities, "finish");
        stratum_sim::register_file_component<TestLever>(entities, "lever");
        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    TEST_CASE_FIXTURE(WorldFixture, "loaded entities")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);
        const flecs::world& entities = world.entities();

        // data/door.zone: a door with a door facing on each wall of its doorway, and a staircase up on the wall above
        auto loaded = [&](const stratum_format::ZoneDocument& document)
        {
            flecs::entity zone = world.create_zone(world.region_id,
                document.zone_id.has_value() ? stratum_core::PersistentId{*document.zone_id} : world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        auto zone_children = [](flecs::entity parent)
        {
            vector<flecs::entity> children;
            parent.children([&](flecs::entity child)
            {
                if (!child.has<stratum_sim::LayoutEntity>())
                {
                    children.push_back(child);
                }
            });

            return children;
        };

        flecs::entity zone = loaded(stratum_format::read_zone("data/door.zone"));
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        vector<flecs::entity> built = zone_children(zone);

        CHECK_CASE_EQUAL("the door and the staircase are the zone's own children", built.size(), 2);
        CHECK_CASE("each has a persistent id", ranges::all_of(built, [](flecs::entity e) { return e.has<stratum_core::PersistentId>(); }));

        flecs::entity door = built.size() == 2 ? built[0] : flecs::entity();
        flecs::entity staircase = built.size() == 2 ? built[1] : flecs::entity();

        CHECK_CASE("the door is its type's", door && door.target(flecs::IsA) == stratum_sim::entity_type(entities, "door"));
        CHECK_CASE("with its own access, closed", door && door.owns<stratum_sim::Access>() && stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        vector<flecs::entity> facings = door ? zone_children(door) : vector<flecs::entity>{};
        id_type west_wall = layout.wall_toward(stratum_sim::discrete_point2_type{3, 1}, cardinal_direction(CardinalDirection::West));
        flecs::entity facing_west = stratum_sim::wall_entity(zone, west_wall);

        CHECK_CASE_EQUAL("its door facings are its children", facings.size(), 2);
        CHECK_CASE("through Parent, each with its persistent id and its door's access", ranges::all_of(facings, [&](flecs::entity e)
            { return e.has<flecs::Parent>() && e.has<stratum_core::PersistentId>() && stratum_sim::access_entity(e) == door; }));
        CHECK_CASE("each sited on its wall, which lists it", facing_west && facing_west.try_get<stratum_sim::AttachedEntities>() != nullptr
            && facing_west.get<stratum_sim::AttachedEntities>().entities == vector<flecs::entity>{facings.empty() ? flecs::entity() : facings[0]});

        flecs::entity facing_type = stratum_sim::entity_type(world.entities(), "door_facing");
        CHECK_CASE("an entity whose type is not loaded is an instance of a type of that name, made without the components a type sets",
            !facings.empty() && facing_type && facings[0].has(flecs::IsA, facing_type) && !facing_type.has<stratum_sim::Access>());

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        CHECK_CASE("the staircase is an instance of its registered type", staircase && staircase_type
            && staircase.has(flecs::IsA, staircase_type));
        CHECK_CASE("it is the access point, as the file lists it", staircase && staircase.has<stratum_core::AccessPoint>()
            && stratum_sim::entry_point(zone, staircase_type, 1) == staircase);
        CHECK_CASE("with its destination", staircase && staircase.get<stratum_core::AccessPoint>().destination.has_value()
            && *staircase.get<stratum_core::AccessPoint>().destination == stratum_core::AccessPoint::destination_type{stratum_core::ZoneExtendedName{"../[+1]"}});

        // The same zone saved: every id in the file is persistent
        string saved_text = R"({ "format": "quetzal.discrete_layout", "version": "0.6", "zone": { "id": 900 },
            "layout": { "components": { "access_points": [ { "entity": 904 } ] } },
            "cells": { "notation": "block", "extent": [5, 3], "origin": [0.0, 0.0, 0.0], "scale": 1.0, "rows": [ "#<###", "#.+.#", "#####" ] },
            "entities": [
              { "id": 901, "type": "door" },
              { "id": 902, "type": "door_facing", "components": { "parent": { "entity": 901 }, "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [1, 0] } } } },
              { "id": 903, "type": "door_facing", "components": { "parent": { "entity": 901 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
              { "id": 904, "type": "staircase", "components": { "site": { "relation": "on", "flat": "floor", "cell_position": [1, 2], "orientation": [0, -1] },
                "access_point": { "destination": { "entity": 7713 }, "order": 0 } } } ] })";
        istringstream saved_stream(saved_text);
        flecs::entity saved = loaded(stratum_format::read_zone(saved_stream, "saved"));

        CHECK_CASE("a saved zone keeps its persistent id", saved.get<stratum_core::PersistentId>() == stratum_core::PersistentId{900});
        CHECK_CASE("and each of its entities its own", world.entity(stratum_core::PersistentId{901}).parent() == saved
            && world.entity(stratum_core::PersistentId{902}).parent() == world.entity(stratum_core::PersistentId{901}));

        flecs::entity bound = world.entity(stratum_core::PersistentId{904});
        const auto* destination = bound ? get_if<stratum_core::PersistentId>(&*bound.get<stratum_core::AccessPoint>().destination) : nullptr;
        CHECK_CASE("a bound destination refers to an access point in a zone not loaded", destination != nullptr && *destination == stratum_core::PersistentId{7713}
            && !world.entity(stratum_core::PersistentId{7713}));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "decoded components")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);
        const flecs::world& entities = world.entities();
        register_test_components(world.entities());

        // Reads data/door.zone with the given components for the layout, the door, and the staircase, and the given room,
        // wall, and flat sections
        auto door_level = [](const string& layout_components, const string& door_components, const string& staircase_components,
            const string& layout_sections = "")
        {
            string text = format(R"({{ "format": "quetzal.discrete_layout", "version": "0.6",
                "layout": {{ "components": {{ "access_points": [ {{ "entity": 4 }} ]{} }} }},
                "cells": {{ "notation": "block", "extent": [5, 3], "origin": [0.0, 0.0, 0.0], "scale": 1.0, "rows": [ "#<###", "#.+.#", "#####" ] }},
                {}
                "entities": [
                  {{ "id": 1, "type": "door"{} }},
                  {{ "id": 2, "type": "door_facing", "components": {{ "parent": {{ "entity": 1 }}, "site": {{ "relation": "on", "wall": {{ "cell_position": [3, 1], "normal": [1, 0] }} }} }} }},
                  {{ "id": 3, "type": "door_facing", "components": {{ "parent": {{ "entity": 1 }}, "site": {{ "relation": "on", "wall": {{ "cell_position": [1, 1], "normal": [-1, 0] }} }} }} }},
                  {{ "id": 4, "type": "staircase", "components": {{ "site": {{ "relation": "on", "flat": "floor", "cell_position": [1, 2], "orientation": [0, -1] }},
                    "access_point": {{ "type": "staircase", "destination": "../[+1]", "order": 0 }}{} }} }} ] }})",
                layout_components.empty() ? "" : ", " + layout_components,
                layout_sections.empty() ? "" : layout_sections + ",",
                door_components.empty() ? "" : R"(, "components": { )" + door_components + " }",
                staircase_components.empty() ? "" : ", " + staircase_components);

            istringstream stream(text);
            return stratum_format::read_zone(stream, "door level");
        };

        stratum_sim::entity_type(entities, "door").set<TestFinish>(TestFinish{{}, 0.875f, {}});

        stratum_format::ZoneDocument document = door_level(R"("finish": { "gloss": 0.25 })", R"("finish": { "grain": 3, "tint": { "red": 0.5 } })",
            R"("lever": { "opens": { "entity": 1 } })");
        CHECK_CASE("a document whose components are registered and fit is accepted", !stratum_sim::check_buildable(entities, document).has_value());

        flecs::entity zone = world.create_zone(world.region_id, world.next_id());
        vector<flecs::entity> referred_to = stratum_sim::load_layout(zone, document);

        flecs::entity door = stratum_sim::access_entity(stratum_sim::find_access_attachment(zone,
            zone_layout(world, zone).wall_toward(stratum_sim::discrete_point2_type{3, 1}, cardinal_direction(CardinalDirection::West))));
        flecs::entity staircase = stratum_sim::entry_point(zone, stratum_sim::entity_type(entities, "staircase"), 1);

        const auto* finish = door.try_get<TestFinish>();
        CHECK_CASE("a component object is decoded into the component registered under its name", finish != nullptr
            && finish->grain == 3 && finish->tint.red == 0.5f);
        CHECK_CASE("a field it does not state is left unset, where the component inherits", finish != nullptr && !finish->gloss.has_value()
            && !finish->tint.blue.has_value());
        CHECK_CASE("and comes from the entity's type along the chain", world.property<TestFinish>(door).has_value()
            && world.property<TestFinish>(door)->gloss == 0.875f);
        CHECK_CASE("the layout's components are the zone's", zone.try_get<TestFinish>() != nullptr && zone.get<TestFinish>().gloss == 0.25f);

        const auto* lever = staircase.try_get<TestLever>();
        CHECK_CASE("a reference is translated to the entity's persistent id", lever != nullptr && lever->opens.entity == door
            && lever->opens.persistent_id == door.get<stratum_core::PersistentId>());
        CHECK_CASE("and what it refers to is returned for the world index", referred_to == vector<flecs::entity>{door});

        auto rejected = [&](const string& layout_components, const string& door_components, const string& expected)
        {
            optional<string> error = stratum_sim::check_buildable(entities, door_level(layout_components, door_components, ""));
            return error.has_value() && *error == expected;
        };

        CHECK_CASE("a name nothing registers is rejected", rejected("", R"("patina": { "age": 3 })",
            "entities[0].components.patina: no component is registered under the name"));
        CHECK_CASE("a field the component does not have is rejected", rejected("", R"("finish": { "shine": 1 })",
            "entities[0].components.finish.shine: not a field of the component"));
        CHECK_CASE("a value its field cannot take is rejected", rejected(R"("finish": { "grain": "oak" })", "",
            "layout.components.finish.grain: a value its field cannot take"));
        CHECK_CASE("a reference where none is expected is rejected", rejected("", R"("lever": { "opens": 1 })",
            "entities[0].components.lever.opens: not a reference to an entity"));

        // Room 0 is west of the door and room 1 east of it. Each room, wall, and flat section states part of a finish
        string layout_sections = R"("rooms": [
              { "cell_position": [1, 1], "components": { "finish": { "gloss": 0.375 } } },
              { "cell_position": [3, 1], "components": { "finish": { "grain": 4, "gloss": 0.125, "tint": { "red": 0.875 } } } } ],
            "walls": [ { "wall": { "cell_position": [1, 1], "normal": [1, 0] }, "components": { "finish": { "grain": 2 } } } ],
            "flats": [ { "role": "floor", "components": { "finish": { "gloss": 0.5, "tint": { "blue": 0.75 } } },
              "rooms": [ { "room": 1, "components": { "finish": { "tint": { "red": 0.25 } } } } ],
              "cells": [ { "cell_position": [3, 1], "components": { "finish": { "grain": 9 } } } ] } ])";

        stratum_format::ZoneDocument sectioned = door_level(R"("finish": { "gloss": 0.0625, "tint": { "blue": 0.125 } })", "", "",
            layout_sections);
        CHECK_CASE("the components of rooms, walls, and flats that are registered and fit are accepted",
            !stratum_sim::check_buildable(entities, sectioned).has_value());

        flecs::entity sectioned_zone = world.create_zone(world.region_id, world.next_id());
        stratum_sim::load_layout(sectioned_zone, sectioned);
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, sectioned_zone);

        flecs::entity east_room = stratum_sim::room_entity(sectioned_zone, 1);
        flecs::entity floor = stratum_sim::flat_entity(sectioned_zone, layout.floor_id());
        flecs::entity floor_east = stratum_sim::flat_room_entity(sectioned_zone, layout.floor_id(), 1);
        flecs::entity floor_cell = stratum_sim::flat_cell_entity(sectioned_zone, layout.floor_id(), stratum_sim::discrete_point2_type{3, 1});
        flecs::entity west_wall = stratum_sim::wall_entity(sectioned_zone,
            layout.wall_toward(stratum_sim::discrete_point2_type{1, 1}, cardinal_direction(CardinalDirection::West)));

        CHECK_CASE("every room and each flat has an entity", stratum_sim::room_entity(sectioned_zone, 0) && east_room && floor
            && stratum_sim::flat_entity(sectioned_zone, layout.ceiling_id()));
        CHECK_CASE("a room's components are its entity's", east_room && east_room.get<TestFinish>().grain == 4);
        CHECK_CASE("a flat's override for a room or a cell is below the flat's entity", floor_east && floor_cell
            && floor_east.parent() == floor && floor_cell.parent() == floor);

        optional<TestFinish> cell_finish = world.property<TestFinish>(floor_cell);
        CHECK_CASE("a flat cell takes what it leaves unset from the flat's override for its room first",
            cell_finish.has_value() && cell_finish->grain == 9 && cell_finish->tint.red == 0.25f);
        CHECK_CASE("then from the room", cell_finish.has_value() && cell_finish->gloss == 0.125f);
        CHECK_CASE("then from the flat, before the layout", cell_finish.has_value() && cell_finish->tint.blue == 0.75f);

        optional<TestFinish> wall_finish = west_wall ? world.property<TestFinish>(west_wall) : nullopt;
        CHECK_CASE("a wall takes what it leaves unset from its room, then the layout", wall_finish.has_value() && wall_finish->grain == 2
            && wall_finish->gloss == 0.375f && wall_finish->tint.blue == 0.125f);

        optional<string> room_error = stratum_sim::check_buildable(entities, door_level("", "", "",
            R"("rooms": [ { "cell_position": [1, 1], "components": { "patina": { "age": 3 } } } ])"));
        CHECK_CASE("a room's component that nothing registers is rejected", room_error.has_value()
            && *room_error == "rooms[0].components.patina: no component is registered under the name");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "properties")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);
        flecs::world& entities = world.entities();
        register_test_components(entities);

        // Room 0 is the west half, room 1 the east half, and cell [3, 2] in no room
        auto layout = make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f,
            stratum_sim::discrete_vector2_type(4, 3));
        layout->set_room_count(2);

        for (stratum_sim::coord_type j = 0; j < 3; ++j)
        {
            for (stratum_sim::coord_type i = 0; i < 4; ++i)
            {
                stratum_sim::discrete_point2_type cell_position{i, j};
                layout->set_cell_open(cell_position, true);

                if (cell_position != stratum_sim::discrete_point2_type{3, 2})
                {
                    layout->set_cell_room(cell_position, i < 2 ? 0 : 1);
                }
            }
        }

        layout->set_cell_walls(stratum_sim::discrete_point2_type {0, 0}, stratum_sim::direction_mask(CardinalDirection::North));
        id_type north_wall = layout->wall_toward(stratum_sim::discrete_point2_type {0, 0}, cardinal_direction(CardinalDirection::North));

        flecs::entity zone = world.create_zone(world.region_id, std::move(layout));
        auto placed_at = [&](stratum_sim::discrete_point2_type cell_position)
        {
            return world.create_entity(zone, stratum_sim::placement_type(zone_layout(world, zone).position(cell_position),
                stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        };

        flecs::entity chest = stratum_sim::create_entity_type(entities, "chest");
        CHECK_CASE("a type is found by its name", stratum_sim::entity_type(entities, "chest") == chest && chest.has(flecs::Prefab));
        CHECK_CASE("a type not loaded is null", !stratum_sim::entity_type(entities, "portcullis"));

        flecs::entity west = stratum_sim::ensure_room_entity(zone, 0);
        CHECK_CASE("a room's entity is made once", stratum_sim::ensure_room_entity(zone, 0) == west && stratum_sim::room_entity(zone, 0) == west);
        CHECK_CASE("a room with none has a null entity", !stratum_sim::room_entity(zone, 1));
        CHECK_CASE("a room's entity has no persistent id", !west.has<stratum_core::PersistentId>());

        flecs::entity plain = placed_at(stratum_sim::discrete_point2_type{0, 1});
        CHECK_CASE("nothing along the chain is empty", !world.property<TestFinish>(plain).has_value());

        chest.set<TestFinish>(TestFinish{7, {}, {0.5f, {}}});
        west.set<TestFinish>(TestFinish{1, 0.25f, {0.0f, {}}});
        zone.set<TestFinish>(TestFinish{2, 0.75f, {0.0f, 0.125f}});

        flecs::entity oak = placed_at(stratum_sim::discrete_point2_type{1, 1}).is_a(chest);
        CHECK_CASE("an entity of a type has no copy of a component that inherits", !oak.owns<TestFinish>());

        chest.get_mut<TestFinish>().grain = 8;
        chest.modified<TestFinish>();
        optional<TestFinish> finish = world.property<TestFinish>(oak);

        CHECK_CASE("a field the type sets is the entity's, as the type is now", finish.has_value() && finish->grain == 8
            && finish->tint.red == 0.5f);
        CHECK_CASE("a field the type leaves unset comes from the room", finish.has_value() && finish->gloss == 0.25f);
        CHECK_CASE("and one the room leaves unset from the layout", finish.has_value() && finish->tint.blue == 0.125f);

        oak.set<TestFinish>(TestFinish{{}, 0.5f, {{}, 1.0f}});
        finish = world.property<TestFinish>(oak);

        CHECK_CASE("the entity's own field wins", finish.has_value() && finish->gloss == 0.5f && finish->tint.blue == 1.0f);
        CHECK_CASE("and a field it leaves unset comes from its type", finish.has_value() && finish->grain == 8 && finish->tint.red == 0.5f);

        finish = world.property<TestFinish>(placed_at(stratum_sim::discrete_point2_type{3, 1}));
        CHECK_CASE("a room with no entity is skipped", finish.has_value() && finish->grain == 2 && finish->gloss == 0.75f);

        flecs::entity sconce = world.entities().entity();
        stratum_sim::reparent(sconce, zone, stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::WallId{north_wall}, {}});
        finish = world.property<TestFinish>(sconce);
        CHECK_CASE("an entity on a wall is in the room of the wall's cell", finish.has_value() && finish->grain == 1);

        flecs::entity key = world.entities().entity();
        stratum_sim::reparent(key, plain, stratum_sim::Site{stratum_sim::Relation::In, {}, {}});
        finish = world.property<TestFinish>(key);
        CHECK_CASE("an entity sited on its parent is in its parent's room", finish.has_value() && finish->grain == 1);

        finish = world.property<TestFinish>(west);
        CHECK_CASE("a room's own chain is the room, then the layout", finish.has_value() && finish->grain == 1 && finish->tint.blue == 0.125f);

        world.unload_zone(zone);
        CHECK_CASE("unloading a zone deletes its rooms' entities", !west.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "layout entities")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_core::PersistentId{});
        TestWorld world(id_allocator);

        auto layout = make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f,
            stratum_sim::discrete_vector2_type(3, 3));
        layout->set_flats(0.0f, 1.0f);
        layout->set_cell_open(stratum_sim::discrete_point2_type {1, 1}, true);
        layout->set_cell_walls(stratum_sim::discrete_point2_type {1, 1}, stratum_sim::direction_mask(CardinalDirection::North));
        id_type north_wall = layout->wall_toward(stratum_sim::discrete_point2_type {1, 1}, cardinal_direction(CardinalDirection::North));
        id_type floor_id = layout->floor_id();

        flecs::entity zone = world.create_zone(world.region_id, std::move(layout));

        auto attached_to = [](flecs::entity layout_entity)
        {
            const auto* attached = layout_entity.try_get<stratum_sim::AttachedEntities>();
            return attached != nullptr ? attached->entities : vector<flecs::entity> {};
        };

        auto sited = [&](const stratum_sim::address_type& address)
        {
            flecs::entity e = world.entities().entity();
            stratum_sim::reparent(e, zone, stratum_sim::Site{stratum_sim::Relation::On, address, {}});
            return e;
        };

        CHECK_CASE("a wall with nothing on it has no entity", !stratum_sim::wall_entity(zone, north_wall));

        flecs::entity torch = sited(stratum_sim::WallId{north_wall});
        flecs::entity wall = stratum_sim::wall_entity(zone, north_wall);
        CHECK_CASE("siting an entity on a wall makes the wall's entity", wall && wall.parent() == zone);
        CHECK_CASE("which lists it", attached_to(wall) == vector<flecs::entity>{torch});
        CHECK_CASE("and has no persistent id", !wall.has<stratum_core::PersistentId>());

        flecs::entity plaque = sited(stratum_sim::WallId{north_wall});
        CHECK_CASE("a second entity on the wall is listed on the same entity", stratum_sim::wall_entity(zone, north_wall) == wall
            && attached_to(wall) == vector<flecs::entity>{torch, plaque});

        stratum_sim::set_placement(torch, stratum_sim::placement_type(stratum_sim::point_type(0.5f, 0.5f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        CHECK_CASE("moving it on the wall lists it once", attached_to(wall).size() == 2);

        plaque.destruct();
        CHECK_CASE("deleting an entity takes it off the list", attached_to(wall) == vector<flecs::entity>{torch});

        stratum_sim::reparent(torch, zone, stratum_sim::Site{stratum_sim::Relation::In, {}, {}});
        CHECK_CASE("a wall's entity goes when nothing is sited on it", !wall.is_alive() && !stratum_sim::wall_entity(zone, north_wall));

        flecs::entity basin = sited(stratum_sim::FlatCellPosition{floor_id, stratum_sim::discrete_point2_type{1, 1}});
        flecs::entity floor_cell = stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point2_type{1, 1});
        CHECK_CASE("a flat cell's entity lists what is sited on it", floor_cell && attached_to(floor_cell) == vector<flecs::entity>{basin});

        floor_cell.add<stratum_sim::Blocking>();
        basin.destruct();
        CHECK_CASE("one with components of its own stays", floor_cell.is_alive()
            && stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point2_type{1, 1}) == floor_cell);

        flecs::entity lever = sited(stratum_sim::FlatCellPosition{floor_id, stratum_sim::discrete_point2_type{1, 1}});
        floor_cell.destruct();
        CHECK_CASE("deleting a flat cell's entity leaves what was on it unsited", lever.is_alive() && !lever.has<stratum_sim::Site>());
        CHECK_CASE("and the cell with none", !stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point2_type{1, 1}));

        // In readonly mode, as inside a system, a stage queues the operations until it is merged
        world.entities().readonly_begin();
        flecs::world stage = world.entities().get_stage(0);
        flecs::entity sconce = stage.entity();
        flecs::entity hook = stage.entity();
        stratum_sim::reparent(sconce, zone, stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::WallId{north_wall}, {}});
        stratum_sim::reparent(hook, zone, stratum_sim::Site{stratum_sim::Relation::On, stratum_sim::WallId{north_wall}, {}});
        const bool pending = !stratum_sim::wall_entity(zone, north_wall);
        world.entities().readonly_end();
        CHECK_CASE("entities sited through a stage are sited when it is merged", pending);
        CHECK_CASE("and all are listed", attached_to(stratum_sim::wall_entity(zone, north_wall)) == vector<flecs::entity>{sconce, hook});

        wall = stratum_sim::wall_entity(zone, north_wall);
        world.unload_zone(zone);
        CHECK_CASE("unloading a zone deletes its layout entities", !wall.is_alive());

        return;
    }

    TEST_SUITE_END();

} // namespace
