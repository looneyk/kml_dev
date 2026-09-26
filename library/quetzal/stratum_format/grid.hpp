#if !defined(QUETZAL_STRATUM_FORMAT_GRID_HPP)
#define QUETZAL_STRATUM_FORMAT_GRID_HPP
//------------------------------------------------------------------------------
// stratum_format
// grid.hpp
//------------------------------------------------------------------------------

#include "Document.hpp"
#include <set>
#include <vector>

namespace quetzal::stratum_format
{

    // The geometry the rows define, used both to validate a file and to build a layout from it

    bool on_grid(const Cells& cells, const discrete_point_type& cell_position);

    // The cell must lie on the grid
    bool floor_cell(const Cells& cells, const discrete_point_type& cell_position);

    // The cell must lie on the grid. A door cell exists only in the block notation
    bool floor_door_cell(const Cells& cells, const discrete_point_type& cell_position);

    // Returns true if the cell is written '<' or '>'. '<' is the entrance point of any non-local transporter --- a staircase leading
    // up, a waypoint, a portal --- and '>' a staircase leading down, the entrance of a tower entered from below. The cell
    // must lie on the grid
    bool stair_cell(const Cells& cells, const discrete_point_type& cell_position);

    // Returns true if a cell on the grid has a wall on its side in a cardinal direction. Beyond the grid is solid
    bool has_wall(const Cells& cells, const discrete_point_type& cell_position, const discrete_vector_type& direction);

    // The cells of each room, by room id: the fill from its location, or the floor/door cells within its perimeter.
    // The document must be one that read accepted
    std::vector<std::set<discrete_point_type>> room_cells(const Document& document);

    // The walls of each door marked in the rows, one for every side it presents to a floor cell: both sides of a door edge
    // in the wall notation, and in the block notation the faces a doorway block presents to the floor on either side
    std::vector<std::vector<WallAddress>> door_walls(const Cells& cells);

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_GRID_HPP
