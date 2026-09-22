#if !defined(QUETZAL_STRATUM_SIM_GRID_MAPPING_HPP)
#define QUETZAL_STRATUM_SIM_GRID_MAPPING_HPP
//------------------------------------------------------------------------------
// stratum_sim
// GridMapping.hpp
//------------------------------------------------------------------------------

#include "types.hpp"

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The position and spacing of a zone's cell grid in the world
    struct GridMapping
    {
        point_type origin; // Corner of cell (0, 0)
        value_type scale = value_type(1);
        discrete_vector_type extent; // Cell count in each dimension

        bool contains(const discrete_point_type& cell_position) const;

        // These do no bounds check or clamping
        discrete_point_type cell_position_at(const point_type& position) const;
        point_type position(const discrete_point_type& cell_position) const; // Cell center
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GRID_MAPPING_HPP
