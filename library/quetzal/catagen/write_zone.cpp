//------------------------------------------------------------------------------
// catagen
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

    using namespace quetzal::catagen;
    using stored_entity_id_type = quetzal::stratum_format::stored_entity_id_type;
    using raw_values_type = quetzal::stratum_format::raw_values_type;

    //--------------------------------------------------------------------------
    // Formats a parameter as the token its command line option takes
    template<typename T>
    string token(const T& value)
    {
        ostringstream os;
        os << quetzal::separator_delim(",") << value;
        return os.str();
    }

    //--------------------------------------------------------------------------
    template<typename T>
    string interval_token(const quetzal::math::Interval<T>& interval)
    {
        return token(interval.lower()) + "-" + token(interval.upper());
    }

    //--------------------------------------------------------------------------
    char cell_glyph(CellType type)
    {
        switch (type)
        {
            case CellType::Solid: return ' ';
            case CellType::Open: return '.';
            case CellType::Staircase: return '>';
        }

        assert(false);
        return ' ';
    }

    //--------------------------------------------------------------------------
    // The two orientations differ only in the glyph a wall is drawn with
    char wall_glyph(WallType type, char glyph_wall)
    {
        switch (type)
        {
            case WallType::Open: return ' ';
            case WallType::Solid: return glyph_wall;
            case WallType::Door: return '+';
        }

        assert(false);
        return glyph_wall;
    }

    //--------------------------------------------------------------------------
    bool stair_cell(const Zone& zone, const point_type& cell)
    {
        if (!zone.contains(cell))
        {
            return false;
        }

        return zone.cell(cell) == CellType::Staircase;
    }

    //--------------------------------------------------------------------------
    bool floor_cell(const Zone& zone, const point_type& cell)
    {
        return zone.contains(cell) && zone.cell(cell) == CellType::Open;
    }

    //--------------------------------------------------------------------------
    // A stair cell is a block, so beside one there is a wall where floor is, even across an open wall, and none elsewhere
    WallType written_wall(const Zone& zone, WallType type, const point_type& a, const point_type& b)
    {
        if (!stair_cell(zone, a) && !stair_cell(zone, b))
        {
            return type;
        }

        if (!floor_cell(zone, a) && !floor_cell(zone, b))
        {
            return WallType::Open;
        }

        return type == WallType::Open ? WallType::Solid : type;
    }

    //--------------------------------------------------------------------------
    // The first row is the highest, the origin being the lower left cell
    vector<string> rows(const Zone& zone)
    {
        const vector_type& extent = zone.extent();
        coord_type width = 2 * extent.i() + 1;
        coord_type height = 2 * extent.j() + 1;

        vector<string> result;

        for (coord_type y = height - 1; y >= 0; --y)
        {
            string row(static_cast<size_t>(width), ' ');

            for (coord_type x = 0; x < width; ++x)
            {
                coord_type i = x / 2;
                coord_type j = y / 2;
                bool cell_x = x % 2 != 0;
                bool cell_y = y % 2 != 0;

                if (cell_x && cell_y)
                {
                    row[x] = cell_glyph(zone.cell({i, j}));
                }
                else if (cell_x)
                {
                    row[x] = wall_glyph(written_wall(zone, zone.wall_horizontal(i, j), {i, j - 1}, {i, j}), '-');
                }
                else if (cell_y)
                {
                    row[x] = wall_glyph(written_wall(zone, zone.wall_vertical(i, j), {i - 1, j}, {i, j}), '|');
                }
            }

            result.push_back(std::move(row));
        }

        for (const auto& staircase : zone.staircases())
        {
            if (staircase.rise() == Rise::Up)
            {
                point_type position = staircase.placement().position();
                result[static_cast<size_t>(height - 2 - 2 * position.j())][static_cast<size_t>(2 * position.i() + 1)] = '<';
            }
        }

        return result;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::catagen::make_document(const Zone& zone, const Parameters& parameters)
{
    raw_values_type values = raw_values_type::object();
    values["extent"] = token(parameters.extent);
    values["scale"] = interval_token(parameters.scale);
    values["spacing"] = interval_token(parameters.spacing);
    values["density"] = token(parameters.density);
    values["loop"] = token(parameters.loop);
    values["stairs_down"] = interval_token(parameters.staircase_down_count);
    values["stairs_up"] = interval_token(parameters.staircase_up_count);

    if (parameters.position.has_value())
    {
        values["position"] = token(*parameters.position);
    }

    values["seed"] = token(parameters.seed.value());

    stratum_format::Document document;
    document.version = stratum_format::format_version;
    document.generator = {{"name", "catagen"}, {"parameters", values}};

    if (!zone.name().empty())
    {
        document.components["name"] = zone.name();
    }

    document.position = parameters.position;
    document.cells.notation = stratum_format::Notation::Wall;
    document.cells.extent = zone.extent();
    document.cells.rows = rows(zone);

    // Local ids are assigned in the order written, so the same parameters give the same ids: each door, then its door
    // facings in the order the reader finds the walls for its glyph, and the staircases last
    stored_entity_id_type id = 0;

    for (const auto& walls : stratum_format::door_walls(document.cells))
    {
        stored_entity_id_type door_id = ++id;
        document.entities.push_back({door_id, "door", nullopt, nullopt, {}});

        for (const auto& address : walls)
        {
            document.entities.push_back({++id, "door_facing", stratum_format::Site {stratum_format::Relation::On, address, nullopt, nullopt},
                door_id, {}});
        }
    }

    size_t staircase_count = 0;

    for (const auto& staircase : zone.staircases())
    {
        // The staircase's cell is a block, and it is sited on the wall its open neighbor presents to that block
        const directed_point_type placement = staircase.placement();
        const vector_type open_side = placement.direction();
        stratum_format::WallAddress address {placement.position() + open_side, open_side};

        stratum_format::RelocatorTarget target {std::nullopt, stratum_format::RelativeZone {staircase.rise() == Rise::Down ? -1 : 1}, std::nullopt};
        stratum_format::AccessPoint access_point {"staircase", staircase_count++, target};

        document.access_points.push_back(++id);
        document.entities.push_back({id, "staircase", stratum_format::Site {stratum_format::Relation::On, address, nullopt, nullopt}, nullopt,
            std::move(access_point), {}});
    }

    return document;
}

//------------------------------------------------------------------------------
void quetzal::catagen::write_zone(ostream& os, const Zone& zone, const Parameters& parameters)
{
    stratum_format::write(os, make_document(zone, parameters));
    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::write_grid(ostream& os, const Zone& zone)
{
    for (const auto& row : rows(zone))
    {
        os << row << '\n';
    }

    return;
}
