#if !defined(QUETZAL_STRATUM_SIM_CELL_HPP)
#define QUETZAL_STRATUM_SIM_CELL_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Cell.hpp
//------------------------------------------------------------------------------

#include "DirectionMask.hpp"
#include "quetzal/common/id.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // One square of a zone's grid
    struct Cell
    {
        bool open = false; // True if an actor may stand here
        DirectionMask walls = DirectionMask::None; // The sides with a wall facing into the cell
        id_type room_id = nullid; // The room that contains the cell, or nullid
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CELL_HPP
