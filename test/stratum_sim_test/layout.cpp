//------------------------------------------------------------------------------
// stratum_sim_test
// layout.cpp
//------------------------------------------------------------------------------

#include "test_support.hpp"
#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/DiscretePlacementIndex.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <set>
#include <string>
#include <type_traits>

using namespace std;
using namespace quetzal;
using namespace stratum_sim_test;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    TEST_SUITE_BEGIN("layout");

    //--------------------------------------------------------------------------
    // Opens a rectangle of cells in a hand built layout and walls its outward sides
    void open_walled(stratum_sim::DiscreteLayout& layout, stratum_sim::coord_type i0, stratum_sim::coord_type j0,
        stratum_sim::coord_type i1, stratum_sim::coord_type j1)
    {
        for (stratum_sim::coord_type j = j0; j < j1; ++j)
        {
            for (stratum_sim::coord_type i = i0; i < i1; ++i)
            {
                stratum_sim::DirectionMask walls = stratum_sim::DirectionMask::None;
                walls |= i == i1 - 1 ? stratum_sim::DirectionMask::East : stratum_sim::DirectionMask::None;
                walls |= j == j1 - 1 ? stratum_sim::DirectionMask::North : stratum_sim::DirectionMask::None;
                walls |= i == i0 ? stratum_sim::DirectionMask::West : stratum_sim::DirectionMask::None;
                walls |= j == j0 ? stratum_sim::DirectionMask::South : stratum_sim::DirectionMask::None;

                layout.set_cell_open(stratum_sim::discrete_point_type {i, j}, true);
                layout.set_cell_walls(stratum_sim::discrete_point_type {i, j}, walls);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The layouts that are declared but not implemented; compiling their headers is most of the check
    TEST_CASE("layout hierarchy")
    {
        CHECK_CASE("a discrete layout is a Layout",
            is_base_of_v<stratum_sim::Layout, stratum_sim::DiscreteLayout>);
        CHECK_CASE("and can be built, being the one that is written",
            !is_abstract_v<stratum_sim::DiscreteLayout>);

        CHECK_CASE("a continuous layout is a Layout too",
            is_base_of_v<stratum_sim::Layout, stratum_sim::ContinuousLayout>);
        CHECK_CASE("and cannot be built, nothing behind it being written",
            is_abstract_v<stratum_sim::ContinuousLayout>);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("zone layout")
    {
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(4, 3)};
        open_walled(layout, 0, 0, 4, 3);

        CHECK_CASE_EQUAL("a wall on every side the rectangle faces out of", wall_counts(layout).sides, 14);
        CHECK_CASE("a wall is on the side of the cell it faces", layout.wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::East)) != nullid);
        CHECK_CASE("and nothing within the rectangle", layout.wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)) == nullid);
        CHECK_CASE("every side of a cell is a wall of its own",
            layout.wall_toward(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::West))
                != layout.wall_toward(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::South)));

        layout.set_flats(0.0f, 2.5f);
        CHECK_CASE_EQUAL("a layout has two flats", layout.flat_store_count(), 2);
        CHECK_CASE("the floor is not a ceiling", !layout.flat(layout.floor_id()).ceiling());
        CHECK_CASE("the ceiling is a ceiling", layout.flat(layout.ceiling_id()).ceiling());
        CHECK_CASE("the floor keeps its height", layout.flat(layout.floor_id()).height() == 0.0f);
        CHECK_CASE("the ceiling keeps its height", layout.flat(layout.ceiling_id()).height() == 2.5f);

        layout.set_room_count(2);
        CHECK_CASE_EQUAL("a layout keeps the room count it was given", layout.room_count(), 2);

        stratum_sim::DiscreteLayout moved = std::move(layout);
        CHECK_CASE("a moved layout takes the grid", moved.grid().extent == stratum_sim::discrete_vector_type(4, 3));
        CHECK_CASE("and the cells", moved.open(stratum_sim::discrete_point_type {1, 1}));
        CHECK_CASE("and the flats", moved.flat_store_count() == 2 && moved.flat(moved.ceiling_id()).height() == 2.5f);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("rooms")
    {
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};
        layout.set_room_count(1);
        layout.set_cell_room(stratum_sim::discrete_point_type {0, 0}, 0);
        layout.set_cell_room(stratum_sim::discrete_point_type {1, 0}, 0);

        CHECK_CASE_EQUAL("a layout keeps how many rooms there are", layout.room_count(), 1);
        CHECK_CASE("a cell names the room covering it", layout.room_at(stratum_sim::discrete_point_type {1, 0}) == 0);
        CHECK_CASE("a cell no room covers has none", layout.room_at(stratum_sim::discrete_point_type {5, 5}) == nullid);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("grid")
    {
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        CHECK_CASE("a point maps to its cell", layout.cell_position_at(stratum_sim::point_type(2.7f, 3.2f, 0.0f)) == stratum_sim::discrete_point_type(2, 3));
        CHECK_CASE("a cell maps back to its center", layout.position(stratum_sim::discrete_point_type {2, 3}).x() == 2.5f);
        CHECK_CASE("round tripping a cell center returns the cell", layout.cell_position_at(layout.position(stratum_sim::discrete_point_type {5, 6})) == stratum_sim::discrete_point_type(5, 6));
        CHECK_CASE("a point left of the origin maps to a negative cell", layout.cell_position_at(stratum_sim::point_type(-0.5f, 0.5f, 0.0f)) == stratum_sim::discrete_point_type(-1, 0));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("flats")
    {
        // A zone with no height data loads a constant
        stratum_sim::Flat floor(2.0f);

        CHECK_CASE("a flat is a floor unless it is made a ceiling", !floor.ceiling());
        CHECK_CASE("a flat with no room or cell heights has none", !floor.room_height(0) && !floor.cell_height(stratum_sim::discrete_point_type {1, 1}));

        floor.set_room_height(0, -0.25f);
        floor.set_cell_height(stratum_sim::discrete_point_type {1, 1}, 0.5f);
        CHECK_CASE("a room height is kept", floor.room_height(0) == -0.25f);
        CHECK_CASE("and a cell height", floor.cell_height(stratum_sim::discrete_point_type {1, 1}) == 0.5f);

        // A cell's floor height comes from the cell, then its room, then the flat
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        CHECK_CASE("with no floor a cell sits on the grid's own plane", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 0.0f);

        layout.set_flats(1.5f, 4.0f);
        CHECK_CASE("with one it sits on the floor", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 1.5f);

        layout.set_room_count(1);
        layout.set_cell_room(stratum_sim::discrete_point_type {1, 1}, 0);
        layout.set_cell_room(stratum_sim::discrete_point_type {2, 1}, 0);
        layout.flat(layout.floor_id()).set_room_height(0, 1.0f);
        CHECK_CASE("a room's height overrides the flat's", layout.position(stratum_sim::discrete_point_type {2, 1}).z() == 1.0f);

        layout.flat(layout.floor_id()).set_cell_height(stratum_sim::discrete_point_type {1, 1}, 2.5f);
        CHECK_CASE("and a cell's overrides the room's", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 2.5f);
        CHECK_CASE("a cell no room covers takes the flat's", layout.height(layout.floor_id(), stratum_sim::discrete_point_type {5, 5}) == 1.5f);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("blocking")
    {
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        // Two rooms one cell tall side by side, so their shared boundary is exactly one cell edge, walled from both sides
        open_walled(layout, 0, 0, 2, 1);
        open_walled(layout, 2, 0, 4, 1);

        // Within one room there is no wall at all, so nothing can block
        stratum_sim::Layout::crossable_function_type shut;
        CHECK_CASE("cells inside one area are always crossable", layout.crossable(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::East), shut));
        CHECK_CASE("crossing within an area is symmetric", layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::West), shut));
        CHECK_CASE("a wall nothing crosses blocks", !layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), shut));

        // Open one side only, which is what a one-way passage is
        id_type wall_a = layout.wall_toward(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East));
        id_type wall_b = layout.wall_toward(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West));
        CHECK_CASE("the two sides of the shared edge are two walls", wall_a != nullid && wall_b != nullid && wall_a != wall_b);

        // The caller's predicate decides which walls may be crossed
        std::set<id_type> open_walls {wall_a};
        stratum_sim::Layout::crossable_function_type crossable = [&](id_type id) { return open_walls.contains(id); };

        CHECK_CASE("opening a wall lets a mover out", layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), crossable));
        CHECK_CASE("the other side is untouched, so the way back stays shut", !layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

        open_walls.insert(wall_b);
        CHECK_CASE("opening the other side opens the way back", layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

        // The two sides of an edge are independent, so one side may have a wall the other has not
        layout.set_cell_walls(stratum_sim::discrete_point_type {2, 0}, stratum_sim::DirectionMask::North | stratum_sim::DirectionMask::South
            | stratum_sim::DirectionMask::East);
        open_walls.clear();
        CHECK_CASE("a wall on one side of an edge blocks from that side alone", !layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), crossable)
            && layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("movement")
    {
        // A corridor four cells long, walled at both ends, and a cell beyond its east end
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        open_walled(layout, 0, 0, 4, 1);
        open_walled(layout, 4, 0, 5, 1);
        id_type corridor_end = layout.wall_toward(stratum_sim::discrete_point_type {3, 0}, cardinal_direction(CardinalDirection::East));

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        stratum_sim::placement_type start(layout.position(stratum_sim::discrete_point_type {0, 0}), east);

        stratum_sim::MoveResult one = layout.move(start, east, 1.0f);
        CHECK_CASE("a step ends in the next cell", layout.cell_position_at(one.placement.position()) == stratum_sim::discrete_point_type(1, 0));
        CHECK_CASE("a step with nothing in the way reports nothing", one.blocking_wall_id == nullid);
        CHECK_CASE("moving does not turn", one.placement.direction().x() == 1.0f);

        stratum_sim::MoveResult three = layout.move(start, east, 3.0f);
        CHECK_CASE("distance is in cells", layout.cell_position_at(three.placement.position()) == stratum_sim::discrete_point_type(3, 0));

        // The far wall of the corridor has a cell behind it but nothing crosses it
        stratum_sim::MoveResult far = layout.move(start, east, 7.0f);
        CHECK_CASE("a mover stops at the first block", layout.cell_position_at(far.placement.position()) == stratum_sim::discrete_point_type(3, 0));
        CHECK_CASE("and reports what stopped it", far.blocking_wall_id != nullid);
        CHECK_CASE("what stopped it is the wall on its own side of the edge", far.blocking_wall_id == corridor_end);

        // Opening that wall lets it through
        stratum_sim::Layout::crossable_function_type crossable = [id = far.blocking_wall_id](id_type wall_id) { return wall_id == id; };
        stratum_sim::MoveResult through = layout.move(start, east, 7.0f, crossable, stratum_sim::Layout::blocker_function_type());
        CHECK_CASE("an opened wall no longer stops it", layout.cell_position_at(through.placement.position()) == stratum_sim::discrete_point_type(4, 0));
        CHECK_CASE("it stops at the next block instead", through.blocking_wall_id != nullid && through.blocking_wall_id != far.blocking_wall_id);

        // Solid in every other direction
        stratum_sim::MoveResult blocked = layout.move(start, north, 3.0f);
        CHECK_CASE("a mover held against a wall does not move", layout.cell_position_at(blocked.placement.position()) == stratum_sim::discrete_point_type(0, 0));
        CHECK_CASE("and still reports the wall", blocked.blocking_wall_id != nullid);

        CHECK_CASE("a zero direction moves nothing", layout.cell_position_at(layout.move(start, stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 3.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));
        CHECK_CASE("a zero distance moves nothing", layout.cell_position_at(layout.move(start, east, 0.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));

        // The direction snaps to a cardinal direction
        stratum_sim::MoveResult skewed = layout.move(start, stratum_sim::vector_type(0.9f, 0.4f, 0.0f), 2.0f);
        CHECK_CASE("a direction off axis snaps to the nearest", layout.cell_position_at(skewed.placement.position()) == stratum_sim::discrete_point_type(2, 0));

        // Turning
        stratum_sim::placement_type facing_east(start.position(), east);
        stratum_sim::placement_type left = layout.turn(facing_east, stratum_sim::Rotation::CCW);
        CHECK_CASE("turning counterclockwise from east faces north", left.direction().x() == 0.0f && left.direction().y() == 1.0f);
        CHECK_CASE("turning does not move", layout.same_location(facing_east, left));

        stratum_sim::placement_type right = layout.turn(facing_east, stratum_sim::Rotation::CW);
        CHECK_CASE("turning clockwise from east faces south", right.direction().x() == 0.0f && right.direction().y() == -1.0f);

        stratum_sim::placement_type around = layout.turn(facing_east, stratum_sim::Rotation::Half);
        CHECK_CASE("turning half around from east faces west", around.direction().x() == -1.0f && around.direction().y() == 0.0f);
        CHECK_CASE("four counterclockwise turns return the original facing",
            layout.turn(layout.turn(layout.turn(left, stratum_sim::Rotation::CCW), stratum_sim::Rotation::CCW), stratum_sim::Rotation::CCW).direction().x() == 1.0f);

        // Turning to an absolute direction
        stratum_sim::placement_type faced = layout.face(facing_east, north);
        CHECK_CASE("facing north from east faces north", faced.direction().x() == 0.0f && faced.direction().y() == 1.0f);
        CHECK_CASE("facing does not move", layout.same_location(facing_east, faced));

        stratum_sim::placement_type faced_off_axis = layout.face(facing_east, stratum_sim::vector_type(0.4f, 0.9f, 0.0f));
        CHECK_CASE("a facing off axis snaps to the nearest",
            faced_off_axis.direction().x() == 0.0f && faced_off_axis.direction().y() == 1.0f);

        CHECK_CASE("facing the way already faced changes nothing", layout.face(facing_east, east).direction().x() == 1.0f);
        CHECK_CASE("a zero direction leaves the facing alone",
            layout.face(facing_east, stratum_sim::vector_type(0.0f, 0.0f, 0.0f)).direction().x() == 1.0f);

        // A relative turn resolves to the same facing direction as the absolute turn
        CHECK_CASE("a resolved counterclockwise turn faces where turning does",
            layout.face(facing_east, layout.turn(facing_east, stratum_sim::Rotation::CCW).direction()).direction().y() == 1.0f);

        // Moving in a direction other than the facing direction leaves the facing direction unchanged
        stratum_sim::placement_type facing_north(start.position(), north);
        stratum_sim::MoveResult sidestep = layout.move(facing_north, east, 2.0f);
        CHECK_CASE("absolute movement ignores the mover's facing", layout.cell_position_at(sidestep.placement.position()) == stratum_sim::discrete_point_type(2, 0));
        CHECK_CASE("and leaves that facing untouched", sidestep.placement.direction().y() == 1.0f);

        // same_location is by cell, not by exact position
        stratum_sim::placement_type a(stratum_sim::point_type(2.1f, 0.2f, 0.0f), east);
        stratum_sim::placement_type b(stratum_sim::point_type(2.9f, 0.8f, 0.0f), east);
        CHECK_CASE("two points in one cell are the same location", layout.same_location(a, b));
        CHECK_CASE("a point in the next cell is not", !layout.same_location(a, stratum_sim::placement_type(stratum_sim::point_type(3.5f, 0.5f, 0.0f), east)));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE_FIXTURE(WorldFixture, "adjacency and sight")
    {
        // A three by three room with a doorway off its east side, into a dead end
        auto room = load_open("data/room.zone");
        stratum_sim::DiscreteLayout& layout = room->layout();

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto at = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        CHECK_CASE_EQUAL("the file names no rooms", layout.room_count(), 0);

        // Inside the room nothing separates cells, so a middle cell reaches all four
        CHECK_CASE_EQUAL("a cell amid open floor has four neighbors", layout.neighbors(at(2, 2), room->crossable).size(), 4);
        CHECK_CASE_EQUAL("a corner cell is walled on two sides", layout.neighbors(at(1, 1), room->crossable).size(), 2);
        CHECK_CASE_EQUAL("an edge cell is walled on one", layout.neighbors(at(2, 1), room->crossable).size(), 3);

        CHECK_CASE_EQUAL("an open doorway is a neighbor", layout.neighbors(at(3, 2), room->crossable).size(), 4);
        CHECK_CASE("a neighbor keeps the mover's facing", layout.neighbors(at(2, 2), room->crossable).front().direction().x() == 1.0f);

        id_type doorway = layout.wall_toward(stratum_sim::discrete_point_type {3, 2}, cardinal_direction(CardinalDirection::East));
        CHECK_CASE("the wall into the doorway is found", doorway != nullid);
        CHECK_CASE("and the doorway has no wall facing back",
            layout.wall_toward(stratum_sim::discrete_point_type {4, 2}, cardinal_direction(CardinalDirection::West)) == nullid);

        CHECK_CASE("the layout names what a placement faces", layout.ahead(at(3, 2)) == doorway);
        CHECK_CASE("and nothing where the facing meets no wall", layout.ahead(at(1, 1)) == nullid);
        CHECK_CASE("a facing the grid names nothing faces nothing", layout.ahead(
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), stratum_sim::vector_type())) == nullid);

        // A doorway into a dead end has one wall
        std::set<id_type> open_walls {doorway};
        stratum_sim::Layout::crossable_function_type crossable = [&](id_type wall_id) { return open_walls.contains(wall_id); };

        open_walls.erase(doorway);
        CHECK_CASE_EQUAL("closing it removes the neighbor", layout.neighbors(at(3, 2), crossable).size(), 3);
        open_walls.insert(doorway);

        // Sight
        CHECK_CASE("a cell sees itself", layout.visible(at(2, 2), at(2, 2), crossable));
        CHECK_CASE("open floor is visible across", layout.visible(at(1, 1), at(3, 3), crossable));
        CHECK_CASE("sight is blocked by a solid cell", !layout.visible(at(2, 2), at(2, 4), crossable));
        CHECK_CASE("sight reaches through an open doorway", layout.visible(at(3, 2), at(4, 2), crossable));

        open_walls.erase(doorway);
        CHECK_CASE("closing it blocks sight too", !layout.visible(at(3, 2), at(4, 2), crossable));

        return;
    }

    //--------------------------------------------------------------------------
    // A zone read from a file is walkable by commands ordered by the scheduler, with no rendering
    TEST_CASE_FIXTURE(WorldFixture, "walking a dungeon")
    {
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        TestWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(actor.id(), 100);

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_position_at(stratum_sim::placement_of(actor).position()); };

        CHECK_CASE("the actor starts at the west end", cell_of() == stratum_sim::discrete_point_type(1, 1));

        size_t steps = 0;
        size_t blocks = 0;

        for (size_t turn = 0; turn < 8; ++turn)
        {
            id_type ready = scheduler.next();
            if (ready != actor.id())
            {
                break;
            }

            events.clear();
            commands.push(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 1.0f});
            int cost = stratum_sim::apply(world, commands.front(), events);
            commands.pop();
            scheduler.spend(actor.id(), cost > 0 ? cost : stratum_sim::Scheduler::action_cost);

            for (const stratum_sim::Event& event : events.events())
            {
                steps += event.is<stratum_sim::Moved>() ? 1 : 0;
                blocks += event.is<stratum_sim::Blocked>() ? 1 : 0;
            }
        }

        CHECK_CASE("it walked to the east end", cell_of() == stratum_sim::discrete_point_type(5, 1));
        CHECK_CASE_EQUAL("taking one step per open cell", steps, 4);
        CHECK_CASE("and reported being stopped at the wall", blocks > 0);
        CHECK_CASE("the scheduler advanced through the walk", scheduler.elapsed() >= 8);

        // Turning and walking back
        events.clear();
        stratum_sim::Command about(actor.id(), stratum_sim::TurnTo {
            layout.turn(stratum_sim::placement_of(actor), stratum_sim::Rotation::Half).direction()});
        stratum_sim::apply(world, about, events);
        CHECK_CASE("turning around faces west", stratum_sim::placement_of(actor).direction().x() == -1.0f);

        events.clear();
        stratum_sim::Command back(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 4.0f});
        stratum_sim::apply(world, back, events);

        CHECK_CASE("walking back reaches the west end", cell_of() == stratum_sim::discrete_point_type(1, 1));
        CHECK_CASE("in a single command", events.size() == 1 && events.front().is<stratum_sim::Moved>());

        return;
    }

    //--------------------------------------------------------------------------
    // geometry::Polygon's Cartesian containment, which stratum_sim depends on
    TEST_CASE("polygon containment, cartesian traits")
    {
        auto polygon_of = [](const stratum_sim::points_type& points)
        {
            return stratum_sim::polygon_type(points.begin(), points.end());
        };

        // Normal along z, so the dropped axis is 2
        stratum_sim::polygon_type xy = polygon_of({
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 3.0f, 0.0f),
            stratum_sim::point_type(0.0f, 3.0f, 0.0f)});

        CHECK_CASE("an interior point is contained", xy.contains(stratum_sim::point_type(2.0f, 1.5f, 0.0f)));
        CHECK_CASE("an exterior point in the plane is not", !xy.contains(stratum_sim::point_type(9.0f, 1.5f, 0.0f)));
        CHECK_CASE("a point off the plane is not", !xy.contains(stratum_sim::point_type(2.0f, 1.5f, 1.0f)));

        // Normal along y, so the dropped axis is 1
        stratum_sim::polygon_type xz = polygon_of({
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 0.0f, 3.0f),
            stratum_sim::point_type(0.0f, 0.0f, 3.0f)});

        CHECK_CASE("an interior point of an xz polygon is contained", xz.contains(stratum_sim::point_type(2.0f, 0.0f, 1.5f)));
        CHECK_CASE("an exterior point of an xz polygon is not", !xz.contains(stratum_sim::point_type(9.0f, 0.0f, 1.5f)));

        // Normal along x, so the dropped axis is 0
        stratum_sim::polygon_type yz = polygon_of({
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(0.0f, 4.0f, 0.0f),
            stratum_sim::point_type(0.0f, 4.0f, 3.0f),
            stratum_sim::point_type(0.0f, 0.0f, 3.0f)});

        CHECK_CASE("an interior point of a yz polygon is contained", yz.contains(stratum_sim::point_type(0.0f, 2.0f, 1.5f)));
        CHECK_CASE("an exterior point of a yz polygon is not", !yz.contains(stratum_sim::point_type(0.0f, 9.0f, 1.5f)));

        // Tilted, with no dominant axis
        stratum_sim::polygon_type tilted = polygon_of({
            stratum_sim::point_type(1.0f, 0.0f, 0.0f),
            stratum_sim::point_type(0.0f, 1.0f, 0.0f),
            stratum_sim::point_type(0.0f, 0.0f, 1.0f)});

        CHECK_CASE("an interior point of a tilted polygon is contained", tilted.contains(stratum_sim::point_type(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f)));
        CHECK_CASE("an exterior point of a tilted polygon is not", !tilted.contains(stratum_sim::point_type(1.0f, 1.0f, -1.0f)));

        return;
    }

    TEST_SUITE_END();

} // namespace
