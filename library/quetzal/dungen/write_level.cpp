//------------------------------------------------------------------------------
// dungen
// write_level.cpp
//------------------------------------------------------------------------------

#include "write_level.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/write.hpp"
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;
    using quetzal::id_type;
    using quetzal::nullid;
    using values_type = quetzal::stratum_format::values_type;

    //--------------------------------------------------------------------------
    // A parameter as the token its option takes
    template<typename T>
    string token(const T& value)
    {
        ostringstream os;
        os << quetzal::element_delim(",") << value;
        return os.str();
    }

    //--------------------------------------------------------------------------
    string interval_token(const quetzal::math::Interval<size_t>& interval)
    {
        return token(interval.lower()) + "-" + token(interval.upper());
    }

    //--------------------------------------------------------------------------
    void add_maze1(values_type& values, const Parameters& parameters)
    {
        const auto& maze1 = parameters.maze1;

        values["room_dimension_base"] = token(maze1.room_dimension_base);
        values["room_dimension_variation"] = token(maze1.room_dimension_variation);
        values["room_density"] = token(maze1.room_density);
        values["room_merge"] = maze1.room_merge ? "true" : "false";
        values["corridor_straightness"] = token(maze1.corridor_straightness);
        values["probability_dead_end_removal"] = token(maze1.probability_dead_end_removal);
        values["probability_loop_removal"] = token(maze1.probability_loop_removal);
        values["door_probability"] = token(maze1.door_probability);
        return;
    }

    //--------------------------------------------------------------------------
    void add_grid1(values_type& values, const Parameters& parameters)
    {
        const auto& grid1 = parameters.grid1;

        values["divisions"] = token(grid1.divisions);
        values["room_density"] = token(grid1.room_density);
        values["room_dimension_min"] = token(grid1.room_dimension_min);
        values["room_margin"] = token(grid1.room_margin);
        values["loop"] = token(grid1.loop);
        values["door_probability"] = token(grid1.door_probability);
        return;
    }

    //--------------------------------------------------------------------------
    void add_cave1(values_type& values, const Parameters& parameters)
    {
        const auto& cave1 = parameters.cave1;

        values["fill"] = token(cave1.fill);
        values["smoothing_passes"] = token(cave1.smoothing_passes);
        values["birth_threshold"] = token(cave1.birth_threshold);
        values["survival_threshold"] = token(cave1.survival_threshold);
        return;
    }

    //--------------------------------------------------------------------------
    // The level's own size rather than the requested one, an algorithm being free to round what it was given to something
    // its layout can use
    values_type generator(const Level& level, const Parameters& parameters)
    {
        values_type values = values_type::object();
        values["algorithm"] = token(parameters.algorithm);
        values["size"] = token(level.size());
        values["stairs_down"] = interval_token(parameters.staircase_down_count);
        values["stairs_up"] = interval_token(parameters.staircase_up_count);

        switch (parameters.algorithm)
        {
            case Algorithm::Maze1:
                add_maze1(values, parameters);
                break;

            case Algorithm::Grid1:
                add_grid1(values, parameters);
                break;

            case Algorithm::Cave1:
                add_cave1(values, parameters);
                break;

            default:
                assert(false);
        }

        values["seed"] = token(parameters.seed.value());
        return {{"name", "dungen"}, {"parameters", values}};
    }

    //--------------------------------------------------------------------------
    char glyph(CellType type)
    {
        switch (type)
        {
            case CellType::Null:
            case CellType::Solid: return '#';
            case CellType::Room:
            case CellType::Corridor: return '.';
            case CellType::Door: return '+';
            case CellType::Stair: return '>';
        }

        assert(false);
        return '#';
    }

    //--------------------------------------------------------------------------
    bool floor_stair(CellType type)
    {
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Stair;
    }

    //--------------------------------------------------------------------------
    id_type room_of(const Cell& cell)
    {
        return cell.type() == CellType::Room || cell.type() == CellType::Stair ? cell.room_id() : nullid;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::dungen::write_level(ostream& os, const Level& level, const Parameters& parameters)
{
    auto [nx, ny] = level.size().rep();

    // Every staircase is an entry, the caller choosing among them by what it arrives from
    assert(!level.staircases().empty());
    values_type entry = values_type::array();

    for (const auto& staircase : level.staircases())
    {
        entry.push_back(values_type::array({staircase.position().x(), staircase.position().y()}));
    }

    stratum_format::Document document;
    document.generator = generator(level, parameters);
    document.components = values_type::object();
    document.components["entry"] = std::move(entry);
    document.cells.notation = stratum_format::Notation::Block;
    document.cells.extent = level.size();

    for (coord_type y = ny - 1; y >= 0; --y)
    {
        string row;
        for (coord_type x = 0; x < nx; ++x)
        {
            row += glyph(level.cell({x, y}).type());
        }

        document.cells.rows.push_back(std::move(row));
    }

    // A staircase cell's type does not say which way it leads; its record does
    for (const auto& staircase : level.staircases())
    {
        if (staircase.direction() == StaircaseDirection::Up)
        {
            point_type position = staircase.position();
            document.cells.rows[static_cast<size_t>(ny - 1 - position.y())][static_cast<size_t>(position.x())] = '<';
        }
    }

    // Room floor lies against corridor floor with nothing between, so an edge between two floor/stair cells of different
    // rooms, or of a room and none, is a barrier
    vector<optional<point_type>> locations(level.rooms().size());
    const vector_type east {1, 0};
    const vector_type north {0, 1};

    for (coord_type y = 0; y < ny; ++y)
    {
        for (coord_type x = 0; x < nx; ++x)
        {
            const Cell& cell = level.cell({x, y});
            if (!floor_stair(cell.type()))
            {
                continue;
            }

            id_type room_id = room_of(cell);
            if (room_id != nullid && !locations[room_id].has_value())
            {
                locations[room_id] = point_type {x, y};
            }

            for (const vector_type& direction : {east, north})
            {
                point_type neighbor = point_type {x, y} + direction;
                if (level.cells().contains(neighbor) && floor_stair(level.cell(neighbor).type()) && room_of(level.cell(neighbor)) != room_id)
                {
                    document.barriers.push_back({{x, y}, -direction});
                }
            }
        }
    }

    for (const optional<point_type>& location : locations)
    {
        assert(location.has_value());
        document.rooms.push_back({stratum_format::RoomLocator {*location, {}}, {}});
    }

    // The walls the reader finds for each door glyph, so the entity is at the placement it reads the door at
    for (auto& walls : stratum_format::door_walls(document.cells))
    {
        values_type entity = values_type::object();
        entity["type"] = "door";

        document.attachments.push_back({std::move(walls), nullopt, values_type::array({std::move(entity)})});
    }

    for (const auto& staircase : level.staircases())
    {
        values_type relocator = values_type::object();
        relocator["target"] = staircase.direction() == StaircaseDirection::Down ? "level_down" : "level_up";

        values_type components = values_type::object();
        if (staircase.facing().has_value())
        {
            components["facing"] = values_type::array({staircase.facing()->x(), staircase.facing()->y()});
        }

        components["relocator"] = std::move(relocator);

        values_type entity = values_type::object();
        entity["type"] = "staircase";
        entity["components"] = std::move(components);

        document.attachments.push_back({stratum_format::FlatCell {stratum_format::FlatRole::Floor, staircase.position()}, nullopt,
            values_type::array({std::move(entity)})});
    }

    stratum_format::write(os, document);
    return;
}
