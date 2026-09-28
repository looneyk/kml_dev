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

    // Writes the warning to std::clog
    void log_warning(const std::string& warning);

    // Builds the layout with the algorithm that parameters.algorithm selects; id and name are stored in the Zone
    // Deterministic in its parameters, the seed included
    // Throws std::invalid_argument by way of validate, and std::runtime_error if a staircase count above zero has none
    // placed (place_staircases). Placing fewer staircases than were drawn is a warning
    Zone generate(id_type id, const std::string& name, const Parameters& parameters, const warning_function_type& warning = log_warning);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_HPP
