#if !defined(QUETZAL_DUNGEN_ALGORITHM_HPP)
#define QUETZAL_DUNGEN_ALGORITHM_HPP
//------------------------------------------------------------------------------
// dungen
// Algorithm.hpp
//------------------------------------------------------------------------------

#include <iosfwd>
#include <string>
#include <string_view>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    // Which layout a run builds, each implemented in the generate_ file its name suffixes
    enum class Algorithm
    {
        Maze1, // Rooms dropped at random, a maze flooded through every cell they left, pruned back
        Grid1, // A room inside most sectors of a coarse grid, adjacent sectors joined
        Cave1  // Noise smoothed into caverns, the largest connected region kept
    };

    // The command line and file header spelling, lower case
    std::string to_string(Algorithm algorithm);

    // Throws std::invalid_argument naming what was not an algorithm
    Algorithm to_algorithm(std::string_view s);

    std::ostream& operator<<(std::ostream& os, const Algorithm& algorithm);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_ALGORITHM_HPP
