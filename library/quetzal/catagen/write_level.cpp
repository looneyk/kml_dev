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
                    row[x] = wall_glyph(level.wall_horizontal(i, j), '-');
                }
                else if (cell_y)
                {
                    row[x] = wall_glyph(level.wall_vertical(i, j), '|');
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
    document.components = values_type::object();
    document.components["entry"] = values_type::array();
    document.cells.notation = stratum_format::Notation::Wall;
    document.cells.extent = level.extent();
    document.cells.rows = rows(level);

    // The walls the reader finds for each door glyph, so the entity is at the placement it reads the door at
    for (auto& walls : stratum_format::door_walls(document.cells))
    {
        values_type entity = values_type::object();
        entity["type"] = "door";

        document.attachments.push_back({std::move(walls), nullopt, values_type::array({std::move(entity)})});
    }

    // Every staircase is an entry, the caller choosing among them by what it arrives from
    for (coord_type j = 0; j < level.extent().j(); ++j)
    {
        for (coord_type i = 0; i < level.extent().i(); ++i)
        {
            CellType type = level.cell({i, j});
            if (type != CellType::StairsDown && type != CellType::StairsUp)
            {
                continue;
            }

            document.components["entry"].push_back(values_type::array({i, j}));

            values_type relocator = values_type::object();
            relocator["target"] = type == CellType::StairsDown ? "level_down" : "level_up";

            values_type components = values_type::object();
            optional<CardinalDirection> facing = level.staircase_facing({i, j});
            if (facing.has_value())
            {
                const vector_type& step = cardinal_direction(*facing);
                components["facing"] = values_type::array({step.x(), step.y()});
            }

            components["relocator"] = std::move(relocator);

            values_type entity = values_type::object();
            entity["type"] = "staircase";
            entity["components"] = std::move(components);

            document.attachments.push_back({stratum_format::FlatCell {stratum_format::FlatRole::Floor, point_type {i, j}}, nullopt,
                values_type::array({std::move(entity)})});
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
