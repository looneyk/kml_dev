//------------------------------------------------------------------------------
// stratum_sim_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Layout.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <format>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // Every case reports its own line so that a failure is identifiable by name
    // detail describes the discrepancy and is only written for a failure
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_equal(const string& name, size_t value, size_t value_expected)
    {
        report_case(name, value == value_expected, format("expected {}, got {}", value_expected, value));
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
    }

    // Stand-in command types, sim defines no commands of its own
    //--------------------------------------------------------------------------
    struct Move
    {
        int dx = 0;
        int dy = 0;
    };

    //--------------------------------------------------------------------------
    struct Wait
    {
    };

    // Stand-in event types, sim defines no events of its own
    //--------------------------------------------------------------------------
    struct EntityMoved
    {
        id_type entity_id = nullid;
        int x = 0;
        int y = 0;
    };

    //--------------------------------------------------------------------------
    struct DoorOpened
    {
        id_type entity_id = nullid;
    };

    //--------------------------------------------------------------------------
    stratum_sim::polygon_type rectangle(stratum_sim::value_type x0, stratum_sim::value_type y0, stratum_sim::value_type x1, stratum_sim::value_type y1)
    {
        stratum_sim::points_type points
        {
            stratum_sim::point_type(x0, y0, 0.0f),
            stratum_sim::point_type(x1, y0, 0.0f),
            stratum_sim::point_type(x1, y1, 0.0f),
            stratum_sim::point_type(x0, y1, 0.0f)
        };

        return stratum_sim::polygon_type(points.begin(), points.end());
    }

    //--------------------------------------------------------------------------
    size_t rock_count(const stratum_sim::Layout& layout, const stratum_sim::Layout::ids_type& ids)
    {
        size_t n = 0;
        for (id_type id : ids)
        {
            n += layout.wall(id).partner == nullid ? 1 : 0;
        }

        return n;
    }

    //--------------------------------------------------------------------------
    void test_command_queue()
    {
        report_group("command queue");

        stratum_sim::CommandQueue queue;

        report_case("new queue is empty", queue.empty() && queue.size() == 0);

        queue.push(7, Move {1, 0});
        queue.push(7, Wait {});
        queue.push(9, Move {0, -1});

        report_case_equal("push grows the queue", queue.size(), 3);
        report_case("a populated queue is not empty", !queue.empty());

        const stratum_sim::Command& command = queue.front();
        report_case("front is the first command submitted", command.is<Move>());
        report_case_equal("a command carries its entity id", command.entity_id(), 7);
        report_case("get returns the command", command.get<Move>().dx == 1 && command.get<Move>().dy == 0);
        report_case("try_get is null for another command type", command.try_get<Wait>() == nullptr);

        queue.pop();
        report_case("pop removes the front command", queue.front().is<Wait>());
        report_case_equal("pop shrinks the queue", queue.size(), 2);

        queue.pop();
        report_case_equal("commands are consumed in submission order", queue.front().entity_id(), 9);

        stratum_sim::Command command_prebuilt(4, Move {0, 1});
        queue.push(command_prebuilt);
        report_case_equal("a prebuilt command can be pushed", queue.size(), 2);
        report_case("a prebuilt command keeps its type", queue.commands().back().is<Move>());
        report_case_equal("a prebuilt command keeps its entity id", queue.commands().back().entity_id(), 4);

        stratum_sim::Command command_default;
        report_case("a default command is empty", command_default.empty() && command_default.entity_id() == nullid);

        queue.clear();
        report_case("clear empties the queue", queue.empty());

        return;
    }

    //--------------------------------------------------------------------------
    void test_event_queue()
    {
        report_group("event queue");

        stratum_sim::EventQueue queue;

        report_case("new queue is empty", queue.empty() && queue.size() == 0);

        queue.push(EntityMoved {7, 3, 4});
        queue.push(DoorOpened {9});

        report_case_equal("push grows the queue", queue.size(), 2);

        const stratum_sim::Event& event = queue.front();
        report_case("front is the first event recorded", event.is<EntityMoved>());
        report_case("get returns the event", event.get<EntityMoved>().x == 3 && event.get<EntityMoved>().y == 4);
        report_case_equal("an event payload carries its own fields", event.get<EntityMoved>().entity_id, 7);
        report_case("try_get is null for another event type", event.try_get<DoorOpened>() == nullptr);

        queue.pop();
        report_case("pop removes the front event", queue.front().is<DoorOpened>());
        report_case_equal("pop shrinks the queue", queue.size(), 1);

        queue.push(EntityMoved {9, 0, 0});
        report_case("events iterate in occurrence order", queue.events().front().is<DoorOpened>() && queue.events().back().is<EntityMoved>());

        // Guards the constrained converting constructor, an unconstrained one would win
        // over the copy constructor here and produce an Event holding an Event
        stratum_sim::Event event_source(DoorOpened {5});
        stratum_sim::Event event_copy(event_source);
        report_case("copying an event does not nest it", event_copy.is<DoorOpened>());
        report_case_equal("a copied event keeps its payload", event_copy.get<DoorOpened>().entity_id, 5);

        stratum_sim::Event event_default;
        report_case("a default event is empty", event_default.empty());

        queue.clear();
        report_case("clear empties the queue", queue.empty());

        return;
    }

    //--------------------------------------------------------------------------
    void test_level_layout()
    {
        report_group("level layout");

        stratum_sim::Layout layout;
        stratum_sim::polygon_type outline = rectangle(0.0f, 0.0f, 4.0f, 3.0f);

        id_type region_id = layout.create_region(outline, 0.0f, 2.5f);

        report_case_equal("a region is created", layout.region_store_count(), 1);
        report_case_equal("one facing per outline edge", layout.wall_store_count(), outline.vertex_count());

        stratum_sim::Layout::ids_type walls = layout.region_walls(region_id);
        report_case_equal("the ring walks every facing exactly once", walls.size(), outline.vertex_count());
        report_case_equal("a new region is surrounded by solid rock", rock_count(layout, walls), walls.size());

        bool ring_ordered = true;
        bool prev_inverse = true;
        for (size_t i = 0; i < walls.size(); ++i)
        {
            const stratum_sim::Wall& wall = layout.wall(walls[i]);
            ring_ordered = ring_ordered
                && wall.position.x() == outline.vertex(i).x()
                && wall.position.y() == outline.vertex(i).y();
            prev_inverse = prev_inverse && layout.wall(wall.next).prev == walls[i];
        }

        report_case("ring order follows the outline", ring_ordered);
        report_case("prev is the inverse of next", prev_inverse);
        report_case("every facing knows its region", layout.wall(walls.front()).region == region_id);

        // Flats
        const stratum_sim::Region& region = layout.region(region_id);
        report_case_equal("a region has two flats", layout.flat_store_count(), 2);
        report_case("the floor is not a ceiling", !layout.flat(region.floor).ceiling);
        report_case("the ceiling is a ceiling", layout.flat(region.ceiling).ceiling);
        report_case("the floor keeps its height", layout.flat(region.floor).height == 0.0f);
        report_case("the ceiling keeps its height", layout.flat(region.ceiling).height == 2.5f);
        report_case("a flat knows its region", layout.flat(region.floor).region == region_id);

        layout.clear();
        report_case("clear empties every store", layout.wall_store_count() == 0 && layout.region_store_count() == 0 && layout.flat_store_count() == 0);

        return;
    }

    //--------------------------------------------------------------------------
    void test_partner_linking()
    {
        report_group("partner linking");

        stratum_sim::Layout layout;

        // Sharing the edge at x == 4, traversed in opposite directions by each region
        id_type region_a = layout.create_region(rectangle(0.0f, 0.0f, 4.0f, 3.0f), 0.0f, 2.5f);
        id_type region_b = layout.create_region(rectangle(4.0f, 0.0f, 8.0f, 3.0f), 0.0f, 2.5f);
        id_type region_far = layout.create_region(rectangle(20.0f, 0.0f, 24.0f, 3.0f), 0.0f, 2.5f);

        stratum_sim::Layout::ids_type walls_a = layout.region_walls(region_a);
        stratum_sim::Layout::ids_type walls_b = layout.region_walls(region_b);

        report_case_equal("every facing starts as rock", rock_count(layout, walls_a) + rock_count(layout, walls_b), 8);

        report_case_equal("adjacent regions share exactly one edge", layout.connect_regions(region_a, region_b), 1);
        report_case_equal("only the shared facing of one region leaves rock", rock_count(layout, walls_a), 3);
        report_case_equal("only the shared facing of the other leaves rock", rock_count(layout, walls_b), 3);

        id_type connected_a = nullid;
        for (id_type id : walls_a)
        {
            if (layout.wall(id).partner != nullid)
            {
                connected_a = id;
            }
        }

        report_case("a connected facing was found", connected_a != nullid);

        id_type connected_b = connected_a == nullid ? nullid : layout.wall(connected_a).partner;
        report_case("partners name each other", connected_b != nullid && layout.wall(connected_b).partner == connected_a);
        report_case("a connected pair is opposed", layout.opposed(connected_a, connected_b));
        report_case("opposition is symmetric", layout.opposed(connected_b, connected_a));
        report_case("a facing is never opposed to itself", !layout.opposed(connected_a, connected_a));

        report_case_equal("connecting again finds nothing left to do", layout.connect_regions(region_a, region_b), 0);
        report_case_equal("regions that share no edge connect nothing", layout.connect_regions(region_a, region_far), 0);

        layout.disconnect(connected_a);
        report_case_equal("disconnect returns one region's facing to rock", rock_count(layout, walls_a), 4);
        report_case_equal("disconnect returns the partner to rock too", rock_count(layout, walls_b), 4);

        return;
    }

    //--------------------------------------------------------------------------
    void test_wall_fold()
    {
        report_group("wall fold");

        stratum_sim::Layout layout;

        // A hall with a partition hanging into it from the north wall. The outline runs
        // down one face of the partition and back up the other, so the tip is where the
        // boundary walk folds
        stratum_sim::points_type points
        {
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(10.0f, 0.0f, 0.0f),
            stratum_sim::point_type(10.0f, 6.0f, 0.0f),
            stratum_sim::point_type(5.0f, 6.0f, 0.0f),
            stratum_sim::point_type(5.0f, 2.0f, 0.0f), // the partition tip
            stratum_sim::point_type(5.0f, 6.0f, 0.0f),
            stratum_sim::point_type(0.0f, 6.0f, 0.0f)
        };

        id_type region_id = layout.create_region(stratum_sim::polygon_type(points.begin(), points.end()), 0.0f, 2.5f);
        stratum_sim::Layout::ids_type walls = layout.region_walls(region_id);

        report_case_equal("a spur outline produces one facing per edge", walls.size(), 7);
        report_case_equal("the ring closes over a spur", walls.size(), points.size());
        report_case_equal("every facing starts as rock", rock_count(layout, walls), 7);

        report_case_equal("a fold connects exactly one pair", layout.connect_folds(region_id), 1);
        report_case_equal("only the partition's two faces leave rock", rock_count(layout, walls), 5);

        // The front face runs from the north wall down to the tip
        id_type face_front = nullid;
        for (id_type id : walls)
        {
            const stratum_sim::Wall& wall = layout.wall(id);
            if (wall.position.x() == 5.0f && wall.position.y() == 6.0f && wall.partner != nullid)
            {
                face_front = id;
            }
        }

        report_case("the connected facing is the partition's front face", face_front != nullid);

        bool folds_at_tip = false;
        bool faces_are_partners = false;
        if (face_front != nullid)
        {
            const stratum_sim::Wall& front = layout.wall(face_front);
            id_type face_rear = front.partner;

            // The fold: walking next off the end of one face arrives on the other
            folds_at_tip = front.next == face_rear;
            faces_are_partners = layout.wall(face_rear).partner == face_front
                && layout.wall(face_rear).position.y() == 2.0f;
        }

        report_case("the walk folds at the tip onto the other face", folds_at_tip);
        report_case("the two faces of the partition are partners", faces_are_partners);

        layout.disconnect(face_front);
        report_case_equal("disconnect restores the partition to rock", rock_count(layout, walls), 7);

        return;
    }

    //--------------------------------------------------------------------------
    void test_point_location()
    {
        report_group("point location");

        stratum_sim::Layout layout;
        stratum_sim::polygon_type outline = rectangle(0.0f, 0.0f, 4.0f, 3.0f);

        id_type region_a = layout.create_region(outline, 0.0f, 2.5f);
        id_type region_b = layout.create_region(rectangle(4.0f, 0.0f, 8.0f, 3.0f), 0.0f, 2.5f);

        stratum_sim::polygon_type rebuilt = layout.region_outline(region_a);
        report_case_equal("a rebuilt outline has the original vertex count", rebuilt.vertex_count(), outline.vertex_count());

        bool vertices_match = rebuilt.vertex_count() == outline.vertex_count();
        for (size_t i = 0; i < rebuilt.vertex_count() && vertices_match; ++i)
        {
            vertices_match = rebuilt.vertex(i).x() == outline.vertex(i).x() && rebuilt.vertex(i).y() == outline.vertex(i).y();
        }

        report_case("a rebuilt outline matches the original in order", vertices_match);

        report_case("a point inside a region finds it", layout.region_at(stratum_sim::point_type(2.0f, 1.5f, 0.0f)) == region_a);
        report_case("a point in the other region finds that one", layout.region_at(stratum_sim::point_type(6.0f, 1.5f, 0.0f)) == region_b);
        report_case("a point outside every region finds none", layout.region_at(stratum_sim::point_type(50.0f, 50.0f, 0.0f)) == nullid);

        // Height is disregarded, since a footprint is horizontal and a mover stands above it
        report_case("a point at floor height still locates", layout.region_at(stratum_sim::point_type(2.0f, 1.5f, 2.0f)) == region_a);

        // A region with a partition is still one region on both sides of it
        stratum_sim::points_type points
        {
            stratum_sim::point_type(0.0f, 10.0f, 0.0f),
            stratum_sim::point_type(10.0f, 10.0f, 0.0f),
            stratum_sim::point_type(10.0f, 16.0f, 0.0f),
            stratum_sim::point_type(5.0f, 16.0f, 0.0f),
            stratum_sim::point_type(5.0f, 12.0f, 0.0f),
            stratum_sim::point_type(5.0f, 16.0f, 0.0f),
            stratum_sim::point_type(0.0f, 16.0f, 0.0f)
        };

        id_type hall = layout.create_region(stratum_sim::polygon_type(points.begin(), points.end()), 0.0f, 2.5f);

        report_case("a point west of a partition is in the hall", layout.region_at(stratum_sim::point_type(2.0f, 14.0f, 0.0f)) == hall);
        report_case("a point east of the same partition is in the same hall", layout.region_at(stratum_sim::point_type(8.0f, 14.0f, 0.0f)) == hall);
        report_case_equal("a spur outline rebuilds with its repeated vertex", layout.region_outline(hall).vertex_count(), 7);

        return;
    }

    //--------------------------------------------------------------------------
    void test_grid()
    {
        report_group("grid");

        stratum_sim::Layout layout;
        report_case("a Layout without a Grid is continuous", !layout.discrete());

        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::Cell {8, 8});
        report_case("a Layout carrying a Grid is discrete", layout.discrete());

        report_case("a point maps to its cell", layout.cell_at(stratum_sim::point_type(2.7f, 3.2f, 0.0f)) == stratum_sim::Cell(2, 3));
        report_case("a cell maps back to its centre", layout.cell_position(stratum_sim::Cell {2, 3}).x() == 2.5f);
        report_case("round tripping a cell centre returns the cell", layout.cell_at(layout.cell_position(stratum_sim::Cell {5, 6})) == stratum_sim::Cell(5, 6));
        report_case("a point left of the origin lands in a negative cell", layout.cell_at(stratum_sim::point_type(-0.5f, 0.5f, 0.0f)) == stratum_sim::Cell(-1, 0));

        layout.clear();
        report_case("clear drops the grid too", !layout.discrete());

        return;
    }

    //--------------------------------------------------------------------------
    void test_blocking()
    {
        report_group("blocking");

        stratum_sim::Layout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::Cell {8, 8});

        // Two rooms one cell tall, so their shared boundary is exactly one cell edge
        id_type region_a = layout.create_region(rectangle(0.0f, 0.0f, 2.0f, 1.0f), 0.0f, 2.5f);
        id_type region_b = layout.create_region(rectangle(2.0f, 0.0f, 4.0f, 1.0f), 0.0f, 2.5f);

        // Within one region there is no facing at all, so nothing can block
        report_case("cells inside one region never block", !layout.blocked(stratum_sim::Cell {0, 0}, stratum_sim::Cell {1, 0}));
        report_case("blocking within a region is symmetric", !layout.blocked(stratum_sim::Cell {1, 0}, stratum_sim::Cell {0, 0}));

        // Across the boundary the regions are still rock, so the facing blocks
        report_case("an unconnected boundary blocks", layout.blocked(stratum_sim::Cell {1, 0}, stratum_sim::Cell {2, 0}));

        report_case_equal("the two rooms share one facing pair", layout.connect_regions(region_a, region_b), 1);
        report_case("a connected but impassable facing still blocks", layout.blocked(stratum_sim::Cell {1, 0}, stratum_sim::Cell {2, 0}));

        // Open one side only, which is what a one-way passage is
        id_type facing_a = nullid;
        for (id_type id : layout.region_walls(region_a))
        {
            const stratum_sim::Wall& wall = layout.wall(id);
            if (wall.position.x() == 2.0f && wall.position.y() == 0.0f)
            {
                facing_a = id;
            }
        }

        report_case("the facing on the shared edge was found", facing_a != nullid);

        layout.set_passable(facing_a, true);
        report_case("opening a facing lets a mover out", !layout.blocked(stratum_sim::Cell {1, 0}, stratum_sim::Cell {2, 0}));
        report_case("its partner is untouched, so the way back stays shut", layout.blocked(stratum_sim::Cell {2, 0}, stratum_sim::Cell {1, 0}));

        layout.set_passable(layout.wall(facing_a).partner, true);
        report_case("opening the partner opens the way back", !layout.blocked(stratum_sim::Cell {2, 0}, stratum_sim::Cell {1, 0}));

        // A facing spans a whole polygon edge, so one wall may cover several cell edges
        id_type region_c = layout.create_region(rectangle(0.0f, 4.0f, 2.0f, 6.0f), 0.0f, 2.5f);
        id_type region_d = layout.create_region(rectangle(2.0f, 4.0f, 4.0f, 6.0f), 0.0f, 2.5f);

        report_case_equal("a two cell boundary is still one facing pair", layout.connect_regions(region_c, region_d), 1);
        report_case("the lower cell edge of that boundary blocks", layout.blocked(stratum_sim::Cell {1, 4}, stratum_sim::Cell {2, 4}));
        report_case("the upper cell edge of that boundary blocks", layout.blocked(stratum_sim::Cell {1, 5}, stratum_sim::Cell {2, 5}));

        id_type facing_c = nullid;
        for (id_type id : layout.region_walls(region_c))
        {
            const stratum_sim::Wall& wall = layout.wall(id);
            if (wall.position.x() == 2.0f && wall.position.y() == 4.0f)
            {
                facing_c = id;
            }
        }

        layout.set_passable(facing_c, true);
        report_case("opening one facing opens every cell edge it covers",
            !layout.blocked(stratum_sim::Cell {1, 4}, stratum_sim::Cell {2, 4}) && !layout.blocked(stratum_sim::Cell {1, 5}, stratum_sim::Cell {2, 5}));

        return;
    }

    //--------------------------------------------------------------------------
    void test_scheduler()
    {
        report_group("scheduler");

        stratum_sim::Scheduler scheduler;

        report_case_equal("a new scheduler has no actors", scheduler.count(), 0);
        report_case("with nothing to schedule there is no next actor", scheduler.next() == nullid);

        scheduler.add(1, 100);
        report_case("an added actor is present", scheduler.contains(1));
        report_case_equal("count follows", scheduler.count(), 1);
        report_case_equal("it starts with no energy", scheduler.energy(1), 0);

        report_case("a lone actor is next", scheduler.next() == 1);
        report_case_equal("it waited one unit for a full action", scheduler.elapsed(), 1);

        scheduler.spend(1);
        report_case_equal("spending leaves it empty", scheduler.energy(1), 0);

        // Twice the speed acts twice as often
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 200);

        size_t acts_1 = 0;
        size_t acts_2 = 0;
        for (size_t n = 0; n < 30; ++n)
        {
            id_type actor = scheduler.next();
            acts_1 += actor == 1 ? 1 : 0;
            acts_2 += actor == 2 ? 1 : 0;
            scheduler.spend(actor);
        }

        report_case_equal("the faster actor acts twice as often", acts_2, acts_1 * 2);
        report_case_equal("and every turn went to one of them", acts_1 + acts_2, 30);

        // Equal speeds alternate, lowest id first
        scheduler.clear();
        scheduler.add(7, 100);
        scheduler.add(4, 100);

        id_type first = scheduler.next();
        scheduler.spend(first);
        id_type second = scheduler.next();
        scheduler.spend(second);

        report_case("equal actors break ties by id", first == 4 && second == 7);

        // A costlier action delays the next turn
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 100);

        id_type slow = scheduler.next();
        scheduler.spend(slow, stratum_sim::Scheduler::action_cost * 2);
        report_case("a double cost action yields the next two turns", scheduler.next() != slow);

        // Speed zero never accumulates
        scheduler.clear();
        scheduler.add(1, 0);
        report_case("an actor with no speed never acts", scheduler.next() == nullid);
        report_case("and time does not advance for it", scheduler.elapsed() == 0);

        scheduler.set_speed(1, 50);
        report_case("giving it speed lets it act", scheduler.next() == 1);
        report_case_equal("at half speed it waits two units", scheduler.elapsed(), 2);

        scheduler.remove(1);
        report_case("a removed actor is gone", !scheduler.contains(1) && scheduler.count() == 0);

        return;
    }

    //--------------------------------------------------------------------------
    void test_movement()
    {
        report_group("movement");

        // A corridor four cells long, walled at both ends
        stratum_sim::Layout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::Cell {8, 8});

        id_type corridor = layout.create_region(rectangle(0.0f, 0.0f, 4.0f, 1.0f), 0.0f, 2.5f);
        id_type beyond = layout.create_region(rectangle(4.0f, 0.0f, 5.0f, 1.0f), 0.0f, 2.5f);
        layout.connect_regions(corridor, beyond);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        stratum_sim::placement_type start(layout.cell_position(stratum_sim::Cell {0, 0}), east);

        stratum_sim::MoveResult one = layout.move(start, east, 1.0f);
        report_case("a step lands in the next cell", layout.cell_at(one.placement.position()) == stratum_sim::Cell(1, 0));
        report_case("an unobstructed step reports nothing", one.obstruction == nullid);
        report_case("moving does not turn", one.placement.direction().x() == 1.0f);

        stratum_sim::MoveResult three = layout.move(start, east, 3.0f);
        report_case("distance is in cells", layout.cell_at(three.placement.position()) == stratum_sim::Cell(3, 0));

        // The far wall of the corridor is connected but not passable
        stratum_sim::MoveResult far = layout.move(start, east, 7.0f);
        report_case("a mover stops at the first block", layout.cell_at(far.placement.position()) == stratum_sim::Cell(3, 0));
        report_case("and reports what stopped it", far.obstruction != nullid);
        report_case("the obstruction is a facing of the region it was in", layout.wall(far.obstruction).region == corridor);

        // Opening that facing lets it through
        layout.set_passable(far.obstruction, true);
        stratum_sim::MoveResult through = layout.move(start, east, 7.0f);
        report_case("an opened facing no longer stops it", layout.cell_at(through.placement.position()) == stratum_sim::Cell(4, 0));
        report_case("it stops at the next block instead", through.obstruction != nullid && through.obstruction != far.obstruction);

        // Rock in every other direction
        stratum_sim::MoveResult blocked = layout.move(start, north, 3.0f);
        report_case("a mover held against rock does not move", layout.cell_at(blocked.placement.position()) == stratum_sim::Cell(0, 0));
        report_case("and still reports the obstruction", blocked.obstruction != nullid);

        report_case("a zero direction moves nothing", layout.cell_at(layout.move(start, stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 3.0f).placement.position()) == stratum_sim::Cell(0, 0));
        report_case("a zero distance moves nothing", layout.cell_at(layout.move(start, east, 0.0f).placement.position()) == stratum_sim::Cell(0, 0));

        // Direction is snapped to one the layout owns
        stratum_sim::MoveResult skewed = layout.move(start, stratum_sim::vector_type(0.9f, 0.4f, 0.0f), 2.0f);
        report_case("a direction off axis snaps to the nearest", layout.cell_at(skewed.placement.position()) == stratum_sim::Cell(2, 0));

        // Turning
        stratum_sim::placement_type facing_east(start.position(), east);
        stratum_sim::placement_type left = layout.turn(facing_east, stratum_sim::Turn::Left);
        report_case("turning left from east faces north", left.direction().x() == 0.0f && left.direction().y() == 1.0f);
        report_case("turning does not move", layout.same_location(facing_east, left));

        stratum_sim::placement_type right = layout.turn(facing_east, stratum_sim::Turn::Right);
        report_case("turning right from east faces south", right.direction().x() == 0.0f && right.direction().y() == -1.0f);

        stratum_sim::placement_type around = layout.turn(facing_east, stratum_sim::Turn::Around);
        report_case("turning around from east faces west", around.direction().x() == -1.0f && around.direction().y() == 0.0f);
        report_case("four left turns return the original facing",
            layout.turn(layout.turn(layout.turn(left, stratum_sim::Turn::Left), stratum_sim::Turn::Left), stratum_sim::Turn::Left).direction().x() == 1.0f);

        // Absolute movement passes a direction the mover is not facing, and keeps its facing
        stratum_sim::placement_type facing_north(start.position(), north);
        stratum_sim::MoveResult sidestep = layout.move(facing_north, east, 2.0f);
        report_case("absolute movement ignores the mover's facing", layout.cell_at(sidestep.placement.position()) == stratum_sim::Cell(2, 0));
        report_case("and leaves that facing untouched", sidestep.placement.direction().y() == 1.0f);

        // same_location is by cell, not by exact position
        stratum_sim::placement_type a(stratum_sim::point_type(2.1f, 0.2f, 0.0f), east);
        stratum_sim::placement_type b(stratum_sim::point_type(2.9f, 0.8f, 0.0f), east);
        report_case("two points in one cell are the same location", layout.same_location(a, b));
        report_case("a point in the next cell is not", !layout.same_location(a, stratum_sim::placement_type(stratum_sim::point_type(3.5f, 0.5f, 0.0f), east)));

        return;
    }

    //--------------------------------------------------------------------------
    void test_adjacency_and_sight()
    {
        report_group("adjacency and sight");

        // A three by three room with a doorway off its east side, read so that facings are
        // one cell edge each. Hand built rectangles would give the room one facing spanning
        // three cells, which connect_regions cannot pair against the doorway's single one
        stratum_sim::Layout layout;
        stratum_sim::load_layout("data/room.level", layout);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto at = [&](int i, int j) { return stratum_sim::placement_type(layout.cell_position(stratum_sim::Cell {i, j}), east); };

        report_case_equal("the room and the doorway are two regions", layout.region_store_count(), 2);

        // Inside the room nothing separates cells, so a middle cell reaches all four
        report_case_equal("a cell amid open floor has four neighbours", layout.neighbors(at(2, 2)).size(), 4);
        report_case_equal("a corner cell is walled on two sides", layout.neighbors(at(1, 1)).size(), 2);
        report_case_equal("an edge cell is walled on one", layout.neighbors(at(2, 1)).size(), 3);

        // The reader opens what it connects, so the doorway is reachable
        report_case_equal("an open doorway is a neighbour", layout.neighbors(at(3, 2)).size(), 4);
        report_case("a neighbour keeps the mover's facing", layout.neighbors(at(2, 2)).front().direction().x() == 1.0f);

        id_type doorway = layout.facing_between(stratum_sim::Cell {3, 2}, stratum_sim::Cell {4, 2});
        report_case("the facing into the doorway is found", doorway != nullid);
        report_case("it has a partner on the doorway's side", layout.wall(doorway).partner != nullid);

        layout.set_passable(doorway, false);
        report_case_equal("closing it removes the neighbour", layout.neighbors(at(3, 2)).size(), 3);
        layout.set_passable(doorway, true);

        // Sight
        report_case("a cell sees itself", layout.visible(at(2, 2), at(2, 2)));
        report_case("open floor is visible across", layout.visible(at(1, 1), at(3, 3)));
        report_case("sight is blocked by rock", !layout.visible(at(2, 2), at(2, 4)));
        report_case("sight reaches through an open doorway", layout.visible(at(3, 2), at(4, 2)));

        layout.set_passable(doorway, false);
        report_case("closing it blocks sight too", !layout.visible(at(3, 2), at(4, 2)));

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_load_layout()
    {
        report_group("load layout");

        // A ring of eight open cells around one enclosed rock cell
        stratum_sim::Layout loop;
        stratum_sim::load_layout("data/loop.level", loop);

        report_case("the grid is the file's extent", loop.grid().extent == stratum_sim::Cell(5, 5));
        report_case_equal("contiguous cells merge into one region", loop.region_store_count(), 1);
        report_case_equal("a merged region has one floor and one ceiling", loop.flat_store_count(), 2);

        report_case_equal("the enclosure becomes a hole ring", loop.region(0).holes.size(), 1);
        report_case_equal("the outer ring is one facing per cell edge", loop.region_walls(0).size(), 12);
        report_case_equal("the hole ring encircles the one enclosed cell", loop.region_hole_walls(0, 0).size(), 4);
        report_case_equal("both rings account for every facing", loop.wall_store_count(), 16);

        // Nothing borders it, inside or out
        size_t rock = 0;
        for (const stratum_sim::Wall& wall : loop.wall_store())
        {
            rock += wall.partner == nullid ? 1 : 0;
        }

        report_case_equal("an isolated region is rock all round", rock, 16);
        report_case("cells across the ring are the same region", loop.region_at(stratum_sim::point_type(1.5f, 1.5f, 0.0f)) == loop.region_at(stratum_sim::point_type(3.5f, 3.5f, 0.0f)));

        report_case("an open cell locates", loop.region_at(stratum_sim::point_type(1.5f, 1.5f, 0.0f)) != nullid);
        report_case("the enclosed rock cell does not", loop.region_at(stratum_sim::point_type(2.5f, 2.5f, 0.0f)) == nullid);
        report_case("rock outside the ring does not", loop.region_at(stratum_sim::point_type(0.5f, 0.5f, 0.0f)) == nullid);

        report_case("adjacent open cells do not block", !loop.blocked(stratum_sim::Cell {1, 1}, stratum_sim::Cell {2, 1}));
        report_case("an open cell blocks against rock", loop.blocked(stratum_sim::Cell {1, 1}, stratum_sim::Cell {1, 0}));
        report_case("an open cell blocks against enclosed rock", loop.blocked(stratum_sim::Cell {2, 1}, stratum_sim::Cell {2, 2}));

        // The file's first line is the highest row, so the origin is lower left
        report_case("row order puts the origin at the lower left", loop.region_at(stratum_sim::point_type(3.5f, 1.5f, 0.0f)) != nullid);

        // A doorway is an ordinary open cell to the reader; it becomes its own region
        stratum_sim::Layout door;
        stratum_sim::load_layout("data/door.level", door);

        report_case_equal("a doorway reads as a region", door.region_store_count(), 3);
        report_case("a doorway connects both ways", !door.blocked(stratum_sim::Cell {1, 1}, stratum_sim::Cell {2, 1}) && !door.blocked(stratum_sim::Cell {2, 1}, stratum_sim::Cell {3, 1}));

        // A real generated level, for scale rather than for exact counts
        stratum_sim::Layout generated;
        stratum_sim::load_layout("data/generated.level", generated);

        report_case("a generated level reads", generated.region_store_count() > 0);
        report_case("its grid matches the file", generated.grid().extent == stratum_sim::Cell(31, 31));

        // 571 of its 961 cells are open, 32 of those doorways and shafts that never merge
        report_case("rooms and corridors merged", generated.region_store_count() < 571,
            format("{} regions", generated.region_store_count()));
        report_case("doorways and shafts did not", generated.region_store_count() >= 32);

        size_t connected = 0;
        for (const stratum_sim::Wall& wall : generated.wall_store())
        {
            connected += wall.partner != nullid ? 1 : 0;
        }

        report_case("a generated level is connected", connected > 0);
        report_case("connections are mutual", connected % 2 == 0);

        return;
    }

    //--------------------------------------------------------------------------
    void test_world()
    {
        report_group("world");

        stratum_sim::World world;

        report_case_equal("a new world has no zones", world.zone_count(), 0);

        flecs::entity crypt = world.create_zone("crypt");
        flecs::entity moor = world.create_zone("moor");

        report_case_equal("zones are created", world.zone_count(), 2);
        report_case("a zone is found by name", world.zone("crypt") == crypt);
        report_case("each zone has its own layout", &world.layout(crypt) != &world.layout(moor));

        stratum_sim::load_layout("data/room.level", world.layout(crypt));
        report_case("a zone's layout can be loaded", world.layout(crypt).region_store_count() == 2);
        report_case("the other stays empty", world.layout(moor).region_store_count() == 0);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::placement_type where(world.layout(crypt).cell_position(stratum_sim::Cell {2, 2}), east);

        flecs::entity monster = world.create_entity(crypt, where);
        report_case("an entity is placed in its zone", world.zone_of(monster) == crypt);
        report_case("and carries its placement", monster.try_get<stratum_sim::placement_type>() != nullptr);

        // Migration keeps the id, which is what quests and leashes depend on
        flecs::entity_t id_before = monster.id();
        world.relocate(monster, moor, where);

        report_case("relocating moves it to the other zone", world.zone_of(monster) == moor);
        report_case("its id is unchanged", monster.id() == id_before);
        report_case("it is no longer in the old zone", !monster.has(flecs::ChildOf, crypt));

        world.relocate(monster, crypt, where);
        world.unload_zone(crypt);

        report_case("unloading a zone destroys its contents", !monster.is_alive());
        report_case_equal("and drops its layout", world.zone_count(), 1);
        report_case("the other zone survives", world.zone("moor").is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    void test_commands()
    {
        report_group("commands");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt");
        stratum_sim::load_layout("data/room.level", world.layout(zone));

        stratum_sim::Layout& layout = world.layout(zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.cell_position(stratum_sim::Cell {1, 2}), east));

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_at(actor.get<stratum_sim::placement_type>().position()); };

        // A step along the actor's own facing
        commands.push(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 1.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a step moves the actor", cell_of() == stratum_sim::Cell(2, 2));
        report_case_equal("and reports one event", events.size(), 1);
        report_case("which is a move", events.front().is<stratum_sim::Moved>());
        report_case("naming the actor", events.front().get<stratum_sim::Moved>().entity_id == actor.id());
        report_case("commands are consumed", commands.empty());

        events.clear();

        // Walking into rock reports the obstruction and does not move
        commands.push(actor.id(), stratum_sim::Step {north, 5.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a blocked step does not move", cell_of() == stratum_sim::Cell(2, 3));
        report_case("it moved as far as it could", events.front().is<stratum_sim::Moved>());
        report_case("and reported what stopped it", events.events().back().is<stratum_sim::Blocked>());
        report_case("the obstruction is a wall of the layout", events.events().back().get<stratum_sim::Blocked>().wall_id != nullid);

        events.clear();

        // Held against rock, nothing happens and nothing is spent
        stratum_sim::Command held(actor.id(), stratum_sim::Step {north, 1.0f});
        report_case_equal("a step that cannot move costs nothing", static_cast<size_t>(stratum_sim::apply(world, held, events)), 0);
        report_case("and reports only the block", events.size() == 1 && events.front().is<stratum_sim::Blocked>());

        events.clear();

        // Turning
        commands.push(actor.id(), stratum_sim::TurnTo {stratum_sim::Turn::Right});
        stratum_sim::apply(world, commands, events);

        report_case("turning right from east faces south", actor.get<stratum_sim::placement_type>().direction().y() == -1.0f);
        report_case("and reports a turn", events.front().is<stratum_sim::Turned>());
        report_case("turning does not move", cell_of() == stratum_sim::Cell(2, 3));

        events.clear();

        // Interact with what the actor faces
        stratum_sim::Command interact_rock(actor.id(), stratum_sim::Interact {});
        report_case_equal("interacting with open floor does nothing", static_cast<size_t>(stratum_sim::apply(world, interact_rock, events)), 0);
        report_case("and reports nothing", events.empty());

        // Face the doorway and interact
        world.entities().entity(actor.id()).set<stratum_sim::placement_type>(stratum_sim::placement_type(layout.cell_position(stratum_sim::Cell {3, 2}), east));
        stratum_sim::Command interact_door(actor.id(), stratum_sim::Interact {});

        report_case("interacting with a facing costs an action", stratum_sim::apply(world, interact_door, events) == stratum_sim::Scheduler::action_cost);
        report_case("and names the facing", events.front().is<stratum_sim::Interacted>() && events.front().get<stratum_sim::Interacted>().wall_id != nullid);

        events.clear();

        // A command for an entity that no longer exists is ignored
        stratum_sim::Command orphan(9999, stratum_sim::Step {east, 1.0f});
        report_case_equal("a command for a dead entity costs nothing", static_cast<size_t>(stratum_sim::apply(world, orphan, events)), 0);
        report_case("and produces no events", events.empty());

        return;
    }

    //--------------------------------------------------------------------------
    void test_occupancy()
    {
        report_group("occupancy");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt");
        stratum_sim::load_layout("data/corridor.level", world.layout(zone));

        stratum_sim::Layout& layout = world.layout(zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.cell_position(stratum_sim::Cell {i, j}), east); };

        flecs::entity a = world.create_entity(zone, place(1, 1));
        flecs::entity b = world.create_entity(zone, place(1, 1));
        flecs::entity c = world.create_entity(zone, place(4, 1));

        report_case_equal("two entities share a cell", world.entities_at(zone, stratum_sim::Cell {1, 1}).size(), 2);
        report_case("and are returned in id order", world.entities_at(zone, stratum_sim::Cell {1, 1}).front() == a);
        report_case_equal("an empty cell holds none", world.entities_at(zone, stratum_sim::Cell {3, 1}).size(), 0);
        report_case_equal("a distant one is elsewhere", world.entities_at(zone, stratum_sim::Cell {4, 1}).size(), 1);

        report_case_equal("range reaches its own cell", world.entities_within(zone, place(1, 1), 0.5f).size(), 2);
        report_case_equal("a wider range reaches further", world.entities_within(zone, place(1, 1), 4.0f).size(), 3);
        report_case("the far entity is the third", world.entities_within(zone, place(1, 1), 4.0f).back() == c);

        // Occupancy is per zone
        flecs::entity moor = world.create_zone("moor");
        stratum_sim::load_layout("data/corridor.level", world.layout(moor));
        world.create_entity(moor, place(1, 1));

        report_case_equal("another zone's entities do not appear", world.entities_at(zone, stratum_sim::Cell {1, 1}).size(), 2);
        report_case_equal("they appear in their own", world.entities_at(moor, stratum_sim::Cell {1, 1}).size(), 1);

        b.destruct();
        report_case_equal("a destroyed entity stops occupying", world.entities_at(zone, stratum_sim::Cell {1, 1}).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    // Phase 1's claim: a dungeon read from a file is walkable, driven entirely by
    // commands ordered by the scheduler, with no rendering
    void test_walk()
    {
        report_group("walking a dungeon");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt");
        stratum_sim::load_layout("data/corridor.level", world.layout(zone));

        stratum_sim::Layout& layout = world.layout(zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.cell_position(stratum_sim::Cell {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(actor.id(), 100);

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_at(actor.get<stratum_sim::placement_type>().position()); };

        report_case("the actor starts at the west end", cell_of() == stratum_sim::Cell(1, 1));

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

        report_case("it walked to the east end", cell_of() == stratum_sim::Cell(5, 1));
        report_case_equal("taking one step per open cell", steps, 4);
        report_case("and reported being stopped at the wall", blocks > 0);
        report_case("the scheduler advanced through the walk", scheduler.elapsed() >= 8);

        // Turning and walking back
        events.clear();
        stratum_sim::Command about(actor.id(), stratum_sim::TurnTo {stratum_sim::Turn::Around});
        stratum_sim::apply(world, about, events);
        report_case("turning around faces west", actor.get<stratum_sim::placement_type>().direction().x() == -1.0f);

        events.clear();
        stratum_sim::Command back(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 4.0f});
        stratum_sim::apply(world, back, events);

        report_case("walking back reaches the west end", cell_of() == stratum_sim::Cell(1, 1));
        report_case("in a single command", events.size() == 1 && events.front().is<stratum_sim::Moved>());

        return;
    }

    //--------------------------------------------------------------------------
    // Covers geometry::Polygon's Cartesian containment path, which sim depends on and
    // which nothing else exercises. Belongs in geometry_test once that has a harness
    void test_polygon_containment()
    {
        report_group("polygon containment, cartesian traits");

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

        report_case("an interior point is contained", xy.contains(stratum_sim::point_type(2.0f, 1.5f, 0.0f)));
        report_case("an exterior point in the plane is not", !xy.contains(stratum_sim::point_type(9.0f, 1.5f, 0.0f)));
        report_case("a point off the plane is not", !xy.contains(stratum_sim::point_type(2.0f, 1.5f, 1.0f)));

        // Normal along y, so the dropped axis is 1
        stratum_sim::polygon_type xz = polygon_of({
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 0.0f, 0.0f),
            stratum_sim::point_type(4.0f, 0.0f, 3.0f),
            stratum_sim::point_type(0.0f, 0.0f, 3.0f)});

        report_case("an interior point of an xz polygon is contained", xz.contains(stratum_sim::point_type(2.0f, 0.0f, 1.5f)));
        report_case("an exterior point of an xz polygon is not", !xz.contains(stratum_sim::point_type(9.0f, 0.0f, 1.5f)));

        // Normal along x, so the dropped axis is 0
        stratum_sim::polygon_type yz = polygon_of({
            stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            stratum_sim::point_type(0.0f, 4.0f, 0.0f),
            stratum_sim::point_type(0.0f, 4.0f, 3.0f),
            stratum_sim::point_type(0.0f, 0.0f, 3.0f)});

        report_case("an interior point of a yz polygon is contained", yz.contains(stratum_sim::point_type(0.0f, 2.0f, 1.5f)));
        report_case("an exterior point of a yz polygon is not", !yz.contains(stratum_sim::point_type(0.0f, 9.0f, 1.5f)));

        // Facing no axis in particular, the case axis selection exists for
        stratum_sim::polygon_type tilted = polygon_of({
            stratum_sim::point_type(1.0f, 0.0f, 0.0f),
            stratum_sim::point_type(0.0f, 1.0f, 0.0f),
            stratum_sim::point_type(0.0f, 0.0f, 1.0f)});

        report_case("an interior point of a tilted polygon is contained", tilted.contains(stratum_sim::point_type(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f)));
        report_case("an exterior point of a tilted polygon is not", !tilted.contains(stratum_sim::point_type(1.0f, 1.0f, -1.0f)));

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_command_queue();
    test_event_queue();
    test_level_layout();
    test_partner_linking();
    test_wall_fold();
    test_point_location();
    test_grid();
    test_blocking();
    test_scheduler();
    test_movement();
    test_adjacency_and_sight();
    test_load_layout();
    test_world();
    test_commands();
    test_occupancy();
    test_walk();
    test_polygon_containment();

    return report_results();
}
