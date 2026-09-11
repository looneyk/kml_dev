//------------------------------------------------------------------------------
// stratum_sim_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_sim/Command.hpp"
#include "quetzal/stratum_sim/CommandQueue.hpp"
#include "quetzal/stratum_sim/Event.hpp"
#include "quetzal/stratum_sim/EventQueue.hpp"
#include "quetzal/stratum_sim/Intent.hpp"
#include "quetzal/stratum_sim/ContinuousLayout.hpp"
#include "quetzal/stratum_sim/DiscreteLayout.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/Clock.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/ai.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;
using namespace quetzal;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

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
    // A generator in place of an application's, this library linking none of the
    // real ones. It builds the geometry `data/door.level` describes rather than reading it,
    // so what is exercised is the seam rather than the reader behind it
    class TestGenerator : public stratum_sim::Generator
    {
    public:

        std::unique_ptr<stratum_sim::Layout> generate(const stratum_sim::door_type& door) const override
        {
            auto layout = make_unique<stratum_sim::DiscreteLayout>();

            layout->set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f,
                stratum_sim::discrete_vector_type(5, 3));

            for (stratum_sim::coord_type i = 1; i <= 3; ++i)
            {
                layout->set_cell_open(stratum_sim::discrete_point_type {i, 1}, true);
            }

            // The three open cells as one area, and the doorway block ringed inside it, which
            // is what the block format's loader builds
            // A vertex per cell corner, so a wall is one cell edge as the loader builds them
            stratum_sim::points_type outline
            {
                {1.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {3.0f, 1.0f, 0.0f}, {4.0f, 1.0f, 0.0f},
                {4.0f, 2.0f, 0.0f}, {3.0f, 2.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, {1.0f, 2.0f, 0.0f}
            };

            stratum_sim::points_type ring
            {
                {3.0f, 2.0f, 0.0f}, {3.0f, 1.0f, 0.0f}, {2.0f, 1.0f, 0.0f}, {2.0f, 2.0f, 0.0f}
            };

            vector<stratum_sim::polygon_type> polygons
            {
                stratum_sim::polygon_type(outline.begin(), outline.end()),
                stratum_sim::polygon_type(ring.begin(), ring.end())
            };

            stratum_sim::DiscreteLayout::ids_type starts = layout->create_walls(polygons);

            // The ring's two faces toward floor, which is where a door on a block goes
            if (door)
            {
                door({starts[1] + 0, starts[1] + 2});
            }

            return layout;
        }

        stratum_sim::Format format() const override
        {
            return stratum_sim::Format::Block;
        }
    };

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

    // Substitute command types, sim defines no commands of its own
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

    // Substitute event types, sim defines no events of its own
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
    // The same rectangle outlined one cell edge at a time, as a level file produces it, so
    // that a wall and the cell edge it lies on run end for end. What cell queries need
    stratum_sim::polygon_type cell_rectangle(int i0, int j0, int i1, int j1)
    {
        stratum_sim::points_type points;

        auto add = [&points](int i, int j)
        {
            points.push_back(stratum_sim::point_type(static_cast<float>(i), static_cast<float>(j), 0.0f));
        };

        for (int i = i0; i < i1; ++i) { add(i, j0); }
        for (int j = j0; j < j1; ++j) { add(i1, j); }
        for (int i = i1; i > i0; --i) { add(i, j1); }
        for (int j = j1; j > j0; --j) { add(i0, j); }

        return stratum_sim::polygon_type(points.begin(), points.end());
    }

    //--------------------------------------------------------------------------
    // Marks a hand built layout's cells open, a loaded one taking this from the file. Nowhere
    // to go stops a mover as a wall does, so an unmarked layout is closed throughout
    void mark_open(stratum_sim::DiscreteLayout& layout, stratum_sim::coord_type i0, stratum_sim::coord_type j0,
        stratum_sim::coord_type i1, stratum_sim::coord_type j1)
    {
        for (stratum_sim::coord_type j = j0; j < j1; ++j)
        {
            for (stratum_sim::coord_type i = i0; i < i1; ++i)
            {
                layout.set_cell_open(stratum_sim::discrete_point_type {i, j}, true);
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A layout on its own, with what the file called doors set crossable. A layout asks rather
    // than looks and nothing has attached anything to these walls, so this substitutes for the
    // door entities World builds and asks -- and for what Wall::passable used to encode
    std::unique_ptr<stratum_sim::DiscreteLayout> load_open(const string& filepath)
    {
        std::set<id_type> doors;

        auto layout = stratum_sim::load_layout(filepath, 1.0f, [&](const std::vector<id_type>& walls)
        {
            doors.insert(walls.begin(), walls.end());
        });

        layout->set_crossable([doors](id_type wall_id) { return doors.contains(wall_id); });

        return layout;
    }

    //--------------------------------------------------------------------------
    // A zone's layout as the kind it is. World returns the interface, and most of what
    // is checked here is cells and walls, which live below it
    stratum_sim::DiscreteLayout& zone_layout(stratum_sim::World& world, flecs::entity zone)
    {
        return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
    }

    //--------------------------------------------------------------------------
    // One outline's walls, which run contiguously from the wall create_walls returned for it
    stratum_sim::DiscreteLayout::ids_type outline_walls(id_type wall_id0, size_t vertex_count)
    {
        stratum_sim::DiscreteLayout::ids_type ids;
        ids.reserve(vertex_count);

        for (size_t i = 0; i < vertex_count; ++i)
        {
            ids.push_back(wall_id0 + static_cast<id_type>(i));
        }

        return ids;
    }

    //--------------------------------------------------------------------------
    // The walls of one outline, built and returned in one step, as most structural tests need
    stratum_sim::DiscreteLayout::ids_type build_outline(stratum_sim::DiscreteLayout& layout, const stratum_sim::polygon_type& outline)
    {
        return outline_walls(layout.create_walls({outline}).front(), outline.vertex_count());
    }

    //--------------------------------------------------------------------------
    // A Room over a polygon outline, its wall ids left unset, which is all containment needs
    stratum_sim::Room room_of(const stratum_sim::polygon_type& outline)
    {
        stratum_sim::Room room;

        for (size_t i = 0; i < outline.vertex_count(); ++i)
        {
            stratum_sim::Room::Vertex vertex;
            vertex.position = stratum_sim::point_type(outline.vertex(i).x(), outline.vertex(i).y(), 0.0f);
            room.outline().push_back(vertex);
        }

        return room;
    }

    //--------------------------------------------------------------------------
    // The closed outlines the walls form, by length. Every wall belongs to exactly one, so
    // this is what an area's boundary count is read from now that no record stores it
    vector<size_t> outline_lengths(const stratum_sim::DiscreteLayout& layout)
    {
        vector<bool> seen(layout.wall_store_count(), false);
        vector<size_t> lengths;

        for (id_type id0 = 0; id0 < layout.wall_store_count(); ++id0)
        {
            if (seen[id0])
            {
                continue;
            }

            size_t n = 0;
            id_type id = id0;

            do
            {
                seen[id] = true;
                ++n;
                id = layout.wall(id).next_id;
            } while (id != nullid && id != id0);

            lengths.push_back(n);
        }

        return lengths;
    }

    //--------------------------------------------------------------------------
    // How many of the walls are border walls, a nullid partner being nothing behind it
    size_t border_wall_count(const stratum_sim::DiscreteLayout& layout, const stratum_sim::DiscreteLayout::ids_type& ids)
    {
        size_t n = 0;
        for (id_type id : ids)
        {
            n += layout.wall(id).partner_id == nullid ? 1 : 0;
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
        report_case_equal("a command names its entity id", command.entity_id(), 7);
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
        report_case_equal("an event payload has its own fields", event.get<EntityMoved>().entity_id, 7);
        report_case("try_get is null for another event type", event.try_get<DoorOpened>() == nullptr);

        queue.pop();
        report_case("pop removes the front event", queue.front().is<DoorOpened>());
        report_case_equal("pop shrinks the queue", queue.size(), 1);

        queue.push(EntityMoved {9, 0, 0});
        report_case("events iterate in occurrence order", queue.events().front().is<DoorOpened>() && queue.events().back().is<EntityMoved>());

        // Guards the constrained converting constructor, an unconstrained one would win
        // over the copy constructor here and produce an Event containing an Event
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
    // The layouts that are declared and not written. Compiling their headers at all is most of
    // what this checks; the cases state the two properties a skeleton promises
    void test_layout_hierarchy()
    {
        report_group("layout hierarchy");

        report_case("a discrete layout is a Layout",
            is_base_of_v<stratum_sim::Layout, stratum_sim::DiscreteLayout>);
        report_case("and can be built, being the one that is written",
            !is_abstract_v<stratum_sim::DiscreteLayout>);

        report_case("a continuous layout is a Layout too",
            is_base_of_v<stratum_sim::Layout, stratum_sim::ContinuousLayout>);
        report_case("and cannot be built, nothing behind it being written",
            is_abstract_v<stratum_sim::ContinuousLayout>);

        return;
    }

    //--------------------------------------------------------------------------
    void test_level_layout()
    {
        report_group("level layout");

        stratum_sim::DiscreteLayout layout;
        stratum_sim::polygon_type outline = rectangle(0.0f, 0.0f, 4.0f, 3.0f);

        stratum_sim::DiscreteLayout::ids_type walls = build_outline(layout, outline);

        report_case_equal("one wall per outline edge", layout.wall_store_count(), outline.vertex_count());
        report_case_equal("the outline's walls run contiguously from its start", walls.size(), outline.vertex_count());
        report_case_equal("new walls are all border walls", border_wall_count(layout, walls), walls.size());

        bool outline_ordered = true;
        bool prev_inverse = true;
        for (size_t i = 0; i < walls.size(); ++i)
        {
            const stratum_sim::Wall& wall = layout.wall(walls[i]);
            outline_ordered = outline_ordered
                && wall.position.x() == outline.vertex(i).x()
                && wall.position.y() == outline.vertex(i).y();
            prev_inverse = prev_inverse && layout.wall(wall.next_id).prev_id == walls[i];
        }

        report_case("wall order follows the outline", outline_ordered);
        report_case("prev is the inverse of next", prev_inverse);
        report_case("the last wall closes back on the first", layout.wall(walls.back()).next_id == walls.front());

        // Flats, which bound the layout rather than any one area within it
        layout.set_flats(0.0f, 2.5f);
        report_case_equal("a layout has two flats", layout.flat_store_count(), 2);
        report_case("the floor is not a ceiling", !layout.flat(layout.floor_id()).ceiling());
        report_case("the ceiling is a ceiling", layout.flat(layout.ceiling_id()).ceiling());
        report_case("the floor keeps its height", layout.flat(layout.floor_id()).height() == 0.0f);
        report_case("the ceiling keeps its height", layout.flat(layout.ceiling_id()).height() == 2.5f);

        layout.clear();
        report_case("clear empties every store", layout.wall_store_count() == 0 && layout.room_store_count() == 0 && layout.flat_store_count() == 0);
        report_case("and forgets its flats", layout.floor_id() == nullid && layout.ceiling_id() == nullid);

        return;
    }

    //--------------------------------------------------------------------------
    void test_partner_linking()
    {
        report_group("partner linking");

        stratum_sim::DiscreteLayout layout;

        // Sharing the edge at x == 4, traversed in opposite directions by each outline
        stratum_sim::DiscreteLayout::ids_type walls_a = build_outline(layout, rectangle(0.0f, 0.0f, 4.0f, 3.0f));
        stratum_sim::DiscreteLayout::ids_type walls_b = build_outline(layout, rectangle(4.0f, 0.0f, 8.0f, 3.0f));
        stratum_sim::DiscreteLayout::ids_type walls_far = build_outline(layout, rectangle(20.0f, 0.0f, 24.0f, 3.0f));

        report_case_equal("every wall starts as a border wall", border_wall_count(layout, walls_a) + border_wall_count(layout, walls_b), 8);

        report_case_equal("adjacent outlines share exactly one edge", layout.connect_walls(walls_a, walls_b), 1);
        report_case_equal("only the shared wall of one outline stops being a border wall", border_wall_count(layout, walls_a), 3);
        report_case_equal("only the shared wall of the other does too", border_wall_count(layout, walls_b), 3);

        id_type connected_a = nullid;
        for (id_type id : walls_a)
        {
            if (layout.wall(id).partner_id != nullid)
            {
                connected_a = id;
            }
        }

        report_case("a connected wall was found", connected_a != nullid);

        id_type connected_b = connected_a == nullid ? nullid : layout.wall(connected_a).partner_id;
        report_case("partners name each other", connected_b != nullid && layout.wall(connected_b).partner_id == connected_a);
        report_case("a connected pair is opposed", layout.opposed(connected_a, connected_b));
        report_case("opposition is symmetric", layout.opposed(connected_b, connected_a));
        report_case("a wall is never opposed to itself", !layout.opposed(connected_a, connected_a));

        report_case_equal("connecting again finds nothing left to do", layout.connect_walls(walls_a, walls_b), 0);
        report_case_equal("outlines that share no edge connect nothing", layout.connect_walls(walls_a, walls_far), 0);

        layout.disconnect(connected_a);
        report_case_equal("disconnect returns one outline's wall to a border wall", border_wall_count(layout, walls_a), 4);
        report_case_equal("disconnect returns the partner to one too", border_wall_count(layout, walls_b), 4);

        return;
    }

    //--------------------------------------------------------------------------
    void test_wall_fold()
    {
        report_group("wall fold");

        stratum_sim::DiscreteLayout layout;

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

        stratum_sim::DiscreteLayout::ids_type walls = build_outline(layout, stratum_sim::polygon_type(points.begin(), points.end()));

        report_case_equal("a spur outline produces one wall per edge", walls.size(), 7);
        report_case_equal("the outline closes over a spur", walls.size(), points.size());
        report_case_equal("every wall starts as a border wall", border_wall_count(layout, walls), 7);

        report_case_equal("a fold connects exactly one pair", layout.connect_folds(walls), 1);
        report_case_equal("only the partition's two faces stop being border walls", border_wall_count(layout, walls), 5);

        // The front face runs from the north wall down to the tip
        id_type face_front = nullid;
        for (id_type id : walls)
        {
            const stratum_sim::Wall& wall = layout.wall(id);
            if (wall.position.x() == 5.0f && wall.position.y() == 6.0f && wall.partner_id != nullid)
            {
                face_front = id;
            }
        }

        report_case("the connected wall is the partition's front face", face_front != nullid);

        bool folds_at_tip = false;
        bool faces_are_partners = false;
        if (face_front != nullid)
        {
            const stratum_sim::Wall& front = layout.wall(face_front);
            id_type face_rear = front.partner_id;

            // The fold: walking next off the end of one face arrives on the other
            folds_at_tip = front.next_id == face_rear;
            faces_are_partners = layout.wall(face_rear).partner_id == face_front
                && layout.wall(face_rear).position.y() == 2.0f;
        }

        report_case("the walk folds at the tip onto the other face", folds_at_tip);
        report_case("the two faces of the partition are partners", faces_are_partners);

        layout.disconnect(face_front);
        report_case_equal("disconnect restores the partition to border walls", border_wall_count(layout, walls), 7);

        return;
    }

    //--------------------------------------------------------------------------
    void test_point_location()
    {
        report_group("point location");

        stratum_sim::DiscreteLayout layout;
        stratum_sim::polygon_type outline = rectangle(0.0f, 0.0f, 4.0f, 3.0f);

        id_type room_a = layout.create_room(room_of(outline));
        id_type room_b = layout.create_room(room_of(rectangle(4.0f, 0.0f, 8.0f, 3.0f)));

        const stratum_sim::Room& room = layout.room(room_a);
        report_case_equal("a room keeps one outline vertex per edge", room.vertex_count(), outline.vertex_count());

        bool vertices_match = room.vertex_count() == outline.vertex_count();
        for (size_t i = 0; i < room.vertex_count() && vertices_match; ++i)
        {
            vertices_match = room.vertex(i).position.x() == outline.vertex(i).x()
                && room.vertex(i).position.y() == outline.vertex(i).y();
        }

        report_case("a room's outline keeps the order it was given", vertices_match);

        report_case("a point inside a room finds it", layout.room_at(stratum_sim::point_type(2.0f, 1.5f, 0.0f)) == room_a);
        report_case("a point in the other room finds that one", layout.room_at(stratum_sim::point_type(6.0f, 1.5f, 0.0f)) == room_b);
        report_case("a point outside every room finds none", layout.room_at(stratum_sim::point_type(50.0f, 50.0f, 0.0f)) == nullid);

        // Height is disregarded, since an outline is horizontal and a mover stands above it
        report_case("a point at floor height still locates", layout.room_at(stratum_sim::point_type(2.0f, 1.5f, 2.0f)) == room_a);

        // A room with a partition hanging into it is still one room on both sides of it
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

        id_type hall = layout.create_room(room_of(stratum_sim::polygon_type(points.begin(), points.end())));

        report_case("a point west of a partition is in the hall", layout.room_at(stratum_sim::point_type(2.0f, 14.0f, 0.0f)) == hall);
        report_case("a point east of the same partition is in the same hall", layout.room_at(stratum_sim::point_type(8.0f, 14.0f, 0.0f)) == hall);
        report_case_equal("a spur outline keeps its repeated vertex", layout.room(hall).vertex_count(), 7);

        // Cells are the other half of containment, and the one a view asks
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8));

        stratum_sim::Room cellar = room_of(rectangle(0.0f, 0.0f, 2.0f, 2.0f));
        cellar.add_cell(stratum_sim::discrete_point_type {0, 0});
        cellar.add_cell(stratum_sim::discrete_point_type {1, 0});
        id_type cellar_id = layout.create_room(cellar);

        report_case("a room reports the cells it covers", layout.room(cellar_id).contains(stratum_sim::discrete_point_type {1, 0}));
        report_case("and not one it does not", !layout.room(cellar_id).contains(stratum_sim::discrete_point_type {5, 5}));
        report_case("creating a room records its cells", layout.room_at(stratum_sim::discrete_point_type {0, 0}) == cellar_id);
        report_case("a cell no room covers has none", layout.room_at(stratum_sim::discrete_point_type {5, 5}) == nullid);

        return;
    }

    //--------------------------------------------------------------------------
    void test_grid()
    {
        report_group("grid");

        stratum_sim::DiscreteLayout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8));

        report_case("a point maps to its cell", layout.grid_position_at(stratum_sim::point_type(2.7f, 3.2f, 0.0f)) == stratum_sim::discrete_point_type(2, 3));
        report_case("a cell maps back to its center", layout.position(stratum_sim::discrete_point_type {2, 3}).x() == 2.5f);
        report_case("round tripping a cell center returns the cell", layout.grid_position_at(layout.position(stratum_sim::discrete_point_type {5, 6})) == stratum_sim::discrete_point_type(5, 6));
        report_case("a point left of the origin lands in a negative cell", layout.grid_position_at(stratum_sim::point_type(-0.5f, 0.5f, 0.0f)) == stratum_sim::discrete_point_type(-1, 0));

        return;
    }

    //--------------------------------------------------------------------------
    void test_flats()
    {
        report_group("flats");

        // A constant is what a level with no height data loads, and it applies everywhere
        stratum_sim::Flat floor(2.0f);

        report_case("a flat is a floor unless it says otherwise", !floor.ceiling());
        report_case("an unsampled flat is not sampled", !floor.sampled());
        report_case("and answers its constant anywhere", floor.height(stratum_sim::point_type(100.0f, -50.0f, 0.0f)) == 2.0f);

        floor.set_height(3.0f);
        report_case("moving the constant moves the whole surface", floor.height(stratum_sim::point_type(1.0f, 1.0f, 0.0f)) == 3.0f);

        // Sampling is the same record with more of its features used
        stratum_sim::GridMapping sampling {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(4, 4)};
        floor.set_sampling(sampling);

        report_case("sampling starts every sample at the constant", floor.sampled() && floor.sample(stratum_sim::discrete_point_type {2, 2}) == 3.0f);
        report_case("and the surface still answers by position", floor.height(stratum_sim::point_type(2.5f, 2.5f, 0.0f)) == 3.0f);

        floor.set_sample(stratum_sim::discrete_point_type {1, 1}, 5.0f);
        report_case("a sample can be raised", floor.height(stratum_sim::point_type(1.5f, 1.5f, 0.0f)) == 5.0f);
        report_case("leaving its neighbors where they were", floor.height(stratum_sim::point_type(2.5f, 1.5f, 0.0f)) == 3.0f);

        // Absent is somewhere the surface is not, rather than somewhere it is low
        floor.set_sample(stratum_sim::discrete_point_type {3, 3}, {});
        report_case("a sample can be absent, which is a pit rather than a depth", !floor.height(stratum_sim::point_type(3.5f, 3.5f, 0.0f)));
        report_case("a position off the sampling grid is off the surface", !floor.height(stratum_sim::point_type(9.0f, 9.0f, 0.0f)));

        // The sampling grid is the surface's own and need not be the movement grid
        stratum_sim::Flat fine(0.0f);
        fine.set_sampling(stratum_sim::GridMapping {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 0.5f, stratum_sim::discrete_vector_type(8, 8)});
        fine.set_sample(stratum_sim::discrete_point_type {1, 0}, 1.0f);

        report_case("a height map may be finer than the cells", fine.height(stratum_sim::point_type(0.75f, 0.25f, 0.0f)) == 1.0f
            && fine.height(stratum_sim::point_type(0.25f, 0.25f, 0.0f)) == 0.0f);

        // A cell sits on the floor, which is the one query in the layout that reads a flat
        stratum_sim::DiscreteLayout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8));

        report_case("with no floor a cell sits on the grid's own plane", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 0.0f);

        layout.set_flats(1.5f, 4.0f);
        report_case("with one it sits on the floor", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 1.5f);

        layout.flat(layout.floor_id()).set_sampling(sampling);
        layout.flat(layout.floor_id()).set_sample(stratum_sim::discrete_point_type {1, 1}, 2.5f);
        report_case("and follows it where it is sampled", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 2.5f);

        layout.flat(layout.floor_id()).set_sample(stratum_sim::discrete_point_type {1, 1}, {});
        report_case("a cell the floor is absent under is back on the grid's plane",
            layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 0.0f);

        return;
    }

    //--------------------------------------------------------------------------
    void test_blocking()
    {
        report_group("blocking");

        stratum_sim::DiscreteLayout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8));

        // Two rooms one cell tall, so their shared boundary is exactly one cell edge
        stratum_sim::DiscreteLayout::ids_type walls_a = build_outline(layout, rectangle(0.0f, 0.0f, 2.0f, 1.0f));
        stratum_sim::DiscreteLayout::ids_type walls_b = build_outline(layout, rectangle(2.0f, 0.0f, 4.0f, 1.0f));
        mark_open(layout, 0, 0, 4, 1);

        // Within one outline there is no wall at all, so nothing can block
        report_case("cells inside one area never block", !layout.blocked(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::East)));
        report_case("blocking within an area is symmetric", !layout.blocked(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::West)));

        // Across the boundary the two are still solid, so the wall blocks
        report_case("an unconnected boundary blocks", layout.blocked(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East)));

        report_case_equal("the two areas share one wall pair", layout.connect_walls(walls_a, walls_b), 1);
        report_case("a connected but impassable wall still blocks", layout.blocked(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East)));

        // Open one side only, which is what a one-way passage is
        id_type wall_a = nullid;
        for (id_type id : walls_a)
        {
            const stratum_sim::Wall& wall = layout.wall(id);
            if (wall.position.x() == 2.0f && wall.position.y() == 0.0f)
            {
                wall_a = id;
            }
        }

        report_case("the wall on the shared edge was found", wall_a != nullid);

        // What may be crossed is asked of the layout rather than stored on the wall, so a bare
        // layout uses whatever is set here. World asks the attachments instead
        std::set<id_type> crossable {wall_a};
        layout.set_crossable([&](id_type id) { return crossable.contains(id); });

        report_case("opening a wall lets a mover out", !layout.blocked(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East)));
        report_case("its partner is untouched, so the way back stays shut", layout.blocked(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West)));

        crossable.insert(layout.wall(wall_a).partner_id);
        report_case("opening the partner opens the way back", !layout.blocked(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West)));

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

        // advance and ready are what a scheme other than turn-based uses: time is granted
        // rather than skipped to, so a caller can stop partway and find nobody ready
        scheduler.clear();
        scheduler.add(1, 100);
        scheduler.add(2, 25);

        report_case("nobody is ready before any time is granted", scheduler.ready() == nullid);

        scheduler.advance(1);
        report_case_equal("granting a unit advances elapsed", scheduler.elapsed(), 1);
        report_case("the fast actor is ready", scheduler.ready() == 1);
        report_case_equal("and the slow one has only part of a turn", scheduler.energy(2), 25);

        scheduler.spend(1);
        report_case("after spending, nobody is ready", scheduler.ready() == nullid);

        scheduler.advance(3);
        report_case_equal("the slow actor reaches a full turn in its own time", scheduler.energy(2), 100);
        report_case("but the fast one, having banked three, is readier", scheduler.ready() == 1);
        report_case_equal("with elapsed counting every granted unit", scheduler.elapsed(), 4);

        // next is those two composed, so it arrives where granting one at a time would
        scheduler.clear();
        scheduler.add(1, 25);

        stratum_sim::Scheduler granted;
        granted.add(1, 25);
        for (int i = 0; i < 4; ++i)
        {
            granted.advance(1);
        }

        report_case("next agrees with granting a unit at a time",
            scheduler.next() == granted.ready() && scheduler.elapsed() == granted.elapsed());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_clock()
    {
        report_group("clock");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("corridor", stratum_sim::load_layout("data/corridor.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        flecs::entity player = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}),
                stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::Scheduler scheduler;
        scheduler.add(player.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;

        id_type controlled = static_cast<id_type>(player.id());

        auto intend = [&](stratum_sim::value_type dx, stratum_sim::value_type dy)
        {
            player.set<stratum_sim::Intent>(stratum_sim::Intent {
                stratum_sim::vector_type(dx, dy, 0.0f), 1.0f, stratum_sim::vector_type()});
        };

        // Nobody has accrued anything, so there is nothing to run yet
        report_case("with no time granted nobody runs", clock.run(events, controlled) == nullid);

        clock.grant_turn();
        report_case("granting a turn makes an actor ready", scheduler.ready() == controlled);

        // Asking for nothing hands the controlled actor back rather than running it
        id_type waiting = clock.run(events, controlled);
        report_case("the controlled actor asking for nothing is returned", waiting == controlled);
        report_case_equal("and nothing has happened", events.size(), 0);

        // An intent runs and charges the turn
        intend(1.0f, 0.0f);
        id_type after = clock.run(events, controlled);
        report_case("running the only actor leaves nobody ready", after == nullid);
        report_case("and it moved", layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type {2, 1});
        report_case_equal("its turn was charged", scheduler.energy(player.id()), 0);
        report_case("what it asked for was spent", player.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // An intent lasts one action, so a second turn on the same one does nothing
        clock.grant_turn();
        report_case("a spent intent is not acted on again", clock.run(events, controlled) == controlled);
        report_case("and it did not move", layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type {2, 1});

        // Facing is taken before movement, so the two are one action each
        events.clear();
        player.set<stratum_sim::Intent>(stratum_sim::Intent {
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f), 1.0f, stratum_sim::vector_type(0.0f, 1.0f, 0.0f)});

        clock.run(events, controlled);
        report_case("facing is taken first", player.get<stratum_sim::placement_type>().direction().y() == 1.0f);
        report_case("and the movement asked for is still owed", player.get<stratum_sim::Intent>().direction.x() == 1.0f);
        report_case("which remains until a turn is granted for it",
            layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type {2, 1});

        clock.grant_turn();
        clock.run(events, controlled);
        report_case("the movement follows on the next turn", layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type {3, 1});

        // An action that achieves nothing costs no turn, so walking into a wall is free
        clock.grant_turn();
        while (layout.grid_position_at(player.get<stratum_sim::placement_type>().position()).i() > 1)
        {
            intend(-1.0f, 0.0f);
            clock.run(events, controlled);
            clock.grant_turn();
        }

        long long elapsed_before = scheduler.elapsed();
        intend(-1.0f, 0.0f);
        id_type balked = clock.run(events, controlled);
        report_case("an actor that achieved nothing is returned", balked == controlled);
        report_case("its turn was not charged", scheduler.energy(player.id()) >= stratum_sim::Scheduler::action_cost);
        report_case_equal("and no time passed", scheduler.elapsed(), static_cast<size_t>(elapsed_before));

        // An actor that is not the controlled one waits rather than stopping the turn order.
        // Its own world, so that the energies reached here are not what decides the order
        {
            stratum_sim::World world_two;
            flecs::entity zone_two = world_two.create_zone("corridor", stratum_sim::load_layout("data/corridor.level"));

            const stratum_sim::DiscreteLayout& layout_two = zone_layout(world_two, zone_two);
            stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

            flecs::entity driven = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {1, 1}), east));
            flecs::entity other = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {3, 1}), east));

            stratum_sim::Scheduler scheduler_two;
            scheduler_two.add(driven.id(), 100);
            scheduler_two.add(other.id(), 200); // The readier of the two, so it is reached first

            stratum_sim::Clock clock_two(world_two, scheduler_two);
            stratum_sim::EventQueue events_two;

            scheduler_two.advance(1);
            driven.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});

            id_type ran = clock_two.run(events_two, static_cast<id_type>(driven.id()));

            report_case("every ready actor ran", ran == nullid);
            report_case("the one asking for nothing waited", scheduler_two.energy(other.id()) < stratum_sim::Scheduler::action_cost);
            report_case("and the controlled actor acted even though it was the less ready",
                layout_two.grid_position_at(driven.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type {2, 1});
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Which grant a clock uses is the whole of what makes a world turn-based or real time,
    // and it comes from the load rather than from a host
    void test_driver()
    {
        report_group("time driver");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("corridor", stratum_sim::load_layout("data/corridor.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity player = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(player.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;
        id_type controlled = static_cast<id_type>(player.id());

        auto restart = [&](double units_per_second)
        {
            scheduler.clear();
            scheduler.add(player.id(), 100);
            clock.set_driver(stratum_sim::Driver::RealTime, units_per_second);
        };

        report_case("a clock is turn-based until told otherwise", clock.driver() == stratum_sim::Driver::TurnBased);

        // Turn-based skips ahead, so it needs no elapsed time to make an actor ready
        report_case("turn-based skips ahead to the next actor", clock.advance(events, controlled) == controlled);
        report_case_equal("and passed the time it took to get there", scheduler.elapsed(), 1);

        // Two units a second, so an actor of speed 100 gets two actions a second
        restart(2.0);
        report_case("the driver is what it was set to", clock.driver() == stratum_sim::Driver::RealTime);
        report_case("and keeps the rate with it", clock.units_per_second() == 2.0);

        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        report_case_equal("half a second at two units a second is one unit", scheduler.elapsed(), 1);
        report_case_equal("which is one action at that speed", scheduler.energy(player.id()), stratum_sim::Scheduler::action_cost);

        // The remainder is what keeps a run of short frames from rounding away to nothing
        restart(2.0);
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        report_case_equal("a frame too short to earn a unit grants none", scheduler.elapsed(), 0);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        }

        report_case_equal("but the remainder carries, so four of them earn one", scheduler.elapsed(), 1);

        // Skipping the wait is exactly what real time must not do
        restart(2.0);
        report_case("real time does not skip ahead to make an actor ready",
            clock.advance(events, controlled, stratum_sim::Clock::duration_type::zero()) == nullid);
        report_case_equal("so no time passed", scheduler.elapsed(), 0);

        // Once elapsed time has made it ready, it acts on what it was asking for
        player.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        report_case("an actor made ready by elapsed time acts",
            layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type(2, 1));

        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            player.set<stratum_sim::placement_type>(stratum_sim::placement_type(layout.position(cell), east));
            player.set<stratum_sim::Intent>(stratum_sim::Intent {direction, 1.0f, stratum_sim::vector_type()});
        };

        // A direction held goes on producing steps without being asked for again
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        }

        report_case("a direction held keeps stepping as the time is earned",
            layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type(4, 1));
        report_case("and is still being asked for", player.get<stratum_sim::Intent>().direction.x() == 1.0f);

        // Turn-based drops it instead, which is one step to an intent
        scheduler.clear();
        scheduler.add(player.id(), 100);
        clock.set_driver(stratum_sim::Driver::TurnBased);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled);
        }

        report_case("turn-based takes one step from one intent",
            layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type(2, 1));

        // Held against a wall it stops asking rather than trying without end. That this returns
        // at all is the case: an intent kept over an action that achieved nothing would spin
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, stratum_sim::vector_type(-1.0f, 0.0f, 0.0f));
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));

        report_case("a mover held against a wall does not move",
            layout.grid_position_at(player.get<stratum_sim::placement_type>().position()) == stratum_sim::discrete_point_type(1, 1));
        report_case("and stops asking for what it could not have",
            player.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // A host calls the same entry either way and is told nothing about which it is running
        stratum_sim::Session session_turn;
        session_turn.load("data/corridor.level");

        stratum_sim::Session session_real;
        session_real.load("data/corridor.level", stratum_sim::Driver::RealTime);

        stratum_sim::EventQueue session_events;
        report_case("a turn-based session hands control out with no elapsed time given",
            session_turn.advance(session_events) == static_cast<id_type>(session_turn.player().id()));
        report_case("a real-time one does not, having been given none",
            session_real.advance(session_events) == nullid);

        return;
    }

    //--------------------------------------------------------------------------
    void test_movement()
    {
        report_group("movement");

        // A corridor four cells long, walled at both ends
        stratum_sim::DiscreteLayout layout;
        layout.set_grid(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8));

        stratum_sim::DiscreteLayout::ids_type corridor = build_outline(layout, cell_rectangle(0, 0, 4, 1));
        stratum_sim::DiscreteLayout::ids_type beyond = build_outline(layout, cell_rectangle(4, 0, 5, 1));
        layout.connect_walls(corridor, beyond);
        mark_open(layout, 0, 0, 5, 1);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        stratum_sim::placement_type start(layout.position(stratum_sim::discrete_point_type {0, 0}), east);

        stratum_sim::MoveResult one = layout.move(start, east, 1.0f);
        report_case("a step lands in the next cell", layout.grid_position_at(one.placement.position()) == stratum_sim::discrete_point_type(1, 0));
        report_case("an unobstructed step reports nothing", one.obstruction_wall_id == nullid);
        report_case("moving does not turn", one.placement.direction().x() == 1.0f);

        stratum_sim::MoveResult three = layout.move(start, east, 3.0f);
        report_case("distance is in cells", layout.grid_position_at(three.placement.position()) == stratum_sim::discrete_point_type(3, 0));

        // The far wall of the corridor is connected but not passable
        stratum_sim::MoveResult far = layout.move(start, east, 7.0f);
        report_case("a mover stops at the first block", layout.grid_position_at(far.placement.position()) == stratum_sim::discrete_point_type(3, 0));
        report_case("and reports what stopped it", far.obstruction_wall_id != nullid);
        report_case("the obstruction is a wall of the corridor it was in",
            std::find(corridor.begin(), corridor.end(), far.obstruction_wall_id) != corridor.end());

        // Opening that wall lets it through
        layout.set_crossable([id = far.obstruction_wall_id](id_type wall_id) { return wall_id == id; });
        stratum_sim::MoveResult through = layout.move(start, east, 7.0f);
        report_case("an opened wall no longer stops it", layout.grid_position_at(through.placement.position()) == stratum_sim::discrete_point_type(4, 0));
        report_case("it stops at the next block instead", through.obstruction_wall_id != nullid && through.obstruction_wall_id != far.obstruction_wall_id);

        // Solid in every other direction
        stratum_sim::MoveResult blocked = layout.move(start, north, 3.0f);
        report_case("a mover held against a wall does not move", layout.grid_position_at(blocked.placement.position()) == stratum_sim::discrete_point_type(0, 0));
        report_case("and still reports the obstruction", blocked.obstruction_wall_id != nullid);

        report_case("a zero direction moves nothing", layout.grid_position_at(layout.move(start, stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 3.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));
        report_case("a zero distance moves nothing", layout.grid_position_at(layout.move(start, east, 0.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));

        // Direction is snapped to one the layout owns
        stratum_sim::MoveResult skewed = layout.move(start, stratum_sim::vector_type(0.9f, 0.4f, 0.0f), 2.0f);
        report_case("a direction off axis snaps to the nearest", layout.grid_position_at(skewed.placement.position()) == stratum_sim::discrete_point_type(2, 0));

        // Turning
        stratum_sim::placement_type facing_east(start.position(), east);
        stratum_sim::placement_type left = layout.turn(facing_east, stratum_sim::Rotation::CCW);
        report_case("turning counterclockwise from east faces north", left.direction().x() == 0.0f && left.direction().y() == 1.0f);
        report_case("turning does not move", layout.same_location(facing_east, left));

        stratum_sim::placement_type right = layout.turn(facing_east, stratum_sim::Rotation::CW);
        report_case("turning clockwise from east faces south", right.direction().x() == 0.0f && right.direction().y() == -1.0f);

        stratum_sim::placement_type around = layout.turn(facing_east, stratum_sim::Rotation::Half);
        report_case("turning half around from east faces west", around.direction().x() == -1.0f && around.direction().y() == 0.0f);
        report_case("four counterclockwise turns return the original facing",
            layout.turn(layout.turn(layout.turn(left, stratum_sim::Rotation::CCW), stratum_sim::Rotation::CCW), stratum_sim::Rotation::CCW).direction().x() == 1.0f);

        // Facing absolutely, which is what a command asks for and what the layout snaps
        stratum_sim::placement_type faced = layout.face(facing_east, north);
        report_case("facing north from east faces north", faced.direction().x() == 0.0f && faced.direction().y() == 1.0f);
        report_case("facing does not move", layout.same_location(facing_east, faced));

        stratum_sim::placement_type faced_off_axis = layout.face(facing_east, stratum_sim::vector_type(0.4f, 0.9f, 0.0f));
        report_case("a facing off axis snaps to the nearest",
            faced_off_axis.direction().x() == 0.0f && faced_off_axis.direction().y() == 1.0f);

        report_case("facing the way already faced changes nothing", layout.face(facing_east, east).direction().x() == 1.0f);
        report_case("a zero direction leaves the facing alone",
            layout.face(facing_east, stratum_sim::vector_type(0.0f, 0.0f, 0.0f)).direction().x() == 1.0f);

        // What a relative turn resolves to is what the absolute facing asks for
        report_case("a resolved counterclockwise turn faces where turning does",
            layout.face(facing_east, layout.turn(facing_east, stratum_sim::Rotation::CCW).direction()).direction().y() == 1.0f);

        // Absolute movement passes a direction the mover is not facing, and keeps its facing
        stratum_sim::placement_type facing_north(start.position(), north);
        stratum_sim::MoveResult sidestep = layout.move(facing_north, east, 2.0f);
        report_case("absolute movement ignores the mover's facing", layout.grid_position_at(sidestep.placement.position()) == stratum_sim::discrete_point_type(2, 0));
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

        // A three by three room with a doorway off its east side, read so that walls are
        // one cell edge each. Hand built rectangles would give the room one wall spanning
        // three cells, which connect_walls cannot pair against the doorway's single one
        auto proom = load_open("data/room.level");
        stratum_sim::DiscreteLayout& layout = *proom;

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto at = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        // The doorway is a block belonging to the floor around it, so the floor's boundary
        // encloses its sides and the block has a ring of four of its own
        report_case("the doorway widens the floor's outline", outline_lengths(layout) == vector<size_t> {14, 4});
        report_case_equal("and the file names no rooms, its floor being corridor", layout.room_store_count(), 0);

        // Inside the room nothing separates cells, so a middle cell reaches all four
        report_case_equal("a cell amid open floor has four neighbors", layout.neighbors(at(2, 2)).size(), 4);
        report_case_equal("a corner cell is walled on two sides", layout.neighbors(at(1, 1)).size(), 2);
        report_case_equal("an edge cell is walled on one", layout.neighbors(at(2, 1)).size(), 3);

        // The reader opens what it connects, so the doorway is reachable
        report_case_equal("an open doorway is a neighbor", layout.neighbors(at(3, 2)).size(), 4);
        report_case("a neighbor keeps the mover's facing", layout.neighbors(at(2, 2)).front().direction().x() == 1.0f);

        id_type doorway = layout.wall_toward(stratum_sim::discrete_point_type {3, 2}, cardinal_direction(CardinalDirection::East));
        report_case("the wall into the doorway is found", doorway != nullid);
        report_case("and has no partner, the doorway having no wall of its own",
            layout.wall(doorway).partner_id == nullid);

        // What is in front is the layout's own to decide, so a rule never steps a cell of its own
        report_case("the layout names what a placement faces", layout.ahead(at(3, 2)) == doorway);
        report_case("and nothing where the facing meets no wall", layout.ahead(at(1, 1)) == nullid);
        report_case("a facing the grid names nothing faces nothing", layout.ahead(
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), stratum_sim::vector_type())) == nullid);

        // The doorway's two walls are what a mover crosses either way through it
        std::set<id_type> open_walls {doorway, layout.wall(doorway).partner_id};
        layout.set_crossable([&](id_type wall_id) { return open_walls.contains(wall_id); });

        open_walls.erase(doorway);
        report_case_equal("closing it removes the neighbor", layout.neighbors(at(3, 2)).size(), 3);
        open_walls.insert(doorway);

        // Sight
        report_case("a cell sees itself", layout.visible(at(2, 2), at(2, 2)));
        report_case("open floor is visible across", layout.visible(at(1, 1), at(3, 3)));
        report_case("sight is blocked by a solid cell", !layout.visible(at(2, 2), at(2, 4)));
        report_case("sight reaches through an open doorway", layout.visible(at(3, 2), at(4, 2)));

        open_walls.erase(doorway);
        report_case("closing it blocks sight too", !layout.visible(at(3, 2), at(4, 2)));

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_load_layout()
    {
        report_group("load layout");

        // A ring of eight open cells around one enclosed solid cell
        auto ploop = load_open("data/loop.level");
        const stratum_sim::DiscreteLayout& loop = *ploop;

        report_case("the grid is the file's extent", loop.grid().extent == stratum_sim::discrete_vector_type(5, 5));
        report_case_equal("a layout has one floor and one ceiling", loop.flat_store_count(), 2);

        // One contiguous area, its boundary two closed outlines: around the outside and
        // around the solid cell it encloses
        report_case("the enclosure becomes a second outline", outline_lengths(loop) == vector<size_t> {12, 4});
        report_case_equal("every outline is one wall per cell edge", loop.wall_store_count(), 16);

        // Nothing borders it, inside or out
        size_t border_walls = 0;
        for (const stratum_sim::Wall& wall : loop.wall_store())
        {
            border_walls += wall.partner_id == nullid ? 1 : 0;
        }

        report_case_equal("an isolated area is border wall all round", border_walls, 16);

        report_case("an open cell reads as open", loop.open(stratum_sim::discrete_point_type {1, 1}));
        report_case("the enclosed solid cell does not", !loop.open(stratum_sim::discrete_point_type {2, 2}));
        report_case("a solid cell outside the ring does not", !loop.open(stratum_sim::discrete_point_type {0, 0}));

        report_case("adjacent open cells do not block", !loop.blocked(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)));
        report_case("an open cell blocks against solid", loop.blocked(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::South)));
        report_case("an open cell blocks against enclosed solid", loop.blocked(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North)));

        // The file's first line is the highest row, so the origin is lower left
        report_case("row order puts the origin at the lower left", loop.open(stratum_sim::discrete_point_type {3, 1}));

        // A doorway joins the cells either side into one area, its own ring separate
        auto pdoor = load_open("data/door.level");
        const stratum_sim::DiscreteLayout& door = *pdoor;

        report_case("a doorway is a block within one area", outline_lengths(door) == vector<size_t> {8, 4});
        report_case("a doorway connects both ways", !door.blocked(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)) && !door.blocked(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::East)));

        // Sideways out of the block is the area's own outline, wrapped around the block's side
        report_case("a block is walled sideways", door.blocked(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North))
            && door.blocked(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::South)));
        report_case("and it is a wall that says so", door.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North)) != nullid);

        // The face toward floor is wound about the floor, so the block's own side of it is bare
        report_case("a block sees no wall on the way out", door.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::East)) == nullid);
        report_case("but the floor sees one on the way in", door.wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::West)) != nullid);

        // Corridor floor is not room floor, so a file written entirely in it defines no rooms
        report_case_equal("a level of corridor alone has no rooms", door.room_store_count(), 0);

        // Both load into the same structure, so which format a file was written in is the
        // loader's to report rather than a property of the layout
        stratum_sim::Format level_format = stratum_sim::Format::Wall;
        stratum_sim::load_layout("data/door.level", 1.0f, {}, &level_format);
        report_case("a headerless file is the block format", level_format == stratum_sim::Format::Block);

        stratum_sim::load_layout("data/walls.level", 1.0f, {}, &level_format);
        report_case("and one catagen wrote is the wall format", level_format == stratum_sim::Format::Wall);

        // A real generated level, for scale rather than for exact counts
        auto pgenerated = load_open("data/generated.level");
        const stratum_sim::DiscreteLayout& generated = *pgenerated;

        report_case("a generated level reads", generated.wall_store_count() > 0);
        report_case("its grid matches the file", generated.grid().extent == stratum_sim::discrete_vector_type(31, 31));

        // 292 of its 961 cells are room floor and 247 corridor, the rest solid, doorways
        // and stairways. Rooms come from the first alone
        report_case("its room floor becomes rooms", generated.room_store_count() > 0,
            format("{} rooms", generated.room_store_count()));

        size_t room_cells = 0;
        for (const stratum_sim::Room& room : generated.room_store())
        {
            room_cells += room.cell_count();
        }

        report_case_equal("every room cell belongs to exactly one room", room_cells, 292);
        report_case("a corridor cell belongs to no room", generated.room_at(stratum_sim::discrete_point_type {7, 29}) == nullid);
        report_case("a room cell names the room that covers it", generated.room_at(stratum_sim::discrete_point_type {1, 1}) != nullid);
        report_case("a room's outline closes", generated.room_store().front().vertex_count() >= 4);

        // Two areas never share an edge in this format: a block separates them, and a wall
        // faces either a solid cell or a block. So nothing is partnered, which is the whole of what
        // the block format cannot express and the wall format can
        size_t connected = 0;
        for (const stratum_sim::Wall& wall : generated.wall_store())
        {
            connected += wall.partner_id != nullid ? 1 : 0;
        }

        report_case_equal("no wall of a block level is partnered", connected, 0);

        return;
    }

    //--------------------------------------------------------------------------
    size_t open_cells(const stratum_sim::DiscreteLayout& layout)
    {
        const stratum_sim::discrete_vector_type extent = layout.grid().extent;
        size_t n = 0;

        for (stratum_sim::coord_type j = 0; j < extent.y(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < extent.x(); ++i)
            {
                n += layout.open(stratum_sim::discrete_point_type {i, j}) ? 1 : 0;
            }
        }

        return n;
    }

    //--------------------------------------------------------------------------
    // Open cells reachable from the first by crossing only walls a mover may pass, which is
    // the connectivity movement itself sees rather than one read off an area graph
    size_t cells_reached(const stratum_sim::DiscreteLayout& layout)
    {
        const stratum_sim::discrete_vector_type extent = layout.grid().extent;
        auto slot = [&extent](const stratum_sim::discrete_point_type& grid_position) { return static_cast<size_t>(grid_position.j()) * extent.x() + grid_position.i(); };

        vector<bool> seen(static_cast<size_t>(extent.x()) * extent.y(), false);
        vector<stratum_sim::discrete_point_type> pending;

        for (stratum_sim::coord_type j = 0; j < extent.y() && pending.empty(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < extent.x() && pending.empty(); ++i)
            {
                if (layout.open(stratum_sim::discrete_point_type {i, j}))
                {
                    pending.push_back(stratum_sim::discrete_point_type {i, j});
                    seen[slot(pending.front())] = true;
                }
            }
        }

        size_t count = 0;

        while (!pending.empty())
        {
            stratum_sim::discrete_point_type grid_position = pending.back();
            pending.pop_back();
            ++count;

            const stratum_sim::discrete_vector_type steps[4] {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (const stratum_sim::discrete_vector_type& step : steps)
            {
                stratum_sim::discrete_point_type t = grid_position + step;

                if (!layout.grid().contains(t) || seen[slot(t)] || !layout.open(t) || layout.blocked(grid_position, step))
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
    void test_load_walls()
    {
        report_group("load wall layout");

        // Two rooms of two by three cells sharing one wall, with a door in the middle of it
        // The loader reports the wall pairs a door is attached to rather than marking the walls
        stratum_sim::door_walls_type door_walls;
        auto pwalls = stratum_sim::load_layout("data/walls.level", 1.0f,
            [&](const std::vector<id_type>& walls) { door_walls.push_back(walls); });
        const stratum_sim::DiscreteLayout& walls = *pwalls;

        std::set<id_type> door_wall_ids;
        for (const std::vector<id_type>& door : door_walls)
        {
            door_wall_ids.insert(door.begin(), door.end());
        }
        pwalls->set_crossable([door_wall_ids](id_type wall_id) { return door_wall_ids.contains(wall_id); });

        report_case("the grid is the header's extent", walls.grid().extent == stratum_sim::discrete_vector_type(4, 3));
        report_case("areas sharing a wall are outlined separately", outline_lengths(walls) == vector<size_t> {10, 10});
        report_case_equal("both outlines account for every wall", walls.wall_store_count(), 20);

        // The wall format has one open cell symbol, so it encodes nothing about rooms yet
        report_case_equal("the wall format names no rooms", walls.room_store_count(), 0);

        size_t partnered = 0;
        for (const stratum_sim::Wall& wall : walls.wall_store())
        {
            partnered += wall.partner_id != nullid ? 1 : 0;
        }

        // What the block format cannot express: a wall with a room behind it rather than a solid cell
        report_case_equal("the shared wall is partnered along its whole length", partnered, 6);
        report_case_equal("one pair of it is the door", door_walls.size(), 1);

        // The generator puts the door in the top row of the shared wall at this seed, and the
        // stairs in the middle row of each room, which is why neither is where a hand drawn
        // fixture would put them
        report_case("a mover crosses at the door", !walls.blocked(stratum_sim::discrete_point_type {1, 2}, cardinal_direction(CardinalDirection::East)));
        report_case("and back through it", !walls.blocked(stratum_sim::discrete_point_type {2, 2}, cardinal_direction(CardinalDirection::West)));
        report_case("but not below it", walls.blocked(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)));
        report_case("nor two below", walls.blocked(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East)));
        report_case("and never within one room", !walls.blocked(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::East)));

        report_case_equal("every open cell is reachable through the door", cells_reached(walls), open_cells(walls));

        // The same grid with the extent taken out of its header, which the geometry then has
        // to supply: the last column is never blank, so no row can have been trimmed
        auto pderived = load_open("data/walls_no_extent.level");
        const stratum_sim::DiscreteLayout& derived = *pderived;

        report_case("an extent absent from the header is derived from the grid",
            derived.grid().extent == stratum_sim::discrete_vector_type(4, 3));
        report_case("and reads the same outlines", outline_lengths(derived) == outline_lengths(walls));
        report_case_equal("and the same walls", derived.wall_store_count(), walls.wall_store_count());
        report_case("and the same door", !derived.blocked(stratum_sim::discrete_point_type {1, 2}, cardinal_direction(CardinalDirection::East))
            && derived.blocked(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)));

        // A generated level, for scale and for the properties the generator promises
        auto pgenerated = load_open("data/catagen.level");
        const stratum_sim::DiscreteLayout& generated = *pgenerated;

        report_case("a generated level reads", generated.wall_store_count() > 0);
        report_case("its grid is the header's extent", generated.grid().extent == stratum_sim::discrete_vector_type(24, 14));

        size_t solid = 0;
        for (int j = 0; j < 14; ++j)
        {
            for (int i = 0; i < 24; ++i)
            {
                solid += generated.open(stratum_sim::discrete_point_type {i, j}) ? 0 : 1;
            }
        }

        report_case("a level below full density leaves solid cells", solid > 0, format("{} solid cells", solid));

        size_t connected = 0;
        for (const stratum_sim::Wall& wall : generated.wall_store())
        {
            connected += wall.partner_id != nullid ? 1 : 0;
        }

        report_case("connections are mutual", connected % 2 == 0);

        report_case_equal("every open cell is reachable from the first", cells_reached(generated),
            open_cells(generated));

        // At full density with no spacing every cell is room, so a solid cell here is the spacing
        // and nothing else, and nothing but a corridor joins any two rooms
        auto pspaced = load_open("data/catagen_spaced.level");
        const stratum_sim::DiscreteLayout& spaced = *pspaced;

        report_case("a spaced level reads", spaced.wall_store_count() > 0);
        report_case("its grid is the header's extent", spaced.grid().extent == stratum_sim::discrete_vector_type(30, 18));

        size_t spaced_solid = 0;
        for (int j = 0; j < 18; ++j)
        {
            for (int i = 0; i < 30; ++i)
            {
                spaced_solid += spaced.open(stratum_sim::discrete_point_type {i, j}) ? 0 : 1;
            }
        }

        report_case("spacing leaves solid cells where full density would leave none", spaced_solid > 0,
            format("{} solid cells", spaced_solid));

        report_case_equal("every open cell is still reachable from the first", cells_reached(spaced),
            open_cells(spaced));

        return;
    }

    //--------------------------------------------------------------------------
    // What a level file's doors load as. Nothing reads them yet, so this checks the shape
    void test_doors()
    {
        report_group("doors");

        // Two rooms sharing one wall, with a single door in it
        stratum_sim::door_walls_type door_walls;

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("rooms", stratum_sim::load_layout("data/walls.level", 1.0f,
            [&](const std::vector<id_type>& walls) { door_walls.push_back(walls); }));

        report_case_equal("one door for the one the file names", stratum_sim::create_doors(world, zone, door_walls), 1);

        // A door is an entity in the zone, and its facings are its children
        flecs::entity door;
        world.entities().query_builder<const stratum_sim::Door>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](flecs::entity e, const stratum_sim::Door&) { door = e; });

        report_case("the door is a child of the zone", door && door.parent() == zone);
        report_case("it is closed until something opens it", door.get<stratum_sim::Openable>().open == false);

        stratum_sim::World::entities_type facings;
        world.entities().query_builder<const stratum_sim::DoorFacing>()
            .with(flecs::ChildOf, door)
            .build()
            .each([&](flecs::entity e, const stratum_sim::DoorFacing&) { facings.push_back(e); });

        report_case_equal("it has two facings", facings.size(), 2);
        report_case("each hangs on a wall", facings.size() == 2
            && facings[0].get<stratum_sim::OnWall>().wall_id != nullid
            && facings[1].get<stratum_sim::OnWall>().wall_id != nullid);

        // The two walls are the two sides of one edge, which is what makes them one door
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        id_type wall_a = facings[0].get<stratum_sim::OnWall>().wall_id;
        id_type wall_b = facings[1].get<stratum_sim::OnWall>().wall_id;

        report_case("on walls that are partners", layout.wall(wall_a).partner_id == wall_b
            && layout.wall(wall_b).partner_id == wall_a);
        report_case("and the walls the file called a door", door_walls.size() == 1
            && std::find(door_walls[0].begin(), door_walls[0].end(), wall_a) != door_walls[0].end()
            && std::find(door_walls[0].begin(), door_walls[0].end(), wall_b) != door_walls[0].end());

        // The queries: what is on a wall, what it belongs to, and the other side
        report_case("a wall names the facing on it", stratum_sim::facing_on(zone, wall_a) == facings[0]
            || stratum_sim::facing_on(zone, wall_a) == facings[1]);
        report_case("a wall with nothing on it names nothing", !stratum_sim::facing_on(zone, layout.wall(wall_a).next_id));
        report_case("a facing belongs to its door", stratum_sim::door_of(facings[0]) == door);
        report_case("and the other side is the other facing", stratum_sim::other_facing(facings[0]) == facings[1]
            && stratum_sim::other_facing(facings[1]) == facings[0]);

        // What a mover may cross, which is what the flag will stop deciding
        report_case("a closed door is not crossable", !stratum_sim::crossable(zone, wall_a));

        door.set<stratum_sim::Openable>(stratum_sim::Openable {true});
        report_case("an open one is, from either side", stratum_sim::crossable(zone, wall_a)
            && stratum_sim::crossable(zone, wall_b));

        door.set<stratum_sim::Openable>(stratum_sim::Openable {false});
        facings[0].set<stratum_sim::Concealed>(stratum_sim::Concealed {false});
        door.set<stratum_sim::Openable>(stratum_sim::Openable {true});

        report_case("a concealed facing is a wall until it is found", !stratum_sim::crossable(zone, wall_a));
        report_case("and the other side of it is not concealed", stratum_sim::crossable(zone, wall_b));

        facings[0].set<stratum_sim::Concealed>(stratum_sim::Concealed {true});
        report_case("finding it opens the way", stratum_sim::crossable(zone, wall_a));

        facings[0].remove<stratum_sim::Concealed>();
        door.set<stratum_sim::Openable>(stratum_sim::Openable {false});

        // Interacting with what an actor faces is what opens and shuts one
        // Cell (1, 2) faces east across the shared wall, which is where the door is
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 2}), east));

        stratum_sim::EventQueue door_events;
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact {}), door_events);

        report_case("interacting with the door opens it", door.get<stratum_sim::Openable>().open);

        door_events.clear();
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact {}), door_events);

        report_case("and again shuts it", !door.get<stratum_sim::Openable>().open);

        actor.destruct();

        // Deleting the door takes its facings with it, being one subtree
        door.destruct();

        size_t remaining = 0;
        world.entities().query_builder<const stratum_sim::DoorFacing>()
            .build()
            .each([&](flecs::entity, const stratum_sim::DoorFacing&) { ++remaining; });

        report_case_equal("deleting a door takes its facings", remaining, 0);

        return;
    }

    //--------------------------------------------------------------------------
    void test_world()
    {
        report_group("world");

        stratum_sim::World world;

        report_case_equal("a new world has no zones", world.zone_count(), 0);

        flecs::entity crypt = world.create_zone("crypt", stratum_sim::load_layout("data/room.level"));
        flecs::entity moor = world.create_zone("moor", make_unique<stratum_sim::DiscreteLayout>());

        report_case_equal("zones are created", world.zone_count(), 2);
        report_case("a zone is found by name", world.zone("crypt") == crypt);
        report_case("each zone has its own layout", &world.layout(crypt) != &world.layout(moor));

        report_case("a zone holds the layout it was created with", zone_layout(world, crypt).wall_store_count() == 18);
        report_case("the other stays empty", zone_layout(world, moor).wall_store_count() == 0);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::placement_type where(zone_layout(world, crypt).position(stratum_sim::discrete_point_type {2, 2}), east);

        flecs::entity monster = world.create_entity(crypt, where);
        report_case("an entity is placed in its zone", world.zone_of(monster) == crypt);
        report_case("and has its placement", monster.try_get<stratum_sim::placement_type>() != nullptr);

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
        flecs::entity zone = world.create_zone("crypt", stratum_sim::load_layout("data/room.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 2}), east));

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.grid_position_at(actor.get<stratum_sim::placement_type>().position()); };

        // A step along the actor's own facing
        commands.push(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 1.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a step moves the actor", cell_of() == stratum_sim::discrete_point_type(2, 2));
        report_case_equal("and reports one event", events.size(), 1);
        report_case("which is a move", events.front().is<stratum_sim::Moved>());
        report_case("naming the actor", events.front().get<stratum_sim::Moved>().entity_id == actor.id());
        report_case("commands are consumed", commands.empty());

        events.clear();

        // Walking into a wall reports the obstruction and does not move
        commands.push(actor.id(), stratum_sim::Step {north, 5.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a blocked step does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));
        report_case("it moved as far as it could", events.front().is<stratum_sim::Moved>());
        report_case("and reported what stopped it", events.events().back().is<stratum_sim::Blocked>());
        report_case("the obstruction is a wall of the layout", events.events().back().get<stratum_sim::Blocked>().obstruction_wall_id != nullid);

        events.clear();

        // Held against a wall, nothing happens and nothing is spent
        stratum_sim::Command held(actor.id(), stratum_sim::Step {north, 1.0f});
        report_case_equal("a step that cannot move costs nothing", static_cast<size_t>(stratum_sim::apply(world, held, events)), 0);
        report_case("and reports only the block", events.size() == 1 && events.front().is<stratum_sim::Blocked>());

        events.clear();

        // What a move costs is what it covered, not a flat action
        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            actor.set<stratum_sim::placement_type>(stratum_sim::placement_type(layout.position(cell), direction));
        };

        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command one(actor.id(), stratum_sim::Step {east, 1.0f});
        report_case_equal("a one cell step costs one action",
            static_cast<size_t>(stratum_sim::apply(world, one, events)), static_cast<size_t>(stratum_sim::Scheduler::action_cost));

        events.clear();

        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command two(actor.id(), stratum_sim::Step {east, 2.0f});
        report_case_equal("a two cell step costs two",
            static_cast<size_t>(stratum_sim::apply(world, two, events)), static_cast<size_t>(2 * stratum_sim::Scheduler::action_cost));
        report_case("and crossed them in one command", cell_of() == stratum_sim::discrete_point_type(3, 1));

        events.clear();

        // Stopped short, so charged for the distance actually reached rather than the one asked for
        place(stratum_sim::discrete_point_type {1, 1}, east);
        stratum_sim::Command overreach(actor.id(), stratum_sim::Step {east, 4.0f});
        int cost_short = stratum_sim::apply(world, overreach, events);

        report_case("a step stopped short costs less than it asked for",
            cost_short > 0 && cost_short < 4 * stratum_sim::Scheduler::action_cost);
        report_case("and stopped where the wall is", cell_of() == stratum_sim::discrete_point_type(3, 1));
        report_case_equal("charged an action per cell it did reach",
            static_cast<size_t>(cost_short), static_cast<size_t>(2 * stratum_sim::Scheduler::action_cost));

        events.clear();
        place(stratum_sim::discrete_point_type {2, 3}, east);

        // Turning
        commands.push(actor.id(), stratum_sim::TurnTo {
            layout.turn(actor.get<stratum_sim::placement_type>(), stratum_sim::Rotation::CW).direction()});
        stratum_sim::apply(world, commands, events);

        report_case("turning right from east faces south", actor.get<stratum_sim::placement_type>().direction().y() == -1.0f);
        report_case("and reports a turn", events.front().is<stratum_sim::Turned>());
        report_case("turning does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));

        events.clear();

        // Interact with what the actor faces
        stratum_sim::Command interact_floor(actor.id(), stratum_sim::Interact {});
        report_case_equal("interacting with open floor does nothing", static_cast<size_t>(stratum_sim::apply(world, interact_floor, events)), 0);
        report_case("and reports nothing", events.empty());

        // Face the doorway and interact
        world.entities().entity(actor.id()).set<stratum_sim::placement_type>(stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), east));
        stratum_sim::Command interact_door(actor.id(), stratum_sim::Interact {});

        report_case("interacting with a wall costs an action", stratum_sim::apply(world, interact_door, events) == stratum_sim::Scheduler::action_cost);
        report_case("and names the wall", events.front().is<stratum_sim::Interacted>() && events.front().get<stratum_sim::Interacted>().wall_id != nullid);

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
        flecs::entity zone = world.create_zone("crypt", stratum_sim::load_layout("data/corridor.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        flecs::entity a = world.create_entity(zone, place(1, 1));
        flecs::entity b = world.create_entity(zone, place(1, 1));
        flecs::entity c = world.create_entity(zone, place(4, 1));

        report_case_equal("two entities share a cell", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 2);
        report_case("and are returned in id order", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).front() == a);
        report_case_equal("an empty cell holds none", world.entities_at(zone, stratum_sim::discrete_point_type {3, 1}).size(), 0);
        report_case_equal("a distant one is elsewhere", world.entities_at(zone, stratum_sim::discrete_point_type {4, 1}).size(), 1);

        report_case_equal("range reaches its own cell", world.entities_within(zone, place(1, 1), 0.5f).size(), 2);
        report_case_equal("a wider range reaches further", world.entities_within(zone, place(1, 1), 4.0f).size(), 3);
        report_case("the far entity is the third", world.entities_within(zone, place(1, 1), 4.0f).back() == c);

        // Occupancy is per zone
        flecs::entity moor = world.create_zone("moor", stratum_sim::load_layout("data/corridor.level"));
        world.create_entity(moor, place(1, 1));

        report_case_equal("another zone's entities do not appear", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 2);
        report_case_equal("they appear in their own", world.entities_at(moor, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        b.destruct();
        report_case_equal("a destroyed entity stops occupying", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    // Topology and occupancy compose on World rather than on either layer alone
    void test_world_move()
    {
        report_group("world movement");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt", stratum_sim::load_layout("data/corridor.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };
        auto cell_of = [&](flecs::entity e) { return layout.grid_position_at(e.get<stratum_sim::placement_type>().position()); };

        flecs::entity mover = world.create_entity(zone, place(1, 1));
        mover.add<stratum_sim::Blocking>();

        // Nothing in the way, so the world returns what the layout alone would
        stratum_sim::MoveResult clear = world.move(mover, east, 2.0f);
        report_case("an unobstructed move reaches its distance", layout.grid_position_at(clear.placement.position()) == stratum_sim::discrete_point_type(3, 1));
        report_case("and reports neither kind of obstruction", clear.obstruction_wall_id == nullid && clear.obstruction_entity_id == nullid);

        // A blocking entity stops it where a wall would
        flecs::entity boulder = world.create_entity(zone, place(3, 1));
        boulder.add<stratum_sim::Blocking>();

        stratum_sim::MoveResult stopped = world.move(mover, east, 4.0f);
        report_case("a blocking entity stops the mover short", layout.grid_position_at(stopped.placement.position()) == stratum_sim::discrete_point_type(2, 1));
        report_case("and is named as the obstruction", stopped.obstruction_entity_id == static_cast<id_type>(boulder.id()));
        report_case("rather than as a wall", stopped.obstruction_wall_id == nullid);

        // Presence is what blocks, not merely having a placement
        boulder.remove<stratum_sim::Blocking>();
        stratum_sim::MoveResult past = world.move(mover, east, 4.0f);
        report_case("an entity that does not block is moved through", layout.grid_position_at(past.placement.position()).i() > 3);
        report_case("and nothing is reported as an obstruction", past.obstruction_entity_id == nullid);

        boulder.add<stratum_sim::Blocking>();

        // A mover never blocks itself
        stratum_sim::MoveResult alone = world.move(boulder, east, 1.0f);
        report_case("a mover is not stopped by itself", layout.grid_position_at(alone.placement.position()) == stratum_sim::discrete_point_type(4, 1));

        // Through a command, so that the event and the charge come with it
        stratum_sim::EventQueue events;
        world.entities().entity(mover.id()).set<stratum_sim::placement_type>(place(1, 1));
        world.entities().entity(boulder.id()).set<stratum_sim::placement_type>(place(3, 1));

        stratum_sim::Command step(mover.id(), stratum_sim::Step {east, 4.0f});
        int cost = stratum_sim::apply(world, step, events);

        report_case("the command stopped at the obstruction", cell_of(mover) == stratum_sim::discrete_point_type(2, 1));
        report_case_equal("charged for the one cell it covered",
            static_cast<size_t>(cost), static_cast<size_t>(stratum_sim::Scheduler::action_cost));
        report_case("and reported being blocked", events.events().back().is<stratum_sim::Blocked>());
        report_case("naming the entity rather than a wall",
            events.events().back().get<stratum_sim::Blocked>().obstruction_entity_id == static_cast<id_type>(boulder.id())
            && events.events().back().get<stratum_sim::Blocked>().obstruction_wall_id == nullid);

        // A layout asked directly still resolves its own geometry and nothing else
        stratum_sim::MoveResult geometric = layout.move(place(1, 1), east, 4.0f);
        report_case("the layout alone does not stop for entities", layout.grid_position_at(geometric.placement.position()).i() > 3);

        return;
    }

    //--------------------------------------------------------------------------
    // Everything but the controlled entity decides for itself, and what it decides is held
    // on the entity rather than beside it
    void test_ai()
    {
        report_group("deciding for itself");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("corridor", stratum_sim::load_layout("data/corridor.level"));

        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type west(-1.0f, 0.0f, 0.0f);
        auto at = [&](int i) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, 1}), east); };
        auto cell_of = [&](flecs::entity e) { return layout.grid_position_at(e.get<stratum_sim::placement_type>().position()); };

        flecs::entity player = world.create_entity(zone, at(1));
        player.add<stratum_sim::Blocking>();

        flecs::entity monster = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {5, 1}), west));
        monster.add<stratum_sim::Blocking>();
        monster.set<stratum_sim::Pursue>(stratum_sim::Pursue {static_cast<id_type>(player.id()), stratum_sim::point_type(), false});

        id_type controlled = static_cast<id_type>(player.id());

        // Down one corridor, so it is seen and where it stands is remembered
        stratum_sim::think(world, controlled);

        report_case("a pursuer that can see its target records where", monster.get<stratum_sim::Pursue>().seen);
        report_case("and asks to move toward it", monster.get<stratum_sim::Intent>().direction.x() < 0.0f);
        report_case("the controlled entity is not decided for", player.try_get<stratum_sim::Intent>() == nullptr);

        // Running it closes the distance
        stratum_sim::Scheduler scheduler;
        scheduler.add(player.id(), 100);
        scheduler.add(monster.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;

        // The controlled entity is readiest on a tie, so the pass stops there for a decision it
        // cannot get. Spending its turn is what lets everything else have one
        auto round = [&]()
        {
            stratum_sim::think(world, controlled);
            id_type up = clock.advance(events, controlled);
            report_case_equal("the pass stops at the controlled entity", up, controlled);
            scheduler.spend(up);
            clock.run(events, controlled);
        };

        for (int n = 0; n < 3; ++n)
        {
            round();
        }

        report_case("it closed the distance", cell_of(monster) == stratum_sim::discrete_point_type(2, 1));

        // It does not walk into what it is chasing, occupancy blocking it as a wall would
        for (int n = 0; n < 3; ++n)
        {
            round();
        }

        report_case("and stops where its target stands", cell_of(monster) == stratum_sim::discrete_point_type(2, 1));
        report_case("which is the target's own cell it did not take", cell_of(player) == stratum_sim::discrete_point_type(1, 1));

        // What it recorded outlives the sight of it: moved away unseen, it goes to where it was
        flecs::entity blind = world.create_entity(zone, at(5));
        blind.set<stratum_sim::Pursue>(stratum_sim::Pursue {nullid, layout.position(stratum_sim::discrete_point_type {3, 1}), true});

        stratum_sim::think(world, controlled);
        report_case("a pursuer with no target still goes where it last looked",
            blind.get<stratum_sim::Intent>().direction.x() < 0.0f);
        report_case("and still knows where that was", blind.get<stratum_sim::Pursue>().seen);

        // Arriving there and finding nothing, it gives up rather than standing on it forever
        blind.set<stratum_sim::placement_type>(at(3));
        stratum_sim::think(world, controlled);

        report_case("arriving there with nothing to find, it stops going", !blind.get<stratum_sim::Pursue>().seen);

        // Something with no behavior at all is left alone
        flecs::entity inert = world.create_entity(zone, at(4));
        stratum_sim::think(world, controlled);
        report_case("an entity that decides nothing is not decided for", inert.try_get<stratum_sim::Intent>() == nullptr);

        // A zone constructed rather than read, which is how one normally arrives. The generator
        // is the seam: an interface here and an implementation wherever one was chosen
        {
            stratum_sim::Session created;
            TestGenerator generator;
            created.create(generator);

            report_case("a constructed zone is loaded", created.loaded());
            report_case("with the format its generator named", created.format() == stratum_sim::Format::Block);
            report_case("and a player in it", created.player().is_alive());

            // The doorway has a door, which is what proves the reported walls were built on
            report_case("its doors are attached to the walls the generator reported",
                stratum_sim::facing_on(created.zone(), stratum_sim::layout_as<stratum_sim::DiscreteLayout>(
                    created.layout()).wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East))).is_alive());

            // Reading the same geometry arrives at the same zone, a file being a snapshot of
            // part of what a construction produces
            stratum_sim::Session loaded;
            loaded.load("data/door.level");
            report_case("and a zone read whole agrees with it",
                loaded.format() == created.format()
                && stratum_sim::layout_as<stratum_sim::DiscreteLayout>(loaded.layout()).wall_store_count()
                    == stratum_sim::layout_as<stratum_sim::DiscreteLayout>(created.layout()).wall_store_count());
        }

        // The whole of it through a Session, which is what a host drives
        stratum_sim::Session session;
        session.load("data/corridor.level");

        flecs::entity hunter;
        session.world().entities().query_builder<const stratum_sim::Pursue>()
            .build()
            .each([&](flecs::entity e, const stratum_sim::Pursue&) { hunter = e; });

        report_case("a session places something that decides for itself", hunter.is_alive());

        stratum_sim::EventQueue session_events;
        stratum_sim::discrete_point_type before = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).grid_position_at(hunter.get<stratum_sim::placement_type>().position());

        // The controlled entity waits, which is the only decision a host has to supply
        for (int n = 0; n < 3; ++n)
        {
            session.scheduler().spend(session.advance(session_events));
        }

        stratum_sim::discrete_point_type after = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).grid_position_at(hunter.get<stratum_sim::placement_type>().position());
        report_case("and it closes on the controlled entity while the host only waits", after.i() < before.i());

        return;
    }

    //--------------------------------------------------------------------------
    // Phase 1's claim: a dungeon read from a file is walkable, driven entirely by
    // commands ordered by the scheduler, with no rendering
    void test_walk()
    {
        report_group("walking a dungeon");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt", stratum_sim::load_layout("data/corridor.level"));

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(actor.id(), 100);

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.grid_position_at(actor.get<stratum_sim::placement_type>().position()); };

        report_case("the actor starts at the west end", cell_of() == stratum_sim::discrete_point_type(1, 1));

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

        report_case("it walked to the east end", cell_of() == stratum_sim::discrete_point_type(5, 1));
        report_case_equal("taking one step per open cell", steps, 4);
        report_case("and reported being stopped at the wall", blocks > 0);
        report_case("the scheduler advanced through the walk", scheduler.elapsed() >= 8);

        // Turning and walking back
        events.clear();
        stratum_sim::Command about(actor.id(), stratum_sim::TurnTo {
            layout.turn(actor.get<stratum_sim::placement_type>(), stratum_sim::Rotation::Half).direction()});
        stratum_sim::apply(world, about, events);
        report_case("turning around faces west", actor.get<stratum_sim::placement_type>().direction().x() == -1.0f);

        events.clear();
        stratum_sim::Command back(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 4.0f});
        stratum_sim::apply(world, back, events);

        report_case("walking back reaches the west end", cell_of() == stratum_sim::discrete_point_type(1, 1));
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
    test_layout_hierarchy();
    test_level_layout();
    test_partner_linking();
    test_wall_fold();
    test_point_location();
    test_grid();
    test_flats();
    test_blocking();
    test_scheduler();
    test_clock();
    test_driver();
    test_movement();
    test_adjacency_and_sight();
    test_load_layout();
    test_load_walls();
    test_doors();
    test_world();
    test_commands();
    test_occupancy();
    test_world_move();
    test_ai();
    test_walk();
    test_polygon_containment();

    return report_results();
}
