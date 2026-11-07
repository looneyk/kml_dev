//------------------------------------------------------------------------------
// dungen
// generate_surface1.cpp
//------------------------------------------------------------------------------

#include "generate_surface1.hpp"
#include "place_staircases.hpp"
#include <vector>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Zone quetzal::dungen::generate_surface1(id_type id, const string& name, const Parameters& parameters)
{
    assert(parameters.algorithm == Algorithm::Surface1);
    validate(parameters);

    random_type random(parameters.seed.value());
    auto [nx, ny] = parameters.extent;

    Zone zone(id, name, parameters.extent, CellType::Solid);
    vector<point_type> open_cells;

    for (coord_type y = 1; y < ny - 1; ++y)
    {
        for (coord_type x = 1; x < nx - 1; ++x)
        {
            open_cells.push_back({x, y});
        }
    }

    zone.create_room(open_cells);

    place_staircases(zone, open_cells, parameters.staircase_up_count, parameters.staircase_down_count, random);

    return zone;
}
