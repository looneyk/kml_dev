#if !defined(QUETZAL_STRATUM_SIM_CELL_HPP)
#define QUETZAL_STRATUM_SIM_CELL_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Cell.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // One square of a Zone's grid and what the layout records about it. What a level file
    // encodes per cell, read back without consulting anything else
    struct Cell
    {
        bool open = false; // Whether a mover may stand here
        id_type room_id = nullid; // The room covering it, or nullid where none does
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CELL_HPP
