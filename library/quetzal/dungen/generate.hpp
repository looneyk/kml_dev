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

    // Writes the warning to std::clog
    void log_warning(const std::string& warning);

    // Builds the layout parameters.algorithm selects; id and name are stored in the Level
    // Deterministic in its parameters, the seed included
    // Throws std::invalid_argument by way of validate, and std::runtime_error where a staircase count above zero has none
    // placed (place_staircases). Fewer staircases placed than were drawn is a warning
    Level generate(id_type id, const std::string& name, const Parameters& parameters, const warning_function_type& warning = log_warning);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_GENERATE_HPP
