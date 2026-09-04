#if !defined(QUETZAL_DUNGEN_SHAFTPLACEMENTS_HPP)
#define QUETZAL_DUNGEN_SHAFTPLACEMENTS_HPP
//------------------------------------------------------------------------------
// dungen
// ShaftPlacements.hpp
//------------------------------------------------------------------------------

#include <iosfwd>
#include <string>
#include <string_view>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    // How many shafts a level is given and which way they lead
    enum class ShaftPlacements
    {
        None,     // No shafts at all
        Paired,   // One down and one up
        Multiple  // Several, each direction drawn at random after the first pair
    };

    // The command line and file header spelling, lower case
    std::string to_string(ShaftPlacements placements);

    // Throws std::invalid_argument naming what was not a placement
    ShaftPlacements to_shaft_placements(std::string_view s);

    std::ostream& operator<<(std::ostream& os, const ShaftPlacements& placements);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_SHAFTPLACEMENTS_HPP
