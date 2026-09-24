#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <filesystem>

namespace quetzal::stratum_sim
{

    // Reads a discrete layout file into an empty zone: its ZoneLayout, with a cell per grid position, its walls, the room
    // of each cell and the two flats; its ZoneNotation, the notation the rows are written in; its doors; and its access
    // points
    // Throws std::runtime_error when the reader rejects the file or an access point's type is not registered, and fills
    // nothing until it has accepted it
    void load_layout(flecs::entity zone, const std::filesystem::path& filepath);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
