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

    // The cells while the automaton is running over them, true where the cell is rock. The
    // Level is not written until the shape has settled, a pass having to read the whole of
    // the one before it
    using grid_type = vector<bool>;

    //--------------------------------------------------------------------------
    size_t slot(vector_type size, coord_type x, coord_type y)
    {
        return static_cast<size_t>(y * size.x() + x);
    }

    //--------------------------------------------------------------------------
    // Anything off the grid counts as rock, so that the border holds the cavern in
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
    // Every cell drawn rock or open at random, the border left rock so that no cavern opens
    // onto the edge of the level
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
    // One pass of the automaton over every cell off the border: rock stays rock while enough
    // of its eight neighbors are rock, and open ground turns to rock where more of them are.
    // The pass reads the grid it was given throughout, so every cell turns on the same state
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
    // The cells of the largest region of open ground, a region being what can be reached from
    // one of its cells without crossing rock
    vector<point_type> largest_region(const grid_type& grid, vector_type size)
    {
        auto [nx, ny] = size;

        vector<bool> seen(static_cast<size_t>(nx * ny), false);
        vector<point_type> largest;
        vector<point_type> region;
        vector<point_type> pending;

        for (coord_type y = 0; y < ny; ++y)
        {
            for (coord_type x = 0; x < nx; ++x)
            {
                if (grid[slot(size, x, y)] || seen[slot(size, x, y)])
                {
                    continue;
                }

                region.clear();
                pending.assign(1, point_type {x, y});
                seen[slot(size, x, y)] = true;

                while (!pending.empty())
                {
                    point_type position = pending.back();
                    pending.pop_back();
                    region.push_back(position);

                    for (const auto& step : cardinal_directions)
                    {
                        point_type neighbor = position + step;
                        if (solid(grid, size, neighbor.x(), neighbor.y()) || seen[slot(size, neighbor.x(), neighbor.y())])
                        {
                            continue;
                        }

                        seen[slot(size, neighbor.x(), neighbor.y())] = true;
                        pending.push_back(neighbor);
                    }
                }

                if (region.size() > largest.size())
                {
                    largest = region;
                }
            }
        }

        return largest;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::dungen::Level quetzal::dungen::generate_cave1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Cave1);
    validate(parameters);

    const Cave1Parameters& cave1 = parameters.cave1;
    random_type random(parameters.seed.value());

    grid_type grid = noise(parameters.size, cave1.fill, random);

    for (size_t i = 0; i < cave1.smoothing_passes; ++i)
    {
        grid = smooth(grid, parameters.size, cave1);
    }

    Level level(id, name, parameters.size, CellType::Solid);

    // Only the largest region is kept, so that what the smoothing walled off stays rock and
    // everything the level does hold is reachable from everywhere else it holds
    vector<point_type> region = largest_region(grid, parameters.size);
    for (point_type position : region)
    {
        level.cell(position).set_type(CellType::Corridor);
    }

    // The open ground is all of one region, so any of it will do
    place_staircases(level, region, parameters.staircase_down_count, parameters.staircase_up_count, random);
    move_staircases_off_doorways(level, random);
    face_staircases(level, random);

    return level;
}
