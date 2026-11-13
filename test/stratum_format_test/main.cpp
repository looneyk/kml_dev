//------------------------------------------------------------------------------
// stratum_format_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/log/Log.hpp"
#include "quetzal/stratum_format/read_character.hpp"
#include "quetzal/stratum_format/read_config.hpp"
#include "quetzal/stratum_format/read_game.hpp"
#include "quetzal/stratum_format/GameManifestDocument.hpp"
#include "quetzal/stratum_format/read_world.hpp"
#include "quetzal/stratum_format/read_zone.hpp"
#include "quetzal/stratum_format/references.hpp"
#include "quetzal/stratum_format/RegionSpecsDocument.hpp"
#include "quetzal/stratum_format/AccessPointTarget.hpp"
#include "quetzal/stratum_format/SettingDefinitionsDocument.hpp"
#include "quetzal/stratum_format/SettingsDocument.hpp"
#include "quetzal/stratum_format/WorldDefinitionDocument.hpp"
#include "quetzal/stratum_format/write_character.hpp"
#include "quetzal/stratum_format/write_config.hpp"
#include "quetzal/stratum_format/write_game.hpp"
#include "quetzal/stratum_format/write_world.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
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

    const filesystem::path data_directory = "data";
    const filesystem::path out_directory = "out";

    // Compares regardless of key order
    using map_type = nlohmann::json;

    //--------------------------------------------------------------------------
    // Reads the zone, adding the messages of the warnings it writes to the process-wide log to warnings
    stratum_format::Document read_zone_with_warnings(istream& is, const string& stream_name, vector<string>& warnings)
    {
        log::logger().add_sink([&warnings](const log::LogEntry& entry)
        {
            if (entry.level == log::LogLevel::Warning && entry.category == stratum_format::log_category)
            {
                warnings.push_back(entry.message);
            }
        });

        try
        {
            stratum_format::Document document = stratum_format::read_zone(is, stream_name);
            log::logger().clear_sinks();
            return document;
        }
        catch (...)
        {
            log::logger().clear_sinks();
            throw;
        }
    }

    //--------------------------------------------------------------------------
    stratum_format::raw_values_type parse_values(const string& text)
    {
        return stratum_format::raw_values_type::parse(text);
    }

    //--------------------------------------------------------------------------
    stratum_format::raw_values_type load_values(const filesystem::path& filepath)
    {
        ifstream is(filepath, ios::binary);
        return stratum_format::raw_values_type::parse(is);
    }

    //--------------------------------------------------------------------------
    // Reports whether the file reads, so a file failing to read fails one case, not the run
    optional<stratum_format::Document> read_case(const string& name, const filesystem::path& filepath)
    {
        try
        {
            stratum_format::Document document = stratum_format::read_zone(filepath);
            CHECK_CASE(name, true);
            return document;
        }
        catch (const runtime_error& e)
        {
            CHECK_CASE(name, false, e.what());
            return nullopt;
        }
    }

    //--------------------------------------------------------------------------
    map_type content_map(const stratum_format::raw_values_type& values)
    {
        return map_type::parse(values.dump());
    }

    //--------------------------------------------------------------------------
    map_type generator_map(const optional<stratum_format::Generator>& generator)
    {
        if (!generator.has_value())
        {
            return map_type();
        }

        map_type parameters = map_type::object();
        for (const auto& [key, token] : generator->parameters)
        {
            parameters[key] = token;
        }

        return {{"name", generator->name}, {"parameters", parameters}};
    }

    //--------------------------------------------------------------------------
    map_type cell_position_map(const stratum_format::discrete_point_type& cell_position)
    {
        return map_type::array({cell_position.i(), cell_position.j()});
    }

    //--------------------------------------------------------------------------
    map_type direction_map(const stratum_format::discrete_vector_type& direction)
    {
        return map_type::array({direction.x(), direction.y()});
    }

    //--------------------------------------------------------------------------
    map_type address_map(const stratum_format::WallAddress& address)
    {
        return {{"cell_position", cell_position_map(address.cell_position)}, {"normal", direction_map(address.normal)}};
    }

    //--------------------------------------------------------------------------
    map_type barriers_map(const vector<stratum_format::Barrier>& barriers)
    {
        map_type m = map_type::array();

        for (const auto& barrier : barriers)
        {
            map_type b {{"cell_position", cell_position_map(barrier.cell_position)}};
            b["normal"] = barrier.normal.has_value() ? direction_map(*barrier.normal) : map_type();
            m.push_back(b);
        }

        return m;
    }

    //--------------------------------------------------------------------------
    map_type components_map(const stratum_format::raw_values_type& components)
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
            m["cell_position"] = cell_position_map(locator.cell_position);
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
    // Returns the wall an entity is sited on, or null if it is sited on a flat cell or on its parent
    const stratum_format::WallAddress* wall_address(const stratum_format::Entity& entity)
    {
        if (!entity.site.has_value() || !entity.site->address.has_value())
        {
            return nullptr;
        }

        return get_if<stratum_format::WallAddress>(&*entity.site->address);
    }

    //--------------------------------------------------------------------------
    map_type site_map(const stratum_format::Site& site)
    {
        map_type m;

        if (const auto* address = site.address.has_value() ? get_if<stratum_format::WallAddress>(&*site.address) : nullptr; address != nullptr)
        {
            m["wall"] = address_map(*address);
        }
        else if (const auto* flat_cell = site.address.has_value() ? get_if<stratum_format::FlatCell>(&*site.address) : nullptr; flat_cell != nullptr)
        {
            m["flat"] = string(stratum_format::name(flat_cell->role));
            m["cell_position"] = cell_position_map(flat_cell->cell_position);
        }
        else if (site.address.has_value())
        {
            m["cell_position"] = cell_position_map(get<stratum_format::ZoneCell>(*site.address).cell_position);
        }

        m["relation"] = string(stratum_format::name(site.relation));
        m["position"] = site.position.has_value() ? map_type::array({site.position->x(), site.position->y()}) : map_type();
        m["orientation"] = site.orientation.has_value() ? direction_map(*site.orientation) : map_type();
        return m;
    }

    //--------------------------------------------------------------------------
    map_type access_point_map(const stratum_format::AccessPoint& access_point)
    {
        map_type m;
        m["type"] = access_point.type;
        m["order"] = access_point.order;

        if (access_point.target.has_value())
        {
            const auto* target = get_if<stratum_format::AccessPointTarget>(&*access_point.target);
            m["target"] = target != nullptr ? map_type(stratum_format::format_access_point_target(*target))
                : map_type {{"entity", get<stratum_format::stored_entity_id_type>(*access_point.target)}};
        }

        return m;
    }

    //--------------------------------------------------------------------------
    map_type entity_map(const stratum_format::Entity& entity)
    {
        map_type m = components_map(entity.components);
        m["id"] = entity.id;
        m["type"] = entity.type;
        m["site"] = entity.site.has_value() ? site_map(*entity.site) : map_type();
        m["parent"] = entity.parent_id.has_value() ? map_type(*entity.parent_id) : map_type();
        m["access_point"] = entity.access_point.has_value() ? access_point_map(*entity.access_point) : map_type();
        return m;
    }

    //--------------------------------------------------------------------------
    // The document as nested maps: walls keyed by address and flats by role, since neither has an order of its own
    map_type document_map(const stratum_format::Document& document)
    {
        const stratum_format::Cells& cells = document.cells.value();

        map_type m;
        m["version"] = document.version;
        m["zone"] = document.zone_id.has_value() ? map_type(*document.zone_id) : map_type();
        m["generator"] = generator_map(document.generator);
        m["layout"] = components_map(document.components);
        m["access_points"] = document.access_points;
        m["position"] = document.position.has_value() ? map_type {document.position->x(), document.position->y(), document.position->z()}
            : map_type();
        m["unpopulated"] = document.is_unpopulated;

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

        m["entities"] = map_type::array();
        for (const auto& entity : document.entities)
        {
            m["entities"].push_back(entity_map(entity));
        }

        return m;
    }

    //--------------------------------------------------------------------------
    // A file's text with one change made to it, and the error or warning that reading it reports
    struct MutationCase
    {
        string name;
        function<void(stratum_format::raw_values_type&)> mutate;
        string message; // A fragment of the error, or empty if the changed file is valid
        string warning = {}; // A fragment of a warning the changed file must raise, or empty if none is expected
    };

    //--------------------------------------------------------------------------
    void report_mutations(const filesystem::path& filepath, const vector<MutationCase>& mutation_cases)
    {
        stratum_format::raw_values_type base = load_values(filepath);

        for (const auto& mutation_case : mutation_cases)
        {
            stratum_format::raw_values_type values = base;
            mutation_case.mutate(values);

            istringstream is(values.dump());
            vector<string> warnings;

            try
            {
                read_zone_with_warnings(is, mutation_case.name, warnings);

                bool warned = mutation_case.warning.empty() || std::any_of(warnings.begin(), warnings.end(),
                    [&](const string& warning) { return warning.find(mutation_case.warning) != string::npos; });
                CHECK_CASE(mutation_case.name, mutation_case.message.empty() && warned,
                    warned ? "accepted" : format("accepted without the warning \"{}\"", mutation_case.warning));
            }
            catch (const runtime_error& e)
            {
                string what = e.what();
                bool expected = !mutation_case.message.empty() && what.find(mutation_case.message) != string::npos;
                CHECK_CASE(mutation_case.name, expected,
                    mutation_case.message.empty() ? what : format("expected \"{}\", got \"{}\"", mutation_case.message, what));
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("read, wall notation")
    {
        optional<stratum_format::Document> read = read_case("cistern.json reads", data_directory / "cistern.json");
        if (!read.has_value())
        {
            return;
        }

        const stratum_format::Document& document = *read;

        const stratum_format::Cells& cells = document.cells.value();
        CHECK_CASE("the notation is wall", cells.notation == stratum_format::Notation::Wall);
        CHECK_CASE("the encoding defaults to characters", cells.encoding == stratum_format::Encoding::Characters);
        CHECK_CASE("the extent is 4 by 3", cells.extent.x() == 4 && cells.extent.y() == 3);
        CHECK_CASE_EQUAL("the rows number 2 * 3 + 1", cells.rows.size(), 7);
        CHECK_CASE("the first row written is the highest", cells.rows[1] == "|. .| |>|");

        CHECK_CASE_EQUAL("one barrier", document.barriers.size(), 1);
        CHECK_CASE("the barrier is an edge", document.barriers[0].normal == stratum_format::discrete_vector_type {1, 0});

        CHECK_CASE_EQUAL("two rooms", document.rooms.size(), 2);
        const auto* perimeter = get_if<stratum_format::Perimeter>(&document.rooms[0].identification);
        CHECK_CASE("the first room is identified by its perimeter", perimeter != nullptr && perimeter->junctions.size() == 4);
        const auto* locator = get_if<stratum_format::RoomLocator>(&document.rooms[1].identification);
        CHECK_CASE("the second room is identified by a cell in it",
            locator != nullptr && locator->cell_position == stratum_format::discrete_point_type {3, 1});
        CHECK_CASE("a style is a component kept as written", document.rooms[0].components["style"]["terrain"] == "water");

        CHECK_CASE("the wall address's normal points into its cell",
            document.walls[0].address == stratum_format::WallAddress {{1, 1}, {-1, 0}});

        CHECK_CASE_EQUAL("two flats", document.flats.size(), 2);
        CHECK_CASE("the first flat is the floor", document.flats[0].role == stratum_format::FlatRole::Floor);
        CHECK_CASE("the floor's room entry has its height",
            document.flats[0].rooms.contains(0) && document.flats[0].rooms.at(0).height == -0.25f);
        CHECK_CASE("the ceiling's height", document.flats[1].properties.height == 3.0f);

        CHECK_CASE_EQUAL("seven entities", document.entities.size(), 7);
        CHECK_CASE("a door is a parent, sited on nothing of its own",
            !document.entities[0].site.has_value() && !document.entities[0].parent_id.has_value());
        CHECK_CASE("a door facing is a child of its door", document.entities[1].parent_id == 1u);

        const auto* door_wall = wall_address(document.entities[1]);
        CHECK_CASE("a door facing is sited on a wall of its doorway",
            door_wall != nullptr && *door_wall == stratum_format::WallAddress {{1, 1}, {-1, 0}});

        CHECK_CASE("site and parent are decoded out of components",
            !document.entities[1].components.contains("site") && !document.entities[1].components.contains("parent"));
        CHECK_CASE("a file component the format does not define is kept as written", document.entities[2].components["concealed"] == true);

        const auto* stair_wall = wall_address(document.entities[3]);
        CHECK_CASE("a staircase is sited on the wall on its open side",
            stair_wall != nullptr && *stair_wall == stratum_format::WallAddress {{3, 1}, {0, -1}});
        const auto& stair_access_point = document.entities[3].access_point;
        CHECK_CASE("an access point is decoded out of components", stair_access_point.has_value()
            && stair_access_point->type == "staircase" && stair_access_point->order == 0 && stair_access_point->target.has_value()
            && get<stratum_format::AccessPointTarget>(*stair_access_point->target) == stratum_format::AccessPointTarget {nullopt, stratum_format::RelativeZone {-1}, nullopt}
            && !document.entities[3].components.contains("access_point"));

        const auto& chest_site = document.entities[5].site;
        const auto* flat_cell = chest_site.has_value() && chest_site->address.has_value()
            ? get_if<stratum_format::FlatCell>(&*chest_site->address) : nullptr;
        CHECK_CASE("a chest is sited on a flat cell, with an orientation",
            flat_cell != nullptr && flat_cell->role == stratum_format::FlatRole::Floor
            && flat_cell->cell_position == stratum_format::discrete_point_type {0, 2}
            && document.entities[5].site->orientation == stratum_format::discrete_vector_type {1, 0}
            && document.entities[5].site->relation == stratum_format::Relation::On);

        CHECK_CASE("a key is in its parent chest, with no address of its own",
            document.entities[6].parent_id == 6u && document.entities[6].site.has_value()
            && !document.entities[6].site->address.has_value() && document.entities[6].site->relation == stratum_format::Relation::In);
        CHECK_CASE("a reference in a file component the format does not define is kept as written",
            stratum_format::reference_id(document.entities[6].components["opens"]) == 1u);

        CHECK_CASE("the document is a template, with no zone id of its own", !document.zone_id.has_value());

        CHECK_CASE("the generator has its name and its parameters' tokens", document.generator.has_value() && document.generator->name == "catagen"
            && document.generator->parameters.value("extent") == "4,3" && document.generator->parameters.value("scale") == "3-7");
        CHECK_CASE("layout components are kept as written", document.components["difficulty"] == 3 && document.components["style"] == "cistern");
        CHECK_CASE("the access points are decoded out of them", !document.access_points.empty() && !document.components.contains("access_points"));
        CHECK_CASE("and so is the position", document.position == stratum_format::zone_position_type {0, 0, -2}
            && !document.components.contains("position"));
        CHECK_CASE("and so is the unpopulated flag", document.is_unpopulated && !document.components.contains("unpopulated"));

        stratum_format::raw_values_type unflagged = load_values(data_directory / "cistern.json");
        unflagged["layout"]["components"].erase("unpopulated");
        istringstream unflagged_stream(unflagged.dump());
        CHECK_CASE("an absent unpopulated flag is false", !stratum_format::read_zone(unflagged_stream, "unflagged").is_unpopulated);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("read, block notation")
    {
        optional<stratum_format::Document> read = read_case("block.json reads", data_directory / "block.json");
        if (!read.has_value())
        {
            return;
        }

        const stratum_format::Document& document = *read;

        const stratum_format::Cells& cells = document.cells.value();
        CHECK_CASE("the notation is block", cells.notation == stratum_format::Notation::Block);
        CHECK_CASE_EQUAL("the rows number the extent", cells.rows.size(), 5);
        CHECK_CASE("the scale", cells.scale == 2.0f);

        const auto* locator = get_if<stratum_format::RoomLocator>(&document.rooms[0].identification);
        CHECK_CASE("a room identified by a cell in it has its own barrier", locator != nullptr && locator->barriers.size() == 1
            && !locator->barriers[0].normal.has_value());
        CHECK_CASE("a style naming an asset and overriding part of it is kept as written",
            document.rooms[1].components["style"]["name"] == "hall" && document.rooms[1].components["style"]["lit"] == true);

        const auto& site = document.entities[1].site;
        CHECK_CASE("a site's position on its wall",
            site.has_value() && site->position.has_value() && site->position->x() == 0.5f && site->position->y() == 0.0f);
        CHECK_CASE("a key the format does not define inside a file component is kept",
            document.entities[4].components["custom"]["anything"][2] == 3);

        CHECK_CASE("the floor's cell entry has its height",
            document.flats[0].cells.contains({5, 2}) && document.flats[0].cells.at({5, 2}).height == -0.5f);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("read, runs")
    {
        optional<stratum_format::Document> runs = read_case("runs.json reads", data_directory / "runs.json");
        optional<stratum_format::Document> characters = read_case("cistern.json reads", data_directory / "cistern.json");
        if (!runs.has_value() || !characters.has_value())
        {
            return;
        }

        CHECK_CASE("the encoding is runs", runs->cells.value().encoding == stratum_format::Encoding::Runs);
        CHECK_CASE("rows in runs read as the same rows in characters", runs->cells.value().rows == characters->cells.value().rows);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("read, a pillar on a room's edge")
    {
        read_case("pillar.json reads, the pillar inside the first room's perimeter", data_directory / "pillar.json");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("read, files that cannot be read")
    {
        try
        {
            stratum_format::read_zone(data_directory / "absent.json");
            CHECK_CASE("an absent file", false, "accepted");
        }
        catch (const runtime_error& e)
        {
            CHECK_CASE("an absent file", string(e.what()).find("cannot be opened") != string::npos, e.what());
        }

        try
        {
            istringstream is(R"({ "format": )");
            stratum_format::read_zone(is, "truncated");
            CHECK_CASE("text that is not valid JSON", false, "accepted");
        }
        catch (const runtime_error& e)
        {
            CHECK_CASE("text that is not valid JSON", string(e.what()).find("parse_error") != string::npos, e.what());
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("structure")
    {
        report_mutations(data_directory / "cistern.json",
        {
            {"a format other than this", [](auto& v) { v["format"] = "other"; }, "format: 'other' is not this format"},
            {"a version this does not read", [](auto& v) { v["version"] = "0.2"; }, "version: '0.2' is not a version this reads"},
            {"no cells, with the rest of a layout", [](auto& v) { v.erase("cells"); }, "barriers: in an empty zone, which has no cells"},
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
            {"a room with both perimeter and cell_position", [](auto& v) { v["rooms"][0]["cell_position"] = parse_values("[0, 0]"); },
                "rooms[0]: one of perimeter and cell_position is given, and never both"},
            {"a room with neither perimeter nor cell_position", [](auto& v) { v["rooms"][0].erase("perimeter"); },
                "rooms[0]: one of perimeter and cell_position is given, and never both"},
            {"a barrier that is not an object", [](auto& v) { v["barriers"] = parse_values("[[3, 1]]"); }, "barriers[0]: an object was expected"},
            {"a barrier with no cell position", [](auto& v) { v["barriers"][0].erase("cell_position"); },
                "barriers[0].cell_position: required and absent"},
            {"a normal that is not a direction a cell's sides define", [](auto& v) { v["barriers"][0]["normal"] = parse_values("[1, 1]"); },
                "barriers[0].normal: not one of the four directions a cell's sides define"},
            {"a flat role the format does not define", [](auto& v) { v["flats"][1]["role"] = "wall"; }, "flats[1].role: not a role the format defines"},
            {"components that are not an object", [](auto& v) { v["layout"]["components"] = parse_values("[1]"); }, "layout.components: an object was expected"},
            {"a generator that is not an object", [](auto& v) { v["generator"] = "catagen"; }, "generator: an object was expected"},
            {"a generator with no name", [](auto& v) { v["generator"].erase("name"); }, "generator.name: required and absent"},
            {"a generator with no parameters", [](auto& v) { v["generator"].erase("parameters"); }, "generator.parameters: required and absent"},
            {"a parameter that is not a string", [](auto& v) { v["generator"]["parameters"]["seed"] = 20260829; },
                "generator.parameters.seed: a string was expected"},
            {"a key the format does not define is not tracked", [](auto& v) { v["extra"] = 1; v["rooms"][0]["extra"] = 1; }, ""}
        });

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("entities")
    {
        report_mutations(data_directory / "cistern.json",
        {
            {"entities that are not a list", [](auto& v) { v["entities"] = parse_values("{}"); }, "entities: an array was expected"},
            {"an entity that is not an object", [](auto& v) { v["entities"][0] = 1; }, "entities[0]: an object was expected"},
            {"an entity with no id", [](auto& v) { v["entities"][0].erase("id"); }, "entities[0].id: required and absent"},
            {"an id with a fraction", [](auto& v) { v["entities"][0]["id"] = 1.5; }, "entities[0].id: a whole, non-negative number was expected"},
            {"a negative id", [](auto& v) { v["entities"][0]["id"] = -1; }, "entities[0].id: a whole, non-negative number was expected"},
            {"two entities with one id", [](auto& v) { v["entities"][1]["id"] = 1; }, "entities[1].id: a second entity with the id 1"},
            {"an entity with no type", [](auto& v) { v["entities"][0].erase("type"); }, "entities[0].type: required and absent"},
            {"a type that is not a string", [](auto& v) { v["entities"][0]["type"] = 1; }, "entities[0].type: a string was expected"},
            {"a type the format does not restrict", [](auto& v) { v["entities"][0]["type"] = "anything"; }, ""},
            {"a site that is not an object", [](auto& v) { v["entities"][1]["components"]["site"] = 1; },
                "entities[1].components.site: an object was expected"},
            {"a site with no relation", [](auto& v) { v["entities"][1]["components"]["site"].erase("relation"); },
                "entities[1].components.site.relation: required and absent"},
            {"a relation that is not a string", [](auto& v) { v["entities"][1]["components"]["site"]["relation"] = 1; },
                "entities[1].components.site.relation: a string was expected"},
            {"a relation the format does not define", [](auto& v) { v["entities"][1]["components"]["site"]["relation"] = "beside"; },
                "entities[1].components.site.relation: not a relation the format defines"},
            {"a site on both a wall and a flat cell", [](auto& v) { v["entities"][1]["components"]["site"]["flat"] = "floor"; },
                "entities[1].components.site: wall and flat are never both given"},
            {"a site on its parent where the entity has none", [](auto& v) { v["entities"][3]["components"]["site"].erase("wall"); },
                "entities[3]: sited on its parent and has none"},
            {"a wall address that is not an object", [](auto& v) { v["entities"][1]["components"]["site"]["wall"] = parse_values("[1, 1, -1, 0]"); },
                "entities[1].components.site.wall: an object was expected"},
            {"a wall address with no normal", [](auto& v) { v["entities"][1]["components"]["site"]["wall"].erase("normal"); },
                "entities[1].components.site.wall.normal: required and absent"},
            {"a flat cell with no cell position", [](auto& v) { v["entities"][5]["components"]["site"].erase("cell_position"); },
                "entities[5].components.site.cell_position: required and absent"},
            {"a position that is not two numbers", [](auto& v) { v["entities"][1]["components"]["site"]["position"] = parse_values("[1]"); },
                "entities[1].components.site.position: 2 numbers were expected, and there are 1"},
            {"an orientation that is not a direction", [](auto& v) { v["entities"][5]["components"]["site"]["orientation"] = parse_values("[1, 1]"); },
                "entities[5].components.site.orientation: not one of the four directions a cell's sides define"},
            {"a parent that is not a reference", [](auto& v) { v["entities"][1]["components"]["parent"] = 1; },
                "entities[1].components.parent: not a reference to an entity"},
            {"an entity not sited on the geometry, no entity's child and no entity's parent",
                [](auto& v) { v["entities"].push_back(parse_values(R"({ "id": 8, "type": "torch" })")); },
                "entities[7]: not sited on the geometry, no entity's child and no entity's parent"},
            {"a hierarchy of parents and children sited on the geometry nowhere in it", [](auto& v)
                {
                    v["entities"].push_back(parse_values(R"({ "id": 8, "type": "sack" })"));
                    v["entities"].push_back(parse_values(R"({ "id": 9, "type": "coin", "components": { "parent": { "entity": 8 } } })"));
                }, "entities[7]: no entity in this hierarchy of parents and children is sited on the geometry"},
            {"parents leading back to an entity", [](auto& v)
                { v["entities"][5]["components"]["parent"] = parse_values(R"({ "entity": 7 })"); },
                "entities[5]: its parents lead back to it"},
            {"a reference that is no entity's id", [](auto& v) { v["entities"][6]["components"]["opens"] = parse_values(R"({ "entity": 99 })"); },
                "entities[6].components: refers to 99, which is no entity's id"},
            {"a parent that is no entity's id", [](auto& v) { v["entities"][1]["components"]["parent"]["entity"] = 99; },
                "entities[1].components.parent: refers to 99, which is no entity's id"},
            {"a reference nested in a file component the format does not define", [](auto& v)
                { v["rooms"][0]["components"]["watched_by"] = parse_values(R"([{ "guard": { "entity": 99 } }])"); },
                "rooms[0].components: refers to 99, which is no entity's id"},
            {"a zone id that is not a whole number", [](auto& v) { v["zone"] = parse_values(R"({ "id": -1 })"); },
                "zone.id: a whole, non-negative number was expected"},
            {"a zone section with no id", [](auto& v) { v["zone"] = parse_values("{}"); }, "zone.id: required and absent"},
            {"a saved zone's reference into another zone, which is not checked", [](auto& v)
                {
                    v["zone"] = parse_values(R"({ "id": 7713 })");
                    v["entities"][6]["components"]["opens"] = parse_values(R"({ "entity": 90412 })");
                }, ""},
            {"an actor in a cell of the zone, which is sited on the geometry", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "rat", "components": { "site": { "relation": "in", "cell_position": [0, 0], "orientation": [1, 0] } } })"));
                }, ""},
            {"an actor in a cell of the zone, on rather than in", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "rat", "components": { "site": { "relation": "on", "cell_position": [0, 0], "orientation": [1, 0] } } })"));
                }, "entities[7].components.site.relation: an entity in a cell of the zone has the relation in"},
            {"an actor in a cell of the zone with no orientation", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "rat", "components": { "site": { "relation": "in", "cell_position": [0, 0] } } })"));
                }, "entities[7].components.site: an entity in a cell of the zone has no orientation"},
            {"an actor in a blank cell", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "rat", "components": { "site": { "relation": "in", "cell_position": [2, 2], "orientation": [1, 0] } } })"));
                }, "entities[7].components.site.cell_position: not a cell an actor can stand in"},
            {"an actor in a cell off the grid", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "rat", "components": { "site": { "relation": "in", "cell_position": [9, 9], "orientation": [1, 0] } } })"));
                }, "entities[7].components.site.cell_position: off the grid"}
        });

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("entities the glyphs require")
    {
        report_mutations(data_directory / "cistern.json",
        {
            {"a doorway whose walls have no one parent between them", [](auto& v) { v["entities"][2]["components"]["parent"]["entity"] = 6; },
                "entities: the doorway at [1, 1] normal [-1, 0] and [2, 1] normal [1, 0] has no entity with a child sited on each of its walls"},
            {"a doorway with two doors on it", [](auto& v)
                {
                    v["entities"].push_back(parse_values(R"({ "id": 8, "type": "door" })"));
                    v["entities"].push_back(parse_values(
                        R"({ "id": 9, "type": "door_facing", "components": { "parent": { "entity": 8 },)"
                        R"( "site": { "relation": "on", "wall": { "cell_position": [1, 1], "normal": [-1, 0] } } } })"));
                    v["entities"].push_back(parse_values(
                        R"({ "id": 10, "type": "door_facing", "components": { "parent": { "entity": 8 },)"
                        R"( "site": { "relation": "on", "wall": { "cell_position": [2, 1], "normal": [1, 0] } } } })"));
                }, "has 2 entities with a child sited on each of its walls"},
            {"a door with a child on a wall outside its doorway", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "door_facing", "components": { "parent": { "entity": 1 },)"
                        R"( "site": { "relation": "on", "wall": { "cell_position": [0, 1], "normal": [1, 0] } } } })"));
                }, "entities[7]: sited on a wall that is not one of the walls of its door's doorway"},
            {"a stair cell with no access point on the walls around it, the access point on it sited on the floor instead", [](auto& v)
                {
                    v["entities"][3]["components"]["site"] = parse_values(
                        R"({ "relation": "on", "flat": "floor", "cell_position": [2, 1], "orientation": [0, -1] })");
                }, "", "entities: the stair cell at [3, 2] has no access point sited on a wall its open neighbors present to it"},
            {"an access point on the floor with no orientation", [](auto& v)
                {
                    v["entities"][3]["components"]["site"] = parse_values(R"({ "relation": "on", "flat": "floor", "cell_position": [2, 1] })");
                }, "entities[3].components.site: an access point on the floor or the ceiling with no orientation"},
            {"a stair cell with a second access point on the walls around it", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "portal", "components": { "site": { "relation": "on",)"
                        R"( "wall": { "cell_position": [3, 1], "normal": [0, -1] } }, "access_point": { "type": "portal", "target": "elsewhere", "order": 0 } } })"));
                    v["layout"]["components"]["access_points"].push_back(parse_values(R"({ "entity": 8 })"));
                }, "", "entities[7]: a second access point on the walls around the stair cell at [3, 2]"},
            {"no access points", [](auto& v) { v["layout"]["components"].erase("access_points"); },
                "layout.components.access_points: required and absent"},
            {"a position coordinate that is not a whole number", [](auto& v) { v["layout"]["components"]["position"][2] = -1.5; },
                "layout.components.position[2]: a whole number was expected"},
            {"a position that is not three coordinates", [](auto& v) { v["layout"]["components"]["position"] = {0, -2}; },
                "layout.components.position"},
            {"an unpopulated flag that is not true or false", [](auto& v) { v["layout"]["components"]["unpopulated"] = 1; },
                "layout.components.unpopulated: true or false was expected"},
            {"an unpopulated flag in a zone with a zone id", [](auto& v) { v["zone"] = parse_values(R"({ "id": 7713 })"); },
                "", "layout.components.unpopulated: set in a zone with a zone id"},
            {"no layout, and so no access points", [](auto& v) { v.erase("layout"); }, "layout.components.access_points: required and absent"},
            {"access points that are not a list", [](auto& v) { v["layout"]["components"]["access_points"] = "stairs"; },
                "layout.components.access_points: an array was expected"},
            {"no access point", [](auto& v) { v["layout"]["components"]["access_points"] = parse_values("[]"); },
                "layout.components.access_points: no access point"},
            {"an access point that is not a reference", [](auto& v) { v["layout"]["components"]["access_points"][0] = 4; },
                "layout.components.access_points[0]: not a reference to an entity"},
            {"an access point that is no entity's id", [](auto& v) { v["layout"]["components"]["access_points"][0] = parse_values(R"({ "entity": 99 })"); },
                "layout.components.access_points[0]: refers to 99, which is no entity's id"},
            {"an access point of a saved zone that is no entity's id, which the reference check lets by", [](auto& v)
                {
                    v["zone"] = parse_values(R"({ "id": 7713 })");
                    v["layout"]["components"]["access_points"][0] = parse_values(R"({ "entity": 90412 })");
                }, "layout.components.access_points[0]: refers to 90412, which is no entity's id"},
            {"an access point on an entity with no access_point", [](auto& v) { v["layout"]["components"]["access_points"][0] = parse_values(R"({ "entity": 1 })"); },
                "layout.components.access_points[0]: an entity with no access_point"},
            {"a portal as an access point on a wall that is neither a stair cell's open side nor a door's", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 8, "type": "portal", "components": { "site": { "relation": "on",)"
                        R"( "wall": { "cell_position": [0, 0], "normal": [1, 0] } }, "access_point": { "type": "portal", "target": "elsewhere", "order": 0 } } })"));
                    v["layout"]["components"]["access_points"].push_back(parse_values(R"({ "entity": 8 })"));
                }, "", "layout.components.access_points[2]: an entity sited on a wall that is neither the open side of a cell written '<' or '>' nor a door's wall"},
            {"an access point that is not sited on a surface", [](auto& v)
                {
                    v["entities"].push_back(parse_values(
                        R"({ "id": 9, "type": "portal", "components": { "parent": { "entity": 6 }, "access_point": { "type": "portal",)"
                        R"( "target": "elsewhere", "order": 0 } } })"));
                    v["layout"]["components"]["access_points"][0] = parse_values(R"({ "entity": 9 })");
                }, "layout.components.access_points[0]: an entity that is not sited on a surface: a wall, the floor or the ceiling"},
            {"a staircase leading down alone as the access point, as a tower entered from below has", [](auto& v)
                {
                    v["entities"][4]["components"].erase("access_point");
                    v["layout"]["components"]["access_points"] = parse_values(R"([{ "entity": 4 }])");
                }, ""},
            {"an access_point on an entity that the access points do not list", [](auto& v)
                { v["layout"]["components"]["access_points"] = parse_values(R"([{ "entity": 4 }])"); },
                "entities[4].components.access_point: on an entity that layout.components.access_points does not list"},
            {"an access point with no type", [](auto& v) { v["entities"][3]["components"]["access_point"].erase("type"); },
                "entities[3].components.access_point.type: required and absent"},
            {"an access point whose type is neither its entity's type nor its parent's", [](auto& v)
                { v["entities"][3]["type"] = "hatch"; },
                "", "entities[3].components.access_point.type: the type 'staircase' is neither its entity's type 'hatch' nor its parent's"},
            {"an access point with no order", [](auto& v) { v["entities"][3]["components"]["access_point"].erase("order"); },
                "entities[3].components.access_point.order: required and absent"},
            {"an access point whose order is negative", [](auto& v) { v["entities"][3]["components"]["access_point"]["order"] = -1; },
                "entities[3].components.access_point.order: a whole, non-negative number was expected"},
            {"an access point whose order is past a gap", [](auto& v) { v["entities"][4]["components"]["access_point"]["order"] = 2; },
                "layout.components.access_points[1]: order 2 is past a gap among the access points of the type 'staircase'"},
            {"an access point whose order repeats another's of its type", [](auto& v)
                { v["entities"][4]["components"]["access_point"]["order"] = 0; },
                "entities[4].components.access_point.order: order 0 is another access point's of the type 'staircase'"},
            {"an access point at which an arriving actor is placed in a cell it cannot stand in", [](auto& v)
                {
                    v["entities"][3]["components"]["site"] = parse_values(
                        R"({ "relation": "on", "flat": "floor", "cell_position": [3, 2], "orientation": [0, -1] })");
                }, "entities[3].components.site: an access point at which an arriving actor is placed in a cell it cannot stand in"},
            {"an access point with no target, which is arrival only", [](auto& v) { v["entities"][3]["components"]["access_point"].erase("target"); },
                ""},
            {"an access point whose target is neither a string nor a reference", [](auto& v)
                { v["entities"][3]["components"]["access_point"]["target"] = -1; },
                "entities[3].components.access_point.target: neither a target nor a reference to an entity"},
            {"an access point whose target is a level offset of more than one level", [](auto& v)
                { v["entities"][3]["components"]["access_point"]["target"] = "-2"; }, ""},
            {"an access point whose target is an absolute level in a named region, at a position", [](auto& v)
                { v["entities"][3]["components"]["access_point"]["target"] = "sewers/=-3@grate"; }, ""},
            {"an access point whose target does not match the grammar", [](auto& v)
                { v["entities"][3]["components"]["access_point"]["target"] = "1"; },
                "entities[3].components.access_point.target: not a target written [region/]zone[@position]"},
            {"a template's access point bound", [](auto& v)
                { v["entities"][3]["components"]["access_point"]["target"] = parse_values(R"({ "entity": 5 })"); },
                "entities[3].components.access_point.target: bound, which an access point is only in a saved zone"},
            {"a saved zone's access point bound to an access point in another zone", [](auto& v)
                {
                    v["zone"] = parse_values(R"({ "id": 7713 })");
                    v["entities"][3]["components"]["access_point"]["target"] = parse_values(R"({ "entity": 90412 })");
                }, ""}
        });

        // A door facing as an access point, as a door into another region has, raises no warning
        stratum_format::raw_values_type door_access = load_values(data_directory / "cistern.json");
        door_access["entities"][1]["components"]["access_point"] = parse_values(R"({ "type": "door", "target": "+0", "order": 0 })");
        door_access["layout"]["components"]["access_points"].push_back(parse_values(R"({ "entity": 2 })"));

        vector<string> warnings;
        try
        {
            istringstream is(door_access.dump());
            read_zone_with_warnings(is, "door access point", warnings);
            CHECK_CASE("an access point on a door's wall raises no warning", warnings.empty(), warnings.empty() ? "" : warnings.front());
        }
        catch (const runtime_error& e)
        {
            CHECK_CASE("an access point on a door's wall raises no warning", false, e.what());
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("references")
    {
        stratum_format::raw_values_type components = parse_values(
            R"({ "controls": { "entity": 1 }, "watched_by": [{ "guard": { "entity": 2 } }, { "entity": 3 }],)"
            R"( "lock": "vault", "near": { "entity": 4, "how": "beside" } })");

        vector<stratum_format::stored_entity_id_type> found = stratum_format::references(components);
        CHECK_CASE("every reference is found, in the order they are written",
            found == vector<stratum_format::stored_entity_id_type> {1, 2, 3}, format("found {} of them", found.size()));

        stratum_format::translate_references(components, [](stratum_format::stored_entity_id_type id) { return id + 100; });
        vector<stratum_format::stored_entity_id_type> translated = stratum_format::references(components);
        CHECK_CASE("each reference is replaced by what translate returns for it",
            translated == vector<stratum_format::stored_entity_id_type> {101, 102, 103});
        CHECK_CASE("a value that is not a reference is left as it is",
            components["lock"] == "vault" && components["near"]["entity"] == 4);

        CHECK_CASE("an object whose only key is entity is a reference",
            stratum_format::reference_id(parse_values(R"({ "entity": 7 })")) == 7u);
        CHECK_CASE("an object with a key beside entity is not a reference",
            !stratum_format::reference_id(parse_values(R"({ "entity": 7, "how": "in" })")).has_value());
        CHECK_CASE("an entity that is not a whole, non-negative number is not a reference",
            !stratum_format::reference_id(parse_values(R"({ "entity": "seven" })")).has_value()
            && !stratum_format::reference_id(parse_values(R"({ "entity": -7 })")).has_value());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("geometry, wall notation")
    {
        report_mutations(data_directory / "cistern.json",
        {
            {"floor with no wall toward the edge of the grid", [](auto& v) { v["cells"]["rows"][3] = "|. .+. . "; },
                "cells.rows[3]: no wall at 8 between a floor cell and the edge of the grid"},
            {"floor with no wall toward a blank cell", [](auto& v) { v["cells"]["rows"][5] = "|. .  |<|"; },
                "cells.rows[5]: no wall at 4 between a floor cell and a blank cell"},
            {"floor with no wall toward a stair cell", [](auto& v) { v["cells"]["rows"][2] = "     -   "; },
                "cells.rows[2]: no wall at 7 between a floor cell and a stair cell"},
            {"a stair cell with no wall toward the edge of the grid, being a block", [](auto& v) { v["cells"]["rows"][1] = "|. .| |> "; }, ""},
            {"a barrier off the grid", [](auto& v) { v["barriers"] = parse_values(R"([{ "cell_position": [9, 9] }])"); }, "barriers[0]: off the grid"},
            {"a barrier written from the other side of its edge",
                [](auto& v) { v["barriers"] = parse_values(R"([{ "cell_position": [2, 1], "normal": [-1, 0] }])"); }, ""},
            {"a room cell on a blank cell", [](auto& v) { v["rooms"][1]["cell_position"] = parse_values("[2, 2]"); },
                "rooms[1].cell_position: not a floor cell"},
            {"a room cell on a stair cell", [](auto& v) { v["rooms"][1]["cell_position"] = parse_values("[3, 2]"); },
                "rooms[1].cell_position: not a floor cell"},
            {"a room cell off the grid", [](auto& v) { v["rooms"][1]["cell_position"] = parse_values("[9, 9]"); }, "rooms[1].cell_position: off the grid"},
            {"a room cell closed off by a barrier", [](auto& v) { v["barriers"] = parse_values(R"([{ "cell_position": [3, 1] }])"); },
                "rooms[1].cell_position: a location closed off by a barrier"},
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
            {"two rooms covering one cell", [](auto& v) { v["rooms"].push_back(parse_values(R"({ "cell_position": [0, 0] })")); },
                "rooms[2]: covers the cell [0, 0], which rooms[0] also covers"},
            {"a wall address with no wall in the grid",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [1, 1], "normal": [1, 0] })"); },
                "walls[0].wall: no wall in the grid at that address"},
            {"a wall address from a blank cell",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [2, 2], "normal": [0, 1] })"); },
                "walls[0].wall: not a floor/door cell"},
            {"a wall address from a stair cell",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [3, 2], "normal": [0, -1] })"); },
                "walls[0].wall: not a floor/door cell"},
            {"a wall address on a floor cell's side toward a stair cell",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [3, 1], "normal": [0, -1] })"); }, ""},
            {"a second entry for one wall",
                [](auto& v) { v["walls"].push_back(parse_values(R"({ "wall": { "cell_position": [1, 1], "normal": [-1, 0] } })")); },
                "walls[1].wall: a second entry for one wall"},
            {"an entity on a wall the grid does not have",
                [](auto& v) { v["entities"][2]["components"]["site"]["wall"] = parse_values(R"({ "cell_position": [2, 1], "normal": [-1, 0] })"); },
                "entities[2].components.site.wall: no wall in the grid at that address"},
            {"a flat referring to a room that does not exist", [](auto& v) { v["flats"][0]["rooms"][0]["room"] = 5; },
                "flats[0].rooms[0].room: no room of that id"},
            {"a flat cell off the grid", [](auto& v) { v["flats"][1]["cells"] = parse_values(R"([{ "cell_position": [4, 0] }])"); },
                "flats[1].cells[0].cell_position: off the grid"},
            {"a second flat entry for one room", [](auto& v) { v["flats"][0]["rooms"].push_back(parse_values(R"({ "room": 0 })")); },
                "flats[0].rooms[1].room: a second entry for one room"},
            {"a second flat entry for one cell",
                [](auto& v) { v["flats"][1]["cells"] = parse_values(R"([{ "cell_position": [0, 0] }, { "cell_position": [0, 0] }])"); },
                "flats[1].cells[1].cell_position: a second entry for one cell"},
            {"a second flat in one role", [](auto& v) { v["flats"][1]["role"] = "floor"; }, "flats[1]: a second flat in one role"},
            {"an entity on a flat cell off the grid", [](auto& v) { v["entities"][5]["components"]["site"]["cell_position"] = parse_values("[4, 3]"); },
                "entities[5].components.site.cell_position: off the grid"}
        });

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("geometry, block notation")
    {
        report_mutations(data_directory / "block.json",
        {
            {"a wall on a door cell's side toward floor",
                [](auto& v) { v["walls"][1]["wall"] = parse_values(R"({ "cell_position": [3, 2], "normal": [1, 0] })"); },
                "walls[1].wall: no wall in the grid at that address"},
            {"a wall on a floor cell's side toward a stair cell, which is floor in the block notation",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [4, 2], "normal": [-1, 0] })"); },
                "walls[0].wall: no wall in the grid at that address"},
            {"a room cell on a door cell", [](auto& v) { v["rooms"][0]["cell_position"] = parse_values("[3, 2]"); },
                "rooms[0].cell_position: not a floor cell"},
            {"a room cell on a solid cell", [](auto& v) { v["rooms"][0]["cell_position"] = parse_values("[0, 0]"); },
                "rooms[0].cell_position: not a floor cell"},
            {"a room cell on a stair cell, which is floor in the block notation",
                [](auto& v) { v["rooms"][1] = parse_values(R"({ "cell_position": [5, 2] })"); }, ""},
            {"a room cell closed off by its own barrier", [](auto& v) { v["rooms"][0]["barriers"] = parse_values(R"([{ "cell_position": [1, 1] }])"); },
                "rooms[0].cell_position: a location closed off by a barrier"},
            {"a perimeter leaving the door cell out", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[4, 1], [6, 1], [6, 4], [4, 4]]"); },
                ""},
            {"a perimeter leaving the stair cells out, along a virtual wall",
                [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[4, 1], [5, 1], [5, 3], [6, 3], [6, 4], [4, 4]]"); }, ""},
            {"a perimeter edge between two solid cells", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[3, 1], [6, 1], [6, 4], [3, 4]]"); },
                "rooms[1].perimeter[0]: the edge from this junction between cell [3, 1] inside and cell [3, 0] outside"},
            {"a perimeter covering a cell the fill covers", [](auto& v) { v["rooms"][1]["perimeter"] = parse_values("[[2, 2], [3, 2], [3, 3], [2, 3]]"); },
                "rooms[1]: covers the cell [2, 2], which rooms[0] also covers"},
            {"a wall address off the grid",
                [](auto& v) { v["walls"][0]["wall"] = parse_values(R"({ "cell_position": [7, 1], "normal": [0, 1] })"); }, "walls[0].wall: off the grid"}
        });

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("relocator targets")
    {
        using stratum_format::AbsoluteZone;
        using stratum_format::RelativeZone;
        using stratum_format::AccessPointTarget;

        const vector<pair<string, AccessPointTarget>> targets {
            {"-1", {nullopt, RelativeZone {-1}, nullopt}},
            {"+2", {nullopt, RelativeZone {2}, nullopt}},
            {"+0", {nullopt, RelativeZone {0}, nullopt}},
            {"=0", {nullopt, AbsoluteZone {0}, nullopt}},
            {"=-3", {nullopt, AbsoluteZone {-3}, nullopt}},
            {"sewers/=-1", {"sewers", AbsoluteZone {-1}, nullopt}},
            {"town/market@fountain", {"town", string("market"), "fountain"}},
            {"-1@3,5", {nullopt, RelativeZone {-1}, "3,5"}}
        };

        for (const auto& [text, expected] : targets)
        {
            optional<AccessPointTarget> parsed = stratum_format::parse_access_point_target(text);
            CHECK_CASE(format("{} is read and written back the same", text), parsed == expected
                && stratum_format::format_access_point_target(*parsed) == text);
        }

        for (const string& text : {"", "1", "-", "+-1", "=", "=+2", "+1.5", "sewers/", "../=0", "./-1", "a/b/c", "1a/-1", "-1@", "-1@a@b", "@x", "market-1",
            "-99999999999"})
        {
            CHECK_CASE(format("\"{}\" is not a target", text), !stratum_format::parse_access_point_target(text).has_value());
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A document written by each of the writer's two overloads and read again is equivalent to the one first read
    TEST_CASE("round trip")
    {
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
                map_type map = document_map(stratum_format::read_zone(filepath));
                CHECK_CASE(format("{} written to a file reads back equivalent", filename), map == map_expected, map_type::diff(map_expected, map).dump());
            }
            catch (const runtime_error& e)
            {
                CHECK_CASE(format("{} written to a file reads back equivalent", filename), false, e.what());
            }

            try
            {
                ostringstream os;
                stratum_format::write(os, *document);
                istringstream is(os.str());
                map_type map = document_map(stratum_format::read_zone(is, filename));
                CHECK_CASE(format("{} written to a stream reads back equivalent", filename), map == map_expected, map_type::diff(map_expected, map).dump());
            }
            catch (const runtime_error& e)
            {
                CHECK_CASE(format("{} written to a stream reads back equivalent", filename), false, e.what());
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("empty zone")
    {
        stratum_format::Document document;
        document.zone_id = 7;
        document.position = stratum_format::zone_position_type {0, 0, -2};
        document.zone_spec = KeyValueStore {{"seed", "12"}, {"parameters.denizens", "few"}};

        ostringstream os;
        stratum_format::write(os, document);
        istringstream is(os.str());
        const stratum_format::Document read = stratum_format::read_zone(is, "empty zone");
        CHECK_CASE("an empty zone written reads back with its id, its position and its zone spec, and no cells", !read.cells.has_value()
            && read.zone_id == 7 && read.position == stratum_format::zone_position_type {0, 0, -2} && read.access_points.empty()
            && read.zone_spec.has_value() && read.zone_spec->value("seed") == "12" && read.zone_spec->value("parameters.denizens") == "few",
            os.str());

        auto rejected = [](const string& text, const string& error_expected)
        {
            try
            {
                istringstream is(text);
                stratum_format::read_zone(is, "empty zone");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        const string header = format(R"("format": "{}", "version": "{}")", stratum_format::format_name, stratum_format::format_version);
        CHECK_CASE("an empty zone without a zone spec is an error",
            rejected(format(R"({{ {}, "zone": {{ "id": 7 }} }})", header), "zone_spec: required in an empty zone"));
        CHECK_CASE("and one without a zone id",
            rejected(format(R"({{ {}, "zone_spec": {{ "seed": "12" }} }})", header), "zone: required in an empty zone"));
        CHECK_CASE("and one with entities", rejected(format(R"({{ {}, "zone": {{ "id": 7 }}, "zone_spec": {{ "seed": "12" }}, "entities": [] }})",
            header), "entities: in an empty zone"));

        return;
    }

    //--------------------------------------------------------------------------
    // Each game file document written to a stream is identical to its written form
    TEST_CASE("game files")
    {
        auto written = [](const auto& document)
        {
            ostringstream os;
            stratum_format::write(os, document);
            return os.str();
        };

        auto report_written = [](const string& name, const string& text, const string& text_expected)
        {
            CHECK_CASE(name, text == text_expected, format("expected\n{}got\n{}", text_expected, text));
        };

        stratum_format::GameDocument game;
        game.notation = stratum_format::Notation::Wall;
        game.next_id = 21;
        game.seed = "20260928";
        game.random_engines = {{"gameplay", "1 2 3"}, {"generation", "4 5 6"}};
        const quetzal::settings::Settings::preset_key_type custom_key {"gameplay.difficulty", "custom"};
        game.settings_snapshot.settings.insert("gameplay.difficulty", "custom");
        game.settings_snapshot.presets.emplace(custom_key, quetzal::KeyValueStore {{"gameplay.difficulty.denizen_count_multiplier", "2"}});

        report_written("a game file has the game, the settings and the presets each on a line of its own", written(game),
            "{\n"
            "  \"format\": \"quetzal.stratum_game\",\n"
            "  \"version\": \"0.4\",\n"
            "  \"game\": { \"notation\": \"wall\", \"next_id\": 21, \"seed\": \"20260928\", \"random_engines\": { \"gameplay\": \"1 2 3\", \"generation\": \"4 5 6\" } },\n"
            "  \"settings\": { \"gameplay.difficulty\": \"custom\" },\n"
            "  \"presets\": { \"gameplay.difficulty\": { \"custom\": { \"gameplay.difficulty.denizen_count_multiplier\": \"2\" } } }\n"
            "}\n");

        istringstream game_settings_text(written(game));
        stratum_format::GameDocument game_read_back = stratum_format::read_game(game_settings_text, "game");
        CHECK_CASE("and reads back with its settings and presets as text", game_read_back.settings_snapshot.settings.value("gameplay.difficulty")
            == "custom" && game_read_back.settings_snapshot.presets.at(custom_key).value("gameplay.difficulty.denizen_count_multiplier") == "2");

        game.settings_snapshot = {};
        CHECK_CASE("absent settings and presets are written as empty objects", written(game).find("\"settings\": { },\n  \"presets\": { }\n")
            != string::npos);

        stratum_format::CharacterDocument character;
        character.name = "Ailis";
        character.id = 1;
        character.components = stratum_format::raw_values_type::object();
        character.components["name"] = "Ailis";
        character.components["speed"] = 100;
        character.zone_id = 3;
        character.placement = stratum_format::discrete_directed_point_type({3, 1}, {0, 1});

        report_written("a character file has its name third, and with no inventory has an empty one", written(character),
            "{\n"
            "  \"format\": \"quetzal.stratum_game\",\n"
            "  \"version\": \"0.4\",\n"
            "  \"name\": \"Ailis\",\n"
            "  \"character\": {\n"
            "    \"id\": 1,\n"
            "    \"components\": { \"name\": \"Ailis\", \"speed\": 100 },\n"
            "    \"zone\": 3,\n"
            "    \"placement\": { \"cell_position\": [3, 1], \"orientation\": [0, 1] },\n"
            "    \"inventory\": []\n"
            "  }\n"
            "}\n");

        stratum_format::Entity torch;
        torch.id = 20;
        torch.type = "torch";
        torch.parent_id = 1;
        torch.site = stratum_format::Site {stratum_format::Relation::On, {}, {}, {}};
        character.inventory.push_back(torch);

        report_written("a character file has each entity of its inventory on a line of its own", written(character),
            "{\n"
            "  \"format\": \"quetzal.stratum_game\",\n"
            "  \"version\": \"0.4\",\n"
            "  \"name\": \"Ailis\",\n"
            "  \"character\": {\n"
            "    \"id\": 1,\n"
            "    \"components\": { \"name\": \"Ailis\", \"speed\": 100 },\n"
            "    \"zone\": 3,\n"
            "    \"placement\": { \"cell_position\": [3, 1], \"orientation\": [0, 1] },\n"
            "    \"inventory\": [\n"
            "      { \"id\": 20, \"type\": \"torch\", \"components\": { \"parent\": { \"entity\": 1 }, \"site\": { \"relation\": \"on\" } } }\n"
            "    ]\n"
            "  }\n"
            "}\n");

        stratum_format::WorldDocument::Region surface;
        surface.id = 2;
        surface.name = "surface";
        surface.components = stratum_format::raw_values_type::object();
        surface.components["type"] = "surface";
        surface.spec.store.insert("type", "surface");
        surface.spec.zone_size = stratum_format::discrete_vector_type(40, 30);
        surface.zones = {{5, "zones/5.zone", stratum_format::zone_position_type {0, 0, 0}, "surface_0_0"}};

        stratum_format::WorldDocument::Region dungeon;
        dungeon.id = 4;
        dungeon.name = "barrow";
        dungeon.name_counter = 1;
        dungeon.spec.store.insert("type", "dungeon");
        dungeon.zones = {{3, "zones/3.zone", stratum_format::zone_position_type {0, 0, -1}, "Level -1"}, {6, "zones/6.zone", nullopt, "barrow"},
            {8, "zones/8.zone", nullopt, "barrow_1"}};

        stratum_format::WorldDocument::Region crypt;
        crypt.id = 9;
        crypt.parent_id = 4;
        crypt.name = "crypt";
        crypt.display_name = "The Crypt";
        crypt.spec.store.insert("type", "crypt");
        crypt.zones = {{11, "zones/11.zone", stratum_format::zone_position_type {0, 0, -2}, "crypt_1"}};

        stratum_format::WorldDocument world;
        world.name_counter = 2;
        world.worldspaces = {{1, "surface", 11, {surface}}, {7, "barrow", 18446744073709551615u, {dungeon, crypt}}};
        world.entity_zones = {{10, 5}, {12, 3}};

        report_written("a world file has its name counter, then opens each worldspace with its id, name and seed, then a line per region, which "
            "opens with its id, parent, name, display name, name counter and components, then its spec, then a line per zone, and has a line per "
            "entry of the world index",
            written(world),
            "{\n"
            "  \"format\": \"quetzal.stratum_game\",\n"
            "  \"version\": \"0.4\",\n"
            "  \"world\": {\n"
            "    \"name_counter\": 2,\n"
            "    \"worldspaces\": [\n"
            "      { \"id\": 1, \"name\": \"surface\", \"seed\": \"11\",\n"
            "        \"regions\": [\n"
            "          { \"id\": 2, \"name\": \"surface\", \"name_counter\": 0, \"components\": { \"type\": \"surface\" },\n"
            "            \"spec\": { \"zone_size\": [40, 30], \"type\": \"surface\" },\n"
            "            \"zones\": [\n"
            "              { \"id\": 5, \"name\": \"surface_0_0\", \"file\": \"zones/5.zone\", \"position\": [0, 0, 0] }\n"
            "            ] }\n"
            "        ] },\n"
            "      { \"id\": 7, \"name\": \"barrow\", \"seed\": \"18446744073709551615\",\n"
            "        \"regions\": [\n"
            "          { \"id\": 4, \"name\": \"barrow\", \"name_counter\": 1,\n"
            "            \"spec\": { \"type\": \"dungeon\" },\n"
            "            \"zones\": [\n"
            "              { \"id\": 3, \"name\": \"Level -1\", \"file\": \"zones/3.zone\", \"position\": [0, 0, -1] },\n"
            "              { \"id\": 6, \"name\": \"barrow\", \"file\": \"zones/6.zone\" },\n"
            "              { \"id\": 8, \"name\": \"barrow_1\", \"file\": \"zones/8.zone\" }\n"
            "            ] },\n"
            "          { \"id\": 9, \"parent\": 4, \"name\": \"crypt\", \"display_name\": \"The Crypt\", \"name_counter\": 0,\n"
            "            \"spec\": { \"type\": \"crypt\" },\n"
            "            \"zones\": [\n"
            "              { \"id\": 11, \"name\": \"crypt_1\", \"file\": \"zones/11.zone\", \"position\": [0, 0, -2] }\n"
            "            ] }\n"
            "        ] }\n"
            "    ],\n"
            "    \"entity_zones\": [\n"
            "      [10, 5],\n"
            "      [12, 3]\n"
            "    ]\n"
            "  }\n"
            "}\n");

        bool parsed = true;
        for (const string& text : {written(game), written(character), written(world)})
        {
            parsed = parsed && map_type::accept(text);
        }

        CHECK_CASE("each game file is JSON", parsed);

        filesystem::create_directories(out_directory);
        filesystem::path filepath = out_directory / "world.json";
        stratum_format::write(filepath, world);
        ifstream is(filepath, ios::binary);
        CHECK_CASE("a game file written to a file is identical to one written to a stream", string(istreambuf_iterator<char>(is), {}) == written(world));

        // Each document read back and written again is identical to the first writing
        auto rewritten = [&](const auto& document, const auto& read_back)
        {
            istringstream text(written(document));
            return written(read_back(text));
        };

        CHECK_CASE("a game file reads back as the document written", rewritten(game, [](istream& in) { return stratum_format::read_game(in, "game"); })
            == written(game));
        CHECK_CASE("a character file reads back as the document written, inventory included", rewritten(character,
            [](istream& in) { return stratum_format::read_character(in, "character"); }) == written(character));
        CHECK_CASE("a world file reads back as the document written, world index included", rewritten(world,
            [](istream& in) { return stratum_format::read_world(in, "world"); }) == written(world));
        CHECK_CASE("and from a file", written(stratum_format::read_world(filepath)) == written(world));

        // Each file with one change made to it, and the error the change raises
        auto rejected = [](const auto& read, const string& text, const string& error_expected)
        {
            try
            {
                istringstream in(text);
                read(in);
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        auto read_game = [](istream& in) { return stratum_format::read_game(in, "game"); };
        auto read_character = [](istream& in) { return stratum_format::read_character(in, "character"); };
        auto read_world = [](istream& in) { return stratum_format::read_world(in, "world"); };

        auto changed = [](string text, const string& from, const string& to)
        {
            size_t at = text.find(from);
            assert(at != string::npos);
            return text.replace(at, from.size(), to);
        };

        string game_text = written(game);
        string character_text = written(character);
        string world_text = written(world);

        CHECK_CASE("a game file of another format is rejected", rejected(read_game, changed(game_text, "quetzal.stratum_game",
            "quetzal.discrete_layout"), "format: 'quetzal.discrete_layout' is not this format"));
        CHECK_CASE("and one of another version", rejected(read_game, changed(game_text, "\"0.4\"", "\"0.3\""),
            "version: '0.3' is not a version this reads"));
        CHECK_CASE("a game file whose notation is not a notation is rejected", rejected(read_game, changed(game_text, "\"wall\"", "\"hex\""),
            "game.notation: 'hex' is not a notation"));
        CHECK_CASE("a game file with no next id is rejected", rejected(read_game, changed(game_text, "\"next_id\": 21, ", ""),
            "game.next_id: required and absent"));
        CHECK_CASE("and one with a random engine state that is not text", rejected(read_game, changed(game_text, "\"1 2 3\"", "123"),
            "game.random_engines.gameplay: a string was expected"));
        CHECK_CASE("an entity of the inventory with no parent is rejected", rejected(read_character, changed(character_text,
            "\"parent\": { \"entity\": 1 }, ", ""), "an entity of the inventory has a parent"));
        CHECK_CASE("and one whose parent is neither the character nor in the inventory", rejected(read_character, changed(character_text,
            "\"parent\": { \"entity\": 1 }", "\"parent\": { \"entity\": 7 }"), "entity 7 is neither the character nor an entity of the inventory"));
        CHECK_CASE("and one that is its own parent", rejected(read_character, changed(character_text, "\"parent\": { \"entity\": 1 }",
            "\"parent\": { \"entity\": 20 }"), "its parents form a cycle"));
        CHECK_CASE("and one with an address", rejected(read_character, changed(character_text, "\"relation\": \"on\"",
            "\"relation\": \"in\", \"cell_position\": [1, 1]"), "an entity of the inventory is sited on its parent, with no address"));
        CHECK_CASE("a character file whose name is not its name component's is rejected", rejected(read_character, changed(character_text,
            "\"name\": \"Ailis\",\n  \"character\"", "\"name\": \"Bran\",\n  \"character\""), "name: not the same as the character's name component"));

        auto read_name = [](istream& in) { return stratum_format::read_character_name(in, "character"); };
        istringstream name_text(character_text);
        CHECK_CASE("the character's name is read from the top of the file", read_name(name_text) == "Ailis");

        // Everything after the name is cut, so the file is no longer JSON past it
        string cut_text = character_text.substr(0, character_text.find("\"character\"")) + "{ this is not parsed";
        bool cut_read = false;

        try
        {
            istringstream cut(cut_text);
            cut_read = read_name(cut) == "Ailis";
        }
        catch (const runtime_error&)
        {
        }

        CHECK_CASE("and nothing after the name is parsed", cut_read);
        CHECK_CASE("a name read from a file of another format is rejected", rejected(read_name, changed(character_text, "quetzal.stratum_game",
            "quetzal.discrete_layout"), "format: 'quetzal.discrete_layout' is not what this reads"));
        CHECK_CASE("a world file with two zones of one id is rejected", rejected(read_world, changed(world_text, "\"id\": 6", "\"id\": 5"),
            "a second entity with the id 5"));
        CHECK_CASE("and one with two zones of a worldspace at one position", rejected(read_world, changed(world_text,
            "\"file\": \"zones/6.zone\"", "\"file\": \"zones/6.zone\", \"position\": [0, 0, -1]"), "a second zone of the worldspace at (0, 0, -1)"));
        CHECK_CASE("and one whose worldspace and region have one id", rejected(read_world, changed(world_text, "\"id\": 7", "\"id\": 4"),
            "a second entity with the id 4"));
        CHECK_CASE("and one whose zone has its worldspace's id", rejected(read_world, changed(world_text, "\"id\": 8", "\"id\": 7"),
            "a second entity with the id 7"));
        CHECK_CASE("and one with two zones of a region of one name", rejected(read_world, changed(world_text, "\"barrow_1\"", "\"barrow\""),
            "a second zone of the region named barrow"));
        CHECK_CASE("and one whose world index has an entity in a zone no region lists", rejected(read_world, changed(world_text, "[12, 3]",
            "[12, 4]"), "zone 4 is in no region"));
        CHECK_CASE("and one whose world index has an entity twice", rejected(read_world, changed(world_text, "[12, 3]", "[10, 3]"),
            "a second entry for entity 10"));
        CHECK_CASE("and one whose top-level region has a parent", rejected(read_world, changed(world_text, "{ \"id\": 4, ",
            "{ \"id\": 4, \"parent\": 2, "), "a top-level region with a parent"));
        CHECK_CASE("and one with a subregion whose parent is not an earlier region of its worldspace", rejected(read_world,
            changed(world_text, "\"parent\": 4", "\"parent\": 2"), "a subregion whose parent is not an earlier region of the worldspace"));
        CHECK_CASE("and one whose worldspace's seed is not a seed", rejected(read_world, changed(world_text, "\"seed\": \"11\"", "\"seed\": \"-11\""),
            "a seed was expected"));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("config file")
    {
        stratum_format::ConfigDocument config;
        config.assets_directory = "../../assets";
        config.saves_directory = "saves";
        config.logs_directory = "D:/logs";
        config.zone_count_max = 8;
        config.log_file_count_max = 5;

        ostringstream os;
        stratum_format::write(os, config);
        const string config_text = os.str();
        const string config_text_expected =
            "{\n"
            "  \"format\": \"quetzal.stratum_config\",\n"
            "  \"version\": \"0.1\",\n"
            "  \"assets_directory\": \"../../assets\",\n"
            "  \"saves_directory\": \"saves\",\n"
            "  \"logs_directory\": \"D:/logs\",\n"
            "  \"zone_count_max\": 8,\n"
            "  \"log_file_count_max\": 5\n"
            "}\n";
        CHECK_CASE("a config file has each value that is set on a line of its own", config_text == config_text_expected,
            format("expected\n{}got\n{}", config_text_expected, config_text));

        istringstream is(config_text);
        stratum_format::ConfigDocument read_back = stratum_format::read_config(is, "config");
        CHECK_CASE("and reads back as the document written, with the values not set absent", read_back.assets_directory == "../../assets"
            && read_back.saves_directory == "saves" && read_back.logs_directory == "D:/logs" && read_back.zone_count_max == 8
            && read_back.log_file_count_max == 5 && !read_back.user_directory && !read_back.working_directory && !read_back.cache_directory && !read_back.mods_directory
            && !read_back.user_settings_file);

        istringstream header_only("{ \"format\": \"quetzal.stratum_config\", \"version\": \"0.1\" }");
        stratum_format::ConfigDocument empty = stratum_format::read_config(header_only, "config");
        CHECK_CASE("a config file with only its header sets nothing", !empty.assets_directory && !empty.zone_count_max && !empty.log_file_count_max);

        auto rejected = [&config_text](const string& from, const string& to, const string& error_expected)
        {
            string text = config_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_config(in, "config");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a config file of another format is rejected", rejected("quetzal.stratum_config", "quetzal.stratum_game",
            "format: 'quetzal.stratum_game' is not this format"));
        CHECK_CASE("and one with a key the format does not have", rejected("\"saves_directory\"", "\"save_directory\"",
            "save_directory: not a key of the config file"));
        CHECK_CASE("and one whose zone count maximum is not a count", rejected("\"zone_count_max\": 8", "\"zone_count_max\": -1",
            "zone_count_max"));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("game manifest file")
    {
        const string manifest_text = "{ \"format\": \"quetzal.stratum_game_manifest\", \"version\": \"0.1\", \"notation\": \"wall\", "
            "\"views\": [\"cell\", \"lattice\"], \"view_menu\": true, \"default_view\": \"lattice\", "
            "\"settings\": { \"world.zone_size.x\": \"32\", \"world.zone_size.y\": \"12\" } }";

        istringstream is(manifest_text);
        stratum_format::GameManifestDocument manifest = stratum_format::read_game_manifest(is, "manifest");
        CHECK_CASE("a game manifest file has the notation, the views in order, the view menu, the default view, and the settings",
            manifest.notation == stratum_format::Notation::Wall && manifest.views == vector<string> {"cell", "lattice"} && manifest.view_menu
            && manifest.default_view == "lattice" && manifest.settings.value("world.zone_size.x") == "32"
            && manifest.settings.value("world.zone_size.y") == "12");

        istringstream without_settings_is(manifest_text.substr(0, manifest_text.find(", \"settings\"")) + " }");
        CHECK_CASE("and the settings may be absent", stratum_format::read_game_manifest(without_settings_is, "manifest").settings.empty());

        auto rejected = [&manifest_text](const string& from, const string& to, const string& error_expected)
        {
            string text = manifest_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_game_manifest(in, "manifest");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a game manifest file whose notation is not a notation is rejected", rejected("\"wall\"", "\"hex\"",
            "notation: 'hex' is not a notation"));
        CHECK_CASE("and one with a key the format does not have", rejected("\"view_menu\"", "\"menu\"", "menu: not a key of the game manifest file"));
        CHECK_CASE("and one that lists no view", rejected("[\"cell\", \"lattice\"]", "[]", "views: lists no view"));
        CHECK_CASE("and one that lists a view twice", rejected("[\"cell\", \"lattice\"]", "[\"cell\", \"cell\"]",
            "views[1]: 'cell' is listed more than once"));
        CHECK_CASE("and one whose default view is not one of its views", rejected("\"default_view\": \"lattice\"", "\"default_view\": \"tile\"",
            "default_view: 'tile' is not one of the views"));
        CHECK_CASE("and one with no view menu", rejected("\"view_menu\": true, ", "", "view_menu: required and absent"));
        CHECK_CASE("and one with a setting value that is not text", rejected("\"32\"", "32", "settings.world.zone_size.x"));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("settings file")
    {
        stratum_format::SettingsDocument user_settings;
        const quetzal::settings::Settings::preset_key_type custom_key {"video.quality", "custom"};
        user_settings.snapshot.settings.insert("video.fullscreen", "true");
        user_settings.snapshot.settings.insert("video.quality", "custom");
        user_settings.snapshot.presets.emplace(custom_key, quetzal::KeyValueStore {{"video.shadows", "high"}, {"video.textures", "low"}});

        ostringstream os;
        stratum_format::write(os, user_settings);
        const string user_settings_text = os.str();
        const string user_settings_text_expected =
            "{\n"
            "  \"format\": \"quetzal.stratum_settings\",\n"
            "  \"version\": \"0.1\",\n"
            "  \"settings\": { \"video.fullscreen\": \"true\", \"video.quality\": \"custom\" },\n"
            "  \"presets\": { \"video.quality\": { \"custom\": { \"video.shadows\": \"high\", \"video.textures\": \"low\" } } }\n"
            "}\n";
        CHECK_CASE("a settings file has the settings and the presets each on a line of its own", user_settings_text == user_settings_text_expected,
            format("expected\n{}got\n{}", user_settings_text_expected, user_settings_text));

        istringstream is(user_settings_text);
        stratum_format::SettingsDocument read_back = stratum_format::read_settings(is, "settings");
        CHECK_CASE("and reads back as the document written", read_back.snapshot.settings.value("video.fullscreen") == "true"
            && read_back.snapshot.settings.value("video.quality") == "custom" && read_back.snapshot.presets.size() == 1
            && read_back.snapshot.presets.at(custom_key).value("video.textures") == "low");

        istringstream header_only("{ \"format\": \"quetzal.stratum_settings\", \"version\": \"0.1\" }");
        stratum_format::SettingsDocument empty = stratum_format::read_settings(header_only, "settings");
        CHECK_CASE("a settings file with only its header sets nothing", empty.snapshot.settings.empty()
            && empty.snapshot.presets.empty());

        auto rejected = [&user_settings_text](const string& from, const string& to, const string& error_expected)
        {
            string text = user_settings_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_settings(in, "settings");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a settings file with a key the format does not have is rejected", rejected("\"presets\"", "\"groups\"",
            "groups: not a key of the settings file"));
        CHECK_CASE("and one with a value that is not text", rejected("\"true\"", "true", "settings.video.fullscreen: a string was expected"));
        CHECK_CASE("and one with a preset's value that is not text", rejected("\"low\"", "1",
            "presets.video.quality.custom.video.textures: a string was expected"));
        CHECK_CASE("and one with a group's presets that are not an object", rejected("{ \"custom\": { \"video.shadows\": \"high\", \"video.textures\": \"low\" } }",
            "1", "presets.video.quality: an object was expected"));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("setting definition file")
    {
        using document_type = stratum_format::SettingDefinitionsDocument;

        const string definitions_text =
            "{\n"
            "  \"format\": \"quetzal.stratum_setting_definitions\",\n"
            "  \"version\": \"0.1\",\n"
            "  \"enumerations\": [ { \"name\": \"mod.torches.style\", \"values\": [\"steady\", \"flicker\"] } ],\n"
            "  \"settings\": [\n"
            "    { \"key\": \"video.fullscreen\", \"type\": \"boolean\", \"default\": \"false\", \"scope\": \"user\", \"restart_required\": true,\n"
            "      \"display_name\": \"Fullscreen\", \"category\": \"video\" },\n"
            "    { \"key\": \"gameplay.count\", \"type\": \"real\", \"default\": \"1\", \"scope\": \"game\", \"minimum\": \"0.5\", \"maximum\": \"2\",\n"
            "      \"modifiable\": true, \"description\": \"How many\" }\n"
            "  ],\n"
            "  \"groups\": [\n"
            "    { \"key\": \"gameplay.difficulty\", \"scope\": \"game\", \"modifiable\": true, \"default_preset\": \"normal\", \"display_name\": \"Difficulty\",\n"
            "      \"members\": [\"gameplay.count\"],\n"
            "      \"presets\": [ { \"name\": \"normal\", \"display_name\": \"Normal\", \"values\": { \"gameplay.count\": \"1\" } },\n"
            "        { \"name\": \"custom\", \"modifiable\": true } ] }\n"
            "  ]\n"
            "}\n";

        istringstream is(definitions_text);
        document_type document = stratum_format::read_setting_definitions(is, "definitions");
        CHECK_CASE("a setting definition file reads its enumerations, settings and groups in order", document.enumerations.size() == 1
            && document.enumerations[0].name == "mod.torches.style" && document.enumerations[0].values == vector<string> {"steady", "flicker"}
            && document.settings.size() == 2 && document.settings[0].key == "video.fullscreen" && document.settings[1].key == "gameplay.count"
            && document.groups.size() == 1 && document.groups[0].presets.size() == 2);

        const document_type::Setting& fullscreen = document.settings[0];
        const document_type::Setting& count = document.settings[1];
        CHECK_CASE("a setting has its fields, with those absent empty or false", fullscreen.type_name == "boolean" && fullscreen.default_value == "false"
            && fullscreen.scope == document_type::Scope::User && fullscreen.restart_required && fullscreen.display_name == "Fullscreen"
            && fullscreen.category == "video" && fullscreen.description.empty() && !fullscreen.minimum && count.scope == document_type::Scope::Game
            && count.default_value == "1" && !count.restart_required && count.minimum == "0.5" && count.maximum == "2" && count.description == "How many");
        CHECK_CASE("and a game setting may be modifiable", count.modifiable && !fullscreen.modifiable);

        const document_type::Group& difficulty = document.groups[0];
        CHECK_CASE("a group has its fields, members and presets", difficulty.key == "gameplay.difficulty"
            && difficulty.scope == document_type::Scope::Game && difficulty.modifiable && difficulty.member_keys == vector<string> {"gameplay.count"}
            && difficulty.default_preset_name == "normal" && difficulty.display_name == "Difficulty" && difficulty.presets[0].name == "normal"
            && difficulty.presets[0].values.value("gameplay.count") == "1" && !difficulty.presets[0].modifiable && difficulty.presets[1].modifiable
            && difficulty.presets[1].values.empty());

        istringstream header_only("{ \"format\": \"quetzal.stratum_setting_definitions\", \"version\": \"0.1\" }");
        document_type empty = stratum_format::read_setting_definitions(header_only, "definitions");
        CHECK_CASE("a setting definition file with only its header defines nothing", empty.enumerations.empty() && empty.settings.empty()
            && empty.groups.empty());

        auto rejected = [&definitions_text](const string& from, const string& to, const string& error_expected)
        {
            string text = definitions_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_setting_definitions(in, "definitions");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a setting definition file with a key the format does not have is rejected", rejected("\"restart_required\"", "\"restart\"",
            "settings[0].restart: not a key of the setting definition file"));
        CHECK_CASE("and one with a scope that is not user or game", rejected("\"scope\": \"user\"", "\"scope\": \"player\"",
            "settings[0].scope: 'player' is not a scope"));
        CHECK_CASE("and one with a value that is not text", rejected("\"minimum\": \"0.5\"", "\"minimum\": 0.5",
            "settings[1].minimum: a string was expected"));
        CHECK_CASE("and one with a setting that has no type", rejected("\"type\": \"real\", ", "", "settings[1].type: required and absent"));
        CHECK_CASE("and one with a setting that has no default", rejected("\"default\": \"1\", ", "", "settings[1].default: required and absent"));
        CHECK_CASE("and one with a group that has no presets", rejected("\"presets\"", "\"options\"", "groups[0].options: not a key"));
        CHECK_CASE("and one with a group that has no members", rejected("      \"members\": [\"gameplay.count\"],\n", "",
            "groups[0].members: required and absent"));
        CHECK_CASE("and one with a member that is not text", rejected("[\"gameplay.count\"]", "[1]", "groups[0].members[0]: a string was expected"));

        document_type game = stratum_format::read_setting_definitions(filesystem::path("../../assets") / document_type::filepath);
        CHECK_CASE("the game's setting definition file reads", !game.settings.empty() && game.groups.size() == 1
            && game.groups[0].key == "gameplay.difficulty" && game.groups[0].modifiable);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("region spec file")
    {
        using document_type = stratum_format::RegionSpecsDocument;

        const string region_specs_text =
            "{\n"
            "  \"format\": \"quetzal.stratum_region_specs\",\n"
            "  \"version\": \"0.1\",\n"
            "  \"region_specs\": {\n"
            "    \"surface\": { \"zone.name\":\"&region_name .. '_' .. x .. '_' .. y\", \"seed\": \"42\",\n"
            "      \"extent\": { \"arrangement\": \"ranges\", \"boxes\": [ { \"x\": [0, 9], \"y\": [0, 9], \"z\": 0 } ] }, \"zone_size\": [64, 32] },\n"
            "    \"islands\": { \"parent\": \"surface\", \"extent\": { \"arrangement\": \"positions\", \"positions\": [[3, 3, 0], [4, 4, 0]] } },\n"
            "    \"crypt\": {}\n"
            "  }\n"
            "}\n";

        istringstream is(region_specs_text);
        document_type document = stratum_format::read_region_specs(is, "region specs");
        const stratum_format::RegionSpec& surface = document.region_specs.at("surface");
        const stratum_format::RegionSpec& islands = document.region_specs.at("islands");
        const stratum_format::RegionSpec& crypt = document.region_specs.at("crypt");
        auto range_is = [](const optional<math::Interval<stratum_format::coord_type>>& range, stratum_format::coord_type lower,
            stratum_format::coord_type upper)
        {
            return range.has_value() && range->lower() == lower && range->upper() == upper;
        };

        CHECK_CASE("a region spec file reads each region spec by its name, its store as text", document.region_specs.size() == 3
            && surface.store.value("zone.name") == "&region_name .. '_' .. x .. '_' .. y" && surface.store.size() == 1);
        CHECK_CASE("and its members with a type of their own", surface.seed == 42u && surface.extent.has_value()
            && surface.extent->boxes.size() == 1 && range_is(surface.extent->boxes[0].x_extent, 0, 9) && range_is(surface.extent->boxes[0].z_extent, 0, 0)
            && surface.zone_size == stratum_format::discrete_vector_type(64, 32) && islands.parent == "surface"
            && islands.extent.has_value() && islands.extent->arrangement == stratum_format::RegionExtent::Arrangement::Positions
            && islands.extent->positions.size() == 2 && islands.extent->positions[1] == stratum_format::zone_position_type {4, 4, 0});
        CHECK_CASE("and a spec with neither", crypt.store.empty() && !crypt.name.has_value() && !crypt.extent.has_value()
            && !crypt.zone_size.has_value());

        auto rejected = [&region_specs_text](const string& from, const string& to, const string& error_expected)
        {
            string text = region_specs_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_region_specs(in, "region specs");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a region spec file with a key the format does not have is rejected", rejected("\"region_specs\"", "\"regions\"",
            "regions: not a key of the region spec file"));
        CHECK_CASE("and one with a value that is not text", rejected("\"&region_name .. '_' .. x .. '_' .. y\"", "9",
            "region_specs.surface.zone.name: a string was expected"));
        CHECK_CASE("and one with a region spec that is not an object", rejected("{}", "[]", "region_specs.crypt: an object was expected"));
        CHECK_CASE("and one with a key under a member's name", rejected("\"seed\": \"42\"", "\"zone_size.x\": \"64\"",
            "region_specs.surface.zone_size.x: a part of zone_size, written in its value"));
        CHECK_CASE("and one with a seed that is not a seed", rejected("\"42\"", "\"forty\"", "region_specs.surface.seed: a seed was expected"));
        CHECK_CASE("and one with an arrangement that is neither", rejected("\"positions\", \"positions\"", "\"grid\", \"positions\"",
            "region_specs.islands.extent.arrangement: ranges or positions was expected"));
        CHECK_CASE("and one with a member the arrangement does not have", rejected("\"boxes\"", "\"positions\"",
            "region_specs.surface.extent.positions: not a member of a ranges extent"));
        CHECK_CASE("and one with a range whose lower bound is above its upper bound", rejected("[0, 9], \"y\"", "[9, 0], \"y\"",
            "region_specs.surface.extent.boxes[0].x: a range whose lower bound is above its upper bound"));

        document_type game = stratum_format::read_region_specs(filesystem::path("../../assets") / document_type::filepath);
        CHECK_CASE("the game's region spec file reads", game.region_specs.contains("surface") && game.region_specs.contains("dungeon"));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("world definition file")
    {
        using document_type = stratum_format::WorldDefinitionDocument;

        const string world_definition_text =
            "{\n"
            "  \"format\": \"quetzal.stratum_world_definition\",\n"
            "  \"version\": \"0.1\",\n"
            "  \"starting_region\": \"barrow\",\n"
            "  \"region_specs\": [\n"
            "    [\n"
            "      { \"name\": \"barrow\", \"region_spec\": \"dungeon\", \"extent\": { \"arrangement\": \"ranges\", "
            "\"boxes\": [ { \"x\": 0, \"y\": 0, \"z\": [-10, -1] } ] }, \"zone_size\": [64, 32], \"difficulty\": \"3\" },\n"
            "      { \"name\": \"barrow_crypt\", \"parent\": \"barrow\", \"region_spec\": \"crypt\", \"extent\": { \"arrangement\": \"positions\", "
            "\"positions\": [[0, 0, -5], [0, 0, -6]] } }\n"
            "    ],\n"
            "    []\n"
            "  ]\n"
            "}\n";

        istringstream is(world_definition_text);
        document_type document = stratum_format::read_world_definition(is, "world definition");
        CHECK_CASE("a world definition file reads its starting region, and each tree's region specs in order",
            document.starting_region == "barrow" && document.region_specs.size() == 2
            && document.region_specs[0].size() == 2 && document.region_specs[0][0].name == "barrow"
            && document.region_specs[0][0].store.value("difficulty") == "3" && document.region_specs[0][1].parent == "barrow"
            && document.region_specs[1].empty());

        ostringstream os;
        stratum_format::write(os, document);
        CHECK_CASE("and writes the text it was read from, members with a type of their own before the store", os.str() == world_definition_text);

        auto rejected = [&world_definition_text](const string& from, const string& to, const string& error_expected)
        {
            string text = world_definition_text;
            size_t at = text.find(from);
            assert(at != string::npos);
            text.replace(at, from.size(), to);

            try
            {
                istringstream in(text);
                stratum_format::read_world_definition(in, "world definition");
            }
            catch (const runtime_error& e)
            {
                return string(e.what()).find(error_expected) != string::npos;
            }

            return false;
        };

        CHECK_CASE("a world definition file with a key the format does not have is rejected", rejected("\"region_specs\"", "\"trees\"",
            "trees: not a key of the world definition file"));
        CHECK_CASE("and one with no starting region", rejected("  \"starting_region\": \"barrow\",\n", "",
            "starting_region: required and absent"));
        CHECK_CASE("and one with a tree that is not a list", rejected("[]\n", "{}\n", "region_specs[1]: an array was expected"));
        CHECK_CASE("and one with a region spec that is not an object", rejected("[]\n", "[3]\n", "region_specs[1][0]: an object was expected"));
        CHECK_CASE("and one with a value that is not text", rejected("\"crypt\"", "3", "region_specs[0][1].region_spec: a string was expected"));

        const filesystem::path game_filepath = filesystem::path("../../assets") / document_type::filepath;
        document_type game = stratum_format::read_world_definition(game_filepath);
        CHECK_CASE("the game's world definition file reads, starting in the surface", game.starting_region == "surface"
            && !game.region_specs.empty());

        ifstream game_file(game_filepath, ios::binary);
        ostringstream game_text;
        game_text << game_file.rdbuf();
        ostringstream game_written;
        stratum_format::write(game_written, game);
        CHECK_CASE("and is in the written form", game_written.str() == game_text.str());
        return;
    }

} // namespace
