//------------------------------------------------------------------------------
// stratum_format
// grid.cpp
//------------------------------------------------------------------------------

#include "grid.hpp"
#include <algorithm>
#include <variant>
#include <cassert>

namespace
{

    using namespace quetzal::stratum_format;

    //--------------------------------------------------------------------------
    // x across and y up, in the written grid's own positions
    char glyph_at(const Cells& cells, size_t x, size_t y)
    {
        return cells.rows[cells.rows.size() - 1 - y][x];
    }

    //--------------------------------------------------------------------------
    char cell_glyph(const Cells& cells, const discrete_point_type& cell_position)
    {
        bool wall = cells.notation == Notation::Wall;
        size_t x = static_cast<size_t>(wall ? 2 * cell_position.i() + 1 : cell_position.i());
        size_t y = static_cast<size_t>(wall ? 2 * cell_position.j() + 1 : cell_position.j());
        return glyph_at(cells, x, y);
    }

    //--------------------------------------------------------------------------
    // The edge on a cell's side in a direction, as its position in the wall notation's written grid, which identifies it
    // in either notation
    discrete_point_type edge_position(const discrete_point_type& cell_position, const discrete_vector_type& direction)
    {
        return {2 * cell_position.i() + 1 + direction.x(), 2 * cell_position.j() + 1 + direction.y()};
    }

    //--------------------------------------------------------------------------
    // The places where a fill stops, besides the grid's own walls: edges, by their edge_position, and whole cells
    struct FillBarriers
    {
        std::set<discrete_point_type> edge_positions;
        std::set<discrete_point_type> cell_positions;
    };

    //--------------------------------------------------------------------------
    FillBarriers fill_barriers(const std::vector<Barrier>& barriers)
    {
        FillBarriers result;

        for (const auto& barrier : barriers)
        {
            if (barrier.normal.has_value())
            {
                result.edge_positions.insert(edge_position(barrier.cell_position, -*barrier.normal));
            }
            else
            {
                result.cell_positions.insert(barrier.cell_position);
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Fills from the location across every edge that has no wall, no door, and no barrier of the layout or the room
    std::set<discrete_point_type> fill(const Cells& cells, const RoomLocator& locator, const FillBarriers& layout_barriers)
    {
        FillBarriers room_barriers = fill_barriers(locator.barriers);

        auto barred_edge = [&](const discrete_point_type& position)
        {
            return layout_barriers.edge_positions.contains(position) || room_barriers.edge_positions.contains(position);
        };

        auto barred_cell = [&](const discrete_point_type& cell_position)
        {
            return layout_barriers.cell_positions.contains(cell_position) || room_barriers.cell_positions.contains(cell_position);
        };

        assert(!barred_cell(locator.cell_position));

        std::set<discrete_point_type> result {locator.cell_position};
        std::vector<discrete_point_type> pending {locator.cell_position};

        while (!pending.empty())
        {
            discrete_point_type cell_position = pending.back();
            pending.pop_back();

            for (const auto& direction : quetzal::math::cardinal_directions<coord_type, 2>)
            {
                discrete_point_type neighbor = cell_position + direction;

                if (has_wall(cells, cell_position, direction) || barred_edge(edge_position(cell_position, direction)) || barred_cell(neighbor))
                {
                    continue;
                }

                assert(on_grid(cells, neighbor) && floor_cell(cells, neighbor));

                if (result.insert(neighbor).second)
                {
                    pending.push_back(neighbor);
                }
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Fills row by row between the vertical edges that cross each row
    std::set<discrete_point_type> enclosed(const Cells& cells, const Perimeter& perimeter)
    {
        const std::vector<discrete_point_type>& junctions = perimeter.junctions;
        size_t n = junctions.size();

        auto [lowest, highest] = std::minmax_element(junctions.begin(), junctions.end(),
            [](const discrete_point_type& a, const discrete_point_type& b) { return a.j() < b.j(); });

        std::set<discrete_point_type> result;

        for (coord_type j = lowest->j(); j < highest->j(); ++j)
        {
            std::vector<coord_type> crossings;

            for (size_t k = 0; k < n; ++k)
            {
                const discrete_point_type& a = junctions[k];
                const discrete_point_type& b = junctions[(k + 1) % n];

                if (a.i() == b.i() && std::min(a.j(), b.j()) <= j && j < std::max(a.j(), b.j()))
                {
                    crossings.push_back(a.i());
                }
            }

            std::sort(crossings.begin(), crossings.end());
            assert(crossings.size() % 2 == 0);

            for (size_t k = 0; k + 1 < crossings.size(); k += 2)
            {
                for (coord_type i = crossings[k]; i < crossings[k + 1]; ++i)
                {
                    if (floor_door_cell(cells, {i, j}))
                    {
                        result.insert({i, j});
                    }
                }
            }
        }

        return result;
    }

} // namespace

//------------------------------------------------------------------------------
bool quetzal::stratum_format::on_grid(const Cells& cells, const discrete_point_type& cell_position)
{
    return cell_position.i() >= 0 && cell_position.j() >= 0 && cell_position.i() < cells.extent.x() && cell_position.j() < cells.extent.y();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_format::floor_cell(const Cells& cells, const discrete_point_type& cell_position)
{
    return cell_glyph(cells, cell_position) == '.';
}

//------------------------------------------------------------------------------
bool quetzal::stratum_format::floor_door_cell(const Cells& cells, const discrete_point_type& cell_position)
{
    return floor_cell(cells, cell_position) || cell_glyph(cells, cell_position) == '+';
}

//------------------------------------------------------------------------------
bool quetzal::stratum_format::stair_cell(const Cells& cells, const discrete_point_type& cell_position)
{
    char glyph = cell_glyph(cells, cell_position);
    return glyph == '<' || glyph == '>';
}

//------------------------------------------------------------------------------
bool quetzal::stratum_format::has_wall(const Cells& cells, const discrete_point_type& cell_position, const discrete_vector_type& direction)
{
    if (cells.notation == Notation::Wall)
    {
        discrete_point_type position = edge_position(cell_position, direction);
        return glyph_at(cells, static_cast<size_t>(position.i()), static_cast<size_t>(position.j())) != ' ';
    }

    discrete_point_type neighbor = cell_position + direction;
    if (!on_grid(cells, neighbor))
    {
        return true;
    }

    // Between a doorway block and floor, the wall is on the floor's side alone
    char glyph = cell_glyph(cells, neighbor);
    return glyph == '#' || stair_cell(cells, neighbor) || (glyph == '+' && cell_glyph(cells, cell_position) != '+');
}

//------------------------------------------------------------------------------
std::vector<std::set<quetzal::stratum_format::discrete_point_type>> quetzal::stratum_format::room_cells(const Document& document)
{
    FillBarriers layout_barriers = fill_barriers(document.barriers);

    std::vector<std::set<discrete_point_type>> result;
    result.reserve(document.rooms.size());

    for (const Room& room : document.rooms)
    {
        if (const Perimeter* perimeter = std::get_if<Perimeter>(&room.identification); perimeter != nullptr)
        {
            result.push_back(enclosed(document.cells, *perimeter));
        }
        else
        {
            result.push_back(fill(document.cells, std::get<RoomLocator>(room.identification), layout_barriers));
        }
    }

    return result;
}

//------------------------------------------------------------------------------
std::vector<std::vector<quetzal::stratum_format::WallAddress>> quetzal::stratum_format::door_walls(const Cells& cells)
{
    const auto& directions = quetzal::math::cardinal_directions<coord_type, 2>;
    auto floor_on_grid = [&](const discrete_point_type& cell_position) { return on_grid(cells, cell_position) && floor_cell(cells, cell_position); };

    std::vector<std::vector<WallAddress>> result;

    auto add = [&](std::vector<WallAddress>& walls, const discrete_point_type& cell_position, const discrete_vector_type& normal)
    {
        if (floor_on_grid(cell_position))
        {
            walls.push_back({cell_position, normal});
        }
    };

    if (cells.notation == Notation::Wall)
    {
        // A door edge's position in the written grid has one odd coordinate, the cells either side of it along the other
        for (coord_type y = 0; y < static_cast<coord_type>(cells.rows.size()); ++y)
        {
            for (coord_type x = 0; x < static_cast<coord_type>(cells.rows.front().size()); ++x)
            {
                if ((x % 2 == 1) == (y % 2 == 1) || glyph_at(cells, static_cast<size_t>(x), static_cast<size_t>(y)) != '+')
                {
                    continue;
                }

                std::vector<WallAddress> walls;
                discrete_vector_type across = x % 2 == 0 ? directions[0] : directions[1];
                discrete_point_type after {x / 2, y / 2};

                add(walls, after - across, -across);
                add(walls, after, across);

                if (!walls.empty())
                {
                    result.push_back(std::move(walls));
                }
            }
        }

        return result;
    }

    for (coord_type j = 0; j < cells.extent.y(); ++j)
    {
        for (coord_type i = 0; i < cells.extent.x(); ++i)
        {
            discrete_point_type cell_position {i, j};
            if (cell_glyph(cells, cell_position) != '+')
            {
                continue;
            }

            std::vector<WallAddress> walls;

            for (const auto& direction : directions)
            {
                add(walls, cell_position + direction, direction);
            }

            if (!walls.empty())
            {
                result.push_back(std::move(walls));
            }
        }
    }

    return result;
}
