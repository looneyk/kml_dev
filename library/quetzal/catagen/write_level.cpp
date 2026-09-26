//------------------------------------------------------------------------------
// catagen
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

    using namespace quetzal::catagen;
    using stored_entity_id_type = quetzal::stratum_format::stored_entity_id_type;
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
            case CellType::StairsDown: return '>';
            case CellType::StairsUp: return '<';
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
    bool stair_cell(const Level& level, const point_type& cell)
    {
        if (!level.contains(cell))
        {
            return false;
        }

        CellType type = level.cell(cell);
        return type == CellType::StairsDown || type == CellType::StairsUp;
    }

    //--------------------------------------------------------------------------
    bool floor_cell(const Level& level, const point_type& cell)
    {
        return level.contains(cell) && level.cell(cell) == CellType::Open;
    }

    //--------------------------------------------------------------------------
    // A stair cell is a block, so beside one there is a wall where floor is, even across an open wall, and none elsewhere
    WallType written_wall(const Level& level, WallType type, const point_type& a, const point_type& b)
    {
        if (!stair_cell(level, a) && !stair_cell(level, b))
        {
            return type;
        }

        if (!floor_cell(level, a) && !floor_cell(level, b))
        {
            return WallType::Open;
        }

        return type == WallType::Open ? WallType::Solid : type;
    }

    //--------------------------------------------------------------------------
    // The first row is the highest, the origin being the lower left cell
    vector<string> rows(const Level& level)
    {
        const vector_type& extent = level.extent();
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
                    row[x] = cell_glyph(level.cell({i, j}));
                }
                else if (cell_x)
                {
                    row[x] = wall_glyph(written_wall(level, level.wall_horizontal(i, j), {i, j - 1}, {i, j}), '-');
                }
                else if (cell_y)
                {
                    row[x] = wall_glyph(written_wall(level, level.wall_vertical(i, j), {i - 1, j}, {i, j}), '|');
                }
            }

            result.push_back(std::move(row));
        }

        return result;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::catagen::write_level(ostream& os, const Level& level, const Parameters& parameters)
{
    values_type values = values_type::object();
    values["extent"] = token(parameters.extent);
    values["scale"] = interval_token(parameters.scale);
    values["spacing"] = interval_token(parameters.spacing);
    values["density"] = token(parameters.density);
    values["loop"] = token(parameters.loop);
    values["stairs_down"] = interval_token(parameters.staircase_down_count);
    values["stairs_up"] = interval_token(parameters.staircase_up_count);
    values["seed"] = token(parameters.seed.value());

    stratum_format::Document document;
    document.generator = {{"name", "catagen"}, {"parameters", values}};
    document.cells.notation = stratum_format::Notation::Wall;
    document.cells.extent = level.extent();
    document.cells.rows = rows(level);

    // Local ids in the order written, so the same parameters give the same ids: each door, then its door facings in
    // the order of the walls the reader finds for its glyph, and the staircases last
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

    size_t down_count = 0;
    size_t up_count = 0;

    for (coord_type j = 0; j < level.extent().j(); ++j)
    {
        for (coord_type i = 0; i < level.extent().i(); ++i)
        {
            CellType type = level.cell({i, j});
            if (type != CellType::StairsDown && type != CellType::StairsUp)
            {
                continue;
            }

            // The staircase's cell is a block, and it is sited on the wall its open neighbor presents to that block
            optional<CardinalDirection> open_side = level.staircase_facing({i, j});
            assert(open_side.has_value());
            const vector_type& direction = cardinal_direction(*open_side);
            stratum_format::WallAddress address {point_type {i, j} + direction, direction};

            // Ordered among the staircases leading the same way
            bool down = type == CellType::StairsDown;

            // An arriving actor is placed on the floor it opens onto, facing away from it
            stratum_format::Relocator relocator {down ? "down" : "up", down ? "down" : "up", down ? down_count++ : up_count++,
                stratum_format::discrete_directed_point_type(address.cell_position, direction)};

            document.access_points.push_back(++id);
            document.entities.push_back({id, "staircase", stratum_format::Site {stratum_format::Relation::On, address, nullopt, nullopt}, nullopt,
                std::move(relocator), {}});
        }
    }

    stratum_format::write(os, document);
    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::write_grid(ostream& os, const Level& level)
{
    for (const auto& row : rows(level))
    {
        os << row << '\n';
    }

    return;
}
