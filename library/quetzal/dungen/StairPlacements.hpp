#if !defined(QUETZAL_DUNGEN_STAIRPLACEMENTS_HPP)
#define QUETZAL_DUNGEN_STAIRPLACEMENTS_HPP
//------------------------------------------------------------------------------
// dungen
// StairPlacements.hpp
//------------------------------------------------------------------------------

#include <iosfwd>
#include <string>
#include <string_view>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    // How many stairs a level is given and which way they lead
    enum class StairPlacements
    {
        None,     // No stairs at all
        Paired,   // One down and one up
        Multiple  // Several, each direction drawn at random after the first pair
    };

    // The command line and file header spelling, lower case
    std::string to_string(StairPlacements placements);

    // Throws std::invalid_argument naming what was not a placement
    StairPlacements to_stair_placements(std::string_view s);

    std::ostream& operator<<(std::ostream& os, const StairPlacements& placements);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_STAIRPLACEMENTS_HPP
