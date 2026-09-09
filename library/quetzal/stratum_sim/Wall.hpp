#if !defined(QUETZAL_STRATUM_SIM_WALL_HPP)
#define QUETZAL_STRATUM_SIM_WALL_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Wall.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // One side of an edge. The walls on opposite sides of one edge are
    // partners, and next and prev link a run of them into a closed outline
    // A nullid partner makes it a border wall, with nothing behind it
    struct Wall
    {
        id_type partner_id = nullid;
        id_type next_id = nullid;
        id_type prev_id = nullid;
        point_type position; // Start vertex, the far end being the position of next
                             // Whether a mover may cross is decided by what is attached to
                             // the wall, not by the wall: see attachments.hpp

        // A deleted wall keeps its slot, ids being slots and every one held elsewhere having
        // to stay pointing at what it referred to
        // Invisible metadata as Flags treats it: logically const, so mutable and set through
        // a const method
        bool deleted() const;
        void set_deleted(bool b = true) const;

    private:

        mutable bool m_deleted = false;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_WALL_HPP
