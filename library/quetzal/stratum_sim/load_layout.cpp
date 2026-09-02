//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "load_layout_catagen.hpp"
#include "load_layout_dungen.hpp"
#include "quetzal/common/file_util.hpp"
#include <format>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <cctype>

namespace
{

    using quetzal::id_type;
    using quetzal::nullid;

    using namespace quetzal::stratum_sim::detail;

    //--------------------------------------------------------------------------
    // A grid row never begins with a letter, so a file whose first row does not is one
    // written before headers existed
    bool header_row(const std::string& line)
    {
        return !line.empty() && std::isalpha(static_cast<unsigned char>(line[0])) != 0;
    }

    //--------------------------------------------------------------------------
    Header read_header(const std::string& line)
    {
        Header header;
        std::istringstream is(line);
        is >> header.generator >> header.version;

        std::string token;
        while (is >> token)
        {
            size_t n = token.find('=');
            if (n != std::string::npos)
            {
                header.values[token.substr(0, n)] = token.substr(n + 1);
            }
        }

        return header;
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

    //--------------------------------------------------------------------------
    // The edge one cell shares with an orthogonally adjacent one, as the directed boundary
    // edge that lies on it wound for that cell. The other cell's is its reverse, which is
    // what makes the two walls on it partners
    Edge boundary_edge(const Vertex& cell, const Vertex& other)
    {
        if (other.i > cell.i)
        {
            return {{cell.i + 1, cell.j}, {cell.i + 1, cell.j + 1}};
        }

        if (other.i < cell.i)
        {
            return {{cell.i, cell.j + 1}, {cell.i, cell.j}};
        }

        if (other.j > cell.j)
        {
            return {{cell.i + 1, cell.j + 1}, {cell.i, cell.j + 1}};
        }

        return {{cell.i, cell.j}, {cell.i + 1, cell.j}};
    }

    //--------------------------------------------------------------------------
    using Ring = std::vector<Vertex>;
    using Rings = std::vector<Ring>; // The outlines of one component
    using Outlines = std::vector<Rings>; // By component
    using WallIndex = std::map<std::pair<Vertex, Vertex>, id_type>; // The wall lying on one directed boundary edge

    //--------------------------------------------------------------------------
    // The boundary of every component, as closed outlines. Geometry derived from the map and
    // nothing more: nothing is built here and no layout is touched
    // One directed edge per boundary cell edge, wound with the open space on the left
    Outlines trace_outlines(const Map& map)
    {
        Outlines outlines;
        outlines.reserve(map.component_cells.size());

        for (size_t component = 0; component < map.component_cells.size(); ++component)
        {
            std::map<Vertex, Edge> edges;

            auto add = [&](const Vertex& from, const Vertex& to)
            {
                assert(!edges.contains(from)); // A vertex with two exits is a diagonal pinch
                edges[from] = Edge {from, to};
            };

            for (const Vertex& c : map.component_cells[component])
            {
                auto outside = [&](int i, int j)
                {
                    return i < 0 || j < 0 || i >= static_cast<int>(map.w) || j >= static_cast<int>(map.h)
                        || map.cell_components[static_cast<size_t>(j) * map.w + i] != component;
                };

                if (outside(c.i, c.j - 1))
                {
                    add({c.i, c.j}, {c.i + 1, c.j});
                }

                if (outside(c.i + 1, c.j))
                {
                    add({c.i + 1, c.j}, {c.i + 1, c.j + 1});
                }

                if (outside(c.i, c.j + 1))
                {
                    add({c.i + 1, c.j + 1}, {c.i, c.j + 1});
                }

                if (outside(c.i - 1, c.j))
                {
                    add({c.i, c.j + 1}, {c.i, c.j});
                }
            }

            // Chain the edges into rings
            Rings rings;

            while (!edges.empty())
            {
                Ring ring;
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

            assert(!rings.empty());
            outlines.push_back(rings);
        }

        return outlines;
    }

    //--------------------------------------------------------------------------
    // Everything a mover meets: the flats and the grid, a wall per outline edge, which cells
    // hold floor, and one partnered pair per passage between two components
    // Returns the wall lying on each directed boundary edge, which is what naming an area
    // then needs to say what stands on its border
    WallIndex build(quetzal::stratum_sim::DiscreteLayout& layout, const Map& map, const Outlines& outlines,
        quetzal::stratum_sim::value_type height_ceiling)
    {
        using namespace quetzal::stratum_sim;

        layout.clear();
        layout.set_flats(value_type(0), height_ceiling);
        layout.set_grid(point_type(value_type(0), value_type(0), value_type(0)), value_type(1),
            discrete_vector_type(static_cast<long>(map.w), static_cast<long>(map.h)));

        WallIndex walls;

        for (size_t component = 0; component < outlines.size(); ++component)
        {
            const Rings& rings = outlines[component];

            std::vector<polygon_type> polygons;
            polygons.reserve(rings.size());
            for (const Ring& ring : rings)
            {
                polygons.push_back(to_polygon(ring));
            }

            DiscreteLayout::ids_type component_starts = layout.create_walls(polygons);

            // A ring's walls run contiguously from the wall it starts at
            for (size_t n = 0; n < rings.size(); ++n)
            {
                const Ring& ring = rings[n];
                for (size_t i = 0; i < ring.size(); ++i)
                {
                    walls[{ring[i], ring[(i + 1) % ring.size()]}] = component_starts[n] + i;
                }
            }

            // Openness comes from what the file said of each cell, both formats stating it
            for (const Vertex& c : map.component_cells[component])
            {
                layout.set_cell_open(GridPosition {c.i, c.j}, true);
            }
        }

        // Walls are one cell edge each, so a shared edge is one pair
        for (const Passage& passage : map.passages)
        {
            Edge edge_a = boundary_edge(passage.cell, passage.other);
            Edge edge_b = boundary_edge(passage.other, passage.cell);

            auto a = walls.find({edge_a.from, edge_a.to});
            auto b = walls.find({edge_b.from, edge_b.to});
            assert(a != walls.end() && b != walls.end()); // A passage not on either boundary

            layout.connect(a->second, b->second);
            layout.set_passable(a->second, passage.passable);
            layout.set_passable(b->second, passage.passable);
        }

        return walls;
    }

    //--------------------------------------------------------------------------
    // One Room per contiguous run of the cells the format called room floor, its outline
    // traced the same way a component's is. A format that names no rooms builds none, and
    // corridors, doorways and shafts are never rooms in either
    // A room's outline edge carries the wall standing on it, or nullid where the room opens
    // onto a corridor with nothing between them
    void create_rooms(quetzal::stratum_sim::DiscreteLayout& layout, const Map& map, const WallIndex& walls)
    {
        using namespace quetzal::stratum_sim;

        if (map.cell_room.empty())
        {
            return;
        }

        Map rooms;
        rooms.w = map.w;
        rooms.h = map.h;

        find_components(rooms,
            [&](int i, int j) { return map.cell_room[static_cast<size_t>(j) * map.w + i]; },
            [](const Vertex&, const Vertex&) { return true; });

        Outlines outlines = trace_outlines(rooms);

        for (size_t component = 0; component < rooms.component_cells.size(); ++component)
        {
            Room room;

            // The first outline alone. A room enclosing something it is not needs the rest,
            // which waits on a format that can say so
            const Ring& ring = outlines[component].front();

            for (size_t i = 0; i < ring.size(); ++i)
            {
                auto w = walls.find({ring[i], ring[(i + 1) % ring.size()]});

                Room::Vertex vertex;
                vertex.position = point_type(static_cast<value_type>(ring[i].i),
                    static_cast<value_type>(ring[i].j), value_type(0));
                vertex.wall_id = w != walls.end() ? w->second : nullid;
                vertex.type = vertex.wall_id == nullid || layout.wall(vertex.wall_id).passable
                    ? Room::Type::Open
                    : Room::Type::Wall;

                room.outline().push_back(vertex);
            }

            for (const Vertex& c : rooms.component_cells[component])
            {
                room.add_cell(GridPosition {c.i, c.j});
            }

            layout.create_room(room);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::detail::read_pair(const std::string& s, char separator, long& first, long& second)
{
    size_t n = s.find(separator);
    if (n == std::string::npos)
    {
        return false;
    }

    try
    {
        size_t used = 0;
        first = std::stol(s.substr(0, n), &used);
        if (used != n)
        {
            return false;
        }

        std::string rest = s.substr(n + 1);
        second = std::stol(rest, &used);
        if (used != rest.size())
        {
            return false;
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
std::unique_ptr<quetzal::stratum_sim::DiscreteLayout> quetzal::stratum_sim::load_layout(const std::filesystem::path& filepath, value_type height_ceiling)
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

    Header header;

    if (header_row(lines.front()))
    {
        header = read_header(lines.front());
        lines.erase(lines.begin());

        if (header.version != 1)
        {
            throw std::runtime_error(std::format("level file {} is {} format version {}, which is not one this reads",
                filepath.string(), header.generator, header.version));
        }
    }

    if (lines.empty())
    {
        throw std::runtime_error(std::format("level file {} holds no rows", filepath.string()));
    }

    Map map;

    if (header.generator == "catagen")
    {
        auto [nx, ny] = wall_extent(header, lines, filepath);
        map = read_walls(lines, nx, ny, filepath);
    }
    else if (header.generator.empty() || header.generator == "dungen")
    {
        map = read_block(lines);
    }
    else
    {
        throw std::runtime_error(std::format("level file {} was written by {}, whose format this does not read",
            filepath.string(), header.generator));
    }

    // Built only once every way the read can fail has passed, so a throw leaves nothing behind
    auto layout = std::make_unique<DiscreteLayout>();

    Outlines outlines = trace_outlines(map);
    WallIndex walls = build(*layout, map, outlines, height_ceiling);
    create_rooms(*layout, map, walls);

    return layout;
}
