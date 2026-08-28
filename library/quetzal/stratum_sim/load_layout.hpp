#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include <filesystem>

namespace quetzal::stratum_sim
{

    // Reads dungen's block format, one character per cell, using its own symbols:
    // '#' solid and '0' null are rock, ' ' room and '.' corridor are open floor,
    // '|' is a doorway, '>' a shaft. The first line of the file is the highest row, the
    // grid origin being lower left
    // Contiguous open cells become one region, its boundary traced into an outer ring and
    // a hole ring per enclosure. A doorway or a shaft is its own region even where nothing
    // separates it, so its facings can carry a door or a portal
    // Facings are one cell edge each rather than merged across collinear runs, since a
    // facing spanning two different neighbours could not be connected without splitting
    // Throws std::runtime_error when the file cannot be opened or holds no rows
    void load_layout(const std::filesystem::path& filepath, Layout& layout, value_type height_ceiling = value_type(1));

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
