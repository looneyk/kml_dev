//------------------------------------------------------------------------------
// stratum_view_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/read.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/commands.hpp"
#include "quetzal/stratum_sim/load_layout.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include "quetzal/stratum_view/ascii.hpp"
#include "quetzal/stratum_view/cells.hpp"
#include "quetzal/stratum_view/read_glyphs.hpp"
#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <variant>
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
    // The expected value follows what it is compared against, so a literal needs no suffix
    template<typename T>
    void report_case_equal(const string& name, T value, std::type_identity_t<T> value_expected)
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
    // A level's rows, a row per line
    string rows_of(const string& filepath)
    {
        string s;
        for (const string& row : stratum_format::read(filepath).cells.rows)
        {
            s += row + '\n';
        }

        return s;
    }

    //--------------------------------------------------------------------------
    void test_console()
    {
        report_group("console");

        stratum_view::Console console({4, 3}, '.');

        report_case_equal("it has the width asked for", console.extent().x(), 4);
        report_case_equal("and the height", console.extent().y(), 3);
        report_case("every cell starts filled", console.at({0, 0}) == '.' && console.at({3, 2}) == '.');

        console.set({1, 0}, '@');
        report_case("a set cell holds its character", console.at({1, 0}) == '@');
        report_case("and its neighbors are untouched", console.at({0, 0}) == '.' && console.at({2, 0}) == '.');

        // Row 0 is the lowest, so it prints last
        string s = console.to_string();
        report_case_equal("printing gives one line per row", s.size(), 3 * 5);
        report_case("the highest row prints first", s.substr(0, 4) == "....");
        report_case("and row zero last", s.substr(10, 4) == ".@..");

        console.fill('#');
        report_case("fill replaces everything", console.at({1, 0}) == '#');

        return;
    }

    //--------------------------------------------------------------------------
    void test_difference()
    {
        report_group("console difference");

        stratum_view::Console a({6, 3}, '.');
        stratum_view::Console b = a;

        report_case("two identical consoles differ nowhere", stratum_view::difference(a, b).empty());

        b.set({2, 1}, '@');
        stratum_view::spans_type spans = stratum_view::difference(a, b);
        report_case_equal("one changed cell is one span", spans.size(), 1);
        report_case("covering just that cell", !spans.empty() && spans[0] == stratum_view::Span {{2, 1}, 1});

        b.set({3, 1}, '@');
        spans = stratum_view::difference(a, b);
        report_case_equal("two adjacent cells merge into one span", spans.size(), 1);
        report_case("of two", !spans.empty() && spans[0] == stratum_view::Span {{2, 1}, 2});

        b.set({0, 0}, '#');
        spans = stratum_view::difference(a, b);
        report_case_equal("a separate row is a separate span", spans.size(), 2);
        report_case("and rows come lowest first", spans.size() == 2 && spans[0].position.j() == 0 && spans[1].position.j() == 1);

        // A cell that matches again ends the run
        stratum_view::Console c = a;
        c.set({1, 2}, 'x');
        c.set({3, 2}, 'x');
        spans = stratum_view::difference(a, c);
        report_case_equal("a gap splits a run", spans.size(), 2);

        // Differing sizes give the whole of b, as the cue for a device to repaint
        spans = stratum_view::difference(stratum_view::Console({2, 2}, '.'), a);
        report_case_equal("a size change reports every row", spans.size(), 3);
        report_case("each row entire", !spans.empty() && spans[0] == stratum_view::Span {{0, 0}, 6});

        report_case("difference is oriented, reporting b's content", stratum_view::difference(b, a).size() == stratum_view::difference(a, b).size());

        return;
    }

    //--------------------------------------------------------------------------
    flecs::entity load_zone(stratum_sim::World& world, const string& name, const string& filepath)
    {
        stratum_sim::door_walls_type doors;

        flecs::entity zone = world.create_zone(name, stratum_sim::load_layout(filepath,
            [&](const std::vector<id_type>& walls) { doors.push_back(walls); }));

        stratum_sim::create_doors(world, zone, doors);

        return zone;
    }

    //--------------------------------------------------------------------------
    // The reduction both views consume, before either decides how to show it
    void test_cells()
    {
        report_group("cells");

        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "crypt", "data/corridor.level");

        stratum_view::Cells cells = stratum_view::cells(world, zone);

        report_case_equal("it covers the zone's grid", cells.extent().x(), 7);
        report_case_equal("in both directions", cells.extent().y(), 3);
        report_case("a closed cell is solid", cells.at({0, 1}).type == stratum_view::PlaceType::Solid);
        report_case("floor where it says open", cells.at({1, 1}).type == stratum_view::PlaceType::Floor);
        report_case("no actor before one exists", !cells.at({3, 1}).actor);

        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 1}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        cells = stratum_view::cells(world, zone);
        report_case("an entity stands in its cell", cells.at({3, 1}).actor);
        report_case("and only that one", !cells.at({2, 1}).actor);

        // The whole point of the actor being a layer: the floor it stands on is still there
        report_case("the floor under it is kept", cells.at({3, 1}).type == stratum_view::PlaceType::Floor);

        // A view's mapping is its own; the terminal's is a table of Glyph
        const stratum_view::Glyphs& glyphs = stratum_view::cell_glyphs();
        report_case("glyphs map each place type", glyphs(stratum_view::Place {stratum_view::PlaceType::Solid}) == '#'
            && glyphs(stratum_view::Place {stratum_view::PlaceType::Floor}) == '.');

        report_case("and an actor over whatever it stands on",
            glyphs(stratum_view::Place {stratum_view::PlaceType::Floor, stratum_view::State::None,
                stratum_view::DirectionMask::None, true}) == '@');

        // A line asking for an actor beats one asking for a state, so an actor in an open
        // doorway draws as the actor
        report_case("an actor in an open doorway draws as the actor",
            glyphs(stratum_view::Place {stratum_view::PlaceType::Door, stratum_view::State::Open,
                stratum_view::DirectionMask::None, true}) == '@');

        // A set's expressive range is the set's own, so a line specifying a state beats one that does not
        const stratum_view::Glyphs& lattice_set = stratum_view::lattice_glyphs();
        report_case("a state named draws apart from one not named",
            lattice_set(stratum_view::Place {stratum_view::PlaceType::Door}) == '/'
            && lattice_set(stratum_view::Place {stratum_view::PlaceType::Door, stratum_view::State::Open}) == '\\');
        report_case("and a set saying nothing of a state draws both alike",
            stratum_view::Glyphs(std::vector<stratum_view::Glyph> {{stratum_view::PlaceType::Door, stratum_view::State::None, {}, false, '+'}})
                (stratum_view::Place {stratum_view::PlaceType::Door, stratum_view::State::Open}) == '+');

        report_case("rendering from cells matches rendering from the world",
            stratum_view::render(cells).to_string() == stratum_view::render(world, zone).to_string());

        // difference works over any buffer, which is what having one implementation buys
        stratum_view::Cells other = cells;
        other.set({5, 1}, stratum_view::Place {stratum_view::PlaceType::Solid});
        report_case_equal("differencing works over places too", stratum_view::difference(cells, other).size(), 1);

        return;
    }

    //--------------------------------------------------------------------------
    // A glyph set as a file, and the built-in sets as the contents of the two shipped
    void test_glyph_files()
    {
        report_group("glyph files");

        auto same = [](const stratum_view::Glyphs& a, const stratum_view::Glyphs& b)
        {
            if (a.glyphs().size() != b.glyphs().size())
            {
                return false;
            }

            for (size_t n = 0; n < a.glyphs().size(); ++n)
            {
                const stratum_view::Glyph& x = a.glyphs()[n];
                const stratum_view::Glyph& y = b.glyphs()[n];

                if (x.type != y.type || x.state != y.state || x.mask != y.mask || x.c != y.c)
                {
                    return false;
                }
            }

            return true;
        };

        string root = string(KML_DEV_ASSETS_ROOT) + "/glyphs/";

        report_case("the shipped cell set is the built-in one",
            same(stratum_view::read_glyphs(root + "ascii_cells.glyphs"), stratum_view::cell_glyphs()));
        report_case("and the shipped lattice set is the built-in one",
            same(stratum_view::read_glyphs(root + "ascii_lattice.glyphs"), stratum_view::lattice_glyphs()));

        auto read = [](const string& text)
        {
            istringstream is(text);
            return stratum_view::read_glyphs(is);
        };

        stratum_view::Glyphs set = read("glyphs 0.1\ndoor open '\\'\nwall mask=EW '-'\nfloor ' '\n");
        report_case_equal("a set holds a line per row that names one", set.glyphs().size(), 3);
        report_case("an escaped backslash is the character it names",
            set(stratum_view::Place {stratum_view::PlaceType::Door, stratum_view::State::Open}) == '\\');
        report_case("a quoted space is a glyph like any other",
            set(stratum_view::Place {stratum_view::PlaceType::Floor}) == ' ');
        report_case("a mask is asked for only where it is named",
            set(stratum_view::Place {stratum_view::PlaceType::Wall, stratum_view::State::None,
                stratum_view::DirectionMask::East | stratum_view::DirectionMask::West}) == '-'
            && set(stratum_view::Place {stratum_view::PlaceType::Wall}) == ' ');

        auto throws = [&](const string& text)
        {
            try
            {
                read(text);
            }
            catch (const std::runtime_error&)
            {
                return true;
            }

            return false;
        };

        report_case("a file naming no format is rejected", throws("door '/'\n"));
        report_case("as is one naming another version", throws("glyphs 9.9\n"));
        report_case("a type that is not one is rejected", throws("glyphs 0.1\nchasm '/'\n"));
        report_case("as is a state that is not one", throws("glyphs 0.1\ndoor ajar '/'\n"));
        report_case("a state its type cannot be in is rejected",
            throws("glyphs 0.1\nwall open '|'\n"));
        report_case("as is a line with nothing to draw", throws("glyphs 0.1\ndoor open\n"));

        return;
    }

    //--------------------------------------------------------------------------
    // The reduction the wall-aware view consumes: the cells and what lies between them
    void test_lattice()
    {
        report_group("lattice");

        // Two rooms of two by three cells sharing one wall, with a door in the middle of it
        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "rooms", "data/walls.level");

        stratum_view::Lattice lattice = stratum_view::lattice(world, zone);

        report_case_equal("it covers twice the grid plus one", lattice.extent().x(), 9);
        report_case_equal("in both directions", lattice.extent().y(), 7);

        report_case("a cell sits at an odd position both ways", lattice.at({1, 1}).type == stratum_view::PlaceType::Floor);
        report_case("the wall two rooms share lies between them", lattice.at({4, 1}).type == stratum_view::PlaceType::Wall);
        report_case("and the door in it is a door", lattice.at({4, 5}).type == stratum_view::PlaceType::Door);
        report_case("shut, which is a state rather than a type of its own",
            !stratum_view::any(lattice.at({4, 5}).state & stratum_view::State::Open));
        report_case("a state a wall cannot be in is rejected",
            !stratum_view::applies(stratum_view::PlaceType::Wall, stratum_view::State::Open)
            && stratum_view::applies(stratum_view::PlaceType::Door, stratum_view::State::Open));
        report_case("an edge within one room holds nothing", lattice.at({2, 1}).type == stratum_view::PlaceType::None);
        report_case("the boundary is walled", lattice.at({0, 1}).type == stratum_view::PlaceType::Wall
            && lattice.at({8, 1}).type == stratum_view::PlaceType::Wall);

        report_case("a corner walls reach is a joint", lattice.at({0, 0}).type == stratum_view::PlaceType::Wall);
        report_case("so is one a run passes through", lattice.at({2, 0}).type == stratum_view::PlaceType::Wall);

        // Every corner inside these rooms meets a stair's walls, so the corner with nothing at it is one in the vault
        stratum_sim::World open_world;
        flecs::entity vault = load_zone(open_world, "vault", "data/vault.level");
        report_case("a corner inside a room is not", stratum_view::lattice(open_world, vault).at({10, 4}).type == stratum_view::PlaceType::None);

        // The block format reduces onto the same lattice, its walls existing only against solid
        stratum_sim::World block;
        flecs::entity corridor = load_zone(block, "corridor", "data/corridor.level");

        stratum_view::Lattice cells = stratum_view::lattice(block, corridor);

        report_case_equal("a block level covers the same shape", cells.extent().x(), 15);
        report_case("a closed cell reads as solid", cells.at({1, 1}).type == stratum_view::PlaceType::Solid);
        report_case("an open cell reads as floor", cells.at({3, 3}).type == stratum_view::PlaceType::Floor);
        report_case("open meeting solid is walled", cells.at({2, 3}).type == stratum_view::PlaceType::Wall);

        // A wall exists only on the side facing open space, so an edge has to be asked of
        // both cells: this one has its wall on the west side, the other on the east
        report_case("and so is solid meeting open", cells.at({12, 3}).type == stratum_view::PlaceType::Wall);
        report_case("as is the floor's own boundary", cells.at({3, 2}).type == stratum_view::PlaceType::Wall
            && cells.at({3, 4}).type == stratum_view::PlaceType::Wall);

        report_case("two open cells of one area are not", cells.at({4, 3}).type == stratum_view::PlaceType::None);

        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(block.layout(corridor));
        block.create_entity(corridor, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        cells = stratum_view::lattice(block, corridor);
        report_case("an entity stands in its cell", cells.at({3, 3}).actor);
        report_case("with the floor under it kept", cells.at({3, 3}).type == stratum_view::PlaceType::Floor);

        return;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_view_test runs from its own directory
    void test_render()
    {
        report_group("ascii render");

        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "crypt", "data/corridor.level");

        stratum_view::Console console = stratum_view::render(world, zone);

        report_case_equal("the console covers the zone's grid", console.extent().x(), 7);
        report_case_equal("in both directions", console.extent().y(), 3);

        report_case("a solid cell renders as solid", console.at({0, 1}) == '#');
        report_case("open floor renders as floor", console.at({1, 1}) == '.');
        report_case("the far end is floor too", console.at({5, 1}) == '.');
        report_case("beyond it is solid", console.at({6, 1}) == '#');

        // An actor draws over the floor it stands on
        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {3, 1}), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        console = stratum_view::render(world, zone);
        report_case("an actor draws over its floor", console.at({3, 1}) == '@');
        report_case("and only its own cell", console.at({2, 1}) == '.');

        return;
    }

    //--------------------------------------------------------------------------
    // The wall-aware mapping, on a block format level, where the file is not the lattice
    void test_render_lattice()
    {
        report_group("wall render");

        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "corridor", "data/corridor.level");

        stratum_view::Console console = stratum_view::render_lattice(world, zone);

        report_case_equal("the console covers the lattice", console.extent().x(), 15);
        report_case_equal("in both directions", console.extent().y(), 7);

        report_case("an open cell is a space, not a dot", console.at({3, 3}) == ' ');
        report_case("a solid cell is solid", console.at({1, 1}) == '#');
        report_case("open meeting solid is a vertical wall", console.at({2, 3}) == '|');
        report_case("open over solid is a horizontal one", console.at({3, 2}) == '-');
        report_case("two open cells of one area have nothing between them", console.at({4, 3}) == ' ');

        report_case("a corner with solid all around it is solid", console.at({0, 0}) == '#');
        report_case("a corner two walls turn at is a junction", console.at({2, 2}) == '+');
        report_case("and one a run passes through is the run", console.at({4, 2}) == '-');

        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        world.create_entity(zone, stratum_sim::placement_type(layout.position(stratum_sim::discrete_point_type {1, 1}),
            stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        console = stratum_view::render_lattice(world, zone);
        report_case("an actor draws over the cell it stands in", console.at({3, 3}) == '@');

        return;
    }

    //--------------------------------------------------------------------------
    // An unoccupied level of nothing but solid, floor and its entrance renders back to the file it
    // was loaded from. vault.level is asymmetric top to bottom, so this pins row order too
    void test_round_trip()
    {
        report_group("round trip");

        for (const string& name : {"corridor"s, "vault"s})
        {
            string filepath = format("data/{}.level", name);

            stratum_sim::World world;
            flecs::entity zone = load_zone(world, name, filepath);

            string s = stratum_view::render(world, zone).to_string();
            string expected = rows_of(filepath);

            // The entrance is a block, so the view has only solid to draw there
            for (char& c : expected)
            {
                c = c == '<' || c == '>' ? '#' : c;
            }

            report_case(format("{} renders back to its own rows", name), s == expected, "\n" + s);
        }

        // The wall notation is the lattice written out, so the same check applies to it once the view's glyphs are
        // taken back to the notation's: floor, walls, doors and solid all have to agree with the generator that wrote the
        // file, without either depending on the other. The notation leaves corners blank, so they are not compared
        {
            string filepath = "data/walls.level";

            stratum_sim::World world;
            flecs::entity zone = load_zone(world, "walls", filepath);

            string s = stratum_view::render_lattice(world, zone).to_string();
            string expected = rows_of(filepath);

            // Stairs are blocks, so the view has only solid to draw there, which the notation leaves blank
            for (char& c : expected)
            {
                c = c == '<' || c == '>' ? ' ' : c;
            }

            size_t width = expected.find('\n');
            for (size_t n = 0; n < s.size(); ++n)
            {
                size_t x = n % (width + 1);
                size_t y = n / (width + 1);

                if (s[n] == '\n')
                {
                    continue;
                }

                if (x % 2 == 0 && y % 2 == 0)
                {
                    s[n] = ' ';
                }
                else if (x % 2 == 1 && y % 2 == 1)
                {
                    s[n] = s[n] == '#' ? ' ' : '.';
                }
                else
                {
                    s[n] = s[n] == '#' ? ' ' : s[n] == '/' ? '+' : s[n];
                }
            }

            report_case("a wall notation level renders back to its own rows", s == expected, "\n" + s);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A generated dungeon, which has doorways and stairs as well as rooms and corridors. A stair is a block, so it renders
    // as solid, and a doorway is a block with a door on it, so it renders as the door rather than as the floor it also is
    void test_dungeon()
    {
        report_group("generated dungeon");

        string source = rows_of("data/dungeon.level");

        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "dungeon", "data/dungeon.level");

        stratum_view::Console console = stratum_view::render(world, zone);

        const stratum_view::vector_type& extent = console.extent();

        report_case_equal("the console is as wide as the file", extent.x(), 31);
        report_case_equal("and as tall", extent.y(), 31);

        size_t solid_count = 0;
        size_t floor_count = 0;
        size_t mismatch_count = 0;

        for (stratum_view::coord_type j = 0; j < extent.y(); ++j)
        {
            for (stratum_view::coord_type i = 0; i < extent.x(); ++i)
            {
                // File line 0 is the highest row
                char source_character = source[static_cast<size_t>((extent.y() - 1 - j) * (extent.x() + 1) + i)];
                bool solid = source_character == '#' || source_character == '<' || source_character == '>';

                solid ? ++solid_count : ++floor_count;

                char expected = solid ? '#' : source_character == '+' ? '/' : '.';

                if (console.at({i, j}) != expected)
                {
                    ++mismatch_count;
                }
            }
        }

        report_case_equal("every cell agrees with the file", mismatch_count, 0);
        report_case("it found solid cells", solid_count > 0);
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
    stratum_sim::Command command_of(char c, flecs::entity player, const stratum_sim::DiscreteLayout& layout)
    {
        id_type entity_id = player.id();

        auto step = [entity_id](stratum_sim::value_type dx, stratum_sim::value_type dy)
        {
            return stratum_sim::Command(entity_id, stratum_sim::Step {stratum_sim::vector_type(dx, dy, 0.0f), 1.0f});
        };

        // A relative turn is resolved against the layout here, the command itself sending
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
    string describe(const stratum_sim::Event& event, const stratum_sim::DiscreteLayout& layout)
    {
        if (const stratum_sim::Moved* pevent = event.try_get<stratum_sim::Moved>())
        {
            stratum_sim::discrete_point_type cell_position = layout.cell_position_at(pevent->placement.position());
            return format("moved to ({}, {})", cell_position.i(), cell_position.j());
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
            return pevent->interaction == stratum_sim::Interaction::Opened ? format("opened wall {}", pevent->wall_id)
                : pevent->interaction == stratum_sim::Interaction::Closed ? format("shut wall {}", pevent->wall_id)
                : format("interacted with wall {}", pevent->wall_id);
        }

        return "unrecognized event";
    }

    //--------------------------------------------------------------------------
    // Which view a scenario's frames are drawn with. The two are checked separately: one shows
    // one glyph per cell, the other what lies on the edges between them
    enum class View
    {
        Cells,
        Walls
    };

    //--------------------------------------------------------------------------
    string frame(const stratum_sim::World& world, flecs::entity zone, flecs::entity player, const string& label,
        const stratum_sim::EventQueue& events, View view)
    {
        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        const stratum_sim::placement_type& placement = player.get<stratum_sim::placement_type>();
        stratum_sim::discrete_point_type cell_position = layout.cell_position_at(placement.position());

        string s = format("-- {}\n", label);
        s += view == View::Walls ? stratum_view::render_lattice(world, zone).to_string()
            : stratum_view::render(world, zone).to_string();
        s += format("   cell ({}, {})  facing {}\n", cell_position.i(), cell_position.j(), name_of(placement.direction()));

        for (const stratum_sim::Event& event : events.events())
        {
            s += format("   {}\n", describe(event, layout));
        }

        s += "\n";

        return s;
    }

    //--------------------------------------------------------------------------
    // Plays a fixed command sequence, rendering after each, and returns every frame
    string play(const string& filepath, const stratum_sim::discrete_point_type& start, const string& script, View view)
    {
        stratum_sim::World world;
        flecs::entity zone = load_zone(world, "zone", filepath);

        const stratum_sim::DiscreteLayout& layout = stratum_sim::layout_as<stratum_sim::DiscreteLayout>(world.layout(zone));
        flecs::entity player = world.create_entity(zone, stratum_sim::placement_type(layout.position(start), stratum_sim::vector_type(1.0f, 0.0f, 0.0f)));

        stratum_sim::EventQueue events;
        string s = frame(world, zone, player, "initial", events, view);

        for (char c : script)
        {
            events.clear();
            stratum_sim::apply(world, command_of(c, player, layout), events);
            s += frame(world, zone, player, format("command '{}'", c), events, view);
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
    // Fixed command sequences rendered to text and compared byte for byte against
    // checked-in reference frames, which is what keeps the ASCII view from rotting once
    // it stops being the view anyone looks at
    void test_reference()
    {
        report_group("reference frames");

        struct Scenario
        {
            string name;
            string level;
            stratum_sim::discrete_point_type start;
            string script;
            View view;
        };

        // The last two are the wall view, which is the only one of these that shows what a
        // mover is stopped by: the others are block levels through a cell per glyph, which
        // loses nothing where a wall exists only against solid
        // The two differ in where the door is. A wall format door is an edge between two
        // cells and is drawn on it; a block format door is a cell, and is drawn in it
        const Scenario scenarios[]
        {
            {"corridor_walk", "corridor", {1, 1}, "lllllhh", View::Cells},
            {"vault_tour", "vault", {1, 1}, "llkk<h >j ", View::Cells},
            {"dungeon_walk", "dungeon", {1, 1}, "llllll< kkk< j", View::Cells},
            {"walls_door", "walls", {0, 2}, "ll l lh<<h ", View::Walls},
            {"door_cell", "door", {1, 1}, " l l ", View::Walls}
        };

        // Constructed first, so that it can tell frames written now from ones left behind
        FileValidator validator("out", "reference", "stratum_view_test");

        for (const Scenario& scenario : scenarios)
        {
            write_file(format("out/{}.txt", scenario.name),
                play(format("data/{}.level", scenario.level), scenario.start, scenario.script, scenario.view));
        }

        validator.report_results();
        report_case("every frame matches its reference", validator.result() <= FileValidator::ResultStatus::Unchanged);

        return;
    }

    //--------------------------------------------------------------------------
    // One door entity attached at the walls of each door glyph, and none elsewhere
    bool doors_have_entities(const stratum_format::Document& document, string& detail)
    {
        vector<vector<stratum_format::WallAddress>> doors = stratum_format::door_walls(document.cells);
        vector<bool> matched(doors.size(), false);

        for (const stratum_format::Attachment& attachment : document.attachments)
        {
            const auto* walls = get_if<vector<stratum_format::WallAddress>>(&attachment.attached_to);
            bool door = any_of(attachment.entities.begin(), attachment.entities.end(),
                [](const auto& entity) { return entity.is_object() && entity.value("type", "") == "door"; });

            if (walls == nullptr || !door)
            {
                continue;
            }

            auto i = find(doors.begin(), doors.end(), *walls);
            if (i == doors.end() || matched[static_cast<size_t>(i - doors.begin())])
            {
                const stratum_format::WallAddress& wall = walls->front();
                detail = format("a door entity at wall {}, {}, {}, {} that is {}", wall.cell_position.i(), wall.cell_position.j(), wall.normal.x(),
                    wall.normal.y(), i == doors.end() ? "not a door's" : "a door's second");
                return false;
            }

            matched[static_cast<size_t>(i - doors.begin())] = true;
        }

        size_t missing_count = static_cast<size_t>(count(matched.begin(), matched.end(), false));
        if (missing_count > 0)
        {
            detail = format("{} of {} doors have no door entity", missing_count, doors.size());
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------------------
    // Reads from data/, so stratum_view_test runs from its own directory
    void test_door_entities()
    {
        report_group("door entities");

        for (const string name : {"corridor", "door", "dungeon", "vault", "walls"})
        {
            string detail;
            report_case(format("{} has a door entity at each door", name), doors_have_entities(stratum_format::read(format("data/{}.level", name)), detail),
                detail);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_console();
    test_difference();
    test_cells();
    test_glyph_files();
    test_lattice();
    test_render();
    test_render_lattice();
    test_round_trip();
    test_dungeon();
    test_reference();
    test_door_entities();

    return report_results();
}
