#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include "quetzal/stratum_format/Notation.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Called with the walls of each door in the file, one per side: two for a door in a wall or a doorway between two
    // areas, one for a doorway into a dead end. The loader has no world, so it reports doors rather than creating them
    using door_type = std::function<void(const std::vector<id_type>& wall_ids)>;

    // Reads a discrete layout file and builds its layout: a cell per grid position, its walls, the room of each cell, and
    // the two flats
    // Throws std::runtime_error when the reader rejects the file, and builds nothing until it has accepted it
    // notation, where given, receives the notation the rows were written in
    std::unique_ptr<DiscreteLayout> load_layout(const std::filesystem::path& filepath, const door_type& door = {},
        stratum_format::Notation* notation = nullptr);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
