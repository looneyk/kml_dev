#if !defined(QUETZAL_CATAGEN_GENERATE_HPP)
#define QUETZAL_CATAGEN_GENERATE_HPP
//------------------------------------------------------------------------------
// catagen
// generate.hpp
//------------------------------------------------------------------------------

#include "Level.hpp"
#include "Parameters.hpp"

namespace quetzal::catagen
{

    // Partitions the extent into rooms, keeps as many of them as the density asks for,
    // joins whatever that separated with corridors, and puts a door on enough of the walls
    // between them to make every room reachable
    // Deterministic in its parameters, the seed included, so the recorded header reproduces
    // the level exactly
    // Throws std::invalid_argument by way of validate
    Level generate(const Parameters& parameters);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_GENERATE_HPP
