#if !defined(QUETZAL_CATAGEN_ZONE_HPP)
#define QUETZAL_CATAGEN_ZONE_HPP
//------------------------------------------------------------------------------
// catagen
// Zone.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace quetzal::catagen
{

    //--------------------------------------------------------------------------
    // A rectangle of cells and the walls between them. A wall belongs to the edge two cells share, so the grid has one more
    // line of walls than cells in each direction
    // Rooms, doors and staircases are stored as cell and wall state, with each staircase's facing direction
    class Zone
    {
    public:

        Zone() = default;
        Zone(const std::string& name, const vector_type& extent); // Solid layout, no open areas
        Zone(const Zone&) = default;
        Zone(Zone&&) noexcept = default;
        ~Zone() = default;

        Zone& operator=(const Zone&) = default;
        Zone& operator=(Zone&&) noexcept = default;

        const std::string& name() const;

        const vector_type& extent() const;
        bool contains(const point_type& cell) const;

        CellType cell(const point_type& cell) const;
        void set_cell(const point_type& cell, CellType type);

        // Walls addressed by the line they lie on, including the outermost lines
        // A vertical wall separates cells east to west: wall_vertical(i, j) lies between
        // cells (i - 1, j) and (i, j), so i runs to nx inclusive and j over the cells
        WallType wall_vertical(coord_type i, coord_type j) const;
        void set_wall_vertical(coord_type i, coord_type j, WallType type);

        // A horizontal wall separates cells north to south: wall_horizontal(i, j) lies
        // between cells (i, j - 1) and (i, j), so j runs to ny inclusive and i over the cells
        WallType wall_horizontal(coord_type i, coord_type j) const;
        void set_wall_horizontal(coord_type i, coord_type j, WallType type);

        // The east wall of (i, j) and the west wall of (i + 1, j) are the same wall
        WallType wall(const point_type& cell, CardinalDirection direction) const;
        void set_wall(const point_type& cell, CardinalDirection direction, WallType type);

        // The side of a staircase cell it opens onto
        std::optional<CardinalDirection> staircase_facing(const point_type& cell) const;
        void set_staircase_facing(const point_type& cell, CardinalDirection facing);

        void clear();

    private:

        size_t cell_slot(const point_type& position) const;
        size_t wall_vertical_slot(coord_type i, coord_type j) const;
        size_t wall_horizontal_slot(coord_type i, coord_type j) const;

        std::string m_name;
        box_type m_bounds;
        std::vector<CellType> m_cells;
        std::vector<WallType> m_walls_vertical; // (nx + 1) by ny
        std::vector<WallType> m_walls_horizontal; // nx by (ny + 1)
        std::map<point_type, CardinalDirection> m_staircase_facings;
    };

    point_type neighbor(const point_type& cell, CardinalDirection direction);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_ZONE_HPP
