//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "quetzal/common/file_util.hpp"
#include <algorithm>
#include <format>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>

namespace
{

    using quetzal::id_type;
    using quetzal::nullid;

    enum class Kind
    {
        Rock,
        Open,
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
            case ' ': return Kind::Open;
            case '.': return Kind::Open;
            case '|': return Kind::Doorway;
            case '>': return Kind::Shaft;
        }

        assert(false); // Unrecognized cell code
        return Kind::Rock;
    }

    // A doorway or a shaft is its own component even where nothing separates it from its
    // neighbours, so that its facings can carry a door, or a portal, and their attachments
    //--------------------------------------------------------------------------
    bool solitary(Kind k)
    {
        return k == Kind::Doorway || k == Kind::Shaft;
    }

    //--------------------------------------------------------------------------
    // Lattice point, so that boundary edges chain by exact comparison
    struct Vertex
    {
        int i = 0;
        int j = 0;

        friend auto operator<=>(const Vertex&, const Vertex&) = default;
    };

    //--------------------------------------------------------------------------
    struct Edge
    {
        Vertex from;
        Vertex to;
    };

    //--------------------------------------------------------------------------
    // Twice the signed area. Positive is counterclockwise, which is an outer ring; a hole
    // is traced clockwise because the interior stays on the left throughout
    long long signed_area2(const std::vector<Vertex>& ring)
    {
        long long a = 0;

        for (size_t n = 0; n < ring.size(); ++n)
        {
            const Vertex& p = ring[n];
            const Vertex& q = ring[(n + 1) % ring.size()];
            a += static_cast<long long>(p.i) * q.j - static_cast<long long>(q.i) * p.j;
        }

        return a;
    }

    //--------------------------------------------------------------------------
    quetzal::stratum_sim::polygon_type to_polygon(const std::vector<Vertex>& ring)
    {
        using quetzal::stratum_sim::point_type;
        using quetzal::stratum_sim::value_type;

        quetzal::stratum_sim::points_type points;
        points.reserve(ring.size());

        for (const Vertex& v : ring)
        {
            points.push_back(point_type(static_cast<value_type>(v.i), static_cast<value_type>(v.j), value_type(0)));
        }

        return quetzal::stratum_sim::polygon_type(points.begin(), points.end());
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::stratum_sim::load_layout(const std::filesystem::path& filepath, Layout& layout, value_type height_ceiling)
{
    std::ifstream is(filepath, std::ios_base::binary);
    if (!is)
    {
        throw std::runtime_error(std::format("unable to open level file {}", filepath.string()));
    }

    std::vector<std::string> lines;
    std::string line;

    while (read_line(is, line))
    {
        lines.push_back(line);
    }

    if (lines.empty())
    {
        throw std::runtime_error(std::format("level file {} is empty", filepath.string()));
    }

    size_t h = lines.size();
    size_t w = 0;
    for (const std::string& s : lines)
    {
        w = std::max(w, s.size());
    }

    // Trailing spaces may have been trimmed, and a short line is room floor
    for (std::string& s : lines)
    {
        s.resize(w, ' ');
    }

    // The first line of the file is the highest row
    std::vector<Kind> kinds(w * h);
    for (size_t j = 0; j < h; ++j)
    {
        for (size_t i = 0; i < w; ++i)
        {
            kinds[j * w + i] = kind(lines[h - 1 - j][i]);
        }
    }

    auto at = [&](int i, int j)
    {
        return i < 0 || j < 0 || i >= static_cast<int>(w) || j >= static_cast<int>(h)
            ? Kind::Rock
            : kinds[static_cast<size_t>(j) * w + i];
    };

    // Contiguous open cells with nothing between them are one component. In the block
    // format nothing ever separates two open cells, so only the solitary kinds break it up
    std::vector<size_t> cell_components(w * h, static_cast<size_t>(-1));
    std::vector<std::vector<Vertex>> component_cells;

    for (size_t j = 0; j < h; ++j)
    {
        for (size_t i = 0; i < w; ++i)
        {
            size_t slot = j * w + i;
            if (kinds[slot] == Kind::Rock || cell_components[slot] != static_cast<size_t>(-1))
            {
                continue;
            }

            size_t component = component_cells.size();
            component_cells.push_back({});

            std::vector<Vertex> pending {Vertex {static_cast<int>(i), static_cast<int>(j)}};
            cell_components[slot] = component;

            while (!pending.empty())
            {
                Vertex v = pending.back();
                pending.pop_back();
                component_cells[component].push_back(v);

                if (solitary(at(v.i, v.j)))
                {
                    continue; // Never merges with a neighbour
                }

                const Vertex neighbors[4] {{v.i + 1, v.j}, {v.i - 1, v.j}, {v.i, v.j + 1}, {v.i, v.j - 1}};
                for (const Vertex& n : neighbors)
                {
                    if (at(n.i, n.j) == Kind::Rock || solitary(at(n.i, n.j)))
                    {
                        continue;
                    }

                    size_t n_slot = static_cast<size_t>(n.j) * w + n.i;
                    if (cell_components[n_slot] == static_cast<size_t>(-1))
                    {
                        cell_components[n_slot] = component;
                        pending.push_back(n);
                    }
                }
            }
        }
    }

    layout.clear();
    layout.set_grid(point_type(0.0f, 0.0f, 0.0f), value_type(1), Cell {static_cast<int>(w), static_cast<int>(h)});

    std::vector<id_type> component_regions(component_cells.size(), nullid);

    for (size_t component = 0; component < component_cells.size(); ++component)
    {
        // One directed edge per boundary cell edge, wound with the interior on the left.
        // Not merged across collinear runs: a facing must not span two different
        // neighbours, or connecting it would need splitting
        std::map<Vertex, Edge> edges;

        auto add = [&](const Vertex& from, const Vertex& to)
        {
            assert(!edges.contains(from)); // A vertex with two exits is a diagonal pinch
            edges[from] = Edge {from, to};
        };

        for (const Vertex& c : component_cells[component])
        {
            size_t slot = static_cast<size_t>(c.j) * w + c.i;
            auto outside = [&](int i, int j)
            {
                return at(i, j) == Kind::Rock || cell_components[static_cast<size_t>(j) * w + i] != cell_components[slot];
            };

            if (c.j == 0 || outside(c.i, c.j - 1))
            {
                add({c.i, c.j}, {c.i + 1, c.j});
            }

            if (c.i + 1 == static_cast<int>(w) || outside(c.i + 1, c.j))
            {
                add({c.i + 1, c.j}, {c.i + 1, c.j + 1});
            }

            if (c.j + 1 == static_cast<int>(h) || outside(c.i, c.j + 1))
            {
                add({c.i + 1, c.j + 1}, {c.i, c.j + 1});
            }

            if (c.i == 0 || outside(c.i - 1, c.j))
            {
                add({c.i, c.j + 1}, {c.i, c.j});
            }
        }

        // Chain the edges into rings
        std::vector<std::vector<Vertex>> rings;

        while (!edges.empty())
        {
            std::vector<Vertex> ring;
            Vertex start = edges.begin()->first;
            Vertex v = start;

            do
            {
                auto i = edges.find(v);
                assert(i != edges.end()); // A boundary that does not close
                ring.push_back(v);
                v = i->second.to;
                edges.erase(i);
            } while (v != start);

            rings.push_back(ring);
        }

        std::vector<Vertex> outer;
        std::vector<polygon_type> holes;

        for (const std::vector<Vertex>& ring : rings)
        {
            if (signed_area2(ring) > 0)
            {
                assert(outer.empty()); // One outer ring per component
                outer = ring;
            }
            else
            {
                holes.push_back(to_polygon(ring));
            }
        }

        assert(!outer.empty());
        component_regions[component] = layout.create_region(to_polygon(outer), holes, value_type(0), height_ceiling);

        for (const Vertex& c : component_cells[component])
        {
            layout.set_cell_region(Cell {c.i, c.j}, component_regions[component]);
        }
    }

    // Facings are one cell edge each, so a shared edge is one pair
    for (size_t j = 0; j < h; ++j)
    {
        for (size_t i = 0; i < w; ++i)
        {
            size_t slot = j * w + i;
            if (kinds[slot] == Kind::Rock)
            {
                continue;
            }

            const Vertex neighbors[2] {{static_cast<int>(i) + 1, static_cast<int>(j)}, {static_cast<int>(i), static_cast<int>(j) + 1}};

            for (const Vertex& n : neighbors)
            {
                if (at(n.i, n.j) == Kind::Rock)
                {
                    continue;
                }

                size_t n_slot = static_cast<size_t>(n.j) * w + n.i;
                if (cell_components[n_slot] == cell_components[slot])
                {
                    continue; // One region, so there is no facing between them
                }

                id_type region_a = component_regions[cell_components[slot]];
                id_type region_b = component_regions[cell_components[n_slot]];

                Layout::ids_type walls_a = layout.region_all_walls(region_a);
                Layout::ids_type walls_b = layout.region_all_walls(region_b);

                for (id_type a : walls_a)
                {
                    if (layout.wall(a).partner != nullid)
                    {
                        continue;
                    }

                    for (id_type b : walls_b)
                    {
                        if (layout.wall(b).partner == nullid && layout.opposed(a, b))
                        {
                            layout.connect(a, b);
                            layout.set_passable(a, true);
                            layout.set_passable(b, true);
                            break;
                        }
                    }
                }
            }
        }
    }

    return;
}
