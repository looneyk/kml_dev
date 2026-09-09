//------------------------------------------------------------------------------
// catagen
// write_level.cpp
//------------------------------------------------------------------------------

#include "write_level.hpp"
#include <ostream>
#include <string>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::catagen;

    constexpr char glyph_solid = '#';
    constexpr char glyph_open = ' ';
    constexpr char glyph_stairs_down = '>';
    constexpr char glyph_stairs_up = '<';
    constexpr char glyph_wall_horizontal = '-';
    constexpr char glyph_wall_vertical = '|';
    constexpr char glyph_door = '/';
    constexpr char glyph_corner = '+';

    //--------------------------------------------------------------------------
    // Whatever lies outside the extent is solid
    bool rock(const Level& level, coord_type i, coord_type j)
    {
        point_type cell {i, j};
        return !level.contains(cell) || level.cell(cell) == CellType::Solid;
    }

    //--------------------------------------------------------------------------
    char cell_glyph(CellType type)
    {
        switch (type)
        {
            case CellType::Solid: return glyph_solid;
            case CellType::Open: return glyph_open;
            case CellType::StairsDown: return glyph_stairs_down;
            case CellType::StairsUp: return glyph_stairs_up;
        }

        assert(false);
        return glyph_solid;
    }

    //--------------------------------------------------------------------------
    // The two orientations differ only in the glyph a wall is drawn with
    char wall_glyph(WallType type, char glyph_wall)
    {
        switch (type)
        {
            case WallType::Open: return glyph_open;
            case WallType::Solid: return glyph_wall;
            case WallType::Door: return glyph_door;
        }

        assert(false);
        return glyph_wall;
    }

    //--------------------------------------------------------------------------
    char horizontal_glyph(const Level& level, coord_type i, coord_type j)
    {
        if (rock(level, i, j - 1) && rock(level, i, j))
        {
            assert(level.wall_horizontal(i, j) == WallType::Open);
            return glyph_solid;
        }

        return wall_glyph(level.wall_horizontal(i, j), glyph_wall_horizontal);
    }

    //--------------------------------------------------------------------------
    char vertical_glyph(const Level& level, coord_type i, coord_type j)
    {
        if (rock(level, i - 1, j) && rock(level, i, j))
        {
            assert(level.wall_vertical(i, j) == WallType::Open);
            return glyph_solid;
        }

        return wall_glyph(level.wall_vertical(i, j), glyph_wall_vertical);
    }

    //--------------------------------------------------------------------------
    bool drawn(char glyph)
    {
        return glyph != glyph_open && glyph != glyph_solid;
    }

    //--------------------------------------------------------------------------
    char corner_glyph(const Level& level, coord_type i, coord_type j)
    {
        if (rock(level, i - 1, j - 1) && rock(level, i, j - 1) && rock(level, i - 1, j) && rock(level, i, j))
        {
            return glyph_solid;
        }

        const vector_type& extent = level.extent();

        bool horizontal = (i > 0 && drawn(horizontal_glyph(level, i - 1, j)))
            || (i < extent.i() && drawn(horizontal_glyph(level, i, j)));
        bool vertical = (j > 0 && drawn(vertical_glyph(level, i, j - 1)))
            || (j < extent.j() && drawn(vertical_glyph(level, i, j)));

        if (horizontal && vertical)
        {
            return glyph_corner;
        }

        if (horizontal)
        {
            return glyph_wall_horizontal;
        }

        if (vertical)
        {
            return glyph_wall_vertical;
        }

        return glyph_open;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::catagen::write_level(ostream& os, const Level& level, const Parameters& parameters)
{
    os << "catagen " << format_version
        << " extent=" << parameters.extent.i() << "," << parameters.extent.j()
        << " scale=" << parameters.scale_min << "-" << parameters.scale_max
        << " spacing=" << parameters.spacing_min << "-" << parameters.spacing_max
        << " density=" << parameters.density
        << " loop=" << parameters.loop
        << " seed=" << parameters.seed.value() // Throws rather than write a header that cannot reproduce
        << '\n';

    write_grid(os, level);
    return;
}

//------------------------------------------------------------------------------
void quetzal::catagen::write_grid(ostream& os, const Level& level)
{
    const vector_type& extent = level.extent();
    coord_type width = 2 * extent.i() + 1;
    coord_type height = 2 * extent.j() + 1;

    for (coord_type y = height - 1; y >= 0; --y)
    {
        string line(static_cast<size_t>(width), glyph_open);

        for (coord_type x = 0; x < width; ++x)
        {
            coord_type i = x / 2;
            coord_type j = y / 2;
            bool cell_x = x % 2 != 0;
            bool cell_y = y % 2 != 0;

            if (cell_x && cell_y)
            {
                line[x] = cell_glyph(level.cell({i, j}));
            }
            else if (cell_x)
            {
                line[x] = horizontal_glyph(level, i, j);
            }
            else if (cell_y)
            {
                line[x] = vertical_glyph(level, i, j);
            }
            else
            {
                line[x] = corner_glyph(level, i, j);
            }
        }

        os << line << '\n';
    }

    return;
}
