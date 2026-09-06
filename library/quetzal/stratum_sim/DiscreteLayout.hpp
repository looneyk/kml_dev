#if !defined(QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DiscreteLayout.hpp
//------------------------------------------------------------------------------

#include "Layout.hpp"
#include "Room.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <optional>
#include <vector>

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

    //--------------------------------------------------------------------------
    // The grid a Zone is laid out on, set before anything is built into it
    struct Grid
    {
        point_type origin; // Corner of cell (0, 0)
        value_type spacing = value_type(1);
        discrete_vector_type extent; // Cell counts

        // Checks whether the position given is within the grid
        bool contains(const GridPosition& cell) const;

        // The cell a position falls in, and the centre of a cell. Total in both directions:
        // no bounds check and no clamp, so a position off the grid maps to a cell off it
        GridPosition cell_at(const point_type& position) const;
        point_type cell_position(const GridPosition& cell) const;
    };

    //--------------------------------------------------------------------------
    // A horizontal surface bounding the layout, one below and one above. The two differ by
    // role and nothing else, a stalactite and a stalagmite being one shape inverted
    // A height field rather than a plane. A constant height is the ordinary case and is what
    // a level with no height data loads, so it costs one value and no table; sampling is
    // what a sloped floor, an arched ceiling or a flight of steps needs, and the two are one
    // record with more or fewer of its features used
    // Sampling is over a grid of the surface's own, which need not be the movement grid: a
    // height map may be finer without anything else noticing. See the three grids in
    // sim_zone_model.md
    class Flat
    {
    public:

        using heights_type = std::vector<std::optional<value_type>>;

        Flat() = default;
        explicit Flat(value_type height, bool ceiling = false);

        bool ceiling() const; // Role, floor otherwise
        void set_ceiling(bool b);

        // Where the surface is at a position in the zone, and nothing where it is absent. A
        // pit, a shaft, and a hole dug through the floor are all somewhere the surface is not,
        // rather than somewhere it is low
        // A position off the sampling grid is off the surface and returns nothing
        std::optional<value_type> height(const point_type& position) const;

        // The constant the surface takes wherever it is not sampled, which is everywhere
        // until something is
        value_type height() const;
        void set_height(value_type height);

        // Gives the surface a sampling grid, every sample present and at the constant height.
        // Sampling again replaces what was there
        void set_sampling(const Grid& grid);
        bool sampled() const;
        const Grid& sampling() const; // Must be sampled

        // One sample, which must lie on the sampling grid. Nothing where the surface is absent
        std::optional<value_type> sample(const GridPosition& position) const;
        void set_sample(const GridPosition& position, std::optional<value_type> height);

    private:

        value_type m_height = value_type(0);
        bool m_ceiling = false;
        std::optional<Grid> m_sampling;
        heights_type m_heights; // The sampling grid's extent, empty until it has one
    };

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
        id_type room_at(const GridPosition& cell) const;
        void set_cell_room(const GridPosition& cell, id_type room_id);

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

        void set_grid(const point_type& origin, value_type spacing, const discrete_vector_type& extent);
        const Grid& grid() const;

        // Whether a cell is open floor. Taken from what a level file encodes per cell, over a
        // table of its own, and read without consulting anything else
        // The cell must lie on the grid, an off-grid cell being the caller's error rather
        // than a question with an answer
        bool open(const GridPosition& cell) const;
        void set_cell_open(const GridPosition& cell, bool open);

        // The grid's own granularity, so that crossing one cell costs one action and
        // crossing four costs four
        value_type step_distance() const override;

        // Total: any position floors into a cell without bounds check or clamp, so a position
        // off the grid maps to a cell off the grid rather than failing
        GridPosition cell_at(const point_type& point) const;
        point_type cell_position(const GridPosition& cell) const; // Cell centre, at the grid's height

        // The wall on this cell's side in the given direction, if any
        id_type wall_toward(const GridPosition& cell, const discrete_vector_type& direction) const;

        // Stopped by the wall in the given direction
        bool blocked(const GridPosition& cell, const discrete_vector_type& direction) const;

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

        walls_type m_wall_store;
        rooms_type m_room_store;
        flats_type m_flat_store;
        id_type m_floor_id = nullid;
        id_type m_ceiling_id = nullid;
        std::optional<Grid> m_grid;
        std::vector<bool> m_cell_open; // Grid extent, false where the cell is solid
        ids_type m_cell_rooms; // Grid extent, nullid where no room covers the cell
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DISCRETE_LAYOUT_HPP
