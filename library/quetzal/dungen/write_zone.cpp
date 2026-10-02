//------------------------------------------------------------------------------
// dungen
// write_zone.cpp
//------------------------------------------------------------------------------

#include "write_zone.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/stratum_format/grid.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
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
    using stored_entity_id_type = quetzal::stratum_format::stored_entity_id_type;
    using raw_values_type = quetzal::stratum_format::raw_values_type;

    //--------------------------------------------------------------------------
    // Formats a parameter as the token its command line option takes
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
    void add_maze1(raw_values_type& values, const Parameters& parameters)
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
    void add_grid1(raw_values_type& values, const Parameters& parameters)
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
    void add_cave1(raw_values_type& values, const Parameters& parameters)
    {
        const auto& cave1 = parameters.cave1;

        values["fill"] = token(cave1.fill);
        values["smoothing_passes"] = token(cave1.smoothing_passes);
        values["birth_threshold"] = token(cave1.birth_threshold);
        values["survival_threshold"] = token(cave1.survival_threshold);
        return;
    }

    //--------------------------------------------------------------------------
    // Writes the Zone's own extent rather than the requested one, since an algorithm may round the requested extent to one its
    // layout can use
    raw_values_type generator(const Zone& zone, const Parameters& parameters)
    {
        raw_values_type values = raw_values_type::object();
        values["algorithm"] = token(parameters.algorithm);
        values["extent"] = token(zone.extent());
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

        if (parameters.level.has_value())
        {
            values["level"] = token(*parameters.level);
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
            case CellType::Staircase: return '>';
        }

        assert(false);
        return '#';
    }

    //--------------------------------------------------------------------------
    bool floor(CellType type)
    {
        return type == CellType::Room || type == CellType::Corridor;
    }

    //--------------------------------------------------------------------------
    id_type room_of(const Cell& cell)
    {
        return cell.type() == CellType::Room ? cell.room_id() : nullid;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::dungen::make_document(const Zone& zone, const Parameters& parameters)
{
    auto [nx, ny] = zone.extent().rep();

    assert(!zone.staircases().empty());

    stratum_format::Document document;
    document.version = stratum_format::format_version;
    document.generator = generator(zone, parameters);

    if (!zone.name().empty())
    {
        document.components["name"] = zone.name();
    }

    document.level = parameters.level;
    document.cells.notation = stratum_format::Notation::Block;
    document.cells.extent = zone.extent();

    for (coord_type y = ny - 1; y >= 0; --y)
    {
        string row;
        for (coord_type x = 0; x < nx; ++x)
        {
            row += glyph(zone.cell({x, y}).type());
        }

        document.cells.rows.push_back(std::move(row));
    }

    // A staircase cell's type does not encode which way it leads; the Staircase does
    for (const auto& staircase : zone.staircases())
    {
        if (staircase.rise() == Rise::Up)
        {
            point_type position = staircase.position();
            document.cells.rows[static_cast<size_t>(ny - 1 - position.y())][static_cast<size_t>(position.x())] = '<';
        }
    }

    // Room floor lies against corridor floor with nothing between, so an edge between two floor cells of different rooms,
    // or of a room and none, is a barrier
    vector<optional<point_type>> locations(zone.rooms().size());
    const vector_type east {1, 0};
    const vector_type north {0, 1};

    for (coord_type y = 0; y < ny; ++y)
    {
        for (coord_type x = 0; x < nx; ++x)
        {
            const Cell& cell = zone.cell({x, y});
            if (!floor(cell.type()))
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
                if (zone.cells().contains(neighbor) && floor(zone.cell(neighbor).type()) && room_of(zone.cell(neighbor)) != room_id)
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

    // Local ids are assigned in the order written, so the same parameters give the same ids: each door, then its door
    // facings in the order the reader finds the walls for its glyph, and the staircases last
    stored_entity_id_type id = 0;

    for (const auto& walls : stratum_format::door_walls(document.cells))
    {
        stored_entity_id_type door_id = ++id;
        document.entities.push_back({door_id, "door", nullopt, nullopt, {}});

        for (const auto& address : walls)
        {
            document.entities.push_back({++id, "door_facing", stratum_format::Site {stratum_format::Relation::On, address, nullopt, nullopt}, door_id, {}});
        }
    }

    size_t staircase_count = 0;

    for (const auto& staircase : zone.staircases())
    {
        // The staircase's cell is a block, and it is sited on the wall its open neighbor presents to that block
        const vector_type& open_side = staircase.direction();
        stratum_format::WallAddress address {staircase.position() + open_side, open_side};

        // An arriving actor is placed on the floor it opens onto, facing away from it
        stratum_format::RelocatorTarget target {std::nullopt, stratum_format::RelativeZone {staircase.rise() == Rise::Down ? -1 : 1}, std::nullopt};
        stratum_format::Relocator relocator {target, "staircase", staircase_count++,
            stratum_format::discrete_directed_point_type(address.cell_position, open_side)};

        document.access_points.push_back(++id);
        document.entities.push_back({id, "staircase", stratum_format::Site {stratum_format::Relation::On, address, nullopt, nullopt}, nullopt, std::move(relocator), {}});
    }

    return document;
}

//------------------------------------------------------------------------------
void quetzal::dungen::write_zone(ostream& os, const Zone& zone, const Parameters& parameters)
{
    stratum_format::write(os, make_document(zone, parameters));
    return;
}
