//------------------------------------------------------------------------------
// stratum_format_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_format/read.hpp"
#include "quetzal/stratum_format/write.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace std;
using namespace quetzal;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    const filesystem::path data_directory = "data";
    const filesystem::path out_directory = "out";

    // Key order plays no part in its comparison, which is what lets two documents compare as equivalent
    using map_type = nlohmann::json;

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

    //--------------------------------------------------------------------------
    stratum_format::values_type parse_values(const string& text)
    {
        return stratum_format::values_type::parse(text);
    }

    //--------------------------------------------------------------------------
    stratum_format::values_type load_values(const filesystem::path& filepath)
    {
        ifstream is(filepath, ios::binary);
        return stratum_format::values_type::parse(is);
    }

    //--------------------------------------------------------------------------
    // Reports whether the file reads, so that a file failing to read fails one case rather than the run
    optional<stratum_format::Document> read_case(const string& name, const filesystem::path& filepath)
    {
        try
        {
            stratum_format::Document document = stratum_format::read(filepath);
            report_case(name, true);
            return document;
        }
        catch (const runtime_error& e)
        {
            report_case(name, false, e.what());
            return nullopt;
        }
    }

    //--------------------------------------------------------------------------
    map_type content_map(const stratum_format::values_type& values)
    {
        return map_type::parse(values.dump());
    }

    //--------------------------------------------------------------------------
    map_type cell_position_map(const stratum_format::discrete_point_type& cell_position)
    {
        return map_type::array({cell_position.i(), cell_position.j()});
    }

    //--------------------------------------------------------------------------
    map_type address_map(const stratum_format::WallAddress& address)
    {
        return map_type::array({address.cell_position.i(), address.cell_position.j(), address.normal.x(), address.normal.y()});
    }

    //--------------------------------------------------------------------------
    map_type barriers_map(const vector<stratum_format::Barrier>& barriers)
    {
        map_type m = map_type::array();

        for (const auto& barrier : barriers)
        {
            if (barrier.normal.has_value())
            {
                m.push_back(address_map({barrier.cell_position, *barrier.normal}));
            }
            else
            {
                m.push_back(cell_position_map(barrier.cell_position));
            }
        }

        return m;
    }

    //--------------------------------------------------------------------------
    map_type components_map(const stratum_format::values_type& components)
    {
        return {{"components", content_map(components)}};
    }

    //--------------------------------------------------------------------------
    map_type room_map(const stratum_format::Room& room)
    {
        map_type m = components_map(room.components);

        if (const auto* perimeter = get_if<stratum_format::Perimeter>(&room.identification); perimeter != nullptr)
        {
            map_type junctions = map_type::array();
            for (const auto& junction : perimeter->junctions)
            {
                junctions.push_back(cell_position_map(junction));
            }

            m["perimeter"] = junctions;
        }
        else
        {
            const auto& locator = get<stratum_format::RoomLocator>(room.identification);
            m["location"] = cell_position_map(locator.cell_position);
            m["barriers"] = barriers_map(locator.barriers);
        }

        return m;
    }

    //--------------------------------------------------------------------------
    map_type flat_properties_map(const stratum_format::FlatProperties& properties)
    {
        map_type m = components_map(properties.components);
        m["height"] = properties.height.has_value() ? map_type(*properties.height) : map_type();
        return m;
    }

    //--------------------------------------------------------------------------
    map_type flat_map(const stratum_format::Flat& flat)
    {
        map_type m = flat_properties_map(flat.properties);

        m["rooms"] = map_type::object();
        for (const auto& [room_id, properties] : flat.rooms)
        {
            m["rooms"][to_string(room_id)] = flat_properties_map(properties);
        }

        m["cells"] = map_type::object();
        for (const auto& [cell_position, properties] : flat.cells)
        {
            m["cells"][cell_position_map(cell_position).dump()] = flat_properties_map(properties);
        }

        return m;
    }

    //--------------------------------------------------------------------------
    map_type attachment_map(const stratum_format::Attachment& attachment)
    {
        map_type m;

        if (const auto* walls = get_if<vector<stratum_format::WallAddress>>(&attachment.attached_to); walls != nullptr)
        {
            m["walls"] = map_type::array();
            for (const auto& address : *walls)
            {
                m["walls"].push_back(address_map(address));
            }
        }
        else
        {
            const auto& flat_cell = get<stratum_format::FlatCell>(attachment.attached_to);
            m["flat"] = string(stratum_format::name(flat_cell.role));
            m["cell"] = cell_position_map(flat_cell.cell_position);
        }

        m["position"] = attachment.position.has_value() ? map_type::array({attachment.position->x(), attachment.position->y()}) : map_type();
        m["entities"] = content_map(attachment.entities);
        return m;
    }

    //--------------------------------------------------------------------------
    // The document as nested maps: walls keyed by address and flats by role, since neither has an order of its own
    map_type document_map(const stratum_format::Document& document)
    {
        const stratum_format::Cells& cells = document.cells;

        map_type m;
        m["version"] = document.version;
        m["generator"] = content_map(document.generator);
        m["layout"] = components_map(document.components);

        m["cells"] =
        {
            {"notation", string(stratum_format::name(cells.notation))},
            {"encoding", string(stratum_format::name(cells.encoding))},
            {"extent", map_type::array({cells.extent.x(), cells.extent.y()})},
            {"origin", map_type::array({cells.origin[0], cells.origin[1], cells.origin[2]})},
            {"scale", cells.scale},
            {"rows", cells.rows}
        };

        m["barriers"] = barriers_map(document.barriers);

        m["rooms"] = map_type::array();
        for (const auto& room : document.rooms)
        {
            m["rooms"].push_back(room_map(room));
        }

        m["walls"] = map_type::object();
        for (const auto& wall : document.walls)
        {
            m["walls"][address_map(wall.address).dump()] = components_map(wall.components);
        }

        m["flats"] = map_type::object();
        for (const auto& flat : document.flats)
        {
            m["flats"][string(stratum_format::name(flat.role))] = flat_map(flat);
        }

        m["attachments"] = map_type::array();
        for (const auto& attachment : document.attachments)
        {
            m["attachments"].push_back(attachment_map(attachment));
        }

        return m;
    }

    //--------------------------------------------------------------------------
    // A file's text with one change made to it, and what reading it reports
    struct MutationCase
    {
        string name;
        function<void(stratum_format::values_type&)> mutate;
        string message; // A fragment of the error, or empty where the changed file is valid
    };

    //--------------------------------------------------------------------------
    void report_mutations(const filesystem::path& filepath, const vector<MutationCase>& mutation_cases)
    {
        stratum_format::values_type base = load_values(filepath);

        for (const auto& mutation_case : mutation_cases)
        {
            stratum_format::values_type values = base;
            mutation_case.mutate(values);

            istringstream is(values.dump());

            try
            {
                stratum_format::read(is, mutation_case.name);
                report_case(mutation_case.name, mutation_case.message.empty(), "accepted");
            }
            catch (const runtime_error& e)
            {
                string what = e.what();
                bool expected = !mutation_case.message.empty() && what.find(mutation_case.message) != string::npos;
                report_case(mutation_case.name, expected,
                    mutation_case.message.empty() ? what : format("expected \"{}\", got \"{}\"", mutation_case.message, what));
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_read_wall_notation()
    {
        report_group("read, wall notation");

        optional<stratum_format::Document> read = read_case("cistern.json reads", data_directory / "cistern.json");
        if (!read.has_value())
        {
            return;
        }

        const stratum_format::Document& document = *read;

        report_case("the notation is wall", document.cells.notation == stratum_format::Notation::Wall);
        report_case("the encoding defaults to characters", document.cells.encoding == stratum_format::Encoding::Characters);
        report_case("the extent is 4 by 3", document.cells.extent.x() == 4 && document.cells.extent.y() == 3);
        report_case_equal("the rows number 2 * 3 + 1", document.cells.rows.size(), 7);
        report_case("the first row written is the highest", document.cells.rows[1] == "|. .| |>|");

        report_case_equal("one barrier", document.barriers.size(), 1);
        report_case("the barrier is an edge", document.barriers[0].normal == stratum_format::discrete_vector_type {1, 0});

        report_case_equal("two rooms", document.rooms.size(), 2);
        const auto* perimeter = get_if<stratum_format::Perimeter>(&document.rooms[0].identification);
        report_case("the first room is identified by its perimeter", perimeter != nullptr && perimeter->junctions.size() == 4);
        const auto* locator = get_if<stratum_format::RoomLocator>(&document.rooms[1].identification);
        report_case("the second room is identified by its location",
            locator != nullptr && locator->cell_position == stratum_format::discrete_point_type {3, 1});
        report_case("a style is a component kept as written", document.rooms[0].components["style"]["terrain"] == "water");

        report_case("the wall address's normal points into its cell",
            document.walls[0].address == stratum_format::WallAddress {{1, 1}, {-1, 0}});

        report_case_equal("two flats", document.flats.size(), 2);
        report_case("the first flat is the floor", document.flats[0].role == stratum_format::FlatRole::Floor);
        report_case("the floor's room entry has its height",
            document.flats[0].rooms.contains(0) && document.flats[0].rooms.at(0).height == -0.25f);
        report_case("the ceiling's height", document.flats[1].properties.height == 3.0f);

        const auto* walls = get_if<vector<stratum_format::WallAddress>>(&document.attachments[0].attached_to);
        report_case("a door is attached to two walls", walls != nullptr && walls->size() == 2);
        const auto* flat_cell = get_if<stratum_format::FlatCell>(&document.attachments[1].attached_to);
        report_case("a staircase is attached to a floor cell", flat_cell != nullptr && flat_cell->role == stratum_format::FlatRole::Floor
            && flat_cell->cell_position == stratum_format::discrete_point_type {3, 2});
        report_case("entities are kept as written", document.attachments[1].entities[0]["components"]["relocator"]["target"] == "level_down");

        report_case("the generator keeps its parameters in the order written",
            document.generator["parameters"].begin().key() == "extent" && next(document.generator["parameters"].begin()).key() == "scale");
        report_case("layout components are kept as written", document.components["difficulty"] == 3 && document.components["style"] == "cistern");

        return;
    }

    //--------------------------------------------------------------------------
    void test_read_block_notation()
    {
        report_group("read, block notation");

        optional<stratum_format::Document> read = read_case("block.json reads", data_directory / "block.json");
        if (!read.has_value())
        {
            return;
        }

        const stratum_format::Document& document = *read;

        report_case("the notation is block", document.cells.notation == stratum_format::Notation::Block);
        report_case_equal("the rows number the extent", document.cells.rows.size(), 5);
        report_case("the scale", document.cells.scale == 2.0f);

        const auto* locator = get_if<stratum_format::RoomLocator>(&document.rooms[0].identification);
        report_case("a location room has its own barrier", locator != nullptr && locator->barriers.size() == 1
            && !locator->barriers[0].normal.has_value());
        report_case("a style naming an asset and overriding part of it is kept as written",
            document.rooms[1].components["style"]["name"] == "hall" && document.rooms[1].components["style"]["lit"] == true);

        const auto& attachment = document.attachments[0];
        report_case("an attachment's position", attachment.position.has_value() && attachment.position->x() == 0.5f && attachment.position->y() == 0.0f);
        report_case("a key the format does not define inside an entity is kept", document.attachments[1].entities[0]["custom"]["anything"][2] == 3);

        report_case("the floor's cell entry has its height",
            document.flats[0].cells.contains({5, 2}) && document.flats[0].cells.at({5, 2}).height == -0.5f);

        return;
    }

    //--------------------------------------------------------------------------
    void test_read_runs()
    {
        report_group("read, runs");

        optional<stratum_format::Document> runs = read_case("runs.json reads", data_directory / "runs.json");
        optional<stratum_format::Document> characters = read_case("cistern.json reads", data_directory / "cistern.json");
        if (!runs.has_value() || !characters.has_value())
        {
            return;
        }

        report_case("the encoding is runs", runs->cells.encoding == stratum_format::Encoding::Runs);
        report_case("rows in runs read as the same rows in characters", runs->cells.rows == characters->cells.rows);

        return;
    }

    //--------------------------------------------------------------------------
    void test_read_pillar()
    {
        report_group("read, a pillar on a room's edge");

        read_case("pillar.json reads, the pillar inside the first room's perimeter", data_directory / "pillar.json");

        return;
    }

    //--------------------------------------------------------------------------
    void test_read_failures()
    {
        report_group("read, files that cannot be read");

        try
        {
            stratum_format::read(data_directory / "absent.json");
            report_case("an absent file", false, "accepted");
        }
        catch (const runtime_error& e)
        {
            report_case("an absent file", string(e.what()).find("cannot be opened") != string::npos, e.what());
        }

        try
        {
            istringstream is(R"({ "format": )");
            stratum_format::read(is, "truncated");
            report_case("text that is not valid JSON", false, "accepted");
        }
        catch (const runtime_error& e)
        {
            report_case("text that is not valid JSON", string(e.what()).find("parse_error") != string::npos, e.what());
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_structure()
    {
        report_group("structure");

        report_mutations(data_directory / "cistern.json",
        {
            {"a format other than this", [](auto& v) { v["format"] = "other"; }, "format: 'other' is not this format"},
            {"a version this does not read", [](auto& v) { v["version"] = "0.2"; }, "version: '0.2' is not a version this reads"},
            {"no cells", [](auto& v) { v.erase("cells"); }, "cells: required and absent"},
            {"a notation the format does not define", [](auto& v) { v["cells"]["notation"] = "hex"; }, "cells.notation: not a notation the format defines"},
            {"an encoding the format does not define", [](auto& v) { v["cells"]["encoding"] = "zip"; }, "cells.encoding: not an encoding the format defines"},
            {"an extent of no cells", [](auto& v) { v["cells"]["extent"] = parse_values("[0, 3]"); }, "cells.extent: an extent of no cells"},
            {"a coordinate with a fraction", [](auto& v) { v["cells"]["extent"] = parse_values("[4.5, 3]"); }, "cells.extent[0]: a whole number was expected"},
            {"a scale of zero", [](auto& v) { v["cells"]["scale"] = 0.0; }, "cells.scale: a scale must be positive"},
            {"a row too few", [](auto& v) { v["cells"]["rows"].erase(0); }, "cells.rows: 7 rows were expected, and there are 6"},
            {"a row too narrow", [](auto& v) { v["cells"]["rows"][1] = "|. .| |>"; }, "cells.rows[1]: a row of 9 is expected, and this one is 8"},
            {"a glyph at a position that does not take it", [](auto& v) { v["cells"]["rows"][0] = "#- -   - "; },
                "cells.rows[0]: '#' at 0 is not a glyph that position accepts"},
            {"a run count of zero", [](auto& v) { v["cells"]["encoding"] = "runs"; v["cells"]["rows"][2] = "0 "; }, "cells.rows[2]: a run count of zero"},
            {"a run count with no glyph after it", [](auto& v) { v["cells"]["encoding"] = "runs"; v["cells"]["rows"][2] = "5 -3"; },
                "cells.rows[2]: a run count with no glyph after it"},
            {"a run count past the row width", [](auto& v) { v["cells"]["encoding"] = "runs"; v["cells"]["rows"][2] = "99 "; },
                "cells.rows[2]: a run count past the row width of 9"},
            {"a room with both perimeter and location", [](auto& v) { v["rooms"][0]["location"] = parse_values("[0, 0]"); },
                "rooms[0]: one of perimeter and location is given, and never both"},
            {"a room with neither perimeter nor location", [](auto& v) { v["rooms"][0].erase("perimeter"); },
                "rooms[0]: one of perimeter and location is given, and never both"},
            {"a barrier of three numbers", [](auto& v) { v["barriers"] = parse_values("[[3, 1, 1]]"); },
                "barriers[0]: 2 numbers for a cell or 4 for one of its faces were expected, and there are 3"},
            {"a normal that is not a cell face's", [](auto& v) { v["barriers"] = parse_values("[[3, 1, 1, 1]]"); }, "barriers[0]: not the normal of a cell face"},
            {"an attachment on both walls and a flat cell", [](auto& v) { v["attachments"][1]["walls"] = parse_values("[[1, 1, -1, 0]]"); },
                "attachments[1]: one of walls and flat is given, and never both"},
            {"an attachment on no wall", [](auto& v) { v["attachments"][0]["walls"] = parse_values("[]"); }, "attachments[0].walls: an attachment on no wall"},
            {"a flat role the format does not define", [](auto& v) { v["flats"][1]["role"] = "wall"; }, "flats[1].role: not a role the format defines"},
            {"components that are not an object", [](auto& v) { v["layout"]["components"] = parse_values("[1]"); }, "layout.components: an object was expected"},
            {"entities that are not an array", [](auto& v) { v["attachments"][0]["entities"] = parse_values("{}"); }, "attachments[0].entities: an array was expected"},
            {"a generator that is not an object", [](auto& v) { v["generator"] = "catagen"; }, "generator: an object was expected"},
            {"no entry", [](auto& v) { v["layout"]["components"].erase("entry"); }, "layout.components.entry: required and absent"},
            {"no layout, and so no entry", [](auto& v) { v.erase("layout"); }, "layout.components.entry: required and absent"},
            {"an entry that is not a list", [](auto& v) { v["layout"]["components"]["entry"] = "stairs"; },
                "layout.components.entry: an array was expected"},
            {"an entry of no cells", [](auto& v) { v["layout"]["components"]["entry"] = parse_values("[]"); },
                "layout.components.entry: no entry cell"},
            {"an entry cell that is not a cell", [](auto& v) { v["layout"]["components"]["entry"][0] = "stairs"; },
                "layout.components.entry[0]: an array was expected"},
            {"a key the format does not define is not tracked", [](auto& v) { v["extra"] = 1; v["rooms"][0]["extra"] = 1; }, ""}
        });

        return;
    }

    //--------------------------------------------------------------------------
    void test_geometry_wall_notation()
    {
        report_group("geometry, wall notation");

        report_mutations(data_directory / "cistern.json",
        {
            {"floor with no wall toward the edge of the grid", [](auto& v) { v["cells"]["rows"][3] = "|. .+. . "; },
                "cells.rows[3]: no wall at 8 between a floor cell and the edge of the grid"},
            {"floor with no wall toward a blank cell", [](auto& v) { v["cells"]["rows"][5] = "|. .  |<|"; },
                "cells.rows[5]: no wall at 4 between a floor cell and a blank cell"},
            {"floor with no wall toward a stair cell", [](auto& v) { v["cells"]["rows"][2] = "     -   "; },
                "cells.rows[2]: no wall at 7 between a floor cell and a stair cell"},
            {"a stair cell with no wall toward the edge of the grid, being a block", [](auto& v) { v["cells"]["rows"][1] = "|. .| |> "; }, ""},
            {"a barrier off the grid", [](auto& v) { v["barriers"] = parse_values("[[9, 9]]"); }, "barriers[0]: off the grid"},
            {"a barrier written from the other side of its edge", [](auto& v) { v["barriers"] = parse_values("[[2, 1, -1, 0]]"); }, ""},
            {"a location on a blank cell", [](auto& v) { v["rooms"][1]["location"] = parse_values("[2, 2]"); }, "rooms[1].location: not a floor cell"},
            {"a location on a stair cell", [](auto& v) { v["rooms"][1]["location"] = parse_values("[3, 2]"); }, "rooms[1].location: not a floor cell"},
            {"a location off the grid", [](auto& v) { v["rooms"][1]["location"] = parse_values("[9, 9]"); }, "rooms[1].location: off the grid"},
            {"a location closed off by a barrier", [](auto& v) { v["barriers"] = parse_values("[[3, 1]]"); },
                "rooms[1].location: a location closed off by a barrier"},
            {"a perimeter of three junctions", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [2, 0], [2, 3]]"); },
                "rooms[0].perimeter: a perimeter of fewer than four junctions"},
            {"a perimeter junction off the grid", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [5, 0], [5, 3], [0, 3]]"); },
                "rooms[0].perimeter[1]: off the grid"},
            {"a perimeter edge of no length", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [2, 0], [2, 0], [2, 3], [0, 3]]"); },
                "rooms[0].perimeter[1]: an edge of no length to the next junction"},
            {"a diagonal perimeter edge", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [2, 0], [2, 3], [1, 3], [0, 2]]"); },
                "rooms[0].perimeter[3]: an edge to the next junction that is neither horizontal nor vertical"},
            {"a perimeter doubling back", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [2, 0], [1, 0], [1, 3], [0, 3]]"); },
                "rooms[0].perimeter[1]: the perimeter doubles back at this junction"},
            {"a perimeter crossing itself", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [1, 0], [1, 2], [2, 2], [2, 1], [0, 1]]"); },
                "rooms[0].perimeter[4]: the edge from this junction meets the edge from junction 1"},
            {"a perimeter along a virtual wall", [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[0, 0], [1, 0], [1, 3], [0, 3]]"); }, ""},
            {"a perimeter edge between a blank cell and the edge of the grid",
                [](auto& v) { v["rooms"][0]["perimeter"] = parse_values("[[2, 0], [3, 0], [3, 1], [2, 1]]"); },
                "rooms[0].perimeter[0]: the edge from this junction between cell [2, 0] inside and cell [2, -1] outside"},
            {"two rooms covering one cell", [](auto& v) { v["rooms"].push_back(parse_values(R"({ "location": [0, 0] })")); },
                "rooms[2]: covers the cell [0, 0], which rooms[0] also covers"},
            {"a wall address with no wall in the grid", [](auto& v) { v["walls"][0]["wall"] = parse_values("[1, 1, 1, 0]"); },
                "walls[0].wall: no wall in the grid at that address"},
            {"a wall address from a blank cell", [](auto& v) { v["walls"][0]["wall"] = parse_values("[2, 2, 0, 1]"); },
                "walls[0].wall: not a floor/door cell"},
            {"a wall address from a stair cell", [](auto& v) { v["walls"][0]["wall"] = parse_values("[3, 2, 0, -1]"); },
                "walls[0].wall: not a floor/door cell"},
            {"a wall address on a floor cell's side toward a stair cell", [](auto& v) { v["walls"][0]["wall"] = parse_values("[3, 1, 0, -1]"); }, ""},
            {"a second entry for one wall", [](auto& v) { v["walls"].push_back(parse_values(R"({ "wall": [1, 1, -1, 0] })")); },
                "walls[1].wall: a second entry for one wall"},
            {"an attachment on a wall the grid does not have", [](auto& v) { v["attachments"][0]["walls"][1] = parse_values("[2, 1, -1, 0]"); },
                "attachments[0].walls[1]: no wall in the grid at that address"},
            {"a flat referring to a room that does not exist", [](auto& v) { v["flats"][0]["rooms"][0]["room"] = 5; },
                "flats[0].rooms[0].room: no room of that id"},
            {"a flat cell off the grid", [](auto& v) { v["flats"][1]["cells"] = parse_values(R"([{ "cell": [4, 0] }])"); },
                "flats[1].cells[0].cell: off the grid"},
            {"a second flat entry for one room", [](auto& v) { v["flats"][0]["rooms"].push_back(parse_values(R"({ "room": 0 })")); },
                "flats[0].rooms[1].room: a second entry for one room"},
            {"a second flat entry for one cell", [](auto& v) { v["flats"][1]["cells"] = parse_values(R"([{ "cell": [0, 0] }, { "cell": [0, 0] }])"); },
                "flats[1].cells[1].cell: a second entry for one cell"},
            {"a second flat in one role", [](auto& v) { v["flats"][1]["role"] = "floor"; }, "flats[1]: a second flat in one role"},
            {"an attachment on a flat cell off the grid", [](auto& v) { v["attachments"][1]["cell"] = parse_values("[4, 3]"); },
                "attachments[1].cell: off the grid"},
            {"an entry cell off the grid", [](auto& v) { v["layout"]["components"]["entry"][1] = parse_values("[9, 9]"); },
                "layout.components.entry[1]: off the grid"},
            {"an entry cell on a blank cell", [](auto& v) { v["layout"]["components"]["entry"][1] = parse_values("[2, 2]"); },
                "layout.components.entry[1]: not a cell written '<' or '>'"},
            {"an entry cell on floor", [](auto& v) { v["layout"]["components"]["entry"][1] = parse_values("[0, 0]"); },
                "layout.components.entry[1]: not a cell written '<' or '>'"},
            {"a staircase leading down alone as the entry, as a tower entered from below has", [](auto& v)
                { v["layout"]["components"]["entry"] = parse_values("[[3, 2]]"); }, ""},
            {"an entry cell with nothing attached", [](auto& v) { v["attachments"].erase(2); },
                "layout.components.entry[1]: no entity with a relocator attached to the floor there"},
            {"an entry cell whose staircase is on the ceiling", [](auto& v) { v["attachments"][2]["flat"] = "ceiling"; },
                "layout.components.entry[1]: no entity with a relocator attached to the floor there"},
            {"an entry cell whose staircase has no relocator", [](auto& v) { v["attachments"][2]["entities"][0]["components"].erase("relocator"); },
                "layout.components.entry[1]: no entity with a relocator attached to the floor there"},
            {"an entry cell whose floor cell has the relocator rather than an entity", [](auto& v)
                {
                    v["attachments"][2]["entities"][0]["components"].erase("relocator");
                    v["flats"][0]["cells"] = parse_values(R"([{ "cell": [3, 0], "components": { "relocator": { "target": "level_up" } } }])");
                }, "layout.components.entry[1]: no entity with a relocator attached to the floor there"},
            {"an entry cell on a waypoint, a type the format does not restrict", [](auto& v) { v["attachments"][2]["entities"][0]["type"] = "waypoint"; }, ""},
            {"an entry cell on a type no one has defined", [](auto& v) { v["attachments"][2]["entities"][0]["type"] = "anything"; }, ""}
        });

        return;
    }

    //--------------------------------------------------------------------------
    void test_geometry_block_notation()
    {
        report_group("geometry, block notation");

        report_mutations(data_directory / "block.json",
        {
            {"a wall on a door cell's side toward floor", [](auto& v) { v["walls"][1]["wall"] = parse_values("[3, 2, 1, 0]"); },
                "walls[1].wall: no wall in the grid at that address"},
            {"a wall on a floor cell's side toward a stair cell", [](auto& v) { v["walls"][0]["wall"] = parse_values("[4, 2, -1, 0]"); }, ""},
            {"a location on a door cell", [](auto& v) { v["rooms"][0]["location"] = parse_values("[3, 2]"); }, "rooms[0].location: not a floor cell"},
            {"a location on a solid cell", [](auto& v) { v["rooms"][0]["location"] = parse_values("[0, 0]"); }, "rooms[0].location: not a floor cell"},
            {"a location on a stair cell", [](auto& v) { v["rooms"][0]["location"] = parse_values("[5, 2]"); }, "rooms[0].location: not a floor cell"},
            {"a location closed off by its own barrier", [](auto& v) { v["rooms"][0]["barriers"] = parse_values("[[1, 1]]"); },
                "rooms[0].location: a location closed off by a barrier"},
            {"a perimeter leaving the door cell out", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[4, 1], [5, 1], [5, 3], [6, 3], [6, 4], [4, 4]]"); },
                ""},
            {"a perimeter edge between a stair cell and a solid cell", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[4, 1], [6, 1], [6, 4], [4, 4]]"); },
                "rooms[1].perimeter[0]: the edge from this junction between cell [5, 1] inside and cell [5, 0] outside"},
            {"a perimeter edge between two solid cells", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[3, 1], [6, 1], [6, 4], [3, 4]]"); },
                "rooms[1].perimeter[0]: the edge from this junction between cell [3, 1] inside and cell [3, 0] outside"},
            {"a perimeter covering a cell the fill covers", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[2, 2], [3, 2], [3, 3], [2, 3]]"); },
                "rooms[1]: covers the cell [2, 2], which rooms[0] also covers"},
            {"a wall address off the grid", [](auto& v) { v["walls"][0]["wall"] = parse_values("[7, 1, 0, 1]"); }, "walls[0].wall: off the grid"}
        });

        return;
    }

    //--------------------------------------------------------------------------
    // Written and read again, by each of the writer's two overloads, the document is equivalent to the one first read
    void test_round_trip()
    {
        report_group("round trip");

        filesystem::create_directories(out_directory);

        for (const auto& filename : {"cistern.json", "block.json", "pillar.json", "runs.json"})
        {
            optional<stratum_format::Document> document = read_case(format("{} reads", filename), data_directory / filename);
            if (!document.has_value())
            {
                continue;
            }

            map_type map_expected = document_map(*document);

            try
            {
                filesystem::path filepath = out_directory / filename;
                stratum_format::write(filepath, *document);
                map_type map = document_map(stratum_format::read(filepath));
                report_case(format("{} written to a file reads back equivalent", filename), map == map_expected, map_type::diff(map_expected, map).dump());
            }
            catch (const runtime_error& e)
            {
                report_case(format("{} written to a file reads back equivalent", filename), false, e.what());
            }

            try
            {
                ostringstream os;
                stratum_format::write(os, *document);
                istringstream is(os.str());
                map_type map = document_map(stratum_format::read(is, filename));
                report_case(format("{} written to a stream reads back equivalent", filename), map == map_expected, map_type::diff(map_expected, map).dump());
            }
            catch (const runtime_error& e)
            {
                report_case(format("{} written to a stream reads back equivalent", filename), false, e.what());
            }
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_read_wall_notation();
    test_read_block_notation();
    test_read_runs();
    test_read_pillar();
    test_read_failures();
    test_structure();
    test_geometry_wall_notation();
    test_geometry_block_notation();
    test_round_trip();

    return report_results();
}
