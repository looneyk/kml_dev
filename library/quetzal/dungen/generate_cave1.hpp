#if !defined(QUETZAL_DUNGEN_GENERATE_CAVE1_HPP)
#define QUETZAL_DUNGEN_GENERATE_CAVE1_HPP
//------------------------------------------------------------------------------
// dungen
// generate_cave1.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // Every cell drawn solid or open at random, smoothed into caverns by a cellular automaton
    // over the eight neighbors of a cell, then every region but the largest filled back in
    // No rooms and no doors: open floor is corridor, and the border is left solid
    // Reads parameters.cave1; parameters.algorithm is expected to be Algorithm::Cave1
    // Throws std::invalid_argument by way of validate, and as place_staircases does
    Level generate_cave1(id_type id, const std::string& name, const Parameters& parameters, const warning_function_type& warning);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_CAVE1_HPP
