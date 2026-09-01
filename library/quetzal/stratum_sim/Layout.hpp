#if !defined(QUETZAL_STRATUM_SIM_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Layout.hpp
//------------------------------------------------------------------------------

#include "Room.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <functional>
#include <optional>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // One side of what stands on an edge. The walls on opposite sides of one edge are
    // partners, and next and prev link a run of them into a closed outline
    // A nullid partner means solid rock: nothing behind it, and not minable. Since a
    // Zone's extent is finite and bounded by rock, that is also its outer boundary
    struct Wall
    {
        id_type partner_id = nullid;
        id_type next_id = nullid;
        id_type prev_id = nullid;
        point_type position; // Start vertex, the far end being the position of next
        bool passable = false; // Crossing outward through this one wall, per side, so that
                               // one-way and secret doors need no special case
                               // no, all doors are attachments, this is deprecated.

        // A deleted wall keeps its slot, ids being slots and every one held elsewhere having
        // to stay pointing at what it named
        // Invisible metadata as Flags treats it: logically const, so mutable and set through
        // a const method
        bool deleted() const;
        void set_deleted(bool b = true) const;

    private:

        mutable bool m_deleted = false;
    };

    //--------------------------------------------------------------------------
    // A horizontal surface bounding the layout, one below and one above. The two differ by
    // role and nothing else, a stalactite and a stalagmite being one thing inverted
    struct Flat
    {
        value_type height = value_type(0); // Uniform; a sampled height field replaces this
        bool ceiling = false; // Role, floor otherwise
    };

    //--------------------------------------------------------------------------
    // The grid a Zone is laid out on, set before anything is built into it
    struct Grid
    {
        point_type origin; // Corner of cell (0, 0)
        value_type spacing = value_type(1);
        DiscreteVector extent; // Cell counts, which is a displacement rather than a position

        // Whether a cell lies on the grid at all. Cells run from (0, 0) to the extent, so
        // anything negative is off it, as is anything at or past the extent. cell_at maps
        // any position to a cell and a step off the edge is expressible, so this is what
        // every caller that indexes by cell asks first
        bool contains(const GridPosition& cell) const;
    };

    //--------------------------------------------------------------------------
    // How far around, in the sense the layout's own axes are wound
    enum class Rotation
    {
        CCW,
        CW,
        Half
    };

    //--------------------------------------------------------------------------
    // Where the mover ended up, which may be short of what was asked or unmoved, and what
    // stopped it. Both nullid when nothing did, and never both set
    struct MoveResult
    {
        placement_type placement;
        id_type obstruction_wall_id = nullid; // The wall it was stopped by
        id_type obstruction_entity_id = nullid; // What was standing in the way, whatever the caller counts as one
    };

    //--------------------------------------------------------------------------
    // How one Zone is laid out: its walls, its flats, and the rooms named over them, on a
    // grid that quantizes movement
    // Static topology rather than game objects, so it is held as arrays indexed by id rather
    // than as entities. Entities refer into it by id
    // This is the discrete flat layout and nothing more general. See sim_zone_model.md for
    // where it sits among the layouts that are not written
    class Layout
    {
    public:

        using walls_type = std::vector<Wall>;
        using rooms_type = std::vector<Room>;
        using flats_type = std::vector<Flat>;
        using ids_type = std::vector<id_type>;
        using placements_type = std::vector<placement_type>;

        Layout() = default;
        Layout(const Layout&) = default;
        Layout(Layout&&) noexcept = default;
        ~Layout() = default;

        Layout& operator=(const Layout&) = default;
        Layout& operator=(Layout&&) noexcept = default;

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

        // The floor and the ceiling bounding the layout, one apiece however many rooms it
        // holds. nullid until set
        id_type floor_id() const;
        id_type ceiling_id() const;
        void set_flats(value_type height_floor, value_type height_ceiling);

        // One Wall per outline edge, linked into a closed ring in outline order. Every
        // partner is nullid, so new walls are surrounded by solid rock until something is
        // connected to them
        // Returns the wall each outline starts at. A ring's walls are contiguous from there,
        // so a caller that knows an outline's length knows which walls came from it
        ids_type create_walls(const std::vector<polygon_type>& outlines);

        // A room the caller has built the outline and cell list of. Rooms are metadata: what
        // one covers is recorded here and read back, and nothing in movement consults them
        id_type create_room(const Room& room);

        // The room whose outline contains the point, height disregarded. nullid when none
        // does, which every cell outside a room answers with, corridors included
        // Tests every room's outline, so prefer the cell form
        id_type room_at(const point_type& point) const;

        // The room occupying a cell, nullid where none does. A direct lookup into a table
        // filled as the rooms are created
        // The cell must lie on the grid, an off-grid cell being the caller's error rather
        // than a question with an answer
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
        // and a caller that built the walls knows which they are
        size_t connect_walls(const ids_type& wall_ids_a, const ids_type& wall_ids_b);

        // Connects the opposed walls within one set, which is what a wall run folding back
        // on itself amounts to: a free standing wall's two sides are partners
        size_t connect_folds(const ids_type& wall_ids);

        // Sets whether a mover may cross this one wall outward. Its partner is untouched,
        // which is what leaves one-way passages expressible
        void set_passable(id_type wall_id, bool passable);

        void set_grid(const point_type& origin, value_type spacing, const DiscreteVector& extent);
        const Grid& grid() const;

        // Whether a cell holds floor. Layout data of its own, taken from what a level file
        // says of each cell, and answered without consulting anything else
        // The cell must lie on the grid, an off-grid cell being the caller's error rather
        // than a question with an answer
        bool open(const GridPosition& cell) const;
        void set_cell_open(const GridPosition& cell, bool open);

        // How far one action of movement covers, which is what a move's cost is measured
        // against. The grid's own granularity, so that crossing one cell costs one action
        // and crossing four costs four
        value_type step_distance() const;

        GridPosition cell_at(const point_type& point) const;
        point_type cell_position(const GridPosition& cell) const; // Cell centre, at the grid's height

        // The wall on this cell's side of the edge it presents in that direction, nullid
        // when nothing stands there. The direction is one step along an axis
        // Nothing beyond the cell itself is read, so the neighbour need not exist: a cell on
        // the boundary answers with the wall that closes the layout
        // One wall lies on one cell edge, so the two match end for end. Scans the walls
        // rather than consulting an index; an edge keyed index would replace the scan
        // without changing this signature
        id_type wall_toward(const GridPosition& cell, const DiscreteVector& direction) const;

        // Whether a mover stepping that way is stopped
        bool blocked(const GridPosition& cell, const DiscreteVector& direction) const;

        // Whatever stands at a placement and stops a mover entering it, nullid when nothing
        // does. What counts as one is the caller's business; the layout only asks, so it
        // stays ignorant of entities while still stopping for them
        using obstructed_type = std::function<id_type(const placement_type&)>;

        // Where the mover ends up, stopping at the first wall that blocks it or the first
        // placement the caller says is occupied. The direction snaps to one of the grid's own
        // and the distance rounds to whole cells; rounding is the layout's business, not the
        // caller's
        // Topology and occupancy are answered in one sweep rather than one after the other
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance,
            const obstructed_type& obstructed = {}) const;

        placement_type turn(const placement_type& placement, Rotation rotation) const;

        // The placement facing the direction most nearly asked for, snapped to one of the
        // grid's own as move snaps the direction it steps in. Unchanged when the direction is
        // zero, there being nothing asked for
        placement_type face(const placement_type& placement, const vector_type& direction) const;

        // Whether the two stand in the same cell
        bool same_location(const placement_type& a, const placement_type& b) const;

        // Adjacency for AI, the placements reachable in one step
        placements_type neighbors(const placement_type& placement) const;

        // Line of sight, by walking the cells the ray between them passes through. A grid
        // field of view algorithm gives a different, more roguelike answer and would
        // replace this
        bool visible(const placement_type& from, const placement_type& to) const;

        void clear();

    private:

        walls_type m_wall_store;
        rooms_type m_room_store;
        flats_type m_flat_store;
        id_type m_floor_id = nullid;
        id_type m_ceiling_id = nullid;
        std::optional<Grid> m_grid;
        std::vector<bool> m_cell_open; // Grid extent, false where there is rock
        ids_type m_cell_rooms; // Grid extent, nullid where no room covers the cell
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LAYOUT_HPP
