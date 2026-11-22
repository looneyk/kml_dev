#if !defined(QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DiscreteLayout.hpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Flat.hpp"
#include "GridMapping.hpp"
#include "LayoutBase.hpp"
#include "Site.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A zone's layout with its geometry on a grid, stored as arrays indexed by id
    // A wall is one side of one cell edge, a bit on the cell it faces, independent of the other side
    class DiscreteLayout : public LayoutBase
    {
    public:

        using flats_type = std::vector<Flat>;
        using cells_type = std::vector<Cell>;
        using cell_positions_type = std::vector<discrete_point_type>;

        using LayoutBase::move; // Hidden by the override otherwise

        DiscreteLayout(const point_type& origin, value_type scale, const discrete_vector_type& extent);
        DiscreteLayout(const DiscreteLayout&) = default;
        DiscreteLayout(DiscreteLayout&&) noexcept = default;
        ~DiscreteLayout() = default;

        DiscreteLayout& operator=(const DiscreteLayout&) = default;
        DiscreteLayout& operator=(DiscreteLayout&&) noexcept = default;

        const Flat& flat(id_type flat_id) const;
        Flat& flat(id_type flat_id);
        size_t flat_store_count() const;
        const flats_type& flat_store() const;

        // The floor and the ceiling flats bounding the layout
        id_type floor_id() const;
        id_type ceiling_id() const;
        void set_flats(value_type height_floor, value_type height_ceiling);

        // The height of a cell if it has one, the height of the room if not, the flat height if neither
        value_type height(id_type flat_id, const discrete_point_type& cell_position) const;

        size_t room_count() const;
        void set_room_count(size_t room_count);

        id_type room_at(const discrete_point_type& cell_position) const;
        void set_cell_room(const discrete_point_type& cell_position, id_type room_id);

        const GridMapping& grid() const;

        // Open floor
        bool open(const discrete_point_type& cell_position) const;
        void set_cell_open(const discrete_point_type& cell_position, bool open);

        // The walls facing into a cell
        DirectionMask walls(const discrete_point_type& cell_position) const;
        void set_cell_walls(const discrete_point_type& cell_position, DirectionMask walls);

        // The grid scale
        value_type step_distance() const override;

        // These do not perform bounds check or clamp
        discrete_point_type cell_position_at(const point_type& point) const;
        point_type position(const discrete_point_type& cell_position) const; // Cell center, on the floor

        // The wall on this cell's side in the given direction, if any. Its id encodes the cell and the direction
        id_type wall_toward(const discrete_point_type& cell_position, const discrete_vector_type& direction) const;

        // The cell a wall is on and the direction it faces from that cell, decoded from the id that wall_toward returns
        discrete_point_type wall_cell_position(id_type wall_id) const;
        const discrete_vector_type& wall_direction(id_type wall_id) const;

        // The placement of an actor following transit to an access point's destination
        placement_type destination_placement(const Site& site, const discrete_vector_type& egress) const;

        // True if there is no wall in that direction; otherwise the result of is_wall_crossable for the wall
        bool crossable(const discrete_point_type& cell_position, const discrete_vector_type& direction, const crossable_function_type& is_wall_crossable) const;

        // Stops at the first blocking wall or occupied cell. The direction snaps to the grid and the distance rounds to
        // whole cells
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const crossable_function_type& is_wall_crossable, const blocker_function_type& blocker_at) const override;

        placement_type turn(const placement_type& placement, Rotation rotation) const override;

        // Snaps to a cardinal direction
        placement_type face(const placement_type& placement, const vector_type& direction) const override;

        // Returns true if the two occupy the same cell
        bool same_location(const placement_type& a, const placement_type& b) const override;

        placements_type neighbors(const placement_type& placement, const crossable_function_type& is_wall_crossable) const override;

        // Line of sight, by walking the cells the ray between them passes through
        bool visible(const placement_type& from, const placement_type& to, const crossable_function_type& is_wall_crossable) const override;

        // The set of cells visible by shadowcasting, not implemented ...
        cell_positions_type field_of_view(const discrete_point_type& cell_position, coord_type range, const crossable_function_type& is_wall_crossable) const;

        // The wall of the placement's cell in its facing direction snapped to a cardinal direction if any
        id_type ahead(const placement_type& placement) const override;

        // A DiscreteProximityIndex over this layout
        std::unique_ptr<ProximityIndexBase> create_proximity_index() const override;

        // The position must be on the grid
        size_t slot(const discrete_point_type& cell_position) const;

    private:

        const Cell& cell(const discrete_point_type& cell_position) const;
        Cell& cell(const discrete_point_type& cell_position);

        flats_type m_flat_store;
        id_type m_floor_id = nullid;
        id_type m_ceiling_id = nullid;
        GridMapping m_grid;
        cells_type m_cells;
        size_t m_room_count = 0;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
