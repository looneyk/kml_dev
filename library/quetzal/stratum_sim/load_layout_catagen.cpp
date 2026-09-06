//------------------------------------------------------------------------------
// stratum_sim
// load_layout_catagen.cpp
//------------------------------------------------------------------------------

#include "load_layout_catagen.hpp"
#include <algorithm>
#include <format>
#include <stdexcept>

namespace
{

    using quetzal::stratum_sim::detail::Vertex;

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
std::pair<size_t, size_t> quetzal::stratum_sim::detail::wall_extent(const Header& header,
    const std::vector<std::string>& lines, const std::filesystem::path& filepath)
{
    auto extent = header.values.find("extent");

    if (extent != header.values.end())
    {
        long nx = 0;
        long ny = 0;

        if (!read_pair(extent->second, ',', nx, ny) || nx <= 0 || ny <= 0)
        {
            throw std::runtime_error(std::format("level file {} has an unreadable extent in its header",
                filepath.string()));
        }

        return {static_cast<size_t>(nx), static_cast<size_t>(ny)};
    }

    size_t width = 0;
    for (const std::string& s : lines)
    {
        width = std::max(width, s.size());
    }

    size_t height = lines.size();

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
    size_t w, size_t h, const std::filesystem::path& filepath)
{
    size_t width = 2 * w + 1;
    size_t height = 2 * h + 1;

    if (lines.size() != height)
    {
        throw std::runtime_error(std::format("level file {} holds {} rows where its extent asks for {}",
            filepath.string(), lines.size(), height));
    }

    auto glyph = [&](size_t x, size_t y) -> char
    {
        const std::string& line = lines[height - 1 - y]; // The first line of the file is the highest row
        return x < line.size() ? line[x] : ' ';
    };

    for (const std::string& s : lines)
    {
        if (s.size() > width)
        {
            throw std::runtime_error(std::format("level file {} holds a row of {} where its extent asks for {}",
                filepath.string(), s.size(), width));
        }
    }

    Map map;
    map.w = w;
    map.h = h;

    std::vector<bool> open(w * h);
    for (size_t j = 0; j < h; ++j)
    {
        for (size_t i = 0; i < w; ++i)
        {
            open[j * w + i] = open_glyph(glyph(2 * i + 1, 2 * j + 1), filepath);
        }
    }

    // A wall addressed from the cell it lies west or south of, which is how the grid stores the
    // line it lies on rather than the cells either side of it
    auto wall_west = [&](int i, int j) { return wall_glyph(glyph(2 * i, 2 * j + 1), filepath); };
    auto wall_south = [&](int i, int j) { return wall_glyph(glyph(2 * i + 1, 2 * j), filepath); };

    auto between = [&](const Vertex& a, const Vertex& b)
    {
        if (b.i > a.i) return wall_west(b.i, b.j);
        if (b.i < a.i) return wall_west(a.i, a.j);
        if (b.j > a.j) return wall_south(b.i, b.j);
        return wall_south(a.i, a.j);
    };

    find_components(map,
        [&](int i, int j) { return open[static_cast<size_t>(j) * w + i]; },
        [&](const Vertex& a, const Vertex& b) { return between(a, b) == WallType::Open; });

    // Every open cell belongs to an area here: the wall format spends nothing on a doorway,
    // so it has no blocks
    map.cell_open.assign(map.w * map.h, false);

    for (size_t slot = 0; slot < map.cell_open.size(); ++slot)
    {
        map.cell_open[slot] = open[slot];
    }

    // The wall format has one open cell symbol and so defines no rooms. cell_room is left
    // empty rather than guessed at, and a catagen level loads with none until the format
    // defines them

    for (size_t j = 0; j < h; ++j)
    {
        for (size_t i = 0; i < w; ++i)
        {
            if (!open[j * w + i])
            {
                continue;
            }

            const Vertex cell {static_cast<int>(i), static_cast<int>(j)};
            const Vertex neighbors[2] {{cell.i + 1, cell.j}, {cell.i, cell.j + 1}};

            for (const Vertex& n : neighbors)
            {
                if (n.i >= static_cast<int>(w) || n.j >= static_cast<int>(h) || !open[static_cast<size_t>(n.j) * w + n.i])
                {
                    continue;
                }

                if (map.cell_components[static_cast<size_t>(n.j) * w + n.i] != map.cell_components[j * w + i])
                {
                    map.passages.push_back({cell, n, between(cell, n) == WallType::Door});
                }
            }
        }
    }

    return map;
}
