#if !defined(QUETZAL_DUNGEN_GENERATE_GRID1_HPP)
#define QUETZAL_DUNGEN_GENERATE_GRID1_HPP
//------------------------------------------------------------------------------
// dungen
// generate_grid1.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // The zone cut into a coarse grid of sectors, a room somewhere inside most of them and a
    // single junction cell inside the rest, then orthogonally adjacent sectors joined by
    // corridors that bend once in the rock between and enter a room through a door
    // Reads parameters.grid1; parameters.algorithm is expected to be Algorithm::Grid1
    // Throws std::invalid_argument by way of validate
    Level generate_grid1(id_type id, const std::string& name, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_GRID1_HPP
