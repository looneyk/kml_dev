#if !defined(QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DiscreteLayout.hpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "Flat.hpp"
#include "GridMapping.hpp"
#include "Layout.hpp"
#include "Room.hpp"
#include "Wall.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <optional>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // How one Zone is laid out: its walls, its flats, and the rooms defined over them, on a
    // grid that quantizes movement
    // Static topology rather than game objects, so it is held as arrays indexed by id rather
    // than as entities. Entities refer into it by id
    // A level with no height data loads a constant floor rather than a different class, so
    // which data a game style loads is what differs rather than which class is built. Flat is
    // implied here, which is why the name does not mention it
    // No provision for continuous movement, which is ContinuousLayout's, and none for a wall
    // spanning more than one cell edge: one wall lies on one cell edge, which is what lets a
    // partner be found by exact match. See sim_zone_model.md
    class DiscreteLayout : public Layout
    {
    public:

        using walls_type = std::vector<Wall>;
        using rooms_type = std::vector<Room>;
        using flats_type = std::vector<Flat>;
        using cells_type = std::vector<Cell>;
        using ids_type = std::vector<id_type>;

        using Layout::move; // The interface's topology-only overload, which overriding move hides

        DiscreteLayout() = default;
        DiscreteLayout(const DiscreteLayout&) = default;
        DiscreteLayout(DiscreteLayout&&) noexcept = default;
        ~DiscreteLayout() = default;

        DiscreteLayout& operator=(const DiscreteLayout&) = default;
        DiscreteLayout& operator=(DiscreteLayout&&) noexcept = default;

        const Wall& wall(id_type wall_id) const;
        Wall& wall(id_type wall_id);
        size_t wall_store_count() const;
        const walls_type& wall_store() const;

        const Room& room(id_type room_id) const;
        Room& room(id_type room_id);
        size_t room_store_count() const;
        const rooms_type& room_store() const;

        const Flat& flat(id_type flat_id) const;
        Flat& flat(id_type flat_id);
        size_t flat_store_count() const;
        const flats_type& flat_store() const;

        // The floor and the ceiling bounding the layout, one apiece
        id_type floor_id() const;
        id_type ceiling_id() const;
        void set_flats(value_type height_floor, value_type height_ceiling);

        // One Wall per outline edge, linked into a closed ring in outline order. Every
        // partner is nullid, so new walls are border walls
        // Returns the wall each outline starts at. A ring's walls are contiguous from there,
        // so a caller given an outline's length can derive which walls came from it
        ids_type create_walls(const std::vector<polygon_type>& outlines);

        // A room the caller has built the outline and cell list of. Rooms are metadata: what
        // one covers is stored here and read back, and nothing in movement consults them
        id_type create_room(const Room& room);

        // The room whose outline contains the point, height disregarded. nullid when none does
        // Tests every room's outline, so prefer the cell form ...
        id_type room_at(const point_type& point) const;

        // The room containing a given cell
        id_type room_at(const discrete_point_type& grid_position) const;
        void set_cell_room(const discrete_point_type& grid_position, id_type room_id);

        // Each wall's start is the other's end, so the two describe one edge from opposite
        // sides. This is what makes them eligible to be partners
        bool opposed(id_type wall_id_a, id_type wall_id_b) const;

        void connect(id_type wall_id_a, id_type wall_id_b);
        void disconnect(id_type wall_id);

        // Connects every opposed pair the two sets of walls share, skipping walls already
        // connected. Returns the number of connections made
        // Walls rather than any grouping over them: what is connected to what is topology,
        // and a caller that built the walls already has their ids
        size_t connect_walls(const ids_type& wall_ids_a, const ids_type& wall_ids_b);

        // Connects the opposed walls within one set, which is what a wall run folding back
        // on itself amounts to: a free standing wall's two sides are partners
        size_t connect_folds(const ids_type& wall_ids);

        void set_grid(const point_type& origin, value_type scale, const discrete_vector_type& extent);
        const GridMapping& grid() const;

        // Whether a cell is open floor. Taken from what a level file encodes per cell, over a
        // table of its own, and read without consulting anything else
        // The cell must lie on the grid, an off-grid cell being the caller's error rather
        // than a question with an answer
        bool open(const discrete_point_type& grid_position) const;
        void set_cell_open(const discrete_point_type& grid_position, bool open);

        // The grid's own granularity, so that crossing one cell costs one action and
        // crossing four costs four
        value_type step_distance() const override;

        // Total: any position floors into a cell without bounds check or clamp, so a position
        // off the grid maps to a cell off the grid rather than failing
        discrete_point_type grid_position_at(const point_type& point) const;
        point_type position(const discrete_point_type& grid_position) const; // Cell center, at the grid's height

        // The wall on this cell's side in the given direction, if any
        id_type wall_toward(const discrete_point_type& grid_position, const discrete_vector_type& direction) const;

        // Stopped by the wall in the given direction
        bool blocked(const discrete_point_type& grid_position, const discrete_vector_type& direction) const;

        // Stops at the first wall that blocks it or the first cell the caller reports
        // occupied. The direction snaps to one of the grid's own and the distance rounds to
        // whole cells
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const obstructed_type& obstructed) const override;

        placement_type turn(const placement_type& placement, Rotation rotation) const override;

        // Snapped to one of the grid's own directions
        placement_type face(const placement_type& placement, const vector_type& direction) const override;

        // Whether the two occupy the same cell
        bool same_location(const placement_type& a, const placement_type& b) const override;

        placements_type neighbors(const placement_type& placement) const override;

        // Line of sight, by walking the cells the ray between them passes through. A grid
        // field of view algorithm produces a different, more roguelike result and would
        // replace this
        bool visible(const placement_type& from, const placement_type& to) const override;

        // The wall on the near side of the edge the placement faces. The facing snaps to one
        // of the grid's own directions, and a facing that snaps to nothing faces nothing
        id_type ahead(const placement_type& placement) const override;

        void clear();

    private:

        // The cell a grid position addresses, which must lie on the grid
        const Cell& cell(const discrete_point_type& grid_position) const;
        Cell& cell(const discrete_point_type& grid_position);

        walls_type m_wall_store;
        rooms_type m_room_store;
        flats_type m_flat_store;
        id_type m_floor_id = nullid;
        id_type m_ceiling_id = nullid;
        std::optional<GridMapping> m_grid;
        cells_type m_cells; // The grid's extent
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
