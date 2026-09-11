//------------------------------------------------------------------------------
// stratum_sim
// load_layout.cpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include "load_layout_catagen.hpp"
#include "load_layout_dungen.hpp"
#include "quetzal/common/file_util.hpp"
#include <charconv>
#include <format>
#include <fstream>
#include <map>
#include <set>
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

    using quetzal::stratum_sim::coord_type;
    using quetzal::stratum_sim::discrete_point_type;

    //--------------------------------------------------------------------------
    // The one format version this reads, both generators writing the same. A leading zero
    // marks pre-release: no file older than it survives a change, and none is kept readable
    constexpr auto format_version = "0.2";

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
    quetzal::stratum_sim::polygon_type to_polygon(const std::vector<discrete_point_type>& ring)
    {
        using quetzal::stratum_sim::point_type;
        using quetzal::stratum_sim::value_type;

        quetzal::stratum_sim::points_type points;
        points.reserve(ring.size());

        for (const auto& grid_position : ring)
        {
            points.push_back(point_type(static_cast<value_type>(grid_position.i()), static_cast<value_type>(grid_position.j()), value_type(0)));
        }

        return quetzal::stratum_sim::polygon_type(points.begin(), points.end());
    }

    //--------------------------------------------------------------------------
    // The edge one cell shares with an orthogonally adjacent one, as the directed boundary
    // edge that lies on it wound for that cell. The other cell's is its reverse, which is
    // what makes the two walls on it partners
    Edge boundary_edge(const discrete_point_type& grid_position, const discrete_point_type& grid_position_other)
    {
        if (grid_position_other.i() > grid_position.i())
        {
            return {{grid_position.i() + 1, grid_position.j()}, {grid_position.i() + 1, grid_position.j() + 1}};
        }

        if (grid_position_other.i() < grid_position.i())
        {
            return {{grid_position.i(), grid_position.j() + 1}, {grid_position.i(), grid_position.j()}};
        }

        if (grid_position_other.j() > grid_position.j())
        {
            return {{grid_position.i() + 1, grid_position.j() + 1}, {grid_position.i(), grid_position.j() + 1}};
        }

        return {{grid_position.i(), grid_position.j()}, {grid_position.i() + 1, grid_position.j()}};
    }

    //--------------------------------------------------------------------------
    using Ring = std::vector<discrete_point_type>;
    using Rings = std::vector<Ring>; // The outlines of one component
    using Outlines = std::vector<Rings>; // By component
    using WallIndex = std::map<std::pair<discrete_point_type, discrete_point_type>, id_type>; // The wall lying on one directed boundary edge

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
            std::map<discrete_point_type, Edge> edges;

            auto add = [&](const discrete_point_type& from, const discrete_point_type& to)
            {
                assert(!edges.contains(from)); // A vertex with two exits is a diagonal pinch
                edges[from] = Edge {from, to};
            };

            for (const auto& grid_position : map.component_cells[component])
            {
                auto outside = [&](coord_type i, coord_type j)
                {
                    return i < 0 || j < 0 || i >= map.extent.x() || j >= map.extent.y()
                        || map.cell_components[map.slot({i, j})] != component;
                };

                if (outside(grid_position.i(), grid_position.j() - 1))
                {
                    add({grid_position.i(), grid_position.j()}, {grid_position.i() + 1, grid_position.j()});
                }

                if (outside(grid_position.i() + 1, grid_position.j()))
                {
                    add({grid_position.i() + 1, grid_position.j()}, {grid_position.i() + 1, grid_position.j() + 1});
                }

                if (outside(grid_position.i(), grid_position.j() + 1))
                {
                    add({grid_position.i() + 1, grid_position.j() + 1}, {grid_position.i(), grid_position.j() + 1});
                }

                if (outside(grid_position.i() - 1, grid_position.j()))
                {
                    add({grid_position.i(), grid_position.j() + 1}, {grid_position.i(), grid_position.j()});
                }
            }

            // Chain the edges into rings
            Rings rings;

            while (!edges.empty())
            {
                Ring ring;
                auto start_grid_position = edges.begin()->first;
                auto grid_position = start_grid_position;

                do
                {
                    auto i = edges.find(grid_position);
                    assert(i != edges.end()); // A boundary that does not close
                    ring.push_back(grid_position);
                    grid_position = i->second.grid_position_to;
                    edges.erase(i);
                } while (grid_position != start_grid_position);

                rings.push_back(ring);
            }

            assert(!rings.empty());
            outlines.push_back(rings);
        }

        return outlines;
    }

    //--------------------------------------------------------------------------
    // Everything a mover meets: the flats and the grid, a wall per outline edge, which cells
    // are floor, and one partnered pair per passage between two components
    // Returns the wall lying on each directed boundary edge, which is what describing an area
    // then needs to describe what its border is made of
    WallIndex build(quetzal::stratum_sim::DiscreteLayout& layout, const Map& map, const Outlines& outlines,
        quetzal::stratum_sim::value_type height_ceiling,
        const quetzal::stratum_sim::door_type& door, std::set<quetzal::id_type>& doors)
    {
        using namespace quetzal::stratum_sim;

        layout.clear();
        layout.set_flats(value_type(0), height_ceiling);
        layout.set_grid(point_type(value_type(0), value_type(0), value_type(0)), value_type(1), map.extent);

        WallIndex walls;

        for (size_t component = 0; component < outlines.size(); ++component)
        {
            const Rings& rings = outlines[component];

            std::vector<polygon_type> polygons;
            polygons.reserve(rings.size());
            for (const auto& ring : rings)
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

        }

        // Openness comes from what the file said of each cell, both formats stating it
        for (coord_type j = 0; j < map.extent.y(); ++j)
        {
            for (coord_type i = 0; i < map.extent.x(); ++i)
            {
                discrete_point_type grid_position {i, j};

                if (map.cell_open[map.slot(grid_position)])
                {
                    layout.set_cell_open(grid_position, true);
                }
            }
        }

        // Walls are one cell edge each, so a shared edge is one pair
        for (const auto& passage : map.passages)
        {
            Edge edge_a = boundary_edge(passage.grid_position, passage.grid_position_other);
            Edge edge_b = boundary_edge(passage.grid_position_other, passage.grid_position);

            auto a = walls.find({edge_a.grid_position_from, edge_a.grid_position_to});
            auto b = walls.find({edge_b.grid_position_from, edge_b.grid_position_to});
            assert(a != walls.end() && b != walls.end()); // A passage not on either boundary

            layout.connect(a->second, b->second);

            // What the file called a door is reported as one pair, for a caller with a world
            // to hang something on. The walls themselves settle nothing about crossing
            if (passage.passable)
            {
                doors.insert(a->second);
                doors.insert(b->second);

                if (door)
                {
                    door({a->second, b->second});
                }
            }
        }

        // A block is a cell of wall a mover may stand in. It belongs to the area around it,
        // so the area's own outline walls the sides it presents to solid cells and a mover leaving
        // sideways meets the wall end wrapping around into the next area. What it presents
        // to open floor is interior to that area and has no outline wall, so the block
        // is ringed with four walls of its own, each wound about the cell outside it
        // A face toward floor therefore has a wall the area sees and the block does not,
        // which is what a door hangs on and what leaves the way out of a block empty
        if (!map.blocks.empty())
        {
            std::vector<polygon_type> polygons;
            std::vector<Ring> rings;
            polygons.reserve(map.blocks.size());
            rings.reserve(map.blocks.size());

            for (const Block& block : map.blocks)
            {
                int i = block.grid_position.i();
                int j = block.grid_position.j();
                rings.push_back({{i + 1, j + 1}, {i + 1, j}, {i, j}, {i, j + 1}});
                polygons.push_back(to_polygon(rings.back()));
            }

            DiscreteLayout::ids_type block_starts = layout.create_walls(polygons);

            for (size_t n = 0; n < rings.size(); ++n)
            {
                const Ring& ring = rings[n];
                for (size_t i = 0; i < ring.size(); ++i)
                {
                    walls[{ring[i], ring[(i + 1) % ring.size()]}] = block_starts[n] + i;
                }
            }
        }

        for (const Block& block : map.blocks)
        {
            quetzal::stratum_sim::DiscreteLayout::ids_type block_walls;

            for (const auto& neighbor : block.neighbor_grid_positions)
            {
                Edge edge = boundary_edge(neighbor, block.grid_position);
                auto w = walls.find({edge.grid_position_from, edge.grid_position_to});

                if (w != walls.end())
                {
                    block_walls.push_back(w->second);
                    doors.insert(w->second);
                }
            }

            if (door && !block_walls.empty())
            {
                door(block_walls);
            }
        }

        return walls;
    }

    //--------------------------------------------------------------------------
    // One Room per contiguous run of the cells the format called room floor, its outline
    // traced the same way a component's is. A format that defines no rooms builds none, and
    // corridors, doorways and stairs are never rooms in either
    // A room's outline edge references the wall lying on it, or nullid where the room opens
    // onto a corridor with nothing between them
    void create_rooms(quetzal::stratum_sim::DiscreteLayout& layout, const Map& map, const WallIndex& walls,
        const std::set<quetzal::id_type>& doors)
    {
        using namespace quetzal::stratum_sim;

        if (map.cell_room.empty())
        {
            return;
        }

        Map rooms;
        rooms.extent = map.extent;

        find_components(rooms,
            [&](coord_type i, coord_type j) { return map.cell_room[map.slot({i, j})]; },
            [](const discrete_point_type&, const discrete_point_type&) { return true; });

        Outlines outlines = trace_outlines(rooms);

        for (size_t component = 0; component < rooms.component_cells.size(); ++component)
        {
            Room room;

            // The first outline alone. A room enclosing something it is not needs the rest,
            // which waits on a format that can express it
            const Ring& ring = outlines[component].front();

            for (size_t i = 0; i < ring.size(); ++i)
            {
                auto w = walls.find({ring[i], ring[(i + 1) % ring.size()]});

                Room::Vertex vertex;
                vertex.position = point_type(static_cast<value_type>(ring[i].i()),
                    static_cast<value_type>(ring[i].j()), value_type(0));
                vertex.wall_id = w != walls.end() ? w->second : nullid;

                // No wall is an opening, a wall the file called a door is a door, and what is
                // left is wall. Nothing here asks whether the door is open: a room's outline
                // describes what its border is made of, not what state it is in
                vertex.type = vertex.wall_id == nullid ? Room::Type::Open
                    : doors.contains(vertex.wall_id) ? Room::Type::Door
                    : Room::Type::Wall;

                room.outline().push_back(vertex);
            }

            for (const auto& grid_position : rooms.component_cells[component])
            {
                room.add_cell(discrete_point_type {grid_position.i(), grid_position.j()});
            }

            layout.create_room(room);
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
std::unique_ptr<quetzal::stratum_sim::DiscreteLayout> quetzal::stratum_sim::load_layout(const std::filesystem::path& filepath,
    value_type height_ceiling, const door_type& door, Format* format)
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

        if (header.version != format_version)
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
    Format format_read = Format::Block;

    if (header.generator == "catagen")
    {
        map = read_walls(lines, wall_extent(header, lines, filepath), filepath);
        format_read = Format::Wall;
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
    std::set<id_type> doors;

    WallIndex walls = build(*layout, map, outlines, height_ceiling, door, doors);
    create_rooms(*layout, map, walls, doors);

    if (format != nullptr)
    {
        *format = format_read;
    }

    return layout;
}
