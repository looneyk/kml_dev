//------------------------------------------------------------------------------
// stratum_sim
// load_layout_catagen.cpp
//------------------------------------------------------------------------------

#include "load_layout_catagen.hpp"
#include <algorithm>
#include <charconv>
#include <format>
#include <stdexcept>

namespace
{

    //--------------------------------------------------------------------------
    enum class WallType
    {
        Open,
        Solid,
        Door
    };

    //--------------------------------------------------------------------------
    // catagen's symbols. A cell is open unless it is solid; the stairs are floor until
    // something reads them as anything more
    bool open_glyph(char c, const std::filesystem::path& filepath)
    {
        switch (c)
        {
            case '#': return false;
            case ' ': return true;
            case '>': return true;
            case '<': return true;
        }

        throw std::runtime_error(std::format("unrecognized cell '{}' in level file {}", c, filepath.string()));
    }

    //--------------------------------------------------------------------------
    // A solid position is written where a wall would be, and is no more crossable than one
    WallType wall_glyph(char c, const std::filesystem::path& filepath)
    {
        switch (c)
        {
            case ' ': return WallType::Open;
            case '-': return WallType::Solid;
            case '|': return WallType::Solid;
            case '#': return WallType::Solid;
            case '/': return WallType::Door;
        }

        throw std::runtime_error(std::format("unrecognized wall '{}' in level file {}", c, filepath.string()));
    }

} // namespace

//------------------------------------------------------------------------------
// Every row reaches the last column, that column being a boundary wall or one of its corners
// and so never blank, which means nothing can have been trimmed and the widest row is the
// width. The grid is two characters across per cell plus one in each direction, so an even
// count in either is not one
quetzal::stratum_sim::discrete_vector_type quetzal::stratum_sim::detail::wall_extent(const Header& header,
    const std::vector<std::string>& lines, const std::filesystem::path& filepath)
{
    auto extent = header.values.find("extent");

    if (extent != header.values.end())
    {
        const std::string& s = extent->second;
        discrete_vector_type v;

        auto result = math::from_chars(s.data(), s.data() + s.size(), v, ",");

        if (result.ec != std::errc {} || result.ptr != s.data() + s.size() || v.x() <= 0 || v.y() <= 0)
        {
            throw std::runtime_error(std::format("level file {} has an unreadable extent in its header",
                filepath.string()));
        }

        return v;
    }

    coord_type width = 0;
    for (const std::string& s : lines)
    {
        width = std::max(width, static_cast<coord_type>(s.size()));
    }

    coord_type height = static_cast<coord_type>(lines.size());

    if (width < 3 || height < 3 || width % 2 == 0 || height % 2 == 0)
    {
        throw std::runtime_error(std::format("level file {} is {} by {}, which is not a wall grid",
            filepath.string(), width, height));
    }

    return {(width - 1) / 2, (height - 1) / 2};
}

//------------------------------------------------------------------------------
// Corners are not read, the walls either side of one determining the same shape
quetzal::stratum_sim::detail::Map quetzal::stratum_sim::detail::read_walls(const std::vector<std::string>& lines,
    const discrete_vector_type& extent, const std::filesystem::path& filepath)
{
    coord_type width = 2 * extent.x() + 1;
    coord_type height = 2 * extent.y() + 1;

    if (lines.size() != static_cast<size_t>(height))
    {
        throw std::runtime_error(std::format("level file {} holds {} rows where its extent asks for {}",
            filepath.string(), lines.size(), height));
    }

    auto glyph = [&](coord_type x, coord_type y) -> char
    {
        const std::string& line = lines[static_cast<size_t>(height - 1 - y)]; // The first line of the file is the highest row
        return x < static_cast<coord_type>(line.size()) ? line[static_cast<size_t>(x)] : ' ';
    };

    for (const std::string& s : lines)
    {
        if (static_cast<coord_type>(s.size()) > width)
        {
            throw std::runtime_error(std::format("level file {} holds a row of {} where its extent asks for {}",
                filepath.string(), s.size(), width));
        }
    }

    Map map;
    map.extent = extent;

    std::vector<bool> open(map.cell_count());
    for (coord_type j = 0; j < extent.y(); ++j)
    {
        for (coord_type i = 0; i < extent.x(); ++i)
        {
            open[map.slot({i, j})] = open_glyph(glyph(2 * i + 1, 2 * j + 1), filepath);
        }
    }

    // A wall addressed from the cell it lies west or south of, which is how the grid stores the
    // line it lies on rather than the cells either side of it
    auto wall_west = [&](coord_type i, coord_type j) { return wall_glyph(glyph(2 * i, 2 * j + 1), filepath); };
    auto wall_south = [&](coord_type i, coord_type j) { return wall_glyph(glyph(2 * i + 1, 2 * j), filepath); };

    auto between = [&](const discrete_point_type& grid_position, const discrete_point_type& grid_position_other)
    {
        if (grid_position_other.i() > grid_position.i()) return wall_west(grid_position_other.i(), grid_position_other.j());
        if (grid_position_other.i() < grid_position.i()) return wall_west(grid_position.i(), grid_position.j());
        if (grid_position_other.j() > grid_position.j()) return wall_south(grid_position_other.i(), grid_position_other.j());
        return wall_south(grid_position.i(), grid_position.j());
    };

    find_components(map,
        [&](coord_type i, coord_type j) { return open[map.slot({i, j})]; },
        [&](const discrete_point_type& grid_position, const discrete_point_type& grid_position_other) { return between(grid_position, grid_position_other) == WallType::Open; });

    // Every open cell belongs to an area here: the wall format spends nothing on a doorway,
    // so it has no blocks
    map.cell_open.assign(map.cell_count(), false);

    for (size_t slot = 0; slot < map.cell_open.size(); ++slot)
    {
        map.cell_open[slot] = open[slot];
    }

    // The wall format has one open cell symbol and so defines no rooms. cell_room is left
    // empty rather than guessed at, and a catagen level loads with none until the format
    // defines them

    for (coord_type j = 0; j < extent.y(); ++j)
    {
        for (coord_type i = 0; i < extent.x(); ++i)
        {
            const discrete_point_type grid_position {i, j};

            if (!open[map.slot(grid_position)])
            {
                continue;
            }

            const discrete_point_type neighbors[2]
            {
                grid_position + cardinal_direction(CardinalDirection::East),
                grid_position + cardinal_direction(CardinalDirection::North)
            };

            for (const auto& neighbor : neighbors)
            {
                if (neighbor.i() >= extent.x() || neighbor.j() >= extent.y() || !open[map.slot(neighbor)])
                {
                    continue;
                }

                if (map.cell_components[map.slot(neighbor)] != map.cell_components[map.slot(grid_position)])
                {
                    map.passages.push_back({grid_position, neighbor, between(grid_position, neighbor) == WallType::Door});
                }
            }
        }
    }

    return map;
}
