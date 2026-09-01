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
        Shaft
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
            case '>': return Kind::Shaft;
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

    // A doorway or a shaft is its own component even where nothing separates it from its
    // neighbours, so that its walls can carry a door, or a portal, and their attachments
    //--------------------------------------------------------------------------
    bool solitary(Kind k)
    {
        return k == Kind::Doorway || k == Kind::Shaft;
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

    find_components(map,
        [&](int i, int j) { return open(at(i, j)); },
        [&](const Vertex& a, const Vertex& b) { return !solitary(at(a.i, a.j)) && !solitary(at(b.i, b.j)); });

    // The one thing either format says about rooms: dungen writes room floor and corridor
    // floor as different symbols
    map.cell_room.assign(map.w * map.h, false);

    for (size_t slot = 0; slot < kinds.size(); ++slot)
    {
        map.cell_room[slot] = kinds[slot] == Kind::Room;
    }

    for (size_t j = 0; j < map.h; ++j)
    {
        for (size_t i = 0; i < map.w; ++i)
        {
            if (at(static_cast<int>(i), static_cast<int>(j)) == Kind::Rock)
            {
                continue;
            }

            const Vertex cell {static_cast<int>(i), static_cast<int>(j)};
            const Vertex neighbors[2] {{cell.i + 1, cell.j}, {cell.i, cell.j + 1}};

            for (const Vertex& n : neighbors)
            {
                if (at(n.i, n.j) == Kind::Rock)
                {
                    continue;
                }

                size_t n_slot = static_cast<size_t>(n.j) * map.w + n.i;
                if (map.cell_components[n_slot] != map.cell_components[j * map.w + i])
                {
                    map.passages.push_back({cell, n, true});
                }
            }
        }
    }

    return map;
}
