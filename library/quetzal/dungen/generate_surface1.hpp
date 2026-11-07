#if !defined(QUETZAL_DUNGEN_GENERATE_SURFACE1_HPP)
#define QUETZAL_DUNGEN_GENERATE_SURFACE1_HPP
//------------------------------------------------------------------------------
// dungen
// generate_surface1.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // Every cell inside the solid border open, with the staircases placed on it
    // No rooms and no doors: open floor is corridor
    // parameters.algorithm is expected to be Algorithm::Surface1
    // Throws std::invalid_argument by way of validate, and std::runtime_error if a staircase count above zero has none placed
    Zone generate_surface1(id_type id, const std::string& name, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_SURFACE1_HPP
