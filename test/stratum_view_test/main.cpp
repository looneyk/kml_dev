//------------------------------------------------------------------------------
// stratum_view_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/file_util.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include "quetzal/stratum_view/cells.hpp"
#include <format>
#include <fstream>
#include <iostream>
#include <string>

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

    //--------------------------------------------------------------------------
    string read_file(const string& filepath)
    {
        ifstream is(filepath, ios_base::binary);
        string s;
        string line;

        while (read_line(is, line))
        {
            s += line;
            s += '\n';
        }

        return s;
    }

    //--------------------------------------------------------------------------
    void test_console()
    {
        report_group("console");

        stratum_view::Console console(4, 3, '.');

        report_case_equal("it has the width asked for", console.width(), 4);
        report_case_equal("and the height", console.height(), 3);
        report_case("every cell starts filled", console.at(0, 0) == '.' && console.at(3, 2) == '.');

        console.set(1, 0, '@');
        report_case("a set cell holds its character", console.at(1, 0) == '@');
        report_case("and its neighbours are untouched", console.at(0, 0) == '.' && console.at(2, 0) == '.');

        // Row 0 is the lowest, so it prints last
        string s = console.to_string();
        report_case_equal("printing gives one line per row", s.size(), 3 * 5);
        report_case("the highest row prints first", s.substr(0, 4) == "....");
        report_case("and row zero last", s.substr(10, 4) == ".@..");

        console.fill('#');
        report_case("fill replaces everything", console.at(1, 0) == '#');

        return;
    }

    //--------------------------------------------------------------------------
    void test_difference()
    {
        report_group("console difference");

        stratum_view::Console a(6, 3, '.');
        stratum_view::Console b = a;

        report_case("two identical consoles differ nowhere", stratum_view::difference(a, b).empty());

        b.set(2, 1, '@');
        stratum_view::spans_type spans = stratum_view::difference(a, b);
        report_case_equal("one changed cell is one span", spans.size(), 1);
        report_case("covering just that cell", !spans.empty() && spans[0] == stratum_view::Span {2, 1, 1});

        b.set(3, 1, '@');
        spans = stratum_view::difference(a, b);
        report_case_equal("two adjacent cells merge into one span", spans.size(), 1);
        report_case("of two", !spans.empty() && spans[0] == stratum_view::Span {2, 1, 2});

        b.set(0, 0, '#');
        spans = stratum_view::difference(a, b);
        report_case_equal("a separate row is a separate span", spans.size(), 2);
        report_case("and rows come lowest first", spans.size() == 2 && spans[0].j == 0 && spans[1].j == 1);

        // A cell that matches again ends the run
        stratum_view::Console c = a;
        c.set(1, 2, 'x');
        c.set(3, 2, 'x');
        spans = stratum_view::difference(a, c);
        report_case_equal("a gap splits a run", spans.size(), 2);

        // Differing sizes give the whole of b, so that a device knows to repaint
        spans = stratum_view::difference(stratum_view::Console(2, 2, '.'), a);
        report_case_equal("a size change reports every row", spans.size(), 3);
        report_case("each row entire", !spans.empty() && spans[0] == stratum_view::Span {0, 0, 6});

        report_case("difference is oriented, reporting b's content", stratum_view::difference(b, a).size() == stratum_view::difference(a, b).size());

        return;
    }

    //--------------------------------------------------------------------------
    // The reduction both views consume, before either decides how to show it
    void test_cells()
    {
        report_group("cells");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt");
        stratum_sim::load_layout("data/corridor.level", world.layout(zone));

        stratum_view::Cells cells = stratum_view::cells(world, zone);

        report_case_equal("it covers the zone's grid", cells.width(), 7);
        report_case_equal("in both directions", cells.height(), 3);
        report_case("rock where the level says rock", cells.at(0, 1) == stratum_view::Kind::Rock);
        report_case("floor where it says open", cells.at(1, 1) == stratum_view::Kind::Floor);
        report_case("no actor before one exists", cells.at(3, 1) != stratum_view::Kind::Actor);

        stratum_sim::Layout& layout = world.layout(zone);
        world.create_entity(zone, stratum_sim::placement_type(layout.cell_position(stratum_sim::GridPosition {3, 1}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        cells = stratum_view::cells(world, zone);
        report_case("an entity takes the cell it stands in", cells.at(3, 1) == stratum_view::Kind::Actor);
        report_case("and only that one", cells.at(2, 1) == stratum_view::Kind::Floor);

        // A view's mapping is its own; the terminal's is Glyphs
        stratum_view::Glyphs glyphs;
        report_case("glyphs map each kind", glyphs(stratum_view::Kind::Rock) == '#'
            && glyphs(stratum_view::Kind::Floor) == '.' && glyphs(stratum_view::Kind::Actor) == '@');

        report_case("rendering from cells matches rendering from the world",
            stratum_view::render(cells).to_string() == stratum_view::render(world, zone).to_string());

        // difference serves any buffer, which is what having one implementation buys
        stratum_view::Cells other = cells;
        other.set(5, 1, stratum_view::Kind::Rock);
        report_case_equal("differencing works over kinds too", stratum_view::difference(cells, other).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_view_test runs from its own directory
    void test_render()
    {
        report_group("ascii render");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("crypt");
        stratum_sim::load_layout("data/corridor.level", world.layout(zone));

        stratum_view::Console console = stratum_view::render(world, zone);

        report_case_equal("the console covers the zone's grid", console.width(), 7);
        report_case_equal("in both directions", console.height(), 3);

        report_case("rock renders as rock", console.at(0, 1) == '#');
        report_case("open floor renders as floor", console.at(1, 1) == '.');
        report_case("the far end is floor too", console.at(5, 1) == '.');
        report_case("beyond it is rock", console.at(6, 1) == '#');

        // An actor draws over the floor it stands on
        stratum_sim::Layout& layout = world.layout(zone);
        world.create_entity(zone, stratum_sim::placement_type(layout.cell_position(stratum_sim::GridPosition {3, 1}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        console = stratum_view::render(world, zone);
        report_case("an actor draws over its floor", console.at(3, 1) == '@');
        report_case("and only its own cell", console.at(2, 1) == '.');

        return;
    }

    //--------------------------------------------------------------------------
    // An unoccupied level of nothing but rock and floor renders back to the file it
    // was loaded from. vault.level is asymmetric top to bottom, so this pins row order too
    void test_round_trip()
    {
        report_group("round trip");

        for (const string& name : {"corridor"s, "vault"s})
        {
            string filepath = format("data/{}.level", name);

            stratum_sim::World world;
            flecs::entity zone = world.create_zone(name);
            stratum_sim::load_layout(filepath, world.layout(zone));

            string s = stratum_view::render(world, zone).to_string();
            report_case(format("{} renders back to its own file", name), s == read_file(filepath), "\n" + s);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A generated dungeon, which has doorways and shafts as well as rooms and corridors.
    // All of them are open, so every cell the file does not call rock must render as floor
    void test_dungeon()
    {
        report_group("generated dungeon");

        string source = read_file("data/dungeon.level");

        stratum_sim::World world;
        flecs::entity zone = world.create_zone("dungeon");
        stratum_sim::load_layout("data/dungeon.level", world.layout(zone));

        stratum_view::Console console = stratum_view::render(world, zone);

        report_case_equal("the console is as wide as the file", console.width(), 31);
        report_case_equal("and as tall", console.height(), 31);

        size_t rock_count = 0;
        size_t floor_count = 0;
        size_t mismatch_count = 0;

        for (size_t j = 0; j < console.height(); ++j)
        {
            for (size_t i = 0; i < console.width(); ++i)
            {
                // File line 0 is the highest row
                char source_character = source[(console.height() - 1 - j) * (console.width() + 1) + i];
                bool rock = source_character == '#';

                rock ? ++rock_count : ++floor_count;

                if (console.at(i, j) != (rock ? '#' : '.'))
                {
                    ++mismatch_count;
                }
            }
        }

        report_case_equal("every cell agrees with the file", mismatch_count, 0);
        report_case("it found rock", rock_count > 0);
        report_case("and open cells", floor_count > 0);

        return;
    }

    //--------------------------------------------------------------------------
    string name_of(const stratum_sim::vector_type& direction)
    {
        auto [dx, dy, dz] = direction.rep();

        if (abs(dx) >= abs(dy))
        {
            return dx >= stratum_sim::value_type(0) ? "east" : "west";
        }

        return dy >= stratum_sim::value_type(0) ? "north" : "south";
    }

    //--------------------------------------------------------------------------
    // The command a script character asks for, empty for anything else. Mapped here rather
    // than taken from the terminal host, which is not what these frames are checking
    stratum_sim::Command command_of(char c, flecs::entity player, const stratum_sim::Layout& layout)
    {
        id_type entity_id = player.id();

        auto step = [entity_id](stratum_sim::value_type dx, stratum_sim::value_type dy)
        {
            return stratum_sim::Command(entity_id, stratum_sim::Step {stratum_sim::vector_type(dx, dy, 0.0f), 1.0f});
        };

        // A relative turn is resolved against the layout here, the command itself carrying
        // the absolute facing it arrived at
        auto turn = [&](stratum_sim::Rotation rotation)
        {
            const stratum_sim::placement_type& placement = player.get<stratum_sim::placement_type>();
            return stratum_sim::Command(entity_id, stratum_sim::TurnTo {layout.turn(placement, rotation).direction()});
        };

        switch (c)
        {
            case 'h': return step(-1.0f, 0.0f);
            case 'j': return step(0.0f, -1.0f);
            case 'k': return step(0.0f, 1.0f);
            case 'l': return step(1.0f, 0.0f);
            case '<': return turn(stratum_sim::Rotation::CCW);
            case '>': return turn(stratum_sim::Rotation::CW);
            case ' ': return stratum_sim::Command(entity_id, stratum_sim::Interact {});
        }

        return {};
    }

    //--------------------------------------------------------------------------
    string describe(const stratum_sim::Event& event, const stratum_sim::Layout& layout)
    {
        if (const stratum_sim::Moved* pevent = event.try_get<stratum_sim::Moved>())
        {
            stratum_sim::GridPosition cell = layout.cell_at(pevent->placement.position());
            return format("moved to ({}, {})", cell.i, cell.j);
        }

        if (const stratum_sim::Blocked* pevent = event.try_get<stratum_sim::Blocked>())
        {
            return pevent->obstruction_entity_id != nullid
                ? format("blocked by {}", pevent->obstruction_entity_id)
                : format("blocked by wall {}", pevent->obstruction_wall_id);
        }

        if (const stratum_sim::Turned* pevent = event.try_get<stratum_sim::Turned>())
        {
            return format("turned to face {}", name_of(pevent->placement.direction()));
        }

        if (const stratum_sim::Interacted* pevent = event.try_get<stratum_sim::Interacted>())
        {
            return format("interacted with wall {}", pevent->wall_id);
        }

        return "unrecognized event";
    }

    //--------------------------------------------------------------------------
    string frame(const stratum_sim::World& world, flecs::entity zone, flecs::entity player, const string& label, const stratum_sim::EventQueue& events)
    {
        const stratum_sim::Layout& layout = world.layout(zone);
        const stratum_sim::placement_type& placement = player.get<stratum_sim::placement_type>();
        stratum_sim::GridPosition cell = layout.cell_at(placement.position());

        string s = format("-- {}\n", label);
        s += stratum_view::render(world, zone).to_string();
        s += format("   cell ({}, {})  facing {}\n", cell.i, cell.j, name_of(placement.direction()));

        for (const stratum_sim::Event& event : events.events())
        {
            s += format("   {}\n", describe(event, layout));
        }

        s += "\n";

        return s;
    }

    //--------------------------------------------------------------------------
    // Plays a fixed command sequence, rendering after each, and returns every frame
    string play(const string& filepath, const stratum_sim::GridPosition& start, const string& script)
    {
        stratum_sim::World world;
        flecs::entity zone = world.create_zone("zone");
        stratum_sim::load_layout(filepath, world.layout(zone));

        const stratum_sim::Layout& layout = world.layout(zone);
        flecs::entity player = world.create_entity(zone, stratum_sim::placement_type(layout.cell_position(start), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::EventQueue events;
        string s = frame(world, zone, player, "initial", events);

        for (char c : script)
        {
            events.clear();
            stratum_sim::apply(world, command_of(c, player, layout), events);
            s += frame(world, zone, player, format("command '{}'", c), events);
        }

        return s;
    }

    //--------------------------------------------------------------------------
    void write_file(const string& filepath, const string& s)
    {
        ofstream os(filepath, ios_base::binary);
        os.write(s.data(), static_cast<streamsize>(s.size()));
        return;
    }

    //--------------------------------------------------------------------------
    // Fixed command sequences rendered to text and gated byte for byte against
    // checked-in reference frames, which is what keeps the ASCII view from rotting once
    // it stops being the view anyone looks at
    void test_reference()
    {
        report_group("reference frames");

        struct Scenario
        {
            string name;
            string level;
            stratum_sim::GridPosition start;
            string script;
        };

        const Scenario scenarios[]
        {
            {"corridor_walk", "corridor", {1, 1}, "lllllhh"},
            {"vault_tour", "vault", {1, 1}, "llkk<h >j "},
            {"dungeon_walk", "dungeon", {1, 1}, "llllllkkk< j"}
        };

        // Constructed first, so that it can tell frames written now from ones left behind
        FileValidator validator("out", "reference", "stratum_view_test");

        for (const Scenario& scenario : scenarios)
        {
            write_file(format("out/{}.txt", scenario.name), play(format("data/{}.level", scenario.level), scenario.start, scenario.script));
        }

        validator.report_results();
        report_case("every frame matches its reference", validator.result() <= FileValidator::ResultStatus::Unchanged);

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_console();
    test_difference();
    test_cells();
    test_render();
    test_round_trip();
    test_dungeon();
    test_reference();

    return report_results();
}
