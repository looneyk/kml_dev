#if !defined(QUETZAL_DUNGEN_GENERATE_MAZE1_HPP)
#define QUETZAL_DUNGEN_GENERATE_MAZE1_HPP
//------------------------------------------------------------------------------
// dungen
// generate_maze1.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // Rooms dropped at random until the density is met, doors punched into their walls, a
    // maze flooded through every cell they left, then dead ends and loops pruned back
    // Both dimensions are rounded down to odd, the maze running along the odd cells
    // Reads parameters.maze1; parameters.algorithm is expected to be Algorithm::Maze1
    // Throws std::invalid_argument by way of validate, and as place_staircases does
    Zone generate_maze1(id_type id, const std::string& name, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_MAZE1_HPP
