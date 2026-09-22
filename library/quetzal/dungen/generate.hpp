#if !defined(QUETZAL_DUNGEN_GENERATE_HPP)
#define QUETZAL_DUNGEN_GENERATE_HPP
//------------------------------------------------------------------------------
// dungen
// generate.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"
#include <string>

namespace quetzal::dungen
{

    // Builds the layout parameters.algorithm selects; id and name are stored in the Level
    // Deterministic in its parameters, the seed included
    // Throws std::invalid_argument by way of validate, and std::runtime_error where no staircase can be placed to be an
    // access point
    Level generate(id_type id, const std::string& name, const Parameters& parameters);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_HPP
