#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_DUNGEN_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_DUNGEN_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout_dungen.hpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include <string>
#include <vector>

namespace quetzal::stratum_sim::detail
{

    // dungen's block format, one character per cell in its own symbols: '#' solid and '0'
    // null are rock, ' ' room and '.' corridor are open floor, '|' is a doorway and '>' a
    // stairway. Nothing separates two open cells, so a doorway or stairway becomes a component
    // of its own even where nothing separates it, and every passage is one a mover may cross
    // The first line given is the highest row, the grid origin being lower left
    Map read_block(const std::vector<std::string>& lines);

} // namespace quetzal::stratum_sim::detail

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_DUNGEN_HPP
