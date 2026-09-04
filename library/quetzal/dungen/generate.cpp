//------------------------------------------------------------------------------
// dungen
// generate.cpp
//------------------------------------------------------------------------------

#include "generate.hpp"
#include "generate_cave1.hpp"
#include "generate_grid1.hpp"
#include "generate_maze1.hpp"
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Level quetzal::dungen::generate(id_type id, const string& name, const Parameters& parameters)
{
    switch (parameters.algorithm)
    {
        case Algorithm::Maze1:
            return generate_maze1(id, name, parameters);

        case Algorithm::Grid1:
            return generate_grid1(id, name, parameters);

        case Algorithm::Cave1:
            return generate_cave1(id, name, parameters);

        default:
            assert(false);
    }

    return Level(id, name, parameters.size, CellType::Solid);
}
