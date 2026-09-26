//------------------------------------------------------------------------------
// dungen
// generate.cpp
//------------------------------------------------------------------------------

#include "generate.hpp"
#include "generate_cave1.hpp"
#include "generate_grid1.hpp"
#include "generate_maze1.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
void quetzal::dungen::log_warning(const string& warning)
{
    clog << "warning: " << warning << endl;
    return;
}

//------------------------------------------------------------------------------
quetzal::dungen::Level quetzal::dungen::generate(id_type id, const string& name, const Parameters& parameters, const warning_function_type& warning)
{
    switch (parameters.algorithm)
    {
        case Algorithm::Maze1:
            return generate_maze1(id, name, parameters, warning);

        case Algorithm::Grid1:
            return generate_grid1(id, name, parameters, warning);

        case Algorithm::Cave1:
            return generate_cave1(id, name, parameters, warning);

        default:
            assert(false);
    }

    return Level(id, name, parameters.size, CellType::Solid);
}
