#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Told of each pair of walls the file called a door, as the walls are built. The loader
    // builds geometry and has no world to build entities in, so it reports rather than places
    // -- which is what lets a caller with a world attach a door without the loader having any
    // notion of what a door is
    // One wall per side it presents: two for a door in a wall and for a doorway between two
    // areas, one for a doorway into a dead end
    using door_type = std::function<void(const std::vector<id_type>& wall_ids)>;

    // Reads a discrete layout file and builds its layout: a cell per grid position, a wall on each side the rows give one,
    // the room covering each cell, and the two flats. See discrete_layout.md
    // Throws std::runtime_error when the reader rejects the file, and builds nothing until it has accepted it
    // notation, where given, is told which notation the rows were written in
    std::unique_ptr<DiscreteLayout> load_layout(const std::filesystem::path& filepath, const door_type& door = {},
        stratum_format::Notation* notation = nullptr);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
