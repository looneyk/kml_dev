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
    // Where a Zone's cell grid sits in the world and how far apart its cells are, set before
    // anything is built into it
    struct GridMapping
    {
        point_type origin; // Corner of cell (0, 0)
        value_type scale = value_type(1);
        discrete_vector_type extent; // Cell counts

        // Checks whether the grid position given is within the grid
        bool contains(const discrete_point_type& grid_position) const;

        // The grid position a point falls in, and the center of a grid position. Total in both
        // directions: no bounds check and no clamp, so a point off the grid maps to a grid
        // position off it
        discrete_point_type grid_position_at(const point_type& position) const;
        point_type position(const discrete_point_type& grid_position) const;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_GRID_MAPPING_HPP
