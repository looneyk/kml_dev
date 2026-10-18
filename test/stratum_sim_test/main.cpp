//------------------------------------------------------------------------------
// stratum_sim_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/file_util.hpp"
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
#include "quetzal/stratum_sim/Config.hpp"
#include "quetzal/stratum_sim/Scheduler.hpp"
#include "quetzal/stratum_sim/Session.hpp"
#include "quetzal/stratum_sim/Game.hpp"
#include "quetzal/stratum_sim/ai.hpp"
#include "quetzal/stratum_sim/insert_definitions.hpp"
#include "quetzal/stratum_sim/load_setting_definitions.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/file_components.hpp"
#include "quetzal/stratum_sim/layout_entities.hpp"
#include "quetzal/stratum_sim/properties.hpp"
#include "quetzal/stratum_sim/specs.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_format/read_setting_definitions.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/RelocatorTarget.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;
using quetzal::stratum_sim::CardinalDirection;
using quetzal::stratum_sim::cardinal_direction;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    static constexpr size_t suite_zone_count_max = 8;

    // A game's saves are in the suite's own output directory
    const stratum_sim::Config g_suite_config {"out/saves", "out/working", suite_zone_count_max};

    // Seeded, so a random choice is the same on every run
    stratum_sim::random_type g_suite_random(1);

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    //--------------------------------------------------------------------------
    // Returns the wall an entity is sited on, or none if it is sited on a flat cell or on its parent
    const stratum_format::WallAddress* wall_address(const stratum_format::Entity& entity)
    {
        if (!entity.site.has_value() || !entity.site->address.has_value())
        {
            return nullptr;
        }

        return get_if<stratum_format::WallAddress>(&*entity.site->address);
    }

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // detail is written only for a failure
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
    // Returns a generator that reads the zone file, at the requested level if there is one, and without the unpopulated
    // flag, which a generator does not set
    stratum_sim::Generation::generator_type file_generator(const string& filepath)
    {
        return [filepath](const stratum_sim::ZoneSpec& zone_spec)
        {
            stratum_format::Document document = stratum_format::read_zone(filepath);
            document.level = zone_spec.value<stratum_sim::level_type>(stratum_sim::level_key);
            document.is_unpopulated = false;
            return document;
        };
    }

    //--------------------------------------------------------------------------
    // Returns a generator selector that chooses the one generator for every zone
    stratum_sim::Generation::generator_selector_type sole_generator_selector(const string& generator_name)
    {
        return [generator_name](const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
        {
            return stratum_sim::Generation::GeneratorSelection {generator_name, {}, {}, zone_spec};
        };
    }

    //--------------------------------------------------------------------------
    // The suite's registries. A region whose spec selects "door" makes zones from data/door.zone, and "staircase" from
    // data/walls.zone, which has one staircase leading up and one leading down. "next_region" chooses a new region from the
    // region spec "staircase" for every zone, and "unregistered_generator" a generator that is not registered. data/door.zone
    // has one staircase, leading up
    stratum_sim::Generation suite_generation()
    {
        stratum_sim::Generation generation;

        generation.m_generators["door"] = file_generator("data/door.zone");
        generation.m_generators["staircase"] = file_generator("data/walls.zone");
        generation.m_generator_selectors["door"] = sole_generator_selector("door");
        generation.m_generator_selectors["staircase"] = sole_generator_selector("staircase");
        generation.m_generator_selectors["unregistered_generator"] = sole_generator_selector("unregistered");
        generation.m_generator_selectors["next_region"] = [](const stratum_sim::World&, flecs::entity, const stratum_sim::ZoneSpec& zone_spec)
        {
            return stratum_sim::Generation::GeneratorSelection {{}, {}, "staircase", zone_spec};
        };
        generation.m_region_specs["staircase"] = stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), "staircase"}};
        generation.m_region_specs["door"] = stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), "door"}};

        return generation;
    }

    stratum_sim::Generation g_suite_generation = suite_generation();

    //--------------------------------------------------------------------------
    stratum_sim::RegionSpec suite_region_spec(const string& generator_selector_name)
    {
        return stratum_sim::RegionSpec {{string(stratum_sim::generator_selector_key), generator_selector_name}};
    }

    //--------------------------------------------------------------------------
    stratum_sim::ZoneSpec suite_zone_spec(stratum_sim::level_type level)
    {
        return stratum_sim::ZoneSpec {{string(stratum_sim::level_key), to_string(level)}};
    }

    //--------------------------------------------------------------------------
    // A world with the game's entity types, as Game's has, and a region for the suite's zones
    struct SuiteWorld : stratum_sim::World
    {
        explicit SuiteWorld(stratum_sim::IdAllocator& id_allocator, size_t zone_count_max = suite_zone_count_max) :
            World(id_allocator, g_suite_random, g_suite_generation, zone_count_max),
            region(create_region(suite_region_spec("staircase")))
        {
            stratum_sim::register_game_types(*this);
        }

        flecs::entity region;
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
    // Counts the walled sides of every open cell, and how many of those edges are walled from the other side too
    struct WallCounts
    {
        size_t sides = 0;
        size_t both_sides = 0;
    };

    WallCounts wall_counts(const stratum_sim::DiscreteLayout& layout)
    {
        WallCounts counts;
        const stratum_sim::GridMapping& grid = layout.grid();

        for (stratum_sim::coord_type j = 0; j < grid.extent.y(); ++j)
        {
            for (stratum_sim::coord_type i = 0; i < grid.extent.x(); ++i)
            {
                stratum_sim::discrete_point_type cell_position {i, j};

                for (const stratum_sim::discrete_vector_type& direction : stratum_sim::cardinal_directions)
                {
                    if (layout.wall_toward(cell_position, direction) == nullid)
                    {
                        continue;
                    }

                    ++counts.sides;

                    stratum_sim::discrete_point_type beyond = cell_position + direction;
                    counts.both_sides += grid.contains(beyond) && layout.wall_toward(beyond, -direction) != nullid ? 1 : 0;
                }
            }
        }

        return counts;
    }

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
    // Reads a zone from a file into a world of its own, with a predicate that crosses its door walls as though every door were
    // open
    struct OpenZone
    {
        explicit OpenZone(const string& filepath) :
            id_allocator(stratum_sim::PersistentId {}),
            world(id_allocator),
            zone(world.create_zone(world.region, world.next_id()))
        {
            stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
            crossable = [open_zone = zone](id_type wall_id) { return static_cast<bool>(stratum_sim::way_through(open_zone, wall_id)); };
        }

        stratum_sim::DiscreteLayout& layout()
        {
            return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        }

        stratum_sim::CountingIdAllocator id_allocator;
        SuiteWorld world;
        flecs::entity zone;
        stratum_sim::Layout::crossable_function_type crossable;
    };

    std::unique_ptr<OpenZone> load_open(const string& filepath)
    {
        return make_unique<OpenZone>(filepath);
    }

    //--------------------------------------------------------------------------
    // Loads a zone from a file with its doors and without denizens
    flecs::entity load_zone_unpopulated(SuiteWorld& world, const string& filepath)
    {
        flecs::entity zone = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(zone, stratum_format::read_zone(filepath));
        return zone;
    }

    //--------------------------------------------------------------------------
    // The access point type the suite enters zones at, and the level offset its relocator targets: a staircase leading up
    flecs::entity suite_entry_type(const stratum_sim::Session& session)
    {
        return stratum_sim::access_type(session.world().entities(), "staircase");
    }

    static constexpr stratum_sim::level_type suite_entry_offset = 1;

    //--------------------------------------------------------------------------
    // Loads the zone file into a region of its own, made from the region spec "staircase", and enters it at the suite's
    // entry type and level offset. Creates the game's character first if the game has none
    void enter_zone_file(stratum_sim::Game& game, const string& filepath)
    {
        if (!game.character().entity())
        {
            game.character().create(stratum_sim::CharacterSpec {});
        }

        flecs::entity region = game.world().create_region(suite_region_spec("staircase"));
        stratum_sim::Session& session = game.session();
        session.enter(game.world().load_zone(region, filepath), suite_entry_type(session), suite_entry_offset);

        return;
    }

    //--------------------------------------------------------------------------
    // A zone's layout as a DiscreteLayout
    stratum_sim::DiscreteLayout& zone_layout(stratum_sim::World& world, flecs::entity zone)
    {
        return stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
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
    // The layouts that are declared but not implemented; compiling their headers is most of the check
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
    void test_zone_layout()
    {
        report_group("zone layout");

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(4, 3)};
        open_walled(layout, 0, 0, 4, 3);

        report_case_equal("a wall on every side the rectangle faces out of", wall_counts(layout).sides, 14);
        report_case("a wall is on the side of the cell it faces", layout.wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::East)) != nullid);
        report_case("and nothing within the rectangle", layout.wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East)) == nullid);
        report_case("every side of a cell is a wall of its own",
            layout.wall_toward(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::West))
                != layout.wall_toward(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::South)));

        layout.set_flats(0.0f, 2.5f);
        report_case_equal("a layout has two flats", layout.flat_store_count(), 2);
        report_case("the floor is not a ceiling", !layout.flat(layout.floor_id()).ceiling());
        report_case("the ceiling is a ceiling", layout.flat(layout.ceiling_id()).ceiling());
        report_case("the floor keeps its height", layout.flat(layout.floor_id()).height() == 0.0f);
        report_case("the ceiling keeps its height", layout.flat(layout.ceiling_id()).height() == 2.5f);

        layout.set_room_count(2);
        report_case_equal("a layout keeps the room count it was given", layout.room_count(), 2);

        stratum_sim::DiscreteLayout moved = std::move(layout);
        report_case("a moved layout takes the grid", moved.grid().extent == stratum_sim::discrete_vector_type(4, 3));
        report_case("and the cells", moved.open(stratum_sim::discrete_point_type {1, 1}));
        report_case("and the flats", moved.flat_store_count() == 2 && moved.flat(moved.ceiling_id()).height() == 2.5f);

        return;
    }

    //--------------------------------------------------------------------------
    void test_rooms()
    {
        report_group("rooms");

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};
        layout.set_room_count(1);
        layout.set_cell_room(stratum_sim::discrete_point_type {0, 0}, 0);
        layout.set_cell_room(stratum_sim::discrete_point_type {1, 0}, 0);

        report_case_equal("a layout keeps how many rooms there are", layout.room_count(), 1);
        report_case("a cell names the room covering it", layout.room_at(stratum_sim::discrete_point_type {1, 0}) == 0);
        report_case("a cell no room covers has none", layout.room_at(stratum_sim::discrete_point_type {5, 5}) == nullid);

        return;
    }

    //--------------------------------------------------------------------------
    void test_grid()
    {
        report_group("grid");

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        report_case("a point maps to its cell", layout.cell_position_at(stratum_sim::point_type(2.7f, 3.2f, 0.0f)) == stratum_sim::discrete_point_type(2, 3));
        report_case("a cell maps back to its center", layout.position(stratum_sim::discrete_point_type {2, 3}).x() == 2.5f);
        report_case("round tripping a cell center returns the cell", layout.cell_position_at(layout.position(stratum_sim::discrete_point_type {5, 6})) == stratum_sim::discrete_point_type(5, 6));
        report_case("a point left of the origin lands in a negative cell", layout.cell_position_at(stratum_sim::point_type(-0.5f, 0.5f, 0.0f)) == stratum_sim::discrete_point_type(-1, 0));

        return;
    }

    //--------------------------------------------------------------------------
    void test_placement_index()
    {
        report_group("placement index");

        using entity_ids_type = stratum_sim::PlacementIndex::entity_ids_type;

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};
        unique_ptr<stratum_sim::PlacementIndex> index = layout.create_placement_index();
        auto* discrete_index = dynamic_cast<stratum_sim::DiscretePlacementIndex*>(index.get());

        report_case("a discrete layout creates an empty discrete placement index", discrete_index != nullptr && index->entity_count() == 0);
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
        report_case("the entities in a cell are in ascending id order", discrete_index->entities_at(stratum_sim::discrete_point_type {2, 3})
            == entity_ids_type {3, 7});
        report_case("and are found by a placement in the cell", index->entities_at(at(2, 3)) == entity_ids_type {3, 7});
        report_case("a cell with nothing in it has no entities", index->entities_at(at(0, 0)).empty());

        index->place(7, at(2, 3));
        report_case("placing an entity again in its cell does not add it twice", index->entity_count() == 3
            && index->entities_at(at(2, 3)) == entity_ids_type {3, 7});

        index->place(7, at(4, 3));
        report_case("placing it in another cell moves it", index->entities_at(at(2, 3)) == entity_ids_type {3}
            && index->entities_at(at(4, 3)) == entity_ids_type {5, 7});

        index->remove(3);
        report_case("a removed entity is in no cell", !index->contains(3) && index->entities_at(at(2, 3)).empty() && index->entity_count() == 2);

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

        report_case("the entities within a range are those a scan finds, over cells or over entities", within_as_scanned);
        report_case("a range reaches a cell center at exactly its distance", index->entities_within(at(2, 3), 2.0f) == scanned_within(at(2, 3), 2.0f)
            && !index->entities_within(at(2, 3), 2.0f).empty());

        return;
    }

    //--------------------------------------------------------------------------
    void test_flats()
    {
        report_group("flats");

        // A zone with no height data loads a constant
        stratum_sim::Flat floor(2.0f);

        report_case("a flat is a floor unless it says otherwise", !floor.ceiling());
        report_case("a flat with no room or cell heights has none", !floor.room_height(0) && !floor.cell_height(stratum_sim::discrete_point_type {1, 1}));

        floor.set_room_height(0, -0.25f);
        floor.set_cell_height(stratum_sim::discrete_point_type {1, 1}, 0.5f);
        report_case("a room height is kept", floor.room_height(0) == -0.25f);
        report_case("and a cell height", floor.cell_height(stratum_sim::discrete_point_type {1, 1}) == 0.5f);

        // A cell's floor height comes from the cell, then its room, then the flat
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        report_case("with no floor a cell sits on the grid's own plane", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 0.0f);

        layout.set_flats(1.5f, 4.0f);
        report_case("with one it sits on the floor", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 1.5f);

        layout.set_room_count(1);
        layout.set_cell_room(stratum_sim::discrete_point_type {1, 1}, 0);
        layout.set_cell_room(stratum_sim::discrete_point_type {2, 1}, 0);
        layout.flat(layout.floor_id()).set_room_height(0, 1.0f);
        report_case("a room's height overrides the flat's", layout.position(stratum_sim::discrete_point_type {2, 1}).z() == 1.0f);

        layout.flat(layout.floor_id()).set_cell_height(stratum_sim::discrete_point_type {1, 1}, 2.5f);
        report_case("and a cell's overrides the room's", layout.position(stratum_sim::discrete_point_type {1, 1}).z() == 2.5f);
        report_case("a cell no room covers takes the flat's", layout.height(layout.floor_id(), stratum_sim::discrete_point_type {5, 5}) == 1.5f);

        return;
    }

    //--------------------------------------------------------------------------
    void test_blocking()
    {
        report_group("blocking");

        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        // Two rooms one cell tall side by side, so their shared boundary is exactly one cell edge, walled from both sides
        open_walled(layout, 0, 0, 2, 1);
        open_walled(layout, 2, 0, 4, 1);

        // Within one room there is no wall at all, so nothing can block
        stratum_sim::Layout::crossable_function_type shut;
        report_case("cells inside one area are always crossable", layout.crossable(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::East), shut));
        report_case("crossing within an area is symmetric", layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::West), shut));
        report_case("a wall nothing crosses blocks", !layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), shut));

        // Open one side only, which is what a one-way passage is
        id_type wall_a = layout.wall_toward(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East));
        id_type wall_b = layout.wall_toward(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West));
        report_case("the two sides of the shared edge are two walls", wall_a != nullid && wall_b != nullid && wall_a != wall_b);

        // The caller's predicate decides which walls may be crossed
        std::set<id_type> open_walls {wall_a};
        stratum_sim::Layout::crossable_function_type crossable = [&](id_type id) { return open_walls.contains(id); };

        report_case("opening a wall lets a mover out", layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), crossable));
        report_case("the other side is untouched, so the way back stays shut", !layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

        open_walls.insert(wall_b);
        report_case("opening the other side opens the way back", layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

        // The two sides of an edge are independent, so one side may have a wall the other has not
        layout.set_cell_walls(stratum_sim::discrete_point_type {2, 0}, stratum_sim::DirectionMask::North | stratum_sim::DirectionMask::South
            | stratum_sim::DirectionMask::East);
        open_walls.clear();
        report_case("a wall on one side of an edge blocks from that side alone", !layout.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), crossable)
            && layout.crossable(stratum_sim::discrete_point_type {2, 0}, cardinal_direction(CardinalDirection::West), crossable));

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

        // advance grants time without skipping ahead, so no actor need be ready
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

        // next arrives where granting one unit at a time would
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

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        flecs::entity character = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}),
                stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::Scheduler scheduler;
        scheduler.add(character.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;

        id_type controlled = static_cast<id_type>(character.id());

        auto intend = [&](stratum_sim::value_type dx, stratum_sim::value_type dy)
        {
            character.set<stratum_sim::Intent>(stratum_sim::Intent {
                stratum_sim::vector_type(dx, dy, 0.0f), 1.0f, stratum_sim::vector_type()});
        };

        report_case("with no time granted nobody runs", clock.run(events, controlled) == nullid);

        clock.grant_turn();
        report_case("granting a turn makes an actor ready", scheduler.ready() == controlled);

        // With no Intent, the controlled actor is returned
        id_type waiting = clock.run(events, controlled);
        report_case("the controlled actor asking for nothing is returned", waiting == controlled);
        report_case_equal("and nothing has happened", events.size(), 0);

        // An intent runs and charges the turn
        intend(1.0f, 0.0f);
        id_type after = clock.run(events, controlled);
        report_case("running the only actor leaves nobody ready", after == nullid);
        report_case("and it moved", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});
        report_case_equal("its turn was charged", scheduler.energy(character.id()), 0);
        report_case("what it asked for was spent", character.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // An intent lasts one action, so a second turn on the same one does nothing
        clock.grant_turn();
        report_case("a spent intent is not acted on again", clock.run(events, controlled) == controlled);
        report_case("and it did not move", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});

        // Facing is taken before movement, so the two are one action each
        events.clear();
        character.set<stratum_sim::Intent>(stratum_sim::Intent {
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f), 1.0f, stratum_sim::vector_type(0.0f, 1.0f, 0.0f)});

        clock.run(events, controlled);
        report_case("facing is taken first", stratum_sim::placement_of(character).direction().y() == 1.0f);
        report_case("and the movement asked for is still owed", character.get<stratum_sim::Intent>().direction.x() == 1.0f);
        report_case("which remains until a turn is granted for it",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {2, 1});

        clock.grant_turn();
        clock.run(events, controlled);
        report_case("the movement follows on the next turn", layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type {3, 1});

        // An action that achieves nothing costs no turn, so walking into a wall is free
        clock.grant_turn();
        while (layout.cell_position_at(stratum_sim::placement_of(character).position()).i() > 1)
        {
            intend(-1.0f, 0.0f);
            clock.run(events, controlled);
            clock.grant_turn();
        }

        long long elapsed_before = scheduler.elapsed();
        intend(-1.0f, 0.0f);
        id_type balked = clock.run(events, controlled);
        report_case("an actor that achieved nothing is returned", balked == controlled);
        report_case("its turn was not charged", scheduler.energy(character.id()) >= stratum_sim::Scheduler::action_cost);
        report_case_equal("and no time passed", scheduler.elapsed(), static_cast<size_t>(elapsed_before));

        // Any other actor with no Intent waits. A world of its own, so the energies above do not decide the order
        {
            SuiteWorld world_two(id_allocator);
            flecs::entity zone_two = load_zone_unpopulated(world_two, "data/corridor.zone");

            const stratum_sim::DiscreteLayout& layout_two = zone_layout(world_two, zone_two);
            stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

            flecs::entity driven = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {1, 1}), east));
            flecs::entity other = world_two.create_entity(zone_two,
                stratum_sim::placement_type(layout_two.position(stratum_sim::discrete_point_type {3, 1}), east));

            stratum_sim::Scheduler scheduler_two;
            scheduler_two.add(driven.id(), 100);
            scheduler_two.add(other.id(), 200); // Faster, so it is reached first

            stratum_sim::Clock clock_two(world_two, scheduler_two);
            stratum_sim::EventQueue events_two;

            scheduler_two.advance(1);
            driven.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});

            id_type ran = clock_two.run(events_two, static_cast<id_type>(driven.id()));

            report_case("every ready actor ran", ran == nullid);
            report_case("the one asking for nothing waited", scheduler_two.energy(other.id()) < stratum_sim::Scheduler::action_cost);
            report_case("and the controlled actor acted even though it was the less ready",
                layout_two.cell_position_at(stratum_sim::placement_of(driven).position()) == stratum_sim::discrete_point_type {2, 1});
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The clock's driver, set when the zone is loaded
    void test_driver()
    {
        report_group("time driver");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity character = world.create_entity(zone,
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(character.id(), 100);

        stratum_sim::Clock clock(world, scheduler);
        stratum_sim::EventQueue events;
        id_type controlled = static_cast<id_type>(character.id());

        auto restart = [&](double units_per_second)
        {
            scheduler.clear();
            scheduler.add(character.id(), 100);
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
        report_case_equal("which is one action at that speed", scheduler.energy(character.id()), stratum_sim::Scheduler::action_cost);

        // The fraction is kept, so short frames add up
        restart(2.0);
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        report_case_equal("a frame too short to earn a unit grants none", scheduler.elapsed(), 0);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.125));
        }

        report_case_equal("but the remainder carries, so four of them earn one", scheduler.elapsed(), 1);

        // Real time does not skip ahead
        restart(2.0);
        report_case("real time does not skip ahead to make an actor ready",
            clock.advance(events, controlled, stratum_sim::Clock::duration_type::zero()) == nullid);
        report_case_equal("so no time passed", scheduler.elapsed(), 0);

        // Once elapsed time has made it ready, it acts on what it was asking for
        character.set<stratum_sim::Intent>(stratum_sim::Intent {east, 1.0f, stratum_sim::vector_type()});
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        report_case("an actor made ready by elapsed time acts",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(2, 1));

        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            stratum_sim::set_placement(character, stratum_sim::placement_type(layout.position(cell), east));
            character.set<stratum_sim::Intent>(stratum_sim::Intent {direction, 1.0f, stratum_sim::vector_type()});
        };

        // A held direction keeps the actor moving
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));
        }

        report_case("a direction held keeps stepping as the time is earned",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(4, 1));
        report_case("and is still being asked for", character.get<stratum_sim::Intent>().direction.x() == 1.0f);

        // Turn-based clears the Intent after one move
        scheduler.clear();
        scheduler.add(character.id(), 100);
        clock.set_driver(stratum_sim::Driver::TurnBased);
        place(stratum_sim::discrete_point_type {1, 1}, east);

        for (int n = 0; n < 3; ++n)
        {
            clock.advance(events, controlled);
        }

        report_case("turn-based takes one step from one intent",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(2, 1));

        // Against a wall the Intent is cleared; the case is that this returns at all
        restart(2.0);
        place(stratum_sim::discrete_point_type {1, 1}, stratum_sim::vector_type(-1.0f, 0.0f, 0.0f));
        clock.advance(events, controlled, stratum_sim::Clock::duration_type(0.5));

        report_case("a mover held against a wall does not move",
            layout.cell_position_at(stratum_sim::placement_of(character).position()) == stratum_sim::discrete_point_type(1, 1));
        report_case("and stops asking for what it could not have",
            character.get<stratum_sim::Intent>().direction.x() == 0.0f);

        // A host calls advance the same way under either driver
        stratum_sim::Game game_turn(g_suite_config);
        stratum_sim::Session& session_turn = game_turn.session();
        enter_zone_file(game_turn, "data/corridor.zone");

        stratum_sim::Game game_real(g_suite_config);
        stratum_sim::Session& session_real = game_real.session();
        session_real.set_driver(stratum_sim::Driver::RealTime);
        enter_zone_file(game_real, "data/corridor.zone");

        stratum_sim::EventQueue session_events;
        report_case("a turn-based session hands control out with no elapsed time given",
            session_turn.advance(session_events) == static_cast<id_type>(session_turn.character().entity().id()));
        report_case("a real-time one does not, having been given none",
            session_real.advance(session_events) == nullid);

        // The character acts through its own intent and commands, the door east of where it enters
        stratum_sim::Game game_door(g_suite_config);
        stratum_sim::Session& session_door = game_door.session();
        enter_zone_file(game_door, "data/door.zone");
        session_door.advance(session_events);

        stratum_sim::Character& character_door = session_door.character();
        character_door.set_intent(stratum_sim::Intent {stratum_sim::vector_type(), 1.0f, stratum_sim::vector_type(1.0f, 0.0f, 0.0f)});
        session_door.advance(session_events);
        report_case("the character takes the intent set on it", stratum_sim::placement_of(character_door.entity()).direction().x() == 1.0f);

        character_door.submit(stratum_sim::Interact {});
        report_case("a command the character submits waits on it", character_door.entity().has<stratum_sim::PendingCommand>());

        session_door.advance(session_events);
        flecs::entity door = stratum_sim::access_entity(stratum_sim::way_through(session_door.zone(),
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session_door.layout()).wall_toward(stratum_sim::discrete_point_type {1, 1},
                cardinal_direction(CardinalDirection::East))));
        report_case("and is taken the next time the character is ready", stratum_sim::access(door) == stratum_sim::AccessState::Open
            && !character_door.entity().has<stratum_sim::PendingCommand>());

        return;
    }

    //--------------------------------------------------------------------------
    void test_movement()
    {
        report_group("movement");

        // A corridor four cells long, walled at both ends, and a cell beyond its east end
        stratum_sim::DiscreteLayout layout {stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f, stratum_sim::discrete_vector_type(8, 8)};

        open_walled(layout, 0, 0, 4, 1);
        open_walled(layout, 4, 0, 5, 1);
        id_type corridor_end = layout.wall_toward(stratum_sim::discrete_point_type {3, 0}, cardinal_direction(CardinalDirection::East));

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        stratum_sim::placement_type start(layout.position(stratum_sim::discrete_point_type {0, 0}), east);

        stratum_sim::MoveResult one = layout.move(start, east, 1.0f);
        report_case("a step lands in the next cell", layout.cell_position_at(one.placement.position()) == stratum_sim::discrete_point_type(1, 0));
        report_case("a step with nothing in the way reports nothing", one.blocking_wall_id == nullid);
        report_case("moving does not turn", one.placement.direction().x() == 1.0f);

        stratum_sim::MoveResult three = layout.move(start, east, 3.0f);
        report_case("distance is in cells", layout.cell_position_at(three.placement.position()) == stratum_sim::discrete_point_type(3, 0));

        // The far wall of the corridor has a cell behind it but nothing crosses it
        stratum_sim::MoveResult far = layout.move(start, east, 7.0f);
        report_case("a mover stops at the first block", layout.cell_position_at(far.placement.position()) == stratum_sim::discrete_point_type(3, 0));
        report_case("and reports what stopped it", far.blocking_wall_id != nullid);
        report_case("what stopped it is the wall on its own side of the edge", far.blocking_wall_id == corridor_end);

        // Opening that wall lets it through
        stratum_sim::Layout::crossable_function_type crossable = [id = far.blocking_wall_id](id_type wall_id) { return wall_id == id; };
        stratum_sim::MoveResult through = layout.move(start, east, 7.0f, crossable, stratum_sim::Layout::blocker_function_type());
        report_case("an opened wall no longer stops it", layout.cell_position_at(through.placement.position()) == stratum_sim::discrete_point_type(4, 0));
        report_case("it stops at the next block instead", through.blocking_wall_id != nullid && through.blocking_wall_id != far.blocking_wall_id);

        // Solid in every other direction
        stratum_sim::MoveResult blocked = layout.move(start, north, 3.0f);
        report_case("a mover held against a wall does not move", layout.cell_position_at(blocked.placement.position()) == stratum_sim::discrete_point_type(0, 0));
        report_case("and still reports the wall", blocked.blocking_wall_id != nullid);

        report_case("a zero direction moves nothing", layout.cell_position_at(layout.move(start, stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 3.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));
        report_case("a zero distance moves nothing", layout.cell_position_at(layout.move(start, east, 0.0f).placement.position()) == stratum_sim::discrete_point_type(0, 0));

        // The direction snaps to a cardinal direction
        stratum_sim::MoveResult skewed = layout.move(start, stratum_sim::vector_type(0.9f, 0.4f, 0.0f), 2.0f);
        report_case("a direction off axis snaps to the nearest", layout.cell_position_at(skewed.placement.position()) == stratum_sim::discrete_point_type(2, 0));

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

        // Turning to an absolute direction
        stratum_sim::placement_type faced = layout.face(facing_east, north);
        report_case("facing north from east faces north", faced.direction().x() == 0.0f && faced.direction().y() == 1.0f);
        report_case("facing does not move", layout.same_location(facing_east, faced));

        stratum_sim::placement_type faced_off_axis = layout.face(facing_east, stratum_sim::vector_type(0.4f, 0.9f, 0.0f));
        report_case("a facing off axis snaps to the nearest",
            faced_off_axis.direction().x() == 0.0f && faced_off_axis.direction().y() == 1.0f);

        report_case("facing the way already faced changes nothing", layout.face(facing_east, east).direction().x() == 1.0f);
        report_case("a zero direction leaves the facing alone",
            layout.face(facing_east, stratum_sim::vector_type(0.0f, 0.0f, 0.0f)).direction().x() == 1.0f);

        // A relative turn resolves to the same facing direction as the absolute turn
        report_case("a resolved counterclockwise turn faces where turning does",
            layout.face(facing_east, layout.turn(facing_east, stratum_sim::Rotation::CCW).direction()).direction().y() == 1.0f);

        // Moving in a direction other than the facing direction leaves the facing direction unchanged
        stratum_sim::placement_type facing_north(start.position(), north);
        stratum_sim::MoveResult sidestep = layout.move(facing_north, east, 2.0f);
        report_case("absolute movement ignores the mover's facing", layout.cell_position_at(sidestep.placement.position()) == stratum_sim::discrete_point_type(2, 0));
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

        // A three by three room with a doorway off its east side, into a dead end
        auto room = load_open("data/room.zone");
        stratum_sim::DiscreteLayout& layout = room->layout();

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto at = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        report_case_equal("the file names no rooms", layout.room_count(), 0);

        // Inside the room nothing separates cells, so a middle cell reaches all four
        report_case_equal("a cell amid open floor has four neighbors", layout.neighbors(at(2, 2), room->crossable).size(), 4);
        report_case_equal("a corner cell is walled on two sides", layout.neighbors(at(1, 1), room->crossable).size(), 2);
        report_case_equal("an edge cell is walled on one", layout.neighbors(at(2, 1), room->crossable).size(), 3);

        report_case_equal("an open doorway is a neighbor", layout.neighbors(at(3, 2), room->crossable).size(), 4);
        report_case("a neighbor keeps the mover's facing", layout.neighbors(at(2, 2), room->crossable).front().direction().x() == 1.0f);

        id_type doorway = layout.wall_toward(stratum_sim::discrete_point_type {3, 2}, cardinal_direction(CardinalDirection::East));
        report_case("the wall into the doorway is found", doorway != nullid);
        report_case("and the doorway has no wall facing back",
            layout.wall_toward(stratum_sim::discrete_point_type {4, 2}, cardinal_direction(CardinalDirection::West)) == nullid);

        report_case("the layout names what a placement faces", layout.ahead(at(3, 2)) == doorway);
        report_case("and nothing where the facing meets no wall", layout.ahead(at(1, 1)) == nullid);
        report_case("a facing the grid names nothing faces nothing", layout.ahead(
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), stratum_sim::vector_type())) == nullid);

        // A doorway into a dead end has one wall
        std::set<id_type> open_walls {doorway};
        stratum_sim::Layout::crossable_function_type crossable = [&](id_type wall_id) { return open_walls.contains(wall_id); };

        open_walls.erase(doorway);
        report_case_equal("closing it removes the neighbor", layout.neighbors(at(3, 2), crossable).size(), 3);
        open_walls.insert(doorway);

        // Sight
        report_case("a cell sees itself", layout.visible(at(2, 2), at(2, 2), crossable));
        report_case("open floor is visible across", layout.visible(at(1, 1), at(3, 3), crossable));
        report_case("sight is blocked by a solid cell", !layout.visible(at(2, 2), at(2, 4), crossable));
        report_case("sight reaches through an open doorway", layout.visible(at(3, 2), at(4, 2), crossable));

        open_walls.erase(doorway);
        report_case("closing it blocks sight too", !layout.visible(at(3, 2), at(4, 2), crossable));

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_load_layout()
    {
        report_group("load layout");

        // A ring of eight open cells around one enclosed solid cell
        auto loop_zone = load_open("data/loop.zone");
        const stratum_sim::DiscreteLayout& loop = loop_zone->layout();

        report_case("the grid is the file's extent", loop.grid().extent == stratum_sim::discrete_vector_type(5, 5));
        report_case_equal("a layout has one floor and one ceiling", loop.flat_store_count(), 2);

        // Twelve sides face the solid outside the ring and four the solid cell it encloses
        report_case_equal("a wall on every side floor meets solid", wall_counts(loop).sides, 16);
        report_case_equal("and none walled from both sides, solid having no walls of its own", wall_counts(loop).both_sides, 0);

        report_case("an open cell reads as open", loop.open(stratum_sim::discrete_point_type {1, 1}));
        report_case("the enclosed solid cell does not", !loop.open(stratum_sim::discrete_point_type {2, 2}));
        report_case("a solid cell outside the ring does not", !loop.open(stratum_sim::discrete_point_type {0, 0}));

        report_case("adjacent open cells are passable", loop.crossable(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East), loop_zone->crossable));
        report_case("an open cell blocks against solid", !loop.crossable(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::South), loop_zone->crossable));
        report_case("an open cell blocks against enclosed solid", !loop.crossable(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North), loop_zone->crossable));

        // The file's first line is the highest row, so the origin is lower left
        report_case("row order puts the origin at the lower left", loop.open(stratum_sim::discrete_point_type {3, 1}));

        // A doorway block between two floor cells
        auto door_zone = load_open("data/door.zone");
        const stratum_sim::DiscreteLayout& door = door_zone->layout();

        report_case("a doorway connects both ways", door.crossable(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East), door_zone->crossable)
            && door.crossable(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::East), door_zone->crossable));

        report_case("a block is walled sideways", !door.crossable(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North), door_zone->crossable)
            && !door.crossable(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::South), door_zone->crossable));
        report_case("and it is a wall that says so", door.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::North)) != nullid);

        // Between a block and floor the wall is on the floor's side, so the block's own side of it is bare
        report_case("a block sees no wall on the way out", door.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::East)) == nullid);
        report_case("but the floor sees one on the way in", door.wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::West)) != nullid);

        report_case_equal("a zone defining no rooms has none", door.room_count(), 0);

        // The loader puts the notation on the zone, the layout not storing it
        report_case("a block notation file's zone has it",
            door_zone->zone.get<stratum_sim::ZoneNotation>().notation == stratum_format::Notation::Block);
        report_case("and a wall notation one's has that",
            load_open("data/walls.zone")->zone.get<stratum_sim::ZoneNotation>().notation == stratum_format::Notation::Wall);

        // A real generated zone, for scale rather than for exact counts
        auto generated_zone = load_open("data/generated.zone");
        const stratum_sim::DiscreteLayout& generated = generated_zone->layout();

        report_case("a generated zone reads", wall_counts(generated).sides > 0);
        report_case("its grid matches the file", generated.grid().extent == stratum_sim::discrete_vector_type(31, 31));
        report_case_equal("its rooms are the ones the file defines", generated.room_count(), 8);

        size_t room_cells = 0;
        for (stratum_sim::coord_type j = 0; j < 31; ++j)
        {
            for (stratum_sim::coord_type i = 0; i < 31; ++i)
            {
                room_cells += generated.room_at(stratum_sim::discrete_point_type {i, j}) != nullid ? 1 : 0;
            }
        }

        // 292 of its 961 cells are room floor and 247 corridor, the rest solid, doorways and stairways
        report_case_equal("every room cell belongs to a room", room_cells, 292);
        report_case("a corridor cell belongs to no room", generated.room_at(stratum_sim::discrete_point_type {7, 29}) == nullid);
        report_case("a room cell names the room that covers it", generated.room_at(stratum_sim::discrete_point_type {1, 1}) != nullid);

        // Two areas never share an edge in the block notation: a block lies between them, and a wall is on the floor's
        // side of it alone. Walling an edge from both sides is what the wall notation can express and this cannot
        report_case_equal("no edge of a block zone is walled from both sides", wall_counts(generated).both_sides, 0);

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
    // Counts the open cells reachable from the first, crossing only the walls that crossable allows
    size_t cells_reached(const stratum_sim::DiscreteLayout& layout, const stratum_sim::Layout::crossable_function_type& crossable)
    {
        const stratum_sim::discrete_vector_type extent = layout.grid().extent;
        auto slot = [&extent](const stratum_sim::discrete_point_type& cell_position) { return static_cast<size_t>(cell_position.j()) * extent.x() + cell_position.i(); };

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
            stratum_sim::discrete_point_type cell_position = pending.back();
            pending.pop_back();
            ++count;

            const stratum_sim::discrete_vector_type steps[4] {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (const stratum_sim::discrete_vector_type& step : steps)
            {
                stratum_sim::discrete_point_type t = cell_position + step;

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
    void test_load_walls()
    {
        report_group("load wall layout");

        // Two rooms of two by three cells sharing one wall, with a door in the middle of it
        auto walls_zone = load_open("data/walls.zone");
        const stratum_sim::DiscreteLayout& walls = walls_zone->layout();
        const stratum_sim::Layout::crossable_function_type& crossable = walls_zone->crossable;

        size_t door_count = 0;
        walls_zone->world.entities().query_builder<const stratum_sim::Access>()
            .with(flecs::ChildOf, walls_zone->zone)
            .build()
            .each([&](flecs::entity, const stratum_sim::Access&) { ++door_count; });

        report_case("the grid is the file's extent", walls.grid().extent == stratum_sim::discrete_vector_type(4, 3));
        report_case_equal("each room is walled on every side it presents", wall_counts(walls).sides, 24);
        report_case_equal("a zone defining no rooms has none", walls.room_count(), 0);

        // A wall with a room behind it instead of a solid cell, which the block notation cannot express
        report_case_equal("the shared wall is walled from both sides wherever both are floor", wall_counts(walls).both_sides, 4);
        report_case_equal("one pair of it is the door", door_count, 1);

        // At this seed the door is in the top row of the shared wall, and the staircases in the middle row of each room
        report_case("a mover crosses at the door", walls.crossable(stratum_sim::discrete_point_type {1, 2}, cardinal_direction(CardinalDirection::East), crossable));
        report_case("and back through it", walls.crossable(stratum_sim::discrete_point_type {2, 2}, cardinal_direction(CardinalDirection::West), crossable));
        report_case("but not below it", !walls.crossable(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::West), crossable));
        report_case("nor two below", !walls.crossable(stratum_sim::discrete_point_type {1, 0}, cardinal_direction(CardinalDirection::East), crossable));
        report_case("and never within one room", walls.crossable(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::East), crossable));

        report_case_equal("every open cell is reachable through the door", cells_reached(walls, crossable), open_cells(walls));

        // A generated zone, for scale and for the properties the generator promises
        auto generated_zone = load_open("data/catagen.zone");
        const stratum_sim::DiscreteLayout& generated = generated_zone->layout();

        report_case("a generated zone reads", wall_counts(generated).sides > 0);
        report_case("its grid is the file's extent", generated.grid().extent == stratum_sim::discrete_vector_type(24, 14));

        size_t solid = 0;
        for (int j = 0; j < 14; ++j)
        {
            for (int i = 0; i < 24; ++i)
            {
                solid += generated.open(stratum_sim::discrete_point_type {i, j}) ? 0 : 1;
            }
        }

        report_case("a zone below full density leaves solid cells", solid > 0, format("{} solid cells", solid));

        report_case("an edge walled from both sides counts once from each", wall_counts(generated).both_sides % 2 == 0);

        report_case_equal("every open cell is reachable from the first", cells_reached(generated, generated_zone->crossable),
            open_cells(generated));

        // At full density with no spacing every cell is room, so a solid cell here is the spacing
        // and nothing else, and nothing but a corridor joins any two rooms
        auto spaced_zone = load_open("data/catagen_spaced.zone");
        const stratum_sim::DiscreteLayout& spaced = spaced_zone->layout();

        report_case("a spaced zone reads", wall_counts(spaced).sides > 0);
        report_case("its grid is the file's extent", spaced.grid().extent == stratum_sim::discrete_vector_type(30, 18));

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

        report_case_equal("every open cell is still reachable from the first", cells_reached(spaced, spaced_zone->crossable),
            open_cells(spaced));

        return;
    }

    //--------------------------------------------------------------------------
    // The door entities that a zone file's doors load as
    void test_doors()
    {
        report_group("doors");

        // Two rooms sharing one wall, with a single door in it
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/walls.zone");

        // A door is an entity in the zone with access, and its door facings are its children
        flecs::entity door;
        size_t door_count = 0;
        world.entities().query_builder<const stratum_sim::Access>()
            .with(flecs::ChildOf, zone)
            .build()
            .each([&](flecs::entity e, const stratum_sim::Access&) { door = e; ++door_count; });

        report_case_equal("one door for the one the file names", door_count, 1);

        report_case("the door is a child of the zone", door && door.parent() == zone);
        report_case("it is closed until something opens it", stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        stratum_sim::World::entities_type facings;
        door.children([&](flecs::entity e) { facings.push_back(e); });

        report_case_equal("it has two facings", facings.size(), 2);
        report_case("each is sited on a wall", facings.size() == 2
            && stratum_sim::wall_id_of(facings[0].get<stratum_sim::Site>()).has_value()
            && stratum_sim::wall_id_of(facings[1].get<stratum_sim::Site>()).has_value());

        // The two walls are the two sides of one edge, which is what makes them one door
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        id_type wall_a = *stratum_sim::wall_id_of(facings[0].get<stratum_sim::Site>());
        id_type wall_b = *stratum_sim::wall_id_of(facings[1].get<stratum_sim::Site>());

        std::set<id_type> edge_sides {layout.wall_toward(stratum_sim::discrete_point_type {1, 2}, cardinal_direction(CardinalDirection::East)),
            layout.wall_toward(stratum_sim::discrete_point_type {2, 2}, cardinal_direction(CardinalDirection::West))};
        report_case("on the two sides of one edge", wall_a != wall_b && edge_sides.contains(wall_a) && edge_sides.contains(wall_b));
        std::set<id_type> file_door_walls;
        vector<vector<stratum_format::WallAddress>> file_doors = stratum_format::door_walls(stratum_format::read_zone("data/walls.zone").cells);

        for (const vector<stratum_format::WallAddress>& addresses : file_doors)
        {
            for (const stratum_format::WallAddress& address : addresses)
            {
                file_door_walls.insert(layout.wall_toward(address.cell_position, -address.normal));
            }
        }

        report_case("and the walls the file called a door", file_doors.size() == 1 && file_door_walls.contains(wall_a)
            && file_door_walls.contains(wall_b));

        // The queries for the entity on a wall and the entity whose access applies to it
        report_case("a wall's way through is the facing on it", stratum_sim::way_through(zone, wall_a) == facings[0]
            || stratum_sim::way_through(zone, wall_a) == facings[1]);
        report_case("a wall with nothing on it has none",
            !stratum_sim::way_through(zone, layout.wall_toward(stratum_sim::discrete_point_type {1, 2}, cardinal_direction(CardinalDirection::North))));
        report_case("a facing has its door's access", stratum_sim::access_entity(facings[0]) == door
            && stratum_sim::access_entity(facings[1]) == door);

        // Cell (1, 2) faces east across the shared wall, which is where the door is
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 2}), east));
        flecs::entity other = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        // Crossing the wall, which depends on the actor
        report_case("a closed door is not crossable", !stratum_sim::crossable(zone, wall_a, actor));

        door.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Open});
        report_case("an open one is, from either side", stratum_sim::crossable(zone, wall_a, actor) && stratum_sim::crossable(zone, wall_b, actor));

        facings[0].set<stratum_sim::Detection>(stratum_sim::Detection {stratum_sim::DetectionState::Concealed, {}});
        report_case("a concealed side is a wall", !stratum_sim::crossable(zone, wall_a, actor) && !stratum_sim::crossable(zone, wall_a, other));
        report_case("and the other side of it is not concealed", stratum_sim::crossable(zone, wall_b, actor));

        facings[0].get_mut<stratum_sim::Detection>().actor_states.push_back(
            stratum_sim::ActorDetection {actor.get<stratum_sim::PersistentId>(), stratum_sim::DetectionState::Revealed});
        report_case("an actor who has found it crosses", stratum_sim::crossable(zone, wall_a, actor));
        report_case("and one who has not does not", !stratum_sim::crossable(zone, wall_a, other));

        facings[0].remove<stratum_sim::Detection>();
        door.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Locked});
        report_case("a locked door is not crossable", !stratum_sim::crossable(zone, wall_a, actor));

        // Interacting with the door an actor faces opens and shuts it
        stratum_sim::EventQueue door_events;
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact {}), door_events);
        report_case("interacting leaves a locked door locked", stratum_sim::access(door) == stratum_sim::AccessState::Locked);

        door.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Closed});
        door_events.clear();
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact {}), door_events);

        report_case("interacting with the door opens it", stratum_sim::access(door) == stratum_sim::AccessState::Open);

        door_events.clear();
        stratum_sim::apply(world, stratum_sim::Command(actor.id(), stratum_sim::Interact {}), door_events);

        report_case("and again shuts it", stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        actor.destruct();
        other.destruct();

        // Deleting the door deletes its door facings
        door.destruct();
        report_case("deleting a door takes its facings", !facings[0].is_alive() && !facings[1].is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    // One door entity per door glyph, its door facings on the walls of that doorway, and no door facing anywhere else
    bool doors_have_entities(const stratum_format::Document& document, string& detail)
    {
        vector<vector<stratum_format::WallAddress>> doors = stratum_format::door_walls(document.cells);
        map<stratum_format::stored_entity_id_type, vector<stratum_format::WallAddress>> walls_by_door;
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

            const auto* address = wall_address(entity);
            if (address == nullptr || !entity.parent_id.has_value())
            {
                detail = format("a door facing with id {} that is not a child attached to a wall", entity.id);
                return false;
            }

            walls_by_door[*entity.parent_id].push_back(*address);
        }

        if (door_count != doors.size() || walls_by_door.size() != doors.size())
        {
            detail = format("{} door glyphs, {} door entities and {} of them with door facings", doors.size(), door_count, walls_by_door.size());
            return false;
        }

        for (const vector<stratum_format::WallAddress>& walls : doors)
        {
            if (none_of(walls_by_door.begin(), walls_by_door.end(), [&walls](const auto& entry)
                { return is_permutation(entry.second.begin(), entry.second.end(), walls.begin(), walls.end()); }))
            {
                const stratum_format::WallAddress& wall = walls.front();
                detail = format("no door with a door facing on each wall of the doorway at [{}, {}] normal [{}, {}]",
                    wall.cell_position.i(), wall.cell_position.j(), wall.normal.x(), wall.normal.y());
                return false;
            }
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_door_entities()
    {
        report_group("door entities");

        for (const string filepath : {"data/catagen.zone", "data/catagen_spaced.zone", "data/corridor.zone", "data/door.zone", "data/generated.zone",
            "data/loop.zone", "data/room.zone", "data/walls.zone"})
        {
            string detail;
            report_case(format("{} has a door entity at each door", filepath), doors_have_entities(stratum_format::read_zone(filepath), detail), detail);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A component whose fields inherit, with a struct member whose fields inherit in turn
    struct SuiteTint
    {
        optional<float> red;
        optional<float> blue;
    };

    struct SuiteFinish
    {
        optional<int> grain;
        optional<float> gloss;
        SuiteTint tint;
    };

    //--------------------------------------------------------------------------
    // A component referring to another entity
    struct SuiteLever
    {
        stratum_sim::EntityReference opens;
    };

    //--------------------------------------------------------------------------
    // Registers each as the component that the file component with its name decodes into
    void register_suite_components(flecs::world& entities)
    {
        stratum_sim::register_optional<int>(entities);
        stratum_sim::register_optional<float>(entities);
        entities.component<SuiteTint>()
            .member(&SuiteTint::red, "red")
            .member(&SuiteTint::blue, "blue");
        entities.component<SuiteFinish>()
            .member(&SuiteFinish::grain, "grain")
            .member(&SuiteFinish::gloss, "gloss")
            .member(&SuiteFinish::tint, "tint");
        entities.component<SuiteLever>()
            .member(&SuiteLever::opens, "opens");

        stratum_sim::register_file_component<SuiteFinish>(entities, "finish");
        stratum_sim::register_file_component<SuiteLever>(entities, "lever");
        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_loaded_entities()
    {
        report_group("loaded entities");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        const flecs::world& entities = world.entities();

        // data/door.zone: a door with a door facing on each wall of its doorway, and a staircase up on the wall above
        auto loaded = [&](const stratum_format::Document& document)
        {
            flecs::entity zone = world.create_zone(world.region,
                document.zone_id.has_value() ? stratum_sim::PersistentId {*document.zone_id} : world.next_id());
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

        report_case_equal("the door and the staircase are the zone's own children", built.size(), 2);
        report_case("each has a persistent id", ranges::all_of(built, [](flecs::entity e) { return e.has<stratum_sim::PersistentId>(); }));

        flecs::entity door = built.size() == 2 ? built[0] : flecs::entity();
        flecs::entity staircase = built.size() == 2 ? built[1] : flecs::entity();

        report_case("the door is its type's", door && door.target(flecs::IsA) == stratum_sim::entity_type(entities, "door"));
        report_case("with its own access, closed", door && door.owns<stratum_sim::Access>() && stratum_sim::access(door) == stratum_sim::AccessState::Closed);

        vector<flecs::entity> facings = door ? zone_children(door) : vector<flecs::entity> {};
        id_type west_wall = layout.wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::West));
        flecs::entity facing_west = stratum_sim::wall_entity(zone, west_wall);

        report_case_equal("its door facings are its children", facings.size(), 2);
        report_case("through Parent, each with its persistent id and its door's access", ranges::all_of(facings, [&](flecs::entity e)
            { return e.has<flecs::Parent>() && e.has<stratum_sim::PersistentId>() && stratum_sim::access_entity(e) == door; }));
        report_case("each sited on its wall, which lists it", facing_west && facing_west.try_get<stratum_sim::AttachedEntities>() != nullptr
            && facing_west.get<stratum_sim::AttachedEntities>().entities == vector<flecs::entity> {facings.empty() ? flecs::entity() : facings[0]});

        flecs::entity staircase_type = stratum_sim::entity_type(world.entities(), "staircase");
        report_case("an entity whose type is not loaded is an instance of a type of that name, made without the components a type sets",
            staircase && staircase_type && staircase.has(flecs::IsA, staircase_type) && !staircase_type.has<stratum_sim::Access>());
        report_case("its relocator has its target", staircase && staircase.has<stratum_sim::Relocator>()
            && get<stratum_format::RelocatorTarget>(staircase.get<stratum_sim::Relocator>().target)
                == stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {1}, nullopt});
        report_case("and it is the access point, as the file lists it", staircase && staircase.has<stratum_sim::AccessPoint>()
            && stratum_sim::entry_point(zone, stratum_sim::access_type(entities, "staircase"), 1) == staircase);

        // The same zone saved: every id in the file is persistent
        string saved_text = R"({ "format": "quetzal.discrete_layout", "version": "0.4", "zone": { "id": 900 },
            "layout": { "components": { "access_points": [ { "entity": 904 } ] } },
            "cells": { "notation": "block", "extent": [5, 3], "origin": [0.0, 0.0, 0.0], "scale": 1.0, "rows": [ "#<###", "#.+.#", "#####" ] },
            "entities": [
              { "id": 901, "type": "door" },
              { "id": 902, "type": "door_facing", "components": { "parent": { "entity": 901 }, "site": { "relation": "on", "wall": { "cell_position": [3, 1], "normal": [1, 0] } } } },
              { "id": 903, "type": "door_facing", "components": { "parent": { "entity": 901 }, "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } },
              { "id": 904, "type": "staircase", "components": { "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [0, -1] } },
                "relocator": { "type": "staircase", "target": { "entity": 7713 }, "order": 0, "placement": { "cell_position": [1, 1], "orientation": [0, -1] } } } } ] })";
        istringstream saved_stream(saved_text);
        flecs::entity saved = loaded(stratum_format::read_zone(saved_stream, "saved"));

        report_case("a saved zone keeps its persistent id", saved.get<stratum_sim::PersistentId>() == stratum_sim::PersistentId {900});
        report_case("and each of its entities its own", world.entity(stratum_sim::PersistentId {901}).parent() == saved
            && world.entity(stratum_sim::PersistentId {902}).parent() == world.entity(stratum_sim::PersistentId {901}));

        flecs::entity bound = world.entity(stratum_sim::PersistentId {904});
        const auto* target = bound ? get_if<stratum_sim::EntityReference>(&bound.get<stratum_sim::Relocator>().target) : nullptr;
        report_case("a bound target refers to a relocator in a zone not loaded", target != nullptr
            && target->persistent_id == stratum_sim::PersistentId {7713} && !target->entity);

        return;
    }

    //--------------------------------------------------------------------------
    void test_decoded_components()
    {
        report_group("decoded components");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        const flecs::world& entities = world.entities();
        register_suite_components(world.entities());

        // Reads data/door.zone with the given components for the layout, the door, and the staircase, and the given room,
        // wall, and flat sections
        auto door_level = [](const string& layout_components, const string& door_components, const string& staircase_components,
            const string& layout_sections = "")
        {
            string text = format(R"({{ "format": "quetzal.discrete_layout", "version": "0.4",
                "layout": {{ "components": {{ "access_points": [ {{ "entity": 4 }} ]{} }} }},
                "cells": {{ "notation": "block", "extent": [5, 3], "origin": [0.0, 0.0, 0.0], "scale": 1.0, "rows": [ "#<###", "#.+.#", "#####" ] }},
                {}
                "entities": [
                  {{ "id": 1, "type": "door"{} }},
                  {{ "id": 2, "type": "door_facing", "components": {{ "parent": {{ "entity": 1 }}, "site": {{ "relation": "on", "wall": {{ "cell_position": [3, 1], "normal": [1, 0] }} }} }} }},
                  {{ "id": 3, "type": "door_facing", "components": {{ "parent": {{ "entity": 1 }}, "site": {{ "relation": "on", "wall": {{ "cell_position": [1, 1], "normal": [-1, 0] }} }} }} }},
                  {{ "id": 4, "type": "staircase", "components": {{ "site": {{ "relation": "on", "wall": {{ "cell_position": [1, 1], "normal": [0, -1] }} }},
                    "relocator": {{ "type": "staircase", "target": "+1", "order": 0, "placement": {{ "cell_position": [1, 1], "orientation": [0, -1] }} }}{} }} }} ] }})",
                layout_components.empty() ? "" : ", " + layout_components,
                layout_sections.empty() ? "" : layout_sections + ",",
                door_components.empty() ? "" : R"(, "components": { )" + door_components + " }",
                staircase_components.empty() ? "" : ", " + staircase_components);

            istringstream stream(text);
            return stratum_format::read_zone(stream, "door level");
        };

        stratum_sim::entity_type(entities, "door").set<SuiteFinish>(SuiteFinish {{}, 0.875f, {}});

        stratum_format::Document document = door_level(R"("finish": { "gloss": 0.25 })", R"("finish": { "grain": 3, "tint": { "red": 0.5 } })",
            R"("lever": { "opens": { "entity": 1 } })");
        report_case("a document whose components are registered and fit is accepted", !stratum_sim::check_buildable(entities, document).has_value());

        flecs::entity zone = world.create_zone(world.region, world.next_id());
        vector<flecs::entity> referred_to = stratum_sim::load_layout(zone, document);

        flecs::entity door = stratum_sim::access_entity(stratum_sim::way_through(zone,
            zone_layout(world, zone).wall_toward(stratum_sim::discrete_point_type {3, 1}, cardinal_direction(CardinalDirection::West))));
        flecs::entity staircase = stratum_sim::entry_point(zone, stratum_sim::access_type(entities, "staircase"), 1);

        const auto* finish = door.try_get<SuiteFinish>();
        report_case("a component object is decoded into the component registered under its name", finish != nullptr
            && finish->grain == 3 && finish->tint.red == 0.5f);
        report_case("a field it does not state is left unset, where the component inherits", finish != nullptr && !finish->gloss.has_value()
            && !finish->tint.blue.has_value());
        report_case("and comes from the entity's type along the chain", world.property<SuiteFinish>(door).has_value()
            && world.property<SuiteFinish>(door)->gloss == 0.875f);
        report_case("the layout's components are the zone's", zone.try_get<SuiteFinish>() != nullptr && zone.get<SuiteFinish>().gloss == 0.25f);

        const auto* lever = staircase.try_get<SuiteLever>();
        report_case("a reference is translated to the entity's persistent id", lever != nullptr && lever->opens.entity == door
            && lever->opens.persistent_id == door.get<stratum_sim::PersistentId>());
        report_case("and what it refers to is returned for the world index", referred_to == vector<flecs::entity> {door});

        auto rejected = [&](const string& layout_components, const string& door_components, const string& expected)
        {
            optional<string> error = stratum_sim::check_buildable(entities, door_level(layout_components, door_components, ""));
            return error.has_value() && *error == expected;
        };

        report_case("a name nothing registers is rejected", rejected("", R"("patina": { "age": 3 })",
            "entities[0].components.patina: no component is registered under the name"));
        report_case("a field the component does not have is rejected", rejected("", R"("finish": { "shine": 1 })",
            "entities[0].components.finish.shine: not a field of the component"));
        report_case("a value its field cannot take is rejected", rejected(R"("finish": { "grain": "oak" })", "",
            "layout.components.finish.grain: a value its field cannot take"));
        report_case("a reference where none is expected is rejected", rejected("", R"("lever": { "opens": 1 })",
            "entities[0].components.lever.opens: not a reference to an entity"));

        // Room 0 is west of the door and room 1 east of it. Each room, wall, and flat section states part of a finish
        string layout_sections = R"("rooms": [
              { "cell_position": [1, 1], "components": { "finish": { "gloss": 0.375 } } },
              { "cell_position": [3, 1], "components": { "finish": { "grain": 4, "gloss": 0.125, "tint": { "red": 0.875 } } } } ],
            "walls": [ { "wall": { "cell_position": [1, 1], "normal": [1, 0] }, "components": { "finish": { "grain": 2 } } } ],
            "flats": [ { "role": "floor", "components": { "finish": { "gloss": 0.5, "tint": { "blue": 0.75 } } },
              "rooms": [ { "room": 1, "components": { "finish": { "tint": { "red": 0.25 } } } } ],
              "cells": [ { "cell_position": [3, 1], "components": { "finish": { "grain": 9 } } } ] } ])";

        stratum_format::Document sectioned = door_level(R"("finish": { "gloss": 0.0625, "tint": { "blue": 0.125 } })", "", "",
            layout_sections);
        report_case("the components of rooms, walls, and flats that are registered and fit are accepted",
            !stratum_sim::check_buildable(entities, sectioned).has_value());

        flecs::entity sectioned_zone = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(sectioned_zone, sectioned);
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, sectioned_zone);

        flecs::entity east_room = stratum_sim::room_entity(sectioned_zone, 1);
        flecs::entity floor = stratum_sim::flat_entity(sectioned_zone, layout.floor_id());
        flecs::entity floor_east = stratum_sim::flat_room_entity(sectioned_zone, layout.floor_id(), 1);
        flecs::entity floor_cell = stratum_sim::flat_cell_entity(sectioned_zone, layout.floor_id(), stratum_sim::discrete_point_type {3, 1});
        flecs::entity west_wall = stratum_sim::wall_entity(sectioned_zone,
            layout.wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::West)));

        report_case("every room and each flat has an entity", stratum_sim::room_entity(sectioned_zone, 0) && east_room && floor
            && stratum_sim::flat_entity(sectioned_zone, layout.ceiling_id()));
        report_case("a room's components are its entity's", east_room && east_room.get<SuiteFinish>().grain == 4);
        report_case("a flat's override for a room or a cell is below the flat's entity", floor_east && floor_cell
            && floor_east.parent() == floor && floor_cell.parent() == floor);

        optional<SuiteFinish> cell_finish = world.property<SuiteFinish>(floor_cell);
        report_case("a flat cell takes what it leaves unset from the flat's override for its room first",
            cell_finish.has_value() && cell_finish->grain == 9 && cell_finish->tint.red == 0.25f);
        report_case("then from the room", cell_finish.has_value() && cell_finish->gloss == 0.125f);
        report_case("then from the flat, before the layout", cell_finish.has_value() && cell_finish->tint.blue == 0.75f);

        optional<SuiteFinish> wall_finish = west_wall ? world.property<SuiteFinish>(west_wall) : nullopt;
        report_case("a wall takes what it leaves unset from its room, then the layout", wall_finish.has_value() && wall_finish->grain == 2
            && wall_finish->gloss == 0.375f && wall_finish->tint.blue == 0.125f);

        optional<string> room_error = stratum_sim::check_buildable(entities, door_level("", "", "",
            R"("rooms": [ { "cell_position": [1, 1], "components": { "patina": { "age": 3 } } } ])"));
        report_case("a room's component that nothing registers is rejected", room_error.has_value()
            && *room_error == "rooms[0].components.patina: no component is registered under the name");

        return;
    }

    //--------------------------------------------------------------------------
    void test_game()
    {
        report_group("game");

        const stratum_sim::Config config {"out/elsewhere/saves", "out/elsewhere/working", 3};
        stratum_sim::Game game(config);

        report_case("a game keeps the config it was constructed with", game.config().saves_directory == config.saves_directory
            && game.config().zone_count_max == config.zone_count_max);
        report_case("and its world keeps no more zones loaded than the config's maximum", game.world().zone_count_max() == 3);

        // A game played in two regions, with a character carrying something, then cleared
        stratum_sim::Game played(g_suite_config);
        stratum_sim::World& world = played.world();
        stratum_sim::Session& session = played.session();
        played.generation() = suite_generation();

        enter_zone_file(played, "data/corridor.zone");
        flecs::entity character = session.character().entity();
        stratum_sim::PersistentId character_id = character.get<stratum_sim::PersistentId>();
        flecs::entity carried = world.entities().entity();
        stratum_sim::set_parent(carried, character);

        flecs::entity elsewhere = world.create_zone(world.create_region(suite_region_spec("door")), stratum_sim::ZoneSpec {});
        world.make_reference(world.create_entity(elsewhere, stratum_sim::placement_type()));

        played.clear();

        report_case("clearing a game ends play", !session.loaded() && session.scheduler().count() == 0);
        report_case("deletes the character and what it carries", !session.character().entity() && !character.is_alive()
            && !carried.is_alive() && !world.entity(character_id));
        report_case("and every region and zone", world.entities().count<stratum_sim::Region>() == 0
            && world.entities().count<stratum_sim::Zone>() == 0 && world.zone_count() == 0);
        report_case("leaving no entity with a persistent id", world.entities().count<stratum_sim::PersistentId>() == 0);
        report_case("and keeps the registrations", stratum_sim::entity_type(world.entities(), "door")
            && stratum_sim::access_type(world.entities(), "staircase") && stratum_sim::file_component(world.entities(), "name") != 0
            && played.generation().m_generator_selectors.contains("door"));

        // One game after another, each created from specs. The games' directories are made afresh on every run
        filesystem::remove_all(g_suite_config.saves_directory);
        filesystem::remove_all(g_suite_config.working_directory);
        stratum_sim::Game sequence(g_suite_config);
        stratum_sim::Session& sequence_session = sequence.session();
        stratum_sim::World& sequence_world = sequence.world();
        sequence.generation() = suite_generation();
        flecs::entity staircase_type = stratum_sim::access_type(sequence_world.entities(), "staircase");

        const stratum_sim::RegionSpec region_spec {{string(stratum_sim::generator_selector_key), "staircase"},
            {string(stratum_sim::entry_access_key), "staircase,1"}, {format("{}{}", stratum_sim::zone_prefix, stratum_sim::level_key), "-1"}};

        auto persistent_id_of = [](flecs::entity e) { return e.get<stratum_sim::PersistentId>().id; };
        auto name_of = [](flecs::entity e) { return e.has<stratum_sim::DisplayName>() ? e.get<stratum_sim::DisplayName>().name : string(); };

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Ailis"});
        flecs::entity first_character = sequence_session.character().entity();
        flecs::entity first_zone = sequence_session.zone();
        flecs::entity first_region = stratum_sim::region_of(first_zone);

        report_case("creating a game makes its directory in the working directory, named by the character",
            filesystem::is_directory(g_suite_config.working_directory / "Ailis"));
        report_case("and the character, with the spec's name and the first id", persistent_id_of(first_character) == 0
            && name_of(first_character) == "Ailis");
        report_case("and starts play in the first zone of a region made from the spec, at the entry point it states",
            sequence_session.loaded() && first_zone.get<stratum_sim::ZoneLevel>().level == -1
            && first_region.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && stratum_sim::placement_of(first_character).position()
                == stratum_sim::entry_point(first_zone, staircase_type, 1).get<stratum_sim::AccessPoint>().placement.position());

        flecs::entity second_zone = stratum_sim::zone_of(sequence_world.relocate_through(first_character,
            stratum_sim::entry_point(first_zone, staircase_type, -1)));
        report_case("the first game is played into a second zone", second_zone && second_zone != first_zone && sequence_world.zone_count() == 2);

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Bran"});
        flecs::entity second_character = sequence_session.character().entity();

        report_case("creating another game leaves none of the first game's regions, zones or character", !first_region.is_alive()
            && !first_zone.is_alive() && !second_zone.is_alive() && !first_character.is_alive()
            && sequence_world.entities().count<stratum_sim::Region>() == 1 && sequence_world.zone_count() == 1);
        report_case("nor its directory, as the working directory has only the game in progress",
            !filesystem::exists(g_suite_config.working_directory / "Ailis") && filesystem::is_directory(g_suite_config.working_directory / "Bran"));

        stratum_sim::PersistentId::type persistent_id_max = 0;
        sequence_world.entities().each([&](flecs::entity, const stratum_sim::PersistentId& persistent_id)
        {
            persistent_id_max = max(persistent_id_max, persistent_id.id);
        });

        report_case("and its ids count from zero again, every one of them the new game's", persistent_id_of(second_character) == 0
            && name_of(second_character) == "Bran"
            && persistent_id_max + 1 == static_cast<stratum_sim::PersistentId::type>(sequence_world.entities().count<stratum_sim::PersistentId>()));

        sequence.quit();
        sequence.menu_open();
        report_case("opening the menu with no game in progress pauses nothing", !sequence_session.paused());

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        sequence.menu_open();
        bool paused_on_open = sequence_session.paused();
        sequence.menu_close();
        report_case("opening the menu pauses play, and closing it resumes play", paused_on_open && !sequence_session.paused());

        sequence.menu_open();
        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        report_case("a new game chosen from the menu is not paused", sequence_session.loaded() && !sequence_session.paused());

        // The game's end
        stratum_sim::EventQueue end_events;
        sequence_session.advance(end_events);
        report_case("a game with its character alive is not over", !sequence_session.over());

        flecs::entity dying = sequence_session.character().entity();
        dying.add<stratum_sim::Dead>();
        long long elapsed_before = sequence_session.scheduler().elapsed();
        end_events.clear();
        id_type after_death = sequence_session.advance(end_events);
        const stratum_sim::Died* died = !end_events.events().empty() ? end_events.events().back().try_get<stratum_sim::Died>() : nullptr;

        report_case("once the character is dead the game is over", sequence_session.over());
        report_case("and advancing runs no actor, reports the death, and returns no actor", after_death == nullid
            && sequence_session.scheduler().elapsed() == elapsed_before && end_events.events().size() == 1 && died != nullptr
            && died->entity_id == static_cast<id_type>(dying.id()));

        {
            stratum_sim::CountingIdAllocator clock_ids(stratum_sim::PersistentId {});
            SuiteWorld clock_world(clock_ids);
            stratum_sim::Scheduler scheduler;
            stratum_sim::Clock clock(clock_world, scheduler);
            flecs::entity controlled = clock_world.entities().entity().add<stratum_sim::Dead>();
            flecs::entity other = clock_world.entities().entity();
            scheduler.add(other.id(), 100);
            scheduler.add(controlled.id(), 100);
            stratum_sim::EventQueue clock_events;

            report_case("a turn-based clock stops at once when the controlled actor is dead",
                clock.advance(clock_events, static_cast<id_type>(controlled.id())) == nullid && scheduler.elapsed() == 0);
        }

        sequence.quit();
        report_case("a game quit is not over", !sequence_session.over());

        sequence.create(region_spec, stratum_sim::CharacterSpec {.name = "Cai"});
        report_case("and a game created after it is not over", !sequence_session.over());

        sequence.quit();
        report_case("quitting leaves no game in progress", !sequence_session.loaded() && !sequence_session.character().entity()
            && sequence_world.entities().count<stratum_sim::Region>() == 0);

        stratum_sim::RegionSpec unregistered_entry = region_spec;
        unregistered_entry.insert(string(stratum_sim::entry_access_key), "trapdoor,1");
        bool unregistered_thrown = false;

        try
        {
            sequence.create(unregistered_entry, stratum_sim::CharacterSpec {.name = "Dai"});
        }
        catch (const runtime_error&)
        {
            unregistered_thrown = true;
        }

        report_case("an entry access point type that is not registered is an error, which leaves no game and no directory",
            unregistered_thrown && !sequence_session.loaded() && sequence_world.entities().count<stratum_sim::Region>() == 0
            && !filesystem::exists(g_suite_config.working_directory / "Dai"));

        // Saving zones, with a maximum of one zone loaded so that leaving a zone and trimming unloads it
        const stratum_sim::Config saving_config {"out/saving/saves", "out/saving/working", 1};
        filesystem::remove_all("out/saving");
        stratum_sim::Game saving(saving_config);
        saving.generation() = suite_generation();
        stratum_sim::CharacterSpec ailis;
        ailis.name = "Ailis";
        saving.create(region_spec, ailis);

        stratum_sim::World& saving_world = saving.world();
        flecs::entity saving_first = saving.session().zone();
        stratum_sim::PersistentId saving_first_id = saving_first.get<stratum_sim::PersistentId>();
        filesystem::path zones_directory = saving_config.working_directory / "Ailis" / "zones";
        auto zone_file = [&](stratum_sim::PersistentId zone_id) { return zones_directory / format("{}.zone", zone_id.id); };

        auto pursuers_in = [&](flecs::entity zone)
        {
            size_t count = 0;
            saving_world.entities().each([&](flecs::entity e, const stratum_sim::Pursue&) { count += stratum_sim::zone_of(e) == zone ? 1 : 0; });
            return count;
        };

        flecs::entity saving_character = saving.character().entity();
        size_t first_pursuer_count = pursuers_in(saving_first);
        flecs::entity first_down = stratum_sim::entry_point(saving_first, staircase_type, -1);
        stratum_sim::PersistentId first_down_id = first_down.get<stratum_sim::PersistentId>();
        flecs::entity second_up = saving_world.relocate_through(saving_character, first_down);
        stratum_sim::PersistentId second_up_id = second_up.get<stratum_sim::PersistentId>();
        flecs::entity saving_second = stratum_sim::zone_of(second_up);
        report_case("leaving a zone does not save it", saving_second && !filesystem::exists(zone_file(saving_first_id)));

        saving_world.trim_zones(saving_second);
        bool saved_back = false;

        if (filesystem::exists(zone_file(saving_first_id)))
        {
            stratum_format::Document saved_document = stratum_format::read_zone(zone_file(saving_first_id));
            saved_back = saved_document.zone_id == saving_first_id.id && saved_document.level == -1;
        }

        report_case("a zone the world unloads is saved first, as a saved zone in the game directory's zones",
            saved_back && !saving_first.is_alive() && saving_world.zone_count() == 1);

        // Back up through the staircase bound to the first zone's, which has unloaded
        stratum_sim::PersistentId saving_second_id = saving_second.get<stratum_sim::PersistentId>();
        flecs::entity back_up = saving_world.relocate_through(saving_character, second_up);
        flecs::entity first_again = back_up ? stratum_sim::zone_of(back_up) : flecs::entity();
        flecs::entity saving_region = first_again ? stratum_sim::region_of(first_again) : flecs::entity();

        report_case("going through a relocator bound to one in a zone that has unloaded reads the zone back and arrives there",
            first_again && first_again.get<stratum_sim::PersistentId>() == saving_first_id && back_up.get<stratum_sim::PersistentId>() == first_down_id
            && stratum_sim::zone_of(saving_character) == first_again);
        report_case("a zone read back keeps its denizens and is not populated again, and its region lists it once", first_again
            && pursuers_in(first_again) == first_pursuer_count && saving_region.get<stratum_sim::Region>().zone_ids.size() == 2);

        saving_world.trim_zones(first_again);
        flecs::entity down_again = saving_world.relocate_through(saving_character, back_up);
        report_case("and the zone left, unloaded in its turn, is read back going the other way", down_again
            && down_again.get<stratum_sim::PersistentId>() == second_up_id && stratum_sim::zone_of(down_again).get<stratum_sim::PersistentId>() == saving_second_id);

        // Saving the game, with the first zone unloaded and the second, which the character is in, loaded. The suite moved the
        // character through the world, so the session is started where the character is
        saving.session().start(down_again);
        filesystem::path game_directory = saving_config.working_directory / "Ailis";
        string save_name = saving.save(stratum_sim::SaveType::Explicit);
        filesystem::path save_directory = saving_config.saves_directory / save_name;

        report_case("a save is a directory named by the character's name and the time of the save", save_name.starts_with("Ailis_")
            && save_name.size() == string("Ailis_20261002T120000").size() && filesystem::is_directory(save_directory));

        auto read_json = [](const filesystem::path& filepath)
        {
            ifstream is(filepath, ios::binary);
            return stratum_format::raw_values_type::parse(is, nullptr, false);
        };

        bool identical = true;
        for (const filesystem::directory_entry& entry : filesystem::recursive_directory_iterator(game_directory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            filesystem::path saved_path = save_directory / filesystem::relative(entry.path(), game_directory);
            ifstream in_game(entry.path(), ios::binary);
            ifstream in_save(saved_path, ios::binary);
            identical = identical && in_save && string(istreambuf_iterator<char>(in_game), {}) == string(istreambuf_iterator<char>(in_save), {});
        }

        report_case("the save is a copy of the game directory, file for file", identical);

        stratum_format::raw_values_type game_json = read_json(save_directory / "game.json");
        stratum_sim::PersistentId largest_id = max({saving_character.get<stratum_sim::PersistentId>(), saving_first_id, saving_second_id,
            saving_region.get<stratum_sim::PersistentId>()});
        report_case("game.json has the next id, past every id in the game, the seed, and the gameplay random engine's state",
            !game_json.is_discarded() && game_json["format"] == "quetzal.stratum_game" && game_json["game"]["next_id"].get<uint64_t>() > largest_id.id
            && !game_json["game"]["seed"].get<string>().empty() && !game_json["game"]["random_engines"]["gameplay"].get<string>().empty());

        stratum_format::raw_values_type character_json = read_json(save_directory / "character.json");
        const stratum_sim::placement_type& character_placement = stratum_sim::placement_of(saving_character);
        stratum_sim::discrete_point_type character_cell = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(saving_world.layout(
            stratum_sim::zone_of(saving_character))).cell_position_at(character_placement.position());
        report_case("character.json has the character's id, components, zone and cell, and an empty inventory", !character_json.is_discarded()
            && character_json["character"]["id"] == saving_character.get<stratum_sim::PersistentId>().id
            && character_json["character"]["components"].contains("speed") && character_json["character"]["zone"] == saving_second_id.id
            && character_json["character"]["placement"]["cell_position"] == stratum_format::raw_values_type {character_cell.i(), character_cell.j()}
            && character_json["character"]["inventory"].empty());

        stratum_format::raw_values_type world_json = read_json(save_directory / "world.json");
        bool zones_listed = !world_json.is_discarded() && world_json["world"]["regions"].size() == 1;
        if (zones_listed)
        {
            const stratum_format::raw_values_type& region_json = world_json["world"]["regions"][0];
            zones_listed = region_json["id"] == saving_region.get<stratum_sim::PersistentId>().id && region_json["zones"].size() == 2
                && region_json["zones"][0]["id"] == saving_first_id.id && region_json["zones"][1]["id"] == saving_second_id.id
                && region_json["spec"].contains(string(stratum_sim::generator_selector_key));

            for (const auto& zone_json : region_json["zones"])
            {
                zones_listed = zones_listed && filesystem::exists(save_directory / zone_json["file"].get<string>());
            }
        }

        report_case("world.json lists the region and its zones, each with its file in the save, loaded or not", zones_listed);

        auto indexed_in = [&](stratum_sim::PersistentId entity_id, stratum_sim::PersistentId zone_id)
        {
            return ranges::any_of(world_json["world"]["entity_zones"], [&](const stratum_format::raw_values_type& entry)
            {
                return entry[0] == entity_id.id && entry[1] == zone_id.id;
            });
        };

        report_case("and the world index, with each bound staircase in its zone", !world_json.is_discarded()
            && indexed_in(first_down_id, saving_first_id) && indexed_in(second_up_id, saving_second_id));

        string second_save_name = saving.save(stratum_sim::SaveType::Explicit);
        report_case("saving again makes another save and keeps the first, waiting for the next second if needed", second_save_name != save_name
            && filesystem::is_directory(save_directory) && filesystem::is_directory(saving_config.saves_directory / second_save_name));

        string auto_save_name = saving.save(stratum_sim::SaveType::Auto);
        string quick_save_name = saving.save(stratum_sim::SaveType::Quick);
        report_case("an auto save and a quick save have their type appended to their names", auto_save_name.starts_with("Ailis_")
            && auto_save_name.ends_with("_autosave") && quick_save_name.starts_with("Ailis_") && quick_save_name.ends_with("_quicksave")
            && filesystem::is_directory(saving_config.saves_directory / auto_save_name)
            && filesystem::is_directory(saving_config.saves_directory / quick_save_name));

        saving_character.set<stratum_sim::DisplayName>(stratum_sim::DisplayName {"Ailis: of the \"Downs\"?"});
        string replaced_save_name = saving.save(stratum_sim::SaveType::Explicit);
        report_case("a character in the name that cannot be in a directory name is replaced in the save's", replaced_save_name.starts_with("Ailis_ of the _Downs__")
            && filesystem::is_directory(saving_config.saves_directory / replaced_save_name));

        // Opening the first save, made with the first zone unloaded and the character in the second
        vector<stratum_sim::SavedGameDetails> saves = saving.saved_games();
        report_case("the saved games are every save, newest first, each with its character's name as entered", saves.size() == 5
            && saves.front().save_name() == replaced_save_name && saves.front().character_name() == "Ailis: of the \"Downs\"?"
            && ranges::any_of(saves, [&](const auto& details) { return details.save_name() == save_name && details.character_name() == "Ailis"; })
            && ranges::is_sorted(saves, ranges::greater(), &stratum_sim::SavedGameDetails::time));

        auto save_type_of = [&](const string& name)
        {
            auto details = ranges::find(saves, name, &stratum_sim::SavedGameDetails::save_name);
            return details != saves.end() ? optional<stratum_sim::SaveType>(details->save_type()) : nullopt;
        };

        report_case("each has its save type", save_type_of(save_name) == stratum_sim::SaveType::Explicit
            && save_type_of(auto_save_name) == stratum_sim::SaveType::Auto && save_type_of(quick_save_name) == stratum_sim::SaveType::Quick);

        stratum_sim::PersistentId saving_character_id = saving_character.get<stratum_sim::PersistentId>();
        stratum_sim::PersistentId region_id = saving_region.get<stratum_sim::PersistentId>();
        string saved_random_engine_state = game_json["game"]["random_engines"]["gameplay"].get<string>();
        stratum_sim::vector_type character_direction = character_placement.direction();

        saving.open(save_name);
        stratum_sim::Session& opened = saving.session();
        flecs::entity opened_character = saving.character().entity();
        flecs::entity opened_zone = opened.zone();

        report_case("opening a save copies it into the working directory as the game directory, named by the character",
            filesystem::is_directory(game_directory) && filesystem::exists(game_directory / "world.json"));
        report_case("and plays in the zone the character was saved in, at its cell, facing as it faced", opened.loaded()
            && opened_zone.get<stratum_sim::PersistentId>() == saving_second_id && zone_layout(saving_world, opened_zone)
                .cell_position_at(stratum_sim::placement_of(opened_character).position()) == character_cell
            && stratum_sim::placement_of(opened_character).direction() == character_direction);
        report_case("with the character's id, name and components, controlled by the player",
            opened_character.get<stratum_sim::PersistentId>() == saving_character_id && opened_character.get<stratum_sim::DisplayName>().name == "Ailis"
            && opened_character.has<stratum_sim::Speed>() && opened_character.has<stratum_sim::Blocking>()
            && opened_character.has<stratum_sim::ControlledActor>());

        flecs::entity opened_region = saving_world.entity(region_id);
        report_case("and the region, with its spec and its zone ids, the first zone not loaded", opened_region
            && opened_region.get<stratum_sim::Region>().zone_ids == vector {saving_first_id, saving_second_id}
            && opened_region.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && !saving_world.entity(saving_first_id) && saving_world.zone_count() == 1);
        report_case("and ids continue past every id in the game", saving_world.next_id().id > largest_id.id);

        flecs::entity opened_second_up = saving_world.entity(second_up_id);
        flecs::entity up_again = opened_second_up ? saving_world.relocate_through(opened_character, opened_second_up) : flecs::entity();
        report_case("going up through the staircase bound to one in the zone not loaded reads that zone back, through the world index",
            up_again && up_again.get<stratum_sim::PersistentId>() == first_down_id
            && stratum_sim::zone_of(up_again).get<stratum_sim::PersistentId>() == saving_first_id);

        string reopened_save_name = saving.save(stratum_sim::SaveType::Explicit);
        stratum_format::raw_values_type reopened_game_json = read_json(saving_config.saves_directory / reopened_save_name / "game.json");
        report_case("the random engine's state is restored, so a save made at once has the same state",
            reopened_game_json["game"]["random_engines"]["gameplay"] == saved_random_engine_state);

        settings::store().set_value<bool>(stratum_sim::Game::reseed_on_open_key, true);
        saving.open(save_name);
        string reseeded_save_name = saving.save(stratum_sim::SaveType::Explicit);
        stratum_format::raw_values_type reseeded_game_json = read_json(saving_config.saves_directory / reseeded_save_name / "game.json");
        report_case("or reseeded, as the settings choose", reseeded_game_json["game"]["random_engines"]["gameplay"] != saved_random_engine_state
            && reseeded_game_json["game"]["seed"] != game_json["game"]["seed"]);
        settings::store().set_value<bool>(stratum_sim::Game::reseed_on_open_key, false);

        saving.open(replaced_save_name);
        report_case("a save whose character's name cannot name a directory opens into one with the characters replaced", opened.loaded()
            && filesystem::is_directory(saving_config.working_directory / "Ailis_ of the _Downs__")
            && !filesystem::exists(game_directory));

        bool absent_thrown = false;

        try
        {
            saving.open("Nobody_20000101T000000");
        }
        catch (const runtime_error&)
        {
            absent_thrown = true;
        }

        report_case("opening a save that is not there is an error, which leaves no game in progress", absent_thrown && !opened.loaded()
            && !saving.character().entity() && saving_world.entities().count<stratum_sim::Region>() == 0);

        // Removing saves
        size_t save_count = saving.saved_games().size();
        saving.remove(second_save_name);
        report_case("removing a save deletes its directory, and it is no longer listed", !filesystem::exists(saving_config.saves_directory
            / second_save_name) && saving.saved_games().size() == save_count - 1);

        filesystem::create_directories(saving_config.saves_directory / "stray");
        auto remove_throws = [&](string_view name)
        {
            try
            {
                saving.remove(name);
            }
            catch (const runtime_error&)
            {
                return true;
            }

            return false;
        };

        report_case("a name that is not a save in the saves directory is not removed: a parent, a path, or none at all",
            remove_throws("..") && remove_throws(format("../{}", save_name)) && remove_throws("Nobody_20000101T000000")
            && remove_throws("") && filesystem::is_directory(saving_config.saves_directory / save_name));
        report_case("nor a directory there with no character file", remove_throws("stray") && filesystem::is_directory(saving_config.saves_directory
            / "stray"));
        filesystem::remove_all(saving_config.saves_directory / "stray");

        saving.open(save_name);
        filesystem::remove_all(zones_directory);
        saving.quit();
        report_case("and a game cleared saves nothing", !filesystem::exists(zones_directory));

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_saved_zone_documents()
    {
        report_group("saved zone documents");

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
            SuiteWorld other(other_ids);
            flecs::entity copy = other.create_zone(other.region, stratum_sim::PersistentId {*read_back.zone_id});
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
            SuiteWorld world(id_allocator);
            RoundTrip trip = round_trip(world.load_zone(world.region, filepath), [](SuiteWorld&, flecs::entity) {});

            report_case(format("{} is written as a saved zone, read back with its geometry, and written again identically", filepath),
                trip.same_geometry && !trip.written.empty() && trip.written == trip.rewritten);
        }

        // data/door.zone: a door on the doorway east of cell (1, 1), and a pursuer that populating places
        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = world.load_zone(world.region, "data/door.zone");

        flecs::entity door = stratum_sim::access_entity(stratum_sim::way_through(zone,
            zone_layout(world, zone).wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East))));
        door.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Open});
        stratum_sim::PersistentId door_id = door.get<stratum_sim::PersistentId>();

        flecs::entity pursuer;
        world.entities().each([&](flecs::entity e, const stratum_sim::Pursue&) { pursuer = e; });
        stratum_sim::PersistentId pursuer_id = pursuer.get<stratum_sim::PersistentId>();
        stratum_sim::discrete_point_type pursuer_cell = zone_layout(world, zone).cell_position_at(stratum_sim::placement_of(pursuer).position());
        pursuer.set<stratum_sim::Pursue>(stratum_sim::Pursue {world.make_reference(door), zone_layout(world, zone).position(stratum_sim::discrete_point_type {1, 1}), true});

        bool door_open = false;
        bool pursuer_kept = false;
        bool character_left_out = true;

        round_trip(zone, [&](SuiteWorld& other, flecs::entity copy)
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

        report_case("a door's state is written and read back", door_open);
        report_case("and a denizen's tags, speed and behavior, its cell, and a reference in its behavior", pursuer_kept);

        // The character, which the game owns, is in the zone but in none of its subtree
        stratum_sim::Game game(g_suite_config);
        enter_zone_file(game, "data/corridor.zone");
        stratum_sim::PersistentId character_id = game.character().entity().get<stratum_sim::PersistentId>();
        stratum_format::Document with_character = stratum_sim::make_document(game.session().zone());
        character_left_out = std::ranges::none_of(with_character.entities, [&](const stratum_format::Entity& e) { return e.id == character_id.id; });
        report_case("the character is not written with the zone it is in", character_left_out);

        return;
    }

    //--------------------------------------------------------------------------
    void test_specs()
    {
        report_group("specs");

        const stratum_sim::RegionSpec spec {{"generator_selector", "common"}, {"name", "The Downs"}, {"zone.level", "-1"},
            {"zone.parameters.denizens", "few"}, {"zones", "3"}};

        report_case("no unknown keys when every key is known", stratum_sim::find_unknown_keys(spec,
            vector<string_view> {"generator_selector", "name", "zone.level", "zone.parameters.denizens", "zones", "difficulty"}).empty());
        report_case("the unknown keys, in key order", stratum_sim::find_unknown_keys(spec, vector<string_view> {"name"})
            == vector<string> {"generator_selector", "zone.level", "zone.parameters.denizens", "zones"});

        const stratum_sim::ZoneSpec zone_spec = stratum_sim::select_prefixed(spec, "zone.");
        report_case("the entries under a prefix, with the prefix removed, and not the key zones under zone.",
            zone_spec.value("level") == "-1" && zone_spec.value("parameters.denizens") == "few" && !zone_spec.contains("s")
            && distance(zone_spec.begin(), zone_spec.end()) == 2);
        const KeyValueStore none =stratum_sim::select_prefixed(spec, "region.");
        report_case("nothing under a prefix that no key has", none.begin() == none.end());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_regions()
    {
        report_group("regions");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

        flecs::entity surface = world.create_region(suite_region_spec("staircase"));
        flecs::entity crypt = world.create_region(suite_region_spec("staircase"));

        report_case("a region has a persistent id", surface.has<stratum_sim::PersistentId>()
            && world.entity(surface.get<stratum_sim::PersistentId>()) == surface);
        report_case("a region has no parent", !surface.parent() && !crypt.parent());

        flecs::entity field = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity chapel = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel, stratum_format::read_zone("data/corridor.zone"));

        report_case("a zone is its region's child", stratum_sim::region_of(chapel) == crypt && chapel.parent() == crypt
            && stratum_sim::region_of(field) == world.region);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        flecs::entity monk = world.create_entity(chapel,
            stratum_sim::placement_type(zone_layout(world, chapel).position(stratum_sim::discrete_point_type {1, 1}), east));
        stratum_sim::EntityReference monk_reference = world.make_reference(monk);

        report_case("the zone of an entity in it is unchanged by the region above it", stratum_sim::zone_of(monk) == chapel
            && !stratum_sim::zone_of(chapel));

        flecs::entity chapel_below = world.create_zone(crypt, world.next_id());
        stratum_sim::load_layout(chapel_below, stratum_format::read_zone("data/room.zone"));
        stratum_sim::PersistentId chapel_id = chapel.get<stratum_sim::PersistentId>();
        report_case_equal("a world counts the zones of every region", world.zone_count(), 3);

        world.unload_region(crypt);

        report_case("unloading a region deletes it and its zones", !crypt.is_alive() && !chapel.is_alive() && !chapel_below.is_alive()
            && !monk.is_alive());
        report_case("and leaves the other regions and their zones", surface.is_alive() && field.is_alive() && world.zone_count() == 1);
        report_case("the world index keeps the zone of an entity something refers to, as an unloaded zone's",
            world.zone_of(monk_reference.persistent_id) == chapel_id && !world.entity(monk_reference.persistent_id));

        // A zone's level, from its file or from the region that makes it
        stratum_format::Document leveled_document = stratum_format::read_zone("data/door.zone");
        leveled_document.level = -3;
        flecs::entity leveled = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(leveled, leveled_document);

        report_case("a zone has the level its file states", leveled.try_get<stratum_sim::ZoneLevel>() != nullptr
            && leveled.get<stratum_sim::ZoneLevel>().level == -3);
        report_case("and none where its file states none", field.try_get<stratum_sim::ZoneLevel>() == nullptr);

        stratum_format::Document named_document = stratum_format::read_zone("data/door.zone");
        named_document.components["name"] = "The Narrow Door";
        bool named_buildable = !stratum_sim::check_buildable(world.entities(), named_document).has_value();
        flecs::entity named = world.create_zone(world.region, world.next_id());
        stratum_sim::load_layout(named, named_document);

        report_case("a zone has the name its file states, as its display name", named_buildable
            && named.try_get<stratum_sim::DisplayName>() != nullptr && named.get<stratum_sim::DisplayName>().name == "The Narrow Door");

        flecs::entity dungeon = world.create_region(suite_region_spec("door"));
        flecs::entity made = world.create_zone(dungeon, suite_zone_spec(-1));

        report_case("a region makes a zone at a level with the generator its selector chooses", stratum_sim::region_of(made) == dungeon
            && made.get<stratum_sim::ZoneLevel>().level == -1 && made.has<stratum_sim::ZoneLayout>());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_level_targets()
    {
        report_group("level targets");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

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
            document.level = level;
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        auto zone_arrived_in = [](flecs::entity arrival) { return arrival ? stratum_sim::zone_of(arrival) : flecs::entity(); };

        flecs::entity dungeon = world.create_region(suite_region_spec("staircase"));

        flecs::entity field = load_at_level(world.region, 0);
        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);
        flecs::entity first_up = up_of(first);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);

        // Resolving does not bind, so the suite binds an arrival to keep it from being found again
        auto bind = [&](flecs::entity arrival)
        {
            if (arrival)
            {
                arrival.set<stratum_sim::Relocator>(stratum_sim::Relocator {world.make_reference(first_up)});
            }
        };

        report_case("moving down finds the unbound staircase that leads back, in its region's zone at the level below",
            world.resolve_relocator_target(first_down) == second_up);

        retarget(first_down, "=-2");
        report_case("an absolute target finds a zone at its level", world.resolve_relocator_target(first_down) == second_up);
        retarget(first_down, "-1");
        retarget(second_up, "=-1");
        report_case("a staircase that leads back with an absolute target is found", world.resolve_relocator_target(first_down) == second_up);
        retarget(second_up, "+1");

        retarget(first_up, "staircase/+1");
        flecs::entity arrival = world.resolve_relocator_target(first_up);
        flecs::entity made = zone_arrived_in(arrival);
        flecs::entity region_made = made ? stratum_sim::region_of(made) : flecs::entity();
        report_case("a target with a region makes a zone in a new region created from the region spec of that name, though a zone at the level has a staircase that leads back", made
            && made != field && region_made != dungeon && region_made != world.region
            && region_made.get<stratum_sim::Region>().region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && region_made.get<stratum_sim::Region>().zone_ids.size() == 1 && made.get<stratum_sim::ZoneLevel>().level == 0
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

        report_case("a region spec that is not registered is an error", region_spec_unregistered);
        retarget(first_up, "+1");

        arrival = world.resolve_relocator_target(down_of(second));
        made = zone_arrived_in(arrival);
        report_case("finding none, it makes a zone at the level in its own region and arrives at its entry point", made
            && stratum_sim::region_of(made) == dungeon && made.get<stratum_sim::ZoneLevel>().level == -3 && arrival == up_of(made));

        arrival = world.resolve_relocator_target(first_up);
        made = zone_arrived_in(arrival);
        report_case("moving up looks only in its own region, so a zone at level 0 in another is not found and one is made", made
            && made != field && stratum_sim::region_of(made) == dungeon && made.get<stratum_sim::ZoneLevel>().level == 0
            && arrival == down_of(made));
        bind(arrival);

        flecs::entity elsewhere = world.create_region(suite_region_spec("staircase"));
        load_at_level(elsewhere, -2);
        world.unload_zone(second);
        made = zone_arrived_in(world.resolve_relocator_target(first_down));
        report_case("an unloaded zone and a zone in an unrelated region are not searched", made
            && stratum_sim::region_of(made) == dungeon && made.get<stratum_sim::ZoneLevel>().level == -2);

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
        report_case("a zone made in a new region with no access point that leads back is an error, and the region is discarded",
            throws(first_up) && world.entities().count<stratum_sim::Region>() == region_count && world.zone_count() == zone_count);
        retarget(first_up, "+1");

        flecs::entity doors = world.create_region(suite_region_spec("door"));
        flecs::entity below_doors = load_at_level(doors, -1);
        zone_count = world.zone_count();
        report_case("a zone made in its own region with no access point that leads back is an error, and only that zone is discarded",
            throws(up_of(below_doors)) && doors.is_alive() && below_doors.is_alive() && world.zone_count() == zone_count
            && doors.get<stratum_sim::Region>().zone_ids.empty());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_relocating_through()
    {
        report_group("relocating through a relocator");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

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
            document.level = level;
            flecs::entity zone = world.create_zone(region, world.next_id());
            stratum_sim::load_layout(zone, document);
            return zone;
        };

        flecs::entity dungeon = world.create_region(suite_region_spec("staircase"));

        flecs::entity first = load_at_level(dungeon, -1);
        flecs::entity second = load_at_level(dungeon, -2);
        flecs::entity first_down = down_of(first);
        flecs::entity second_up = up_of(second);
        stratum_sim::placement_type second_up_placement = second_up.get<stratum_sim::AccessPoint>().placement;

        flecs::entity actor = world.create_entity(first, first_down.get<stratum_sim::AccessPoint>().placement);

        flecs::entity arrival = world.relocate_through(actor, first_down);
        report_case("an actor goes through an unbound relocator to the access point that leads back", arrival == second_up
            && stratum_sim::zone_of(actor) == second);
        report_case("and is placed at its placement", stratum_sim::placement_of(actor).position() == second_up_placement.position()
            && stratum_sim::placement_of(actor).direction() == second_up_placement.direction());
        report_case("both ends are bound to each other", refers_to(first_down, second_up) && refers_to(second_up, first_down));
        report_case("and a bound access point is no longer an entry point", !up_of(second));

        size_t zone_count = world.zone_count();
        arrival = world.relocate_through(actor, second_up);
        report_case("going back through the bound access point arrives at the relocator", arrival == first_down
            && stratum_sim::zone_of(actor) == first && world.zone_count() == zone_count);

        arrival = world.relocate_through(actor, first_down);
        flecs::entity second_down = down_of(second);
        arrival = world.relocate_through(actor, second_down);
        flecs::entity made = arrival ? stratum_sim::zone_of(arrival) : flecs::entity();
        report_case("finding none, it goes to a zone made at the target level and binds its entry point", made
            && made.get<stratum_sim::ZoneLevel>().level == -3 && stratum_sim::zone_of(actor) == made && refers_to(second_down, arrival)
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
        report_case("a named zone is not resolved, which is an error, and nothing moves or binds", throws(named)
            && stratum_sim::zone_of(actor) == made && get<stratum_format::RelocatorTarget>(target_of(named)) == named_target);

        world.unload_zone(first);
        report_case("a bound target whose zone has unloaded, in a world with no game directory to read it back from, is an error, and nothing moves",
            throws(second_up) && stratum_sim::zone_of(actor) == made);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_entering_access_points()
    {
        report_group("entering an access point");

        stratum_sim::Game game(g_suite_config);
        stratum_sim::Session& session = game.session();
        stratum_sim::World& world = game.world();
        game.generation() = suite_generation();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        flecs::entity character = session.character().entity();
        flecs::entity first = session.zone();
        first.set<stratum_sim::ZoneLevel>(stratum_sim::ZoneLevel {-1});

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
        report_case("moving into a wall with a relocator on it is blocked", !relocated(moved_events) && session.zone() == first
            && ranges::any_of(moved_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Blocked>(); }));

        // A closed relocator is not entered
        first_down.set<stratum_sim::Access>(stratum_sim::Access {stratum_sim::AccessState::Closed});
        stratum_sim::EventQueue closed_events;
        interact(closed_events);
        report_case("interacting with a closed relocator opens it rather than entering it", !relocated(closed_events)
            && session.zone() == first && stratum_sim::access(first_down) == stratum_sim::AccessState::Open);
        first_down.remove<stratum_sim::Access>();

        stratum_sim::EventQueue down_events;
        interact(down_events);
        flecs::entity second = session.zone();
        report_case("interacting with a relocator on the wall ahead enters the relocator", relocated(down_events) && second != first
            && stratum_sim::zone_of(character) == second && second.get<stratum_sim::ZoneLevel>().level == -2);
        report_case("and the session plays the zone arrived in, with the character in its turn order", session.scheduler().contains(character.id()));

        stratum_sim::EventQueue up_events;
        step(west, up_events);
        interact(up_events);
        report_case("going back up arrives at the staircase gone down", relocated(up_events) && session.zone() == first
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
        report_case("entering the relocator on the floor of the character's cell relocates the character", relocated(enter_events)
            && session.zone() == second && zone_layout(world, second).cell_position_at(stratum_sim::placement_of(character).position())
                == zone_layout(world, second).cell_position_at(second_down.get<stratum_sim::AccessPoint>().placement.position()));

        // A denizen facing the staircase leading down, in the zone the character left
        flecs::entity denizen = world.create_entity(first, first_down.get<stratum_sim::AccessPoint>().placement);
        stratum_sim::EventQueue denizen_events;
        stratum_sim::apply(world, stratum_sim::Command(static_cast<id_type>(denizen.id()), stratum_sim::Interact {}), denizen_events);
        report_case("an actor other than the controlled actor does not enter a relocator", !relocated(denizen_events)
            && stratum_sim::zone_of(denizen) == first);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_interacting_with_relocators()
    {
        report_group("interacting with a relocator");

        stratum_sim::Game game(g_suite_config);
        stratum_sim::Session& session = game.session();

        // data/walls.zone: the character enters on cell (2, 1), facing the staircase leading down to the east
        enter_zone_file(game, "data/walls.zone");
        session.zone().set<stratum_sim::ZoneLevel>(stratum_sim::ZoneLevel {-1});
        flecs::entity staircase = stratum_sim::entry_point(session.zone(), stratum_sim::access_type(game.world().entities(), "staircase"), -1);

        game.generation() = suite_generation();
        flecs::entity first = session.zone();
        session.character().submit(stratum_sim::Interact {});
        stratum_sim::EventQueue enter_events;
        session.advance(enter_events);
        report_case("interacting with a relocator goes through it", stratum_sim::access_type_name(staircase) == "staircase"
            && session.zone() != first
            && ranges::any_of(enter_events.events(), [](const stratum_sim::Event& event) { return event.is<stratum_sim::Relocated>(); }));

        // A named zone is not resolved. A game whose advance throws is quit, so this one is used for nothing else
        stratum_sim::Game failing(g_suite_config);
        stratum_sim::Session& failing_session = failing.session();
        enter_zone_file(failing, "data/walls.zone");
        failing_session.zone().set<stratum_sim::ZoneLevel>(stratum_sim::ZoneLevel {-1});
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

        report_case("interacting with a relocator whose target is not resolved is an error that passes out of advance, and nothing moves",
            threw && stratum_sim::zone_of(failing_session.character().entity()) == unmoved);

        return;
    }

    //--------------------------------------------------------------------------
    void test_world()
    {
        report_group("world");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

        report_case_equal("a new world has no zones", world.zone_count(), 0);

        flecs::entity crypt = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity moor = world.create_zone(world.region, make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            1.0f, stratum_sim::discrete_vector_type(4, 4)));

        report_case_equal("zones are created", world.zone_count(), 2);
        report_case("a zone is found by its persistent id", world.entity(crypt.get<stratum_sim::PersistentId>()) == crypt);
        report_case("each zone has its own layout", &world.layout(crypt) != &world.layout(moor));

        report_case("a zone holds the layout it was created with", zone_layout(world, crypt).grid().extent == stratum_sim::discrete_vector_type(6, 5));
        report_case("the other stays empty", zone_layout(world, moor).flat_store_count() == 0);

        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::placement_type where(zone_layout(world, crypt).position(stratum_sim::discrete_point_type {2, 2}), east);

        flecs::entity monster = world.create_entity(crypt, where);
        report_case("an entity is placed in its zone", stratum_sim::zone_of(monster) == crypt);
        report_case("and has its placement", monster.try_get<stratum_sim::Site>() != nullptr);

        // Migration keeps the id, which is what quests and leashes depend on
        flecs::entity_t id_before = monster.id();
        world.relocate(monster, moor, where);

        report_case("relocating moves it to the other zone", stratum_sim::zone_of(monster) == moor);
        report_case("its id is unchanged", monster.id() == id_before);
        report_case("it is no longer in the old zone", !monster.has(flecs::ChildOf, crypt));

        // Reparenting moves an entity's children with it
        flecs::entity coin = world.entities().entity();
        stratum_sim::reparent(coin, monster, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});

        report_case("taking something makes it a child of what holds it", coin.parent() == monster);
        report_case("through Parent, where a zone contains its entities through ChildOf", coin.has<flecs::Parent>() && !monster.has<flecs::Parent>());
        report_case("and it has a persistent id, being in a zone", coin.has<stratum_sim::PersistentId>());
        report_case("and sites it there", coin.get<stratum_sim::Site>().relation == stratum_sim::Relation::In
            && !coin.get<stratum_sim::Site>().address.has_value());

        world.relocate(monster, crypt, where);
        report_case("what it carries travels with it", stratum_sim::zone_of(coin) == crypt && coin.parent() == monster);

        stratum_sim::reparent(coin, crypt, stratum_sim::Site {stratum_sim::Relation::In, {}, where});
        report_case("putting it down is the same operation at the zone", coin.parent() == crypt
            && stratum_sim::placement_of(coin).position() == where.position());
        report_case("and it is in the zone through ChildOf again", !coin.has<flecs::Parent>());

        // An entity with no parent is the game's
        flecs::entity wanderer = world.entities().entity();
        world.relocate(wanderer, moor, where);

        report_case("relocating an entity with no parent keeps it the game's, in the zone without being its child",
            !wanderer.parent() && stratum_sim::zone_of(wanderer) == moor && wanderer.has<stratum_sim::PersistentId>());
        report_case("and among the zone's entities", world.entities_at(moor, where) == stratum_sim::World::entities_type {wanderer});

        world.relocate(wanderer, crypt, where);
        report_case("relocating it again moves it to the other zone, still the game's", !wanderer.parent()
            && stratum_sim::zone_of(wanderer) == crypt && world.entities_at(moor, where).empty());

        flecs::entity pouch = world.entities().entity();
        stratum_sim::reparent(pouch, wanderer, stratum_sim::Site {stratum_sim::Relation::On, {}, {}});
        stratum_sim::EntityReference pouch_reference = world.make_reference(pouch);

        world.relocate(wanderer, moor, where);
        report_case("what the game's entity carries moves with it, and its entry in the world index follows",
            stratum_sim::zone_of(pouch) == moor && world.zone_of(pouch_reference.persistent_id) == moor.get<stratum_sim::PersistentId>());

        world.relocate(wanderer, crypt, where);
        world.unload_zone(crypt);

        report_case("unloading a zone destroys its contents", !monster.is_alive());
        report_case_equal("and drops its layout", world.zone_count(), 1);
        report_case("the other zone survives", moor.is_alive());
        report_case("the game's entity in it, and what it carries, are left out of it and in no zone", wanderer.is_alive()
            && pouch.is_alive() && !stratum_sim::zone_of(wanderer) && !stratum_sim::zone_of(pouch));

        // Zones the world makes whole, with their doors and their denizens
        auto pursuers_in = [&](flecs::entity zone)
        {
            stratum_sim::World::entities_type pursuers;

            world.entities().query_builder<const stratum_sim::Pursue>()
                .with(flecs::ChildOf, zone)
                .build()
                .each([&](flecs::entity e, const stratum_sim::Pursue&) { pursuers.push_back(e); });

            return pursuers;
        };

        auto door_east_of_start = [&](flecs::entity zone)
        {
            return stratum_sim::way_through(zone, zone_layout(world, zone).wall_toward(stratum_sim::discrete_point_type {1, 1},
                cardinal_direction(CardinalDirection::East)));
        };

        flecs::entity generated = world.create_zone(world.create_region(suite_region_spec("door")), stratum_sim::ZoneSpec {});
        stratum_sim::World::entities_type generated_pursuers = pursuers_in(generated);

        report_case("a generated zone has its doors", door_east_of_start(generated).is_alive());
        report_case("and the notation its generator named",
            generated.get<stratum_sim::ZoneNotation>().notation == stratum_format::Notation::Block);
        report_case("and is populated, its pursuer scheduled by speed and pursuing whoever is controlled",
            generated_pursuers.size() == 1 && generated_pursuers.front().has<stratum_sim::Speed>()
            && !generated_pursuers.front().get<stratum_sim::Pursue>().target.has_value());

        flecs::entity loaded = world.load_zone(world.region, "data/door.zone");

        auto door_type_east_of_start = [&](flecs::entity zone)
        {
            return stratum_sim::access_entity(door_east_of_start(zone)).has(flecs::IsA, stratum_sim::entity_type(world.entities(), "door"));
        };

        report_case("its door is of the door type, as a loaded one is", door_type_east_of_start(generated));
        report_case("a loaded zone agrees with it", same_geometry(zone_layout(world, loaded), zone_layout(world, generated))
            && door_type_east_of_start(loaded) && pursuers_in(loaded).size() == 1
            && loaded.get<stratum_sim::ZoneNotation>().notation == stratum_format::Notation::Block);
        report_case_equal("creating and loading unload nothing", world.zone_count(), 3);

        report_case("a zone loaded unpopulated is written populated", !stratum_sim::make_document(loaded).is_unpopulated);

        stratum_format::Document prepopulated_document = stratum_format::read_zone("data/door.zone");
        prepopulated_document.is_unpopulated = false;
        filesystem::create_directories("out");
        {
            ofstream prepopulated_file("out/prepopulated.zone", ios::binary);
            stratum_format::write(prepopulated_file, prepopulated_document);
        }

        flecs::entity prepopulated = world.load_zone(world.create_region(suite_region_spec("door")), "out/prepopulated.zone");
        report_case("a zone whose file is not unpopulated is not populated", pursuers_in(prepopulated).empty()
            && door_east_of_start(prepopulated).is_alive());
        world.unload_region(stratum_sim::region_of(prepopulated));

        // Zones created through the registries
        flecs::entity counted = world.create_region(suite_region_spec("staircase"));
        flecs::entity counted_generated = world.create_zone(counted, suite_zone_spec(-1));
        flecs::entity counted_loaded = world.load_zone(counted, "data/door.zone");
        report_case("a region lists the zones created in it, generated or loaded, in order", counted.get<stratum_sim::Region>().zone_ids
            == vector<stratum_sim::PersistentId> {counted_generated.get<stratum_sim::PersistentId>(), counted_loaded.get<stratum_sim::PersistentId>()});

        // data/saved.zone is a saved zone, listed when it was created
        flecs::entity saved = world.load_zone(counted, "data/saved.zone");
        world.unload_zone(saved);
        flecs::entity saved_again = world.load_zone(counted, "data/saved.zone");
        bool saved_unpopulated = true;
        world.entities().each([&](flecs::entity e, const stratum_sim::Pursue&) { saved_unpopulated = saved_unpopulated && stratum_sim::zone_of(e) != saved_again; });
        report_case("a saved zone read back is not listed again, however often it is loaded after unloading", saved_again.is_alive()
            && saved_again.get<stratum_sim::PersistentId>() == stratum_sim::PersistentId {900} && counted.get<stratum_sim::Region>().zone_ids.size() == 2);
        report_case("and is not populated", saved_unpopulated);
        world.unload_zone(saved_again);

        flecs::entity leaving = world.create_region(suite_region_spec("next_region"));
        flecs::entity begun = world.create_zone(leaving, suite_zone_spec(-1));
        flecs::entity begun_region = stratum_sim::region_of(begun);
        const stratum_sim::Region& begun_region_data = begun_region.get<stratum_sim::Region>();
        report_case("a selection of a new region makes the zone the first of a region created from the region spec",
            begun_region != leaving && begun_region_data.region_spec.value(stratum_sim::generator_selector_key) == "staircase"
            && begun_region_data.zone_ids.size() == 1 && leaving.get<stratum_sim::Region>().zone_ids.empty()
            && begun.get<stratum_sim::ZoneLevel>().level == -1);

        auto throws_runtime_error = [](auto f)
        {
            try
            {
                f();
            }
            catch (const runtime_error&)
            {
                return true;
            }

            return false;
        };

        size_t zone_count_before = world.zone_count();
        report_case("a generator selector that is not registered is an error, and no zone is created", throws_runtime_error([&]
            { world.create_zone(world.create_region(suite_region_spec("unregistered")), stratum_sim::ZoneSpec {}); }) && world.zone_count() == zone_count_before);
        report_case("and so is a generator that is not registered", throws_runtime_error([&]
            { world.create_zone(world.create_region(suite_region_spec("unregistered_generator")), stratum_sim::ZoneSpec {}); })
            && world.zone_count() == zone_count_before);

        return;
    }

    //--------------------------------------------------------------------------
    void test_properties()
    {
        report_group("properties");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::world& entities = world.entities();
        register_suite_components(entities);

        // Room 0 is the west half, room 1 the east half, and cell [3, 2] in no room
        auto layout = make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f,
            stratum_sim::discrete_vector_type(4, 3));
        layout->set_room_count(2);

        for (stratum_sim::coord_type j = 0; j < 3; ++j)
        {
            for (stratum_sim::coord_type i = 0; i < 4; ++i)
            {
                stratum_sim::discrete_point_type cell_position {i, j};
                layout->set_cell_open(cell_position, true);

                if (cell_position != stratum_sim::discrete_point_type {3, 2})
                {
                    layout->set_cell_room(cell_position, i < 2 ? 0 : 1);
                }
            }
        }

        layout->set_cell_walls(stratum_sim::discrete_point_type {0, 0}, stratum_sim::direction_mask(CardinalDirection::North));
        id_type north_wall = layout->wall_toward(stratum_sim::discrete_point_type {0, 0}, cardinal_direction(CardinalDirection::North));

        flecs::entity zone = world.create_zone(world.region, std::move(layout));
        auto placed_at = [&](stratum_sim::discrete_point_type cell_position)
        {
            return world.create_entity(zone, stratum_sim::placement_type(zone_layout(world, zone).position(cell_position),
                stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        };

        flecs::entity chest = stratum_sim::create_entity_type(entities, "chest");
        report_case("a type is found by its name", stratum_sim::entity_type(entities, "chest") == chest && chest.has(flecs::Prefab));
        report_case("a type not loaded is null", !stratum_sim::entity_type(entities, "portcullis"));

        flecs::entity west = stratum_sim::ensure_room_entity(zone, 0);
        report_case("a room's entity is made once", stratum_sim::ensure_room_entity(zone, 0) == west && stratum_sim::room_entity(zone, 0) == west);
        report_case("a room with none has a null entity", !stratum_sim::room_entity(zone, 1));
        report_case("a room's entity has no persistent id", !west.has<stratum_sim::PersistentId>());

        flecs::entity plain = placed_at(stratum_sim::discrete_point_type {0, 1});
        report_case("nothing along the chain is empty", !world.property<SuiteFinish>(plain).has_value());

        chest.set<SuiteFinish>(SuiteFinish {7, {}, {0.5f, {}}});
        west.set<SuiteFinish>(SuiteFinish {1, 0.25f, {0.0f, {}}});
        zone.set<SuiteFinish>(SuiteFinish {2, 0.75f, {0.0f, 0.125f}});

        flecs::entity oak = placed_at(stratum_sim::discrete_point_type {1, 1}).is_a(chest);
        report_case("an entity of a type has no copy of a component that inherits", !oak.owns<SuiteFinish>());

        chest.get_mut<SuiteFinish>().grain = 8;
        chest.modified<SuiteFinish>();
        optional<SuiteFinish> finish = world.property<SuiteFinish>(oak);

        report_case("a field the type sets is the entity's, as the type is now", finish.has_value() && finish->grain == 8
            && finish->tint.red == 0.5f);
        report_case("a field the type leaves unset comes from the room", finish.has_value() && finish->gloss == 0.25f);
        report_case("and one the room leaves unset from the layout", finish.has_value() && finish->tint.blue == 0.125f);

        oak.set<SuiteFinish>(SuiteFinish {{}, 0.5f, {{}, 1.0f}});
        finish = world.property<SuiteFinish>(oak);

        report_case("the entity's own field wins", finish.has_value() && finish->gloss == 0.5f && finish->tint.blue == 1.0f);
        report_case("and a field it leaves unset comes from its type", finish.has_value() && finish->grain == 8 && finish->tint.red == 0.5f);

        finish = world.property<SuiteFinish>(placed_at(stratum_sim::discrete_point_type {3, 1}));
        report_case("a room with no entity is skipped", finish.has_value() && finish->grain == 2 && finish->gloss == 0.75f);

        flecs::entity sconce = world.entities().entity();
        stratum_sim::reparent(sconce, zone, stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::WallAddress {north_wall}, {}});
        finish = world.property<SuiteFinish>(sconce);
        report_case("an entity on a wall is in the room of the wall's cell", finish.has_value() && finish->grain == 1);

        flecs::entity key = world.entities().entity();
        stratum_sim::reparent(key, plain, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        finish = world.property<SuiteFinish>(key);
        report_case("an entity sited on its parent is in its parent's room", finish.has_value() && finish->grain == 1);

        finish = world.property<SuiteFinish>(west);
        report_case("a room's own chain is the room, then the layout", finish.has_value() && finish->grain == 1 && finish->tint.blue == 0.125f);

        world.unload_zone(zone);
        report_case("unloading a zone deletes its rooms' entities", !west.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    void test_layout_entities()
    {
        report_group("layout entities");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

        auto layout = make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f), 1.0f,
            stratum_sim::discrete_vector_type(3, 3));
        layout->set_flats(0.0f, 1.0f);
        layout->set_cell_open(stratum_sim::discrete_point_type {1, 1}, true);
        layout->set_cell_walls(stratum_sim::discrete_point_type {1, 1}, stratum_sim::direction_mask(CardinalDirection::North));
        id_type north_wall = layout->wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::North));
        id_type floor_id = layout->floor_id();

        flecs::entity zone = world.create_zone(world.region, std::move(layout));

        auto attached_to = [](flecs::entity layout_entity)
        {
            const auto* attached = layout_entity.try_get<stratum_sim::AttachedEntities>();
            return attached != nullptr ? attached->entities : vector<flecs::entity> {};
        };

        auto sited = [&](const stratum_sim::address_type& address)
        {
            flecs::entity e = world.entities().entity();
            stratum_sim::reparent(e, zone, stratum_sim::Site {stratum_sim::Relation::On, address, {}});
            return e;
        };

        report_case("a wall with nothing on it has no entity", !stratum_sim::wall_entity(zone, north_wall));

        flecs::entity torch = sited(stratum_sim::WallAddress {north_wall});
        flecs::entity wall = stratum_sim::wall_entity(zone, north_wall);
        report_case("siting an entity on a wall makes the wall's entity", wall && wall.parent() == zone);
        report_case("which lists it", attached_to(wall) == vector<flecs::entity> {torch});
        report_case("and has no persistent id", !wall.has<stratum_sim::PersistentId>());

        flecs::entity plaque = sited(stratum_sim::WallAddress {north_wall});
        report_case("a second entity on the wall is listed on the same entity", stratum_sim::wall_entity(zone, north_wall) == wall
            && attached_to(wall) == vector<flecs::entity> {torch, plaque});

        stratum_sim::set_placement(torch, stratum_sim::placement_type(stratum_sim::point_type(0.5f, 0.5f, 0.0f), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        report_case("moving it on the wall lists it once", attached_to(wall).size() == 2);

        plaque.destruct();
        report_case("deleting an entity takes it off the list", attached_to(wall) == vector<flecs::entity> {torch});

        stratum_sim::reparent(torch, zone, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        report_case("a wall's entity goes when nothing is sited on it", !wall.is_alive() && !stratum_sim::wall_entity(zone, north_wall));

        flecs::entity basin = sited(stratum_sim::FlatCellAddress {floor_id, stratum_sim::discrete_point_type {1, 1}});
        flecs::entity floor_cell = stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point_type {1, 1});
        report_case("a flat cell's entity lists what is sited on it", floor_cell && attached_to(floor_cell) == vector<flecs::entity> {basin});

        floor_cell.add<stratum_sim::Blocking>();
        basin.destruct();
        report_case("one with components of its own stays", floor_cell.is_alive()
            && stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point_type {1, 1}) == floor_cell);

        flecs::entity lever = sited(stratum_sim::FlatCellAddress {floor_id, stratum_sim::discrete_point_type {1, 1}});
        floor_cell.destruct();
        report_case("deleting a flat cell's entity leaves what was on it unsited", lever.is_alive() && !lever.has<stratum_sim::Site>());
        report_case("and the cell with none", !stratum_sim::flat_cell_entity(zone, floor_id, stratum_sim::discrete_point_type {1, 1}));

        world.entities().defer_begin();
        flecs::entity sconce = sited(stratum_sim::WallAddress {north_wall});
        flecs::entity hook = sited(stratum_sim::WallAddress {north_wall});
        world.entities().defer_end();
        report_case("entities sited while deferred are all listed", attached_to(stratum_sim::wall_entity(zone, north_wall)) == vector<flecs::entity> {sconce, hook});

        wall = stratum_sim::wall_entity(zone, north_wall);
        world.unload_zone(zone);
        report_case("unloading a zone deletes its layout entities", !wall.is_alive());

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_persistent_ids()
    {
        report_group("persistent ids");

        stratum_sim::CountingIdAllocator counting(stratum_sim::PersistentId {});
        set<stratum_sim::PersistentId> allocated {counting.next(), counting.next()};
        stratum_sim::CountingIdAllocator restored(counting.next());
        allocated.insert(restored.next());
        allocated.insert(restored.next());
        report_case_equal("an allocator restored from an id it returned repeats no id", allocated.size(), 4);

        auto persistent_id = [](flecs::entity e) { return e.get<stratum_sim::PersistentId>(); };

        SuiteWorld world(counting);
        flecs::entity crypt = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::placement_type where(zone_layout(world, crypt).position(stratum_sim::discrete_point_type {2, 2}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f));
        flecs::entity monster = world.create_entity(crypt, where);
        flecs::entity chest = world.create_entity(crypt, where);

        report_case("zones and entities have persistent ids of their own",
            set {persistent_id(crypt), persistent_id(moor), persistent_id(monster), persistent_id(chest)}.size() == 4);
        report_case("an entity is found by its persistent id",
            world.entity(persistent_id(monster)) == monster && world.entity(persistent_id(chest)) == chest);

        stratum_sim::PersistentId chest_id = persistent_id(chest);
        chest.destruct();
        report_case("a destroyed entity is not found", !world.entity(chest_id));

        stratum_sim::EntityReference reference = world.make_reference(monster);
        report_case("a reference resolves to its entity", world.resolve(reference) == monster);
        report_case("the world index has the zone it is in", world.zone_of(reference.persistent_id) == persistent_id(crypt));

        world.relocate(monster, moor, where);
        report_case("relocating it changes its zone in the world index", world.zone_of(reference.persistent_id) == persistent_id(moor));

        flecs::entity coin = world.create_entity(moor, where);
        stratum_sim::EntityReference coin_reference = world.make_reference(coin);
        stratum_sim::reparent(coin, monster, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        world.relocate(monster, crypt, where);
        report_case("what it carries changes zone with it", world.zone_of(coin_reference.persistent_id) == persistent_id(crypt));

        stratum_sim::PersistentId crypt_id = persistent_id(crypt);
        world.unload_zone(crypt);

        report_case("unloading its zone leaves a reference unresolved",
            !world.resolve(reference) && !world.entity(reference.persistent_id));
        report_case("and the world index keeps its zone", world.zone_of(reference.persistent_id) == crypt_id);

        // Loaded again with the id it was saved with, as a saved zone restores it
        flecs::entity crypt_again = load_zone_unpopulated(world, "data/room.zone");
        flecs::entity monster_again = world.entities().entity()
            .child_of(crypt_again)
            .set<stratum_sim::Site>(stratum_sim::Site {stratum_sim::Relation::In, {}, where})
            .set<stratum_sim::PersistentId>(reference.persistent_id);

        report_case("a reference re-resolves once its entity is loaded again", world.resolve(reference) == monster_again);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_access_points()
    {
        report_group("access points");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);

        // Two rooms, a staircase leading up in one and one leading down in the other
        flecs::entity zone = load_zone_unpopulated(world, "data/walls.zone");
        const stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);

        flecs::entity staircase_type = stratum_sim::access_type(world.entities(), "staircase");
        flecs::entity portal_type = stratum_sim::access_type(world.entities(), "portal");

        report_case("the world registers the access point types staircase, door, portal and waypoint", staircase_type && portal_type
            && stratum_sim::access_type(world.entities(), "door") && stratum_sim::access_type(world.entities(), "waypoint")
            && !stratum_sim::access_type(world.entities(), "up") && !stratum_sim::access_type(world.entities(), "chute"));
        report_case("each arriving at its own type", staircase_type.get<stratum_sim::Arrival>().access_type == staircase_type
            && portal_type.get<stratum_sim::Arrival>().access_type == portal_type);

        flecs::entity up = stratum_sim::entry_point(zone, staircase_type, 1);
        flecs::entity down = stratum_sim::entry_point(zone, staircase_type, -1);

        report_case("the loader builds each access point of the file, of its type", up && down && up != down);
        report_case("each sited on the wall the file sites it on", stratum_sim::wall_id_of(up.get<stratum_sim::Site>())
            == layout.wall_toward(stratum_sim::discrete_point_type {2, 1}, cardinal_direction(CardinalDirection::West)));
        report_case("with the order and the placement the file gives it", up.get<stratum_sim::AccessPoint>().order == 0
            && down.get<stratum_sim::AccessPoint>().order == 1
            && layout.cell_position_at(up.get<stratum_sim::AccessPoint>().placement.position()) == stratum_sim::discrete_point_type {2, 1}
            && up.get<stratum_sim::AccessPoint>().placement.direction().x() > 0.0f);
        report_case("an entry point's relocator targets the level at the offset", !stratum_sim::entry_point(zone, staircase_type, -2)
            && stratum_sim::targets_relocator_level(up, 1) && !stratum_sim::targets_relocator_level(up, -1));

        // Of the access points whose relocators target the offset, the lowest-ordered is the entry point
        stratum_format::RelocatorTarget up_target {nullopt, stratum_format::RelativeZone {1}, nullopt};
        flecs::entity up_2 = stratum_sim::create_access_point(zone, staircase_type, up.get<stratum_sim::Site>(), 2,
            up.get<stratum_sim::AccessPoint>().placement);
        up_2.set<stratum_sim::Relocator>(stratum_sim::Relocator {up_target});
        report_case("the entry point is the access point of the type with the lowest order", stratum_sim::entry_point(zone, staircase_type, 1) == up
            && up_2.is_alive());

        // An absolute target's offset is the difference between its level and the zone's
        up_2.set<stratum_sim::Relocator>(stratum_sim::Relocator {stratum_format::RelocatorTarget {nullopt, stratum_format::AbsoluteZone {-1}, nullopt}});
        bool unleveled = !stratum_sim::targets_relocator_level(up_2, 1);
        zone.set<stratum_sim::ZoneLevel>(stratum_sim::ZoneLevel {-2});
        report_case("an absolute target's offset is its difference from the zone's level, and a zone with no level has none",
            unleveled && stratum_sim::targets_relocator_level(up_2, 1) && !stratum_sim::targets_relocator_level(up_2, -1));

        // A portal on the floor, in the middle of a room
        stratum_sim::placement_type portal_placement(layout.position(stratum_sim::discrete_point_type {0, 1}), stratum_sim::vector_type(0.0f, 1.0f, 0.0f));
        flecs::entity portal = stratum_sim::create_access_point(zone, portal_type,
            stratum_sim::Site {stratum_sim::Relation::On, stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {0, 1}}, {}},
            0, portal_placement);
        portal.set<stratum_sim::Relocator>(stratum_sim::Relocator {stratum_format::RelocatorTarget {nullopt, stratum_format::RelativeZone {0}, nullopt}});
        report_case("an access point of any type, on any surface, is an entry point", stratum_sim::entry_point(zone, portal_type, 0) == portal);

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

        report_case("a wall id decodes to its cell and its direction", decoded);

        // Entering places the character in front of up staircase 0, facing away from it
        stratum_sim::Game game(g_suite_config);
        stratum_sim::Session& session = game.session();
        enter_zone_file(game, "data/corridor.zone");

        const stratum_sim::placement_type& start = stratum_sim::placement_of(session.character().entity());
        report_case("the character enters on the floor in front of the staircase leading up",
            stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(start.position()) == stratum_sim::discrete_point_type {1, 1});
        report_case("facing away from it", start.direction().y() < 0.0f && start.direction().x() == 0.0f);

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
            session.enter(portal_only, suite_entry_type(session), suite_entry_offset);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        report_case("entering a zone with no staircase leading up throws", thrown && session.zone() != portal_only);

        session.enter(portal_only, game_portal_type, 0);
        report_case("and entering it at its portal places the character at the portal's placement", session.zone() == portal_only
            && portal_only_layout.cell_position_at(stratum_sim::placement_of(session.character().entity()).position()) == stratum_sim::discrete_point_type {0, 0});

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_sim_test runs from its own directory
    void test_residency()
    {
        report_group("residency");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator, 2);

        auto corridor = [&]() { return load_zone_unpopulated(world, "data/corridor.zone"); };

        flecs::entity a = corridor();
        flecs::entity b = corridor();
        flecs::entity c = corridor();

        report_case_equal("creating zones unloads none, past the maximum or not", world.zone_count(), 3);

        world.trim_zones(a);
        report_case_equal("past the maximum, the zone entered and those never entered are not unloaded", world.zone_count(), 3);

        world.trim_zones(b);
        report_case("the zone entered before is", !a.is_alive() && world.zone_count() == 2);

        world.trim_zones(c);
        report_case("at the maximum, nothing is", b.is_alive() && world.zone_count() == 2);

        flecs::entity d = corridor();
        world.trim_zones(d);
        report_case("the zone entered longest ago is unloaded first", !b.is_alive() && c.is_alive() && d.is_alive());

        // Entering c again makes it the zone entered most recently
        world.trim_zones(c);
        flecs::entity e = corridor();
        world.set_zone_count_max(1);
        world.trim_zones(e);

        report_case("as many are unloaded as the maximum requires, never the zone entered",
            !d.is_alive() && !c.is_alive() && e.is_alive() && world.zone_count() == 1);

        // Deferred, as inside a system or an observer, an unload finishes when the deferred operations run
        flecs::entity f = corridor();
        flecs::entity g = corridor();

        flecs::entity kept = world.create_entity(e, stratum_sim::placement_type(
            zone_layout(world, e).position(stratum_sim::discrete_point_type {1, 1}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));
        stratum_sim::EntityReference kept_reference = world.make_reference(kept);
        stratum_sim::PersistentId e_id = e.get<stratum_sim::PersistentId>();

        world.entities().defer_begin();
        world.trim_zones(f);
        world.trim_zones(g);
        bool pending = e.is_alive() && f.is_alive();
        world.entities().defer_end();

        report_case("while deferred, the zones entered before stay until the deferred operations run", pending);
        report_case("then the two trims in one deferred block unload those two and no more",
            !e.is_alive() && !f.is_alive() && g.is_alive() && world.zone_count() == 1);
        report_case("and an entity referred to in one keeps its zone in the world index",
            world.zone_of(kept_reference.persistent_id) == e_id && !world.entity(kept_reference.persistent_id));

        return;
    }

    //--------------------------------------------------------------------------
    void test_commands()
    {
        report_group("commands");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/room.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        stratum_sim::vector_type north(0.0f, 1.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 2}), east));

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_position_at(stratum_sim::placement_of(actor).position()); };

        // A move in the actor's facing direction
        commands.push(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 1.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a step moves the actor", cell_of() == stratum_sim::discrete_point_type(2, 2));
        report_case_equal("and reports one event", events.size(), 1);
        report_case("which is a move", events.front().is<stratum_sim::Moved>());
        report_case("naming the actor", events.front().get<stratum_sim::Moved>().entity_id == actor.id());
        report_case("commands are consumed", commands.empty());

        events.clear();

        // Walking into a wall reports what stopped it and does not move
        commands.push(actor.id(), stratum_sim::Step {north, 5.0f});
        stratum_sim::apply(world, commands, events);

        report_case("a blocked step does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));
        report_case("it moved as far as it could", events.front().is<stratum_sim::Moved>());
        report_case("and reported what stopped it", events.events().back().is<stratum_sim::Blocked>());
        report_case("what stopped it is a wall of the layout", events.events().back().get<stratum_sim::Blocked>().blocking_wall_id != nullid);

        events.clear();

        // Held against a wall, nothing happens and nothing is spent
        stratum_sim::Command held(actor.id(), stratum_sim::Step {north, 1.0f});
        report_case_equal("a step that cannot move costs nothing", static_cast<size_t>(stratum_sim::apply(world, held, events)), 0);
        report_case("and reports only the block", events.size() == 1 && events.front().is<stratum_sim::Blocked>());

        events.clear();

        // A move costs in proportion to the distance traveled
        auto place = [&](const stratum_sim::discrete_point_type& cell, const stratum_sim::vector_type& direction)
        {
            stratum_sim::set_placement(actor, stratum_sim::placement_type(layout.position(cell), direction));
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

        // Stopped short, it is charged for the distance traveled
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
            layout.turn(stratum_sim::placement_of(actor), stratum_sim::Rotation::CW).direction()});
        stratum_sim::apply(world, commands, events);

        report_case("turning right from east faces south", stratum_sim::placement_of(actor).direction().y() == -1.0f);
        report_case("and reports a turn", events.front().is<stratum_sim::Turned>());
        report_case("turning does not move", cell_of() == stratum_sim::discrete_point_type(2, 3));

        events.clear();

        // Interact with the wall or door the actor faces
        stratum_sim::Command interact_floor(actor.id(), stratum_sim::Interact {});
        report_case_equal("interacting with open floor does nothing", static_cast<size_t>(stratum_sim::apply(world, interact_floor, events)), 0);
        report_case("and reports nothing", events.empty());

        // Face the doorway and interact
        stratum_sim::set_placement(world.entities().entity(actor.id()),
            stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 2}), east));
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

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

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
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");
        world.create_entity(moor, place(1, 1));

        report_case_equal("another zone's entities do not appear", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 2);
        report_case_equal("they appear in their own", world.entities_at(moor, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        b.destruct();
        report_case_equal("a destroyed entity stops occupying", world.entities_at(zone, stratum_sim::discrete_point_type {1, 1}).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    void test_zone_placement_index()
    {
        report_group("zone placement index");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity hall = load_zone_unpopulated(world, "data/corridor.zone");
        flecs::entity moor = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, hall);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };

        // Each entity whose site has no address, found by a scan of the zone, is in the index at its placement, and the
        // index has no other
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

        report_case("a zone loaded from a file has an empty index", world.placement_index(hall).entity_count() == 0 && index_agrees(hall));

        flecs::entity rat = world.create_entity(hall, place(1, 1));
        flecs::entity bat = world.create_entity(hall, place(2, 1));
        report_case("a created entity is indexed", world.placement_index(hall).entity_count() == 2 && index_agrees(hall));

        stratum_sim::set_placement(rat, place(3, 1));
        report_case("a moved entity is indexed where it moved", world.entities_at(hall, stratum_sim::discrete_point_type {3, 1})
            == stratum_sim::World::entities_type {rat} && world.entities_at(hall, stratum_sim::discrete_point_type {1, 1}).empty()
            && index_agrees(hall));

        stratum_sim::EventQueue events;
        stratum_sim::apply(world, stratum_sim::Command(rat.id(), stratum_sim::Step {east, 1.0f}), events);
        report_case("and so is one moved by a command", world.entities_at(hall, stratum_sim::discrete_point_type {4, 1})
            == stratum_sim::World::entities_type {rat} && index_agrees(hall));

        flecs::entity torch = world.create_entity(hall, place(2, 1));
        torch.set<stratum_sim::Site>(stratum_sim::Site {stratum_sim::Relation::On,
            stratum_sim::FlatCellAddress {layout.floor_id(), stratum_sim::discrete_point_type {2, 1}}, {}});
        report_case("an entity sited on an address leaves the index", !world.placement_index(hall).contains(torch.id())
            && world.entities_at(hall, stratum_sim::discrete_point_type {2, 1}) == stratum_sim::World::entities_type {bat} && index_agrees(hall));

        flecs::entity chest = world.create_entity(hall, place(5, 1));
        flecs::entity coin = world.create_entity(hall, place(5, 1));
        stratum_sim::reparent(coin, chest, stratum_sim::Site {stratum_sim::Relation::In, {}, {}});
        report_case("an entity put in another leaves the index", !world.placement_index(hall).contains(coin.id())
            && world.entities_at(hall, stratum_sim::discrete_point_type {5, 1}) == stratum_sim::World::entities_type {chest} && index_agrees(hall));

        world.relocate(chest, moor, place(1, 1));
        report_case("a relocated entity leaves its zone's index for the other's", !world.placement_index(hall).contains(chest.id())
            && world.entities_at(moor, stratum_sim::discrete_point_type {1, 1}) == stratum_sim::World::entities_type {chest}
            && !world.placement_index(moor).contains(coin.id()) && index_agrees(hall) && index_agrees(moor));

        flecs::entity wanderer = world.entities().entity();
        world.relocate(wanderer, hall, place(5, 1));
        report_case("an entity owned by the game is indexed in the zone it is in", world.entities_at(hall, stratum_sim::discrete_point_type {5, 1})
            == stratum_sim::World::entities_type {wanderer} && index_agrees(hall));

        world.relocate(wanderer, moor, place(5, 1));
        report_case("and moves to the index of the zone it is relocated to", !world.placement_index(hall).contains(wanderer.id())
            && world.placement_index(moor).contains(wanderer.id()) && index_agrees(hall) && index_agrees(moor));

        bat.destruct();
        report_case("a deleted entity leaves the index", world.placement_index(hall).entity_count() == 1 && index_agrees(hall));

        world.unload_zone(moor);
        report_case("unloading a zone leaves the game's entity in no zone", wanderer.is_alive() && !stratum_sim::zone_of(wanderer));

        world.relocate(wanderer, hall, place(5, 1));
        report_case("from which it is relocated into another zone's index", world.entities_at(hall, stratum_sim::discrete_point_type {5, 1})
            == stratum_sim::World::entities_type {wanderer} && index_agrees(hall));

        stratum_format::Document saved = stratum_sim::make_document(hall);
        bool saved_without_wanderer = none_of(saved.entities.begin(), saved.entities.end(),
            [&](const stratum_format::Entity& entity) { return entity.id == wanderer.get<stratum_sim::PersistentId>().id; });
        report_case("and is left out of the zone's saved document", saved_without_wanderer);

        flecs::entity loaded = world.load_zone(world.region, "data/door.zone");
        report_case("a populated zone indexes its denizen and not its staircase", world.placement_index(loaded).entity_count() == 1
            && index_agrees(loaded));

        return;
    }

    //--------------------------------------------------------------------------
    // World composes topology and occupancy
    void test_world_move()
    {
        report_group("world movement");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);
        auto place = [&](int i, int j) { return stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {i, j}), east); };
        auto cell_of = [&](flecs::entity e) { return layout.cell_position_at(stratum_sim::placement_of(e).position()); };

        flecs::entity mover = world.create_entity(zone, place(1, 1));
        mover.add<stratum_sim::Blocking>();

        // Nothing in the way, so the world returns what the layout alone would
        stratum_sim::MoveResult clear = world.move(mover, east, 2.0f);
        report_case("a move with nothing in the way reaches its distance", layout.cell_position_at(clear.placement.position()) == stratum_sim::discrete_point_type(3, 1));
        report_case("and reports neither kind of blocker", clear.blocking_wall_id == nullid && clear.blocking_entity_id == nullid);

        // A blocking entity stops it where a wall would
        flecs::entity boulder = world.create_entity(zone, place(3, 1));
        boulder.add<stratum_sim::Blocking>();

        stratum_sim::MoveResult stopped = world.move(mover, east, 4.0f);
        report_case("a blocking entity stops the mover short", layout.cell_position_at(stopped.placement.position()) == stratum_sim::discrete_point_type(2, 1));
        report_case("and is named as the blocker", stopped.blocking_entity_id == static_cast<id_type>(boulder.id()));
        report_case("rather than as a wall", stopped.blocking_wall_id == nullid);

        // Only a Blocking entity blocks
        boulder.remove<stratum_sim::Blocking>();
        stratum_sim::MoveResult past = world.move(mover, east, 4.0f);
        report_case("an entity that does not block is moved through", layout.cell_position_at(past.placement.position()).i() > 3);
        report_case("and nothing is reported as a blocker", past.blocking_entity_id == nullid);

        boulder.add<stratum_sim::Blocking>();

        // An entity never blocks itself
        stratum_sim::MoveResult alone = world.move(boulder, east, 1.0f);
        report_case("a mover is not stopped by itself", layout.cell_position_at(alone.placement.position()) == stratum_sim::discrete_point_type(4, 1));

        // Through a command, so that the event and the charge come with it
        stratum_sim::EventQueue events;
        stratum_sim::set_placement(world.entities().entity(mover.id()), place(1, 1));
        stratum_sim::set_placement(world.entities().entity(boulder.id()), place(3, 1));

        stratum_sim::Command step(mover.id(), stratum_sim::Step {east, 4.0f});
        int cost = stratum_sim::apply(world, step, events);

        report_case("the command stopped at the blocker", cell_of(mover) == stratum_sim::discrete_point_type(2, 1));
        report_case_equal("charged for the one cell it covered",
            static_cast<size_t>(cost), static_cast<size_t>(stratum_sim::Scheduler::action_cost));
        report_case("and reported being blocked", events.events().back().is<stratum_sim::Blocked>());
        report_case("naming the entity rather than a wall",
            events.events().back().get<stratum_sim::Blocked>().blocking_entity_id == static_cast<id_type>(boulder.id())
            && events.events().back().get<stratum_sim::Blocked>().blocking_wall_id == nullid);

        // A layout asked directly still resolves its own geometry and nothing else
        stratum_sim::MoveResult geometric = layout.move(place(1, 1), east, 4.0f);
        report_case("the layout alone does not stop for entities", layout.cell_position_at(geometric.placement.position()).i() > 3);

        return;
    }

    //--------------------------------------------------------------------------
    // Every actor but the controlled one decides for itself
    void test_ai()
    {
        report_group("deciding for itself");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
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

        report_case("a pursuer that can see its target records where", monster.get<stratum_sim::Pursue>().seen);
        report_case("and asks to move toward it", monster.get<stratum_sim::Intent>().direction.x() < 0.0f);
        report_case("the controlled entity is not decided for", character.try_get<stratum_sim::Intent>() == nullptr);

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
        report_case("which is the target's own cell it did not take", cell_of(character) == stratum_sim::discrete_point_type(1, 1));

        // A target gone unseen is sought where it was last seen
        flecs::entity gone = world.create_entity(zone, at(4));
        stratum_sim::EntityReference gone_reference = world.make_reference(gone);
        gone.destruct();

        flecs::entity blind = world.create_entity(zone, at(5));
        blind.set<stratum_sim::Pursue>(stratum_sim::Pursue {gone_reference, layout.position(stratum_sim::discrete_point_type {3, 1}), true});

        stratum_sim::think(world, controlled);
        report_case("a pursuer whose target is gone still goes where it last looked",
            blind.get<stratum_sim::Intent>().direction.x() < 0.0f);
        report_case("and still knows where that was", blind.get<stratum_sim::Pursue>().seen);

        // Arriving there and finding nothing, it stops
        stratum_sim::set_placement(blind, at(3));
        stratum_sim::think(world, controlled);

        report_case("arriving there with nothing to find, it stops going", !blind.get<stratum_sim::Pursue>().seen);

        // With no target, it pursues the controlled actor
        flecs::entity seeker = world.create_entity(zone, at(4));
        seeker.set<stratum_sim::Pursue>(stratum_sim::Pursue {});
        stratum_sim::think(world, controlled);

        const stratum_sim::Pursue& seeking = seeker.get<stratum_sim::Pursue>();
        report_case("a pursuer with no target pursues the controlled actor",
            seeking.seen && layout.cell_position_at(seeking.position_last_seen) == cell_of(character));

        // Something with no behavior at all is left alone
        flecs::entity inert = world.create_entity(zone, at(4));
        stratum_sim::think(world, controlled);
        report_case("an entity that decides nothing is not decided for", inert.try_get<stratum_sim::Intent>() == nullptr);

        // A zone generated rather than read
        {
            stratum_sim::Game game_created(g_suite_config);
            stratum_sim::Session& created = game_created.session();
            game_created.generation() = suite_generation();
            game_created.character().create(stratum_sim::CharacterSpec {});
            created.enter(game_created.world().create_zone(game_created.world().create_region(suite_region_spec("door")), stratum_sim::ZoneSpec {}),
                suite_entry_type(created), suite_entry_offset);

            report_case("a constructed zone is loaded", created.loaded());
            report_case("with the notation its generator named", created.notation() == stratum_format::Notation::Block);
            report_case("and a character in it", created.character().entity().is_alive());

            // A door on the doorway shows the reported walls were built
            report_case("its doors are attached to the walls the generator reported",
                stratum_sim::way_through(created.zone(), stratum_sim::layout_as<stratum_sim::DiscreteLayout>(
                    created.layout()).wall_toward(stratum_sim::discrete_point_type {1, 1}, cardinal_direction(CardinalDirection::East))).is_alive());

            // Reading the same geometry from a file gives the same zone
            stratum_sim::Game game_loaded(g_suite_config);
            stratum_sim::Session& loaded = game_loaded.session();
            enter_zone_file(game_loaded, "data/door.zone");
            report_case("and a zone read whole agrees with it",
                loaded.notation() == created.notation()
                && same_geometry(stratum_sim::layout_as<stratum_sim::DiscreteLayout>(loaded.layout()),
                    stratum_sim::layout_as<stratum_sim::DiscreteLayout>(created.layout())));
        }

        // Through a Session
        stratum_sim::Game game(g_suite_config);
        stratum_sim::Session& session = game.session();
        enter_zone_file(game, "data/corridor.zone");

        flecs::entity hunter;
        session.world().entities().query_builder<const stratum_sim::Pursue>()
            .build()
            .each([&](flecs::entity e, const stratum_sim::Pursue&) { hunter = e; });

        report_case("a session places something that decides for itself", hunter.is_alive());

        stratum_sim::EventQueue session_events;
        stratum_sim::discrete_point_type before = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(stratum_sim::placement_of(hunter).position());

        // The controlled entity waits
        for (int n = 0; n < 3; ++n)
        {
            session.scheduler().spend(session.advance(session_events));
        }

        stratum_sim::discrete_point_type after = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(session.layout()).cell_position_at(stratum_sim::placement_of(hunter).position());
        report_case("and it closes on the controlled entity while the host only waits", after.i() < before.i());
        report_case("the character is the game's, in the zone without being its child",
            !session.character().entity().parent() && session.character().entity().target<stratum_sim::InZone>() == session.zone());

        flecs::entity session_character = session.character().entity();
        flecs::entity zone_left = session.zone();
        enter_zone_file(game, "data/room.zone");

        report_case("loading another zone moves the same character into it", &session.character() == &game.character()
            && session.character().entity() == session_character && stratum_sim::zone_of(session_character) == session.zone());
        report_case("and leaves the zone it left loaded", zone_left.is_alive() && game.world().zone_count() == 2);

        flecs::entity zone_entered = session.zone();
        flecs::entity closed = game.world().create_zone(stratum_sim::region_of(session.zone()), make_unique<stratum_sim::DiscreteLayout>(stratum_sim::point_type(0.0f, 0.0f, 0.0f),
            1.0f, stratum_sim::discrete_vector_type(2, 2)));

        bool thrown = false;

        try
        {
            session.enter(closed, suite_entry_type(session), suite_entry_offset);
        }
        catch (const runtime_error&)
        {
            thrown = true;
        }

        report_case("entering a zone with no open cell throws, the character staying where it was",
            thrown && session.zone() == zone_entered && stratum_sim::zone_of(session_character) == zone_entered);

        // The zone never entered is no candidate
        session.world().set_zone_count_max(1);
        enter_zone_file(game, "data/corridor.zone");

        report_case("past the maximum, loading another unloads the zones the character entered before",
            !zone_left.is_alive() && !zone_entered.is_alive() && closed.is_alive() && stratum_sim::zone_of(session_character) == session.zone());

        return;
    }

    //--------------------------------------------------------------------------
    // A zone read from a file is walkable by commands ordered by the scheduler, with no rendering
    void test_walk()
    {
        report_group("walking a dungeon");

        stratum_sim::CountingIdAllocator id_allocator(stratum_sim::PersistentId {});
        SuiteWorld world(id_allocator);
        flecs::entity zone = load_zone_unpopulated(world, "data/corridor.zone");

        stratum_sim::DiscreteLayout& layout = zone_layout(world, zone);
        stratum_sim::vector_type east(1.0f, 0.0f, 0.0f);

        flecs::entity actor = world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}), east));

        stratum_sim::Scheduler scheduler;
        scheduler.add(actor.id(), 100);

        stratum_sim::CommandQueue commands;
        stratum_sim::EventQueue events;

        auto cell_of = [&]() { return layout.cell_position_at(stratum_sim::placement_of(actor).position()); };

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
            layout.turn(stratum_sim::placement_of(actor), stratum_sim::Rotation::Half).direction()});
        stratum_sim::apply(world, about, events);
        report_case("turning around faces west", stratum_sim::placement_of(actor).direction().x() == -1.0f);

        events.clear();
        stratum_sim::Command back(actor.id(), stratum_sim::Step {stratum_sim::vector_type(0.0f, 0.0f, 0.0f), 4.0f});
        stratum_sim::apply(world, back, events);

        report_case("walking back reaches the west end", cell_of() == stratum_sim::discrete_point_type(1, 1));
        report_case("in a single command", events.size() == 1 && events.front().is<stratum_sim::Moved>());

        return;
    }

    //--------------------------------------------------------------------------
    // geometry::Polygon's Cartesian containment, which stratum_sim depends on
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

        // Tilted, with no dominant axis
        stratum_sim::polygon_type tilted = polygon_of({
            stratum_sim::point_type(1.0f, 0.0f, 0.0f),
            stratum_sim::point_type(0.0f, 1.0f, 0.0f),
            stratum_sim::point_type(0.0f, 0.0f, 1.0f)});

        report_case("an interior point of a tilted polygon is contained", tilted.contains(stratum_sim::point_type(1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f)));
        report_case("an exterior point of a tilted polygon is not", !tilted.contains(stratum_sim::point_type(1.0f, 1.0f, -1.0f)));

        return;
    }

    //--------------------------------------------------------------------------
    void test_insert_definitions()
    {
        report_group("setting definitions inserted");

        settings::Settings test_settings;
        settings::SettingsGroups test_groups(test_settings);
        stratum_sim::insert_definitions(stratum_format::read_setting_definitions(filesystem::path("../../assets")
            / stratum_format::SettingDefinitionsDocument::filepath), test_settings, test_groups);

        const string count_key = "gameplay.difficulty.denizen_count_multiplier";
        report_case("the game's definitions insert its settings, with their menu fields and ranges", test_settings.contains(count_key)
            && test_settings.setting(count_key).scope == settings::SettingScope::Game && test_settings.setting(count_key).category == "difficulty"
            && test_settings.setting(count_key).minimum == "0.25" && !test_settings.bounded_inclusive(count_key, "5"));
        report_case("and its difficulty group, on its default preset", test_groups.contains("gameplay.difficulty")
            && test_groups.group("gameplay.difficulty").selected().name == "normal" && test_groups.group("gameplay.difficulty").display_name() == "Difficulty"
            && test_settings.value<double>(count_key) == 1);

        test_groups.select("gameplay.difficulty", "hard");
        report_case("whose presets set its members", test_settings.value<double>(count_key) == 1.5
            && test_settings.value<double>("gameplay.difficulty.denizen_speed_multiplier") == 1.25);

        stratum_format::SettingDefinitionsDocument document;
        document.enumerations.push_back({"mod.torches.style", {"steady", "flicker"}});
        document.settings.push_back({.key = "mod.torches.style", .type_name = "mod.torches.style", .default_value = "flicker",
            .scope = stratum_format::SettingDefinitionsDocument::Scope::User, .restart_required = true});
        stratum_sim::insert_definitions(document, test_settings, test_groups);
        report_case("an enumeration is registered before the settings that have it", test_settings.registered_type("mod.torches.style")
            && test_settings.text("mod.torches.style") == "flicker" && test_settings.setting("mod.torches.style").restart_required
            && test_settings.setting("mod.torches.style").scope == settings::SettingScope::User);

        bool inserted_twice_throws = false;
        try
        {
            stratum_sim::insert_definitions(document, test_settings, test_groups);
        }
        catch (const runtime_error&)
        {
            inserted_twice_throws = true;
        }

        report_case("and a definition that is invalid is an error", inserted_twice_throws);
        return;
    }

    //--------------------------------------------------------------------------
    void test_config()
    {
        report_group("config");

        // The program directory, with or without a config file
        const filesystem::path program_directory = "out/config";
        filesystem::remove_all(program_directory);
        filesystem::create_directories(program_directory);

        const filesystem::path user_directory = stratum_sim::default_user_directory();
        stratum_sim::Config config = stratum_sim::load_config(program_directory);
        report_case("with no config file, the assets are in the program directory, and the rest in the user directory",
            config.assets_directory == program_directory / "assets" && config.user_directory == user_directory
            && config.saves_directory == user_directory / "saves" && config.working_directory == user_directory / "working"
            && config.logs_directory == user_directory / "logs" && config.cache_directory == user_directory / "cache"
            && config.mods_directory == user_directory / "mods" && config.user_settings_filepath == user_directory / "settings.json"
            && config.zone_count_max == 8);
        report_case("and nothing is written in the program directory", filesystem::is_empty(program_directory));

        const filesystem::path absolute_logs = filesystem::absolute("out/config_logs");
        {
            ofstream os(program_directory / "config.json", ios_base::binary);
            os << "{ \"format\": \"quetzal.stratum_config\", \"version\": \"0.1\", \"assets_directory\": \"../../assets\", "
                << "\"user_directory\": \"user\", \"saves_directory\": \"elsewhere/saves\", \"logs_directory\": \""
                << absolute_logs.generic_string() << "\", \"zone_count_max\": 3 }";
        }

        config = stratum_sim::load_config(program_directory);
        report_case("a config file overrides the values it has, the assets and user directories relative to the program directory",
            config.assets_directory == program_directory / "../../assets" && config.user_directory == program_directory / "user"
            && config.zone_count_max == 3);
        report_case("and the other locations relative to the user directory, an absolute one kept",
            config.saves_directory == program_directory / "user/elsewhere/saves" && config.logs_directory == absolute_logs
            && config.working_directory == program_directory / "user/working");

        {
            ofstream os(program_directory / "config.json", ios_base::binary);
            os << "{ \"format\": \"quetzal.stratum_config\", \"version\": \"0.1\", \"save_directory\": \"saves\" }";
        }

        bool unknown_key_rejected = false;
        try
        {
            stratum_sim::load_config(program_directory);
        }
        catch (const runtime_error&)
        {
            unknown_key_rejected = true;
        }

        report_case("a key the config file does not have is an error", unknown_key_rejected);
        report_case("the default user directory is KML/Stratum in the platform's directory for application data",
            user_directory.generic_string().ends_with("KML/Stratum"));
        report_case("the executable's directory has the suite's executable", filesystem::exists(executable_directory() / "stratum_sim_test.exe"));

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    stratum_sim::load_setting_definitions("../../assets", settings::store(), settings::groups());

    test_command_queue();
    test_event_queue();
    test_layout_hierarchy();
    test_zone_layout();
    test_rooms();
    test_grid();
    test_placement_index();
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
    test_door_entities();
    test_loaded_entities();
    test_decoded_components();
    test_world();
    test_game();
    test_specs();
    test_saved_zone_documents();
    test_regions();
    test_level_targets();
    test_relocating_through();
    test_entering_access_points();
    test_interacting_with_relocators();
    test_properties();
    test_layout_entities();
    test_persistent_ids();
    test_access_points();
    test_residency();
    test_commands();
    test_occupancy();
    test_zone_placement_index();
    test_world_move();
    test_ai();
    test_walk();
    test_polygon_containment();
    test_insert_definitions();
    test_config();

    return report_results();
}
