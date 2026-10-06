//------------------------------------------------------------------------------
// dungen
// generate_cave1.cpp
//------------------------------------------------------------------------------

#include "generate_cave1.hpp"
#include "place_staircases.hpp"
#include <vector>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;

    // The cells during smoothing, true where solid. Each pass reads the whole of the one before, so the Zone is written
    // only once smoothing is done
    using grid_type = vector<bool>;

    //--------------------------------------------------------------------------
    size_t slot(vector_type size, coord_type x, coord_type y)
    {
        return static_cast<size_t>(y * size.x() + x);
    }

    //--------------------------------------------------------------------------
    // Off the grid counts as solid
    bool solid(const grid_type& grid, vector_type size, coord_type x, coord_type y)
    {
        if (x < 0 || y < 0 || x >= size.x() || y >= size.y())
        {
            return true;
        }

        return grid[slot(size, x, y)];
    }

    //--------------------------------------------------------------------------
    size_t solid_neighbors(const grid_type& grid, vector_type size, coord_type x, coord_type y)
    {
        size_t n = 0;

        for (coord_type dy = -1; dy <= 1; ++dy)
        {
            for (coord_type dx = -1; dx <= 1; ++dx)
            {
                if ((dx != 0 || dy != 0) && solid(grid, size, x + dx, y + dy))
                {
                    ++n;
                }
            }
        }

        return n;
    }

    //--------------------------------------------------------------------------
    // Every cell drawn solid or open at random, the border left solid
    grid_type noise(vector_type size, double fill, random_type& random)
    {
        auto [nx, ny] = size;
        grid_type grid(static_cast<size_t>(nx * ny), true);

        for (coord_type y = 1; y < ny - 1; ++y)
        {
            for (coord_type x = 1; x < nx - 1; ++x)
            {
                grid[slot(size, x, y)] = random.gen() < fill;
            }
        }

        return grid;
    }

    //--------------------------------------------------------------------------
    // One automaton pass over every cell off the border: a solid cell stays solid with at least survival_threshold solid
    // neighbors, and an open cell turns solid with at least birth_threshold. Every cell is computed from the input grid
    grid_type smooth(const grid_type& grid, vector_type size, const Cave1Parameters& cave1)
    {
        auto [nx, ny] = size;
        grid_type result = grid;

        for (coord_type y = 1; y < ny - 1; ++y)
        {
            for (coord_type x = 1; x < nx - 1; ++x)
            {
                size_t threshold = grid[slot(size, x, y)] ? cave1.survival_threshold : cave1.birth_threshold;
                result[slot(size, x, y)] = solid_neighbors(grid, size, x, y) >= threshold;
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // The cells of the largest connected area of open cells
    vector<point_type> largest_area(const grid_type& grid, vector_type size)
    {
        auto [nx, ny] = size;

        vector<bool> seen(static_cast<size_t>(nx * ny), false);
        vector<point_type> largest;
        vector<point_type> area;
        vector<point_type> pending;

        for (coord_type y = 0; y < ny; ++y)
        {
            for (coord_type x = 0; x < nx; ++x)
            {
                if (grid[slot(size, x, y)] || seen[slot(size, x, y)])
                {
                    continue;
                }

                area.clear();
                pending.assign(1, point_type {x, y});
                seen[slot(size, x, y)] = true;

                while (!pending.empty())
                {
                    point_type position = pending.back();
                    pending.pop_back();
                    area.push_back(position);

                    for (const auto& direction : cardinal_directions)
                    {
                        point_type neighbor = position + direction;
                        if (solid(grid, size, neighbor.x(), neighbor.y()) || seen[slot(size, neighbor.x(), neighbor.y())])
                        {
                            continue;
                        }

                        seen[slot(size, neighbor.x(), neighbor.y())] = true;
                        pending.push_back(neighbor);
                    }
                }

                if (area.size() > largest.size())
                {
                    largest = area;
                }
            }
        }

        return largest;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::dungen::Zone quetzal::dungen::generate_cave1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Cave1);
    validate(parameters);

    const Cave1Parameters& cave1 = parameters.cave1;
    random_type random(parameters.seed.value());

    grid_type grid = noise(parameters.extent, cave1.fill, random);

    for (size_t i = 0; i < cave1.smoothing_passes; ++i)
    {
        grid = smooth(grid, parameters.extent, cave1);
    }

    Zone zone(id, name, parameters.extent, CellType::Solid);

    // Only the largest area is kept, so every open cell is reachable from every other
    vector<point_type> area = largest_area(grid, parameters.extent);
    for (point_type position : area)
    {
        zone.cell(position).set_type(CellType::Corridor);
    }

    place_staircases(zone, area, parameters.staircase_up_count, parameters.staircase_down_count, random);

    return zone;
}
