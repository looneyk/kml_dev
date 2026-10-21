#if !defined(QUETZAL_CATAGEN_GENERATE_HPP)
#define QUETZAL_CATAGEN_GENERATE_HPP
//------------------------------------------------------------------------------
// catagen
// generate.hpp
//------------------------------------------------------------------------------

#include "Parameters.hpp"
#include "Zone.hpp"
#include <string>

namespace quetzal::catagen
{

    // Partitions the extent into rooms, keeps as many of them as the density specifies,
    // joins them with corridors, and adds doors so that all rooms are reachable
    // Deterministic in its parameters, the seed included
    // Throws std::invalid_argument by way of validate, and std::runtime_error where no staircase can be placed to be an access point
    Zone generate(const std::string& name, const Parameters& parameters);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_GENERATE_HPP
