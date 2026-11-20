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

    //--------------------------------------------------------------------------
    // Formats a parameter as command line token
    template<typename T>
    string token(const T& value)
    {
        ostringstream os;
        os << quetzal::separator_delim(",") << value;
        return os.str();
    }

    //--------------------------------------------------------------------------
    string interval_token(const quetzal::math::Interval<size_t>& interval)
    {
        return token(interval.lower()) + "-" + token(interval.upper());
    }

    //--------------------------------------------------------------------------
    void add_maze1(quetzal::KeyValueStore& values, const Parameters& parameters)
    {
        const auto& maze1 = parameters.maze1;

        values.insert("room_dimension_base", token(maze1.room_dimension_base));
        values.insert("room_dimension_variation", token(maze1.room_dimension_variation));
        values.insert("room_density", token(maze1.room_density));
        values.insert("room_merge", maze1.room_merge ? "true" : "false");
        values.insert("corridor_straightness", token(maze1.corridor_straightness));
        values.insert("probability_dead_end_removal", token(maze1.probability_dead_end_removal));
        values.insert("probability_loop_removal", token(maze1.probability_loop_removal));
        values.insert("door_probability", token(maze1.door_probability));
        return;
    }

    //--------------------------------------------------------------------------
    void add_grid1(quetzal::KeyValueStore& values, const Parameters& parameters)
    {
        const auto& grid1 = parameters.grid1;

        values.insert("divisions", token(grid1.divisions));
        values.insert("room_density", token(grid1.room_density));
        values.insert("room_dimension_min", token(grid1.room_dimension_min));
        values.insert("room_margin", token(grid1.room_margin));
        values.insert("loop", token(grid1.loop));
        values.insert("door_probability", token(grid1.door_probability));
        return;
    }

    //--------------------------------------------------------------------------
    void add_cave1(quetzal::KeyValueStore& values, const Parameters& parameters)
    {
        const auto& cave1 = parameters.cave1;

        values.insert("fill", token(cave1.fill));
        values.insert("smoothing_passes", token(cave1.smoothing_passes));
        values.insert("birth_threshold", token(cave1.birth_threshold));
        values.insert("survival_threshold", token(cave1.survival_threshold));
        return;
    }

    //--------------------------------------------------------------------------
    // Writes the Zone's extent rather than the requested one, since an algorithm may round the requested extent to one its layout can use
    quetzal::stratum_format::Generator generator(const Zone& zone, const Parameters& parameters)
    {
        quetzal::KeyValueStore values;
        values.insert("algorithm", token(parameters.algorithm));
        values.insert("extent", token(zone.extent()));
        values.insert("stairs_down", interval_token(parameters.staircase_down_count));
        values.insert("stairs_up", interval_token(parameters.staircase_up_count));

        if (parameters.staircase_down_region_spec.has_value())
        {
            values.insert("stairs_down_region_spec", parameters.staircase_down_region_spec.value());
        }

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

            case Algorithm::Surface1:
                break;

            default:
                assert(false);
        }

        if (parameters.position.has_value())
        {
            values.insert("position", token(*parameters.position));
        }

        values.insert("seed", token(parameters.seed.value()));
        return {"dungen", std::move(values)};
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
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Staircase;
    }

    //--------------------------------------------------------------------------
    // A staircase in a room keeps the room's id
    id_type room_of(const Cell& cell)
    {
        return cell.type() == CellType::Room || cell.type() == CellType::Staircase ? cell.room_id() : nullid;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::ZoneDocument quetzal::dungen::make_document(const Zone& zone, const Parameters& parameters)
{
    auto [nx, ny] = zone.extent().rep();

    assert(!zone.transit_endpoints().empty());

    stratum_format::ZoneDocument document;
    document.version = stratum_format::format_version;
    document.generator = generator(zone, parameters);

    if (!zone.name().empty())
    {
        document.components["name"] = zone.name();
        document.components["display_name"] = zone.name();
    }

    document.position = parameters.position;
    stratum_format::Cells& cells = document.cells.emplace();
    cells.notation = stratum_format::Notation::Block;
    cells.extent = zone.extent();

    for (coord_type y = ny - 1; y >= 0; --y)
    {
        string row;
        for (coord_type x = 0; x < nx; ++x)
        {
            row += glyph(zone.cell({x, y}).type());
        }

        cells.rows.push_back(std::move(row));
    }

    // A staircase cell's type does not encode which way it leads; the TransitEndpoint does
    for (const auto& endpoint : zone.transit_endpoints())
    {
        if (endpoint.directionality == stratum_format::Directionality::Up)
        {
            const point_type position = endpoint.cell_position;
            cells.rows[static_cast<size_t>(ny - 1 - position.y())][static_cast<size_t>(position.x())] = '<';
        }
    }

    // Room floor lies against corridor floor with nothing between, so an edge between two floor cells of different rooms,
    // or of a room and none, is a barrier
    vector<optional<point_type>> locations(zone.rooms().size());
    const vector_type east{1, 0};
    const vector_type north{0, 1};

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
                locations[room_id] = point_type{x, y};
            }

            for (const vector_type& direction : {east, north})
            {
                point_type neighbor = point_type{x, y} + direction;
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
        document.rooms.push_back({stratum_format::RoomLocator{*location, {}}, {}});
    }

    // Local ids are assigned in the order written, so the same parameters give the same ids: each door, then its door
    // facings in the order the reader finds the walls for its glyph, and the staircases last
    stored_entity_id_type id = 0;

    for (const auto& walls : stratum_format::door_walls(cells))
    {
        stored_entity_id_type door_id = ++id;
        document.entities.push_back({door_id, "door", nullopt, nullopt, {}});

        for (const auto& wall : walls)
        {
            document.entities.push_back({++id, "door_facing", stratum_format::Site{stratum_format::Relation::On, wall, nullopt, nullopt}, door_id, {}});
        }
    }

    size_t staircase_count = 0;

    for (const auto& endpoint : zone.transit_endpoints())
    {
        const stratum_format::FlatCell floor_cell{stratum_format::FlatRole::Floor, endpoint.cell_position};

        const bool leads_down = endpoint.directionality == stratum_format::Directionality::Down;
        const stratum_format::AccessPointDestination target{leads_down ? parameters.staircase_down_region_spec : std::nullopt,
            stratum_format::relative_zone_extended_name(leads_down ? -1 : 1)};
        stratum_format::AccessPoint access_point{endpoint.type, staircase_count++, target};

        // A floor site's orientation is the direction an arriving actor faces
        const stratum_format::Site site{stratum_format::Relation::On, floor_cell, nullopt, endpoint.egress_direction};
        document.access_points.push_back(++id);
        document.entities.push_back({id, endpoint.type, site, nullopt, std::move(access_point), {}});
    }

    return document;
}

//------------------------------------------------------------------------------
void quetzal::dungen::write_zone(ostream& os, const Zone& zone, const Parameters& parameters)
{
    stratum_format::write(os, make_document(zone, parameters));
    return;
}
