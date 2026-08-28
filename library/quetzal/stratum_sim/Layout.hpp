#if !defined(QUETZAL_STRATUM_SIM_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Layout.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <optional>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A vertical facing. Two facings on opposite sides of one wall are partners, and next
    // and prev link the facings of a region into its boundary ring
    // A nullid partner means solid rock: nothing behind it, and not minable. Since a
    // Zone's extent is finite and bounded by rock, that is also its outer boundary
    struct Wall
    {
        id_type partner = nullid;
        id_type next = nullid;
        id_type prev = nullid;
        id_type region = nullid;
        point_type position; // Start vertex, the far end being the position of next
        bool passable = false; // Crossing outward through this facing, per side, so that
                               // one-way and secret doors need no special case
    };

    //--------------------------------------------------------------------------
    // A region's horizontal facing, one floor and one ceiling per region. The two differ
    // by role and nothing else, a stalactite and a stalagmite being one thing inverted
    struct Flat
    {
        id_type region = nullid;
        value_type height = value_type(0); // Uniform; a sampled height field replaces this
        bool ceiling = false; // Role, floor otherwise
    };

    //--------------------------------------------------------------------------
    // A room, corridor, or other bounded area, laid out as a face with holes
    // A hole is whatever the region encloses and is not: rock, a pillar, or another
    // region entirely. Its ring is wound opposite the outer one
    struct Region
    {
        id_type wall = nullid; // Outer ring start, giving it a reproducible start
        std::vector<id_type> holes; // One ring start per hole
        id_type floor = nullid;
        id_type ceiling = nullid;
    };

    //--------------------------------------------------------------------------
    // A cell is a coordinate, not a stored record. Cells exist because the Zone was
    // generated from a discrete layout, so the grid is provenance rather than something
    // derived from the region geometry after the fact
    struct Cell
    {
        int i = 0;
        int j = 0;

        friend bool operator==(const Cell&, const Cell&) = default;
    };

    //--------------------------------------------------------------------------
    // The discrete layout a Zone was generated from. Carrying one makes the Zone
    // discrete; without one it is continuous. The presence of the record is the
    // distinction, so there is no mode to keep consistent with anything
    struct Grid
    {
        point_type origin; // Corner of cell (0, 0)
        value_type spacing = value_type(1);
        Cell extent; // Cell counts, not coordinates
    };

    //--------------------------------------------------------------------------
    enum class Turn
    {
        Left,
        Right,
        Around
    };

    //--------------------------------------------------------------------------
    // Where the mover ended up, which may be short of what was asked or unmoved, and the
    // facing that stopped it. nullid when nothing did
    struct MoveResult
    {
        placement_type placement;
        id_type obstruction = nullid;
    };

    //--------------------------------------------------------------------------
    // How one Zone is laid out: its regions, their boundary facings, and their flats.
    // Static topology rather than game objects, so it is held as arrays indexed by id
    // rather than as entities. Entities refer into it by id
    // Concrete while there is one implementation. It becomes the interface a continuous Zone
    // also satisfies when there is a second, and nothing about the interface moves then
    class Layout
    {
    public:

        using walls_type = std::vector<Wall>;
        using regions_type = std::vector<Region>;
        using flats_type = std::vector<Flat>;
        using ids_type = std::vector<id_type>;
        using placements_type = std::vector<placement_type>;

        Layout() = default;
        Layout(const Layout&) = default;
        Layout(Layout&&) noexcept = default;
        ~Layout() = default;

        Layout& operator=(const Layout&) = default;
        Layout& operator=(Layout&&) noexcept = default;

        const Wall& wall(id_type id) const;
        Wall& wall(id_type id);
        size_t wall_store_count() const;
        const walls_type& wall_store() const;

        const Region& region(id_type id) const;
        Region& region(id_type id);
        size_t region_store_count() const;
        const regions_type& region_store() const;

        const Flat& flat(id_type id) const;
        Flat& flat(id_type id);
        size_t flat_store_count() const;
        const flats_type& flat_store() const;

        // One Wall facing per outline edge, linked into a closed ring in outline order,
        // plus a floor and a ceiling. Every partner is nullid, so a new region is
        // surrounded by solid rock until something is connected to it
        id_type create_region(const polygon_type& outline, value_type height_floor, value_type height_ceiling);
        id_type create_region(const polygon_type& outline, const std::vector<polygon_type>& holes, value_type height_floor, value_type height_ceiling);

        ids_type region_walls(id_type region_id) const; // Outer ring, in ring order
        ids_type region_hole_walls(id_type region_id, size_t hole) const;
        ids_type region_all_walls(id_type region_id) const; // Outer ring then each hole

        polygon_type region_outline(id_type region_id) const;
        polygon_type region_hole_outline(id_type region_id, size_t hole) const;

        // The region whose horizontal footprint contains the point, height disregarded.
        // nullid when none does. Tests every region's outline, so prefer the cell form
        id_type region_at(const point_type& point) const;

        // The region occupying a cell, nullid for rock. A direct lookup, recorded when the
        // Zone was generated rather than derived from the geometry afterwards
        id_type region_at(const Cell& cell) const;
        void set_cell_region(const Cell& cell, id_type region_id);

        // Each facing's start is the other's end, so the two describe one wall from
        // opposite sides. This is what makes them eligible to be partners
        bool opposed(id_type wall_id_a, id_type wall_id_b) const;

        void connect(id_type wall_id_a, id_type wall_id_b);
        void disconnect(id_type wall_id);

        // Connects every opposed pair the two regions share, skipping facings already
        // connected. Returns the number of connections made
        // The same region may be passed twice, which is what connect_folds does
        size_t connect_regions(id_type region_id_a, id_type region_id_b);

        // Connects the opposed facings within one region, which is what a wall run
        // folding back on itself amounts to: a free standing wall's two faces are
        // partners, and both bound the same region
        size_t connect_folds(id_type region_id);

        // Sets whether a mover may cross this one facing outward. Its partner is
        // untouched, which is what leaves one-way passages expressible
        void set_passable(id_type wall_id, bool passable);

        bool discrete() const;
        void set_grid(const point_type& origin, value_type spacing, const Cell& extent);
        const Grid& grid() const;

        Cell cell_at(const point_type& point) const;
        point_type cell_position(const Cell& cell) const; // Cell centre, at the grid's height

        // The facing on cell_a's side of the edge it shares with an orthogonally adjacent
        // cell_b, nullid when the two lie within one region and nothing separates them
        // Scans the facings rather than consulting an index. An edge keyed index would
        // replace the scan without changing this signature
        id_type facing_between(const Cell& cell_a, const Cell& cell_b) const;

        // Whether a mover is stopped crossing that edge
        bool blocked(const Cell& cell_a, const Cell& cell_b) const;

        // Where the mover ends up, stopping at the first facing that blocks it. A
        // discrete layout snaps the direction to one of its own and rounds the distance
        // to whole cells; rounding is the layout's business, not the caller's
        MoveResult move(const placement_type& placement, const vector_type& direction, value_type distance) const;

        placement_type turn(const placement_type& placement, Turn turn) const;

        bool same_location(const placement_type& a, const placement_type& b) const;

        // Adjacency for AI, the placements reachable in one step. A continuous layout
        // answers with navmesh nodes instead
        placements_type neighbors(const placement_type& placement) const;

        // Line of sight, by walking the cells the ray between them passes through. A grid
        // field of view algorithm gives a different, more roguelike answer and would
        // replace this
        bool visible(const placement_type& from, const placement_type& to) const;

        void clear();

    private:

        ids_type ring_walls(id_type wall_id0) const;
        polygon_type ring_outline(id_type wall_id0) const;

        walls_type m_wall_store;
        regions_type m_region_store;
        flats_type m_flat_store;
        std::optional<Grid> m_grid;
        ids_type m_cell_regions; // Grid extent, nullid where there is rock
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LAYOUT_HPP
