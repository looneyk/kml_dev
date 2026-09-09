#if !defined(QUETZAL_CATAGEN_LEVEL_HPP)
#define QUETZAL_CATAGEN_LEVEL_HPP
//------------------------------------------------------------------------------
// catagen
// Level.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <vector>

namespace quetzal::catagen
{

    //--------------------------------------------------------------------------
    // A rectangle of cells and the walls between them. A wall belongs to the edge two cells
    // share rather than to either of them, so adjoining rooms cost nothing to separate and
    // the grid holds one more line of walls than cells in each direction
    // The whole of what a generator produces; rooms, doors and stairs are held here as
    // cell and wall state rather than as objects of their own
    class Level
    {
    public:

        Level() = default;
        explicit Level(const vector_type& extent); // Solid layout, no open areas
        Level(const Level&) = default;
        Level(Level&&) noexcept = default;
        ~Level() = default;

        Level& operator=(const Level&) = default;
        Level& operator=(Level&&) noexcept = default;

        const vector_type& extent() const;
        bool contains(const point_type& cell) const;

        CellType cell(const point_type& cell) const;
        void set_cell(const point_type& cell, CellType type);

        // Walls addressed by the line they lie on rather than by a cell and a side, which is
        // how they are stored and the only form that reaches the outermost line of each
        // A vertical wall separates cells east to west: wall_vertical(i, j) lies between
        // cells (i - 1, j) and (i, j), so i runs to nx inclusive and j over the cells
        WallType wall_vertical(coord_type i, coord_type j) const;
        void set_wall_vertical(coord_type i, coord_type j, WallType type);

        // A horizontal wall separates cells north to south: wall_horizontal(i, j) lies
        // between cells (i, j - 1) and (i, j), so j runs to ny inclusive and i over the cells
        WallType wall_horizontal(coord_type i, coord_type j) const;
        void set_wall_horizontal(coord_type i, coord_type j, WallType type);

        // The wall on one side of a cell. The east wall of (i, j) and the west wall of
        // (i + 1, j) are one wall, so either name reaches it
        WallType wall(const point_type& cell, Direction direction) const;
        void set_wall(const point_type& cell, Direction direction, WallType type);

        void clear();

    private:

        size_t cell_slot(const point_type& position) const;
        size_t wall_vertical_slot(coord_type i, coord_type j) const;
        size_t wall_horizontal_slot(coord_type i, coord_type j) const;

        box_type m_bounds;
        std::vector<CellType> m_cells;
        std::vector<WallType> m_walls_vertical; // (nx + 1) by ny
        std::vector<WallType> m_walls_horizontal; // nx by (ny + 1)
    };

    point_type neighbor(const point_type& cell, Direction direction);
    Direction opposite(Direction direction);

} // namespace quetzal::catagen

#endif // QUETZAL_CATAGEN_LEVEL_HPP
