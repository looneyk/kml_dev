#if !defined(QUETZAL_DUNGEN_GENERATE_HPP)
#define QUETZAL_DUNGEN_GENERATE_HPP
//------------------------------------------------------------------------------
// dungen
// generate.hpp
//------------------------------------------------------------------------------

#include "Zone.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // Builds the layout with the selected algorithm
    // Deterministic in its parameters, the seed included
    // Throws std::invalid_argument by way of validate, and std::runtime_error if no staircase can be placed
    // Placing fewer staircases than were drawn is a warning condition
    Zone generate(id_type id, const std::string& name, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_HPP
