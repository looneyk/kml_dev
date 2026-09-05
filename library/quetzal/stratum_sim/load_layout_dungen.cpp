//------------------------------------------------------------------------------
// stratum_sim
// load_layout_dungen.cpp
//------------------------------------------------------------------------------

#include "load_layout_dungen.hpp"
#include <algorithm>
#include <cassert>

namespace
{

    using quetzal::stratum_sim::detail::Map;
    using quetzal::stratum_sim::detail::Vertex;

    //--------------------------------------------------------------------------
    enum class Kind
    {
        Rock,
        Room,
        Corridor,
        Doorway,
        Stairway
    };

    //--------------------------------------------------------------------------
    // dungen's symbols, from dungen::cell_types. '0' is its Null type, marking a cell the
    // generator never assigned
    Kind kind(char c)
    {
        switch (c)
        {
            case '#': return Kind::Rock;
            case '0': return Kind::Rock;
            case ' ': return Kind::Room;
            case '.': return Kind::Corridor;
            case '|': return Kind::Doorway;
            case '>': return Kind::Stairway;
        }

        assert(false); // Unrecognized cell code
        return Kind::Rock;
    }

    //--------------------------------------------------------------------------
    // Room and corridor are both open floor and are laid out alike. They differ only in
    // what the level calls them, which is what makes the rooms nameable
    bool open(Kind k)
    {
        return k != Kind::Rock;
    }

    //--------------------------------------------------------------------------
    // A doorway or stairway is a cell of wall a mover may stand in rather than part of the
    // area around it. The boundary of each neighbouring area runs along its face, wound about
    // that area like the rest of that area's wall, so a door hangs on the face and a mover
    // inside meets only backs -- which are no walls at all to a query, and so let it out
    bool block(Kind k)
    {
        return k == Kind::Doorway || k == Kind::Stairway;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::detail::Map quetzal::stratum_sim::detail::read_block(const std::vector<std::string>& lines)
{
    Map map;
    map.h = lines.size();
    map.w = 0;

    for (const std::string& s : lines)
    {
        map.w = std::max(map.w, s.size());
    }

    // Trailing spaces may have been trimmed, and a short line is room floor
    std::vector<Kind> kinds(map.w * map.h, Kind::Room);

    for (size_t j = 0; j < map.h; ++j)
    {
        const std::string& line = lines[map.h - 1 - j]; // The first line of the file is the highest row
        for (size_t i = 0; i < map.w; ++i)
        {
            kinds[j * map.w + i] = kind(i < line.size() ? line[i] : ' ');
        }
    }

    auto at = [&](int i, int j)
    {
        return i < 0 || j < 0 || i >= static_cast<int>(map.w) || j >= static_cast<int>(map.h)
            ? Kind::Rock
            : kinds[static_cast<size_t>(j) * map.w + i];
    };

    // A block is part of the area it stands in, so the area's boundary wraps around the rock
    // either side of it and its own sides are walled by that boundary rather than by nothing
    find_components(map,
        [&](int i, int j) { return open(at(i, j)); },
        [&](const Vertex&, const Vertex&) { return true; });

    map.cell_open.assign(map.w * map.h, false);

    for (size_t j = 0; j < map.h; ++j)
    {
        for (size_t i = 0; i < map.w; ++i)
        {
            map.cell_open[j * map.w + i] = open(at(static_cast<int>(i), static_cast<int>(j)));
        }
    }

    // Each block and the open cells its faces adjoin, which is where a door hangs
    for (size_t j = 0; j < map.h; ++j)
    {
        for (size_t i = 0; i < map.w; ++i)
        {
            int bi = static_cast<int>(i);
            int bj = static_cast<int>(j);

            if (!block(at(bi, bj)))
            {
                continue;
            }

            Block b;
            b.cell = {bi, bj};

            for (const Vertex& step : {Vertex {1, 0}, Vertex {0, 1}, Vertex {-1, 0}, Vertex {0, -1}})
            {
                int ni = bi + step.i;
                int nj = bj + step.j;

                if (open(at(ni, nj)) && !block(at(ni, nj)))
                {
                    b.neighbors.push_back({ni, nj});
                }
            }

            map.blocks.push_back(b);
        }
    }

    // The one thing either format says about rooms: dungen writes room floor and corridor
    // floor as different symbols
    map.cell_room.assign(map.w * map.h, false);

    for (size_t slot = 0; slot < kinds.size(); ++slot)
    {
        map.cell_room[slot] = kinds[slot] == Kind::Room;
    }

    // No passages: nothing separates two open cells in this format except a block, and a
    // block is not somewhere two areas meet -- it is a cell of wall between them

    return map;
}
