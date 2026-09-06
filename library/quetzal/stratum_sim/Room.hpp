#if !defined(QUETZAL_STRATUM_SIM_ROOM_HPP)
#define QUETZAL_STRATUM_SIM_ROOM_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Room.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A named area within a Zone: the outline around it and the cells it covers
    // Metadata for the player and the level designer. Nothing here decides whether a cell
    // is open or whether a mover may pass, those being the layout's to decide
    class Room
    {
    public:

        //----------------------------------------------------------------------
        // The wall on the outline leaving a vertex
        enum class Type
        {
            Wall,
            Door,
            Open
        };

        //----------------------------------------------------------------------
        // One vertex of the outline, with what leaves it toward the next
        // id and type are kept for reference and acted on nowhere yet
        struct Vertex
        {
            point_type position;
            id_type wall_id = nullid; // The wall, or nullid at a corridor opening
            Type type = Type::Wall;
        };

        using outline_type = std::vector<Vertex>;
        using cells_type = std::vector<GridPosition>;

        Room() = default;
        explicit Room(const outline_type& outline);
        Room(const outline_type& outline, const cells_type& cells);
        Room(const Room&) = default;
        Room(Room&&) noexcept = default;
        ~Room() = default;

        Room& operator=(const Room&) = default;
        Room& operator=(Room&&) noexcept = default;

        // The Zone the room is part of. nullid until it is placed in one
        id_type zone_id() const;
        void set_zone_id(id_type zone_id);

        // What the room is called, what it looks like, and anything else said about it, all
        // of it on the entity rather than here. nullid until one is attached
        id_type entity_id() const;
        void set_entity_id(id_type entity_id);

        const outline_type& outline() const;
        outline_type& outline();
        void set_outline(const outline_type& outline);

        size_t vertex_count() const;
        const Vertex& vertex(size_t n) const;

        // The cells the room covers, which is what containment is tested against. A room is
        // not required to be one contiguous run of them
        const cells_type& cells() const;
        cells_type& cells();
        void set_cells(const cells_type& cells);
        void add_cell(const GridPosition& cell);

        size_t cell_count() const;
        bool contains(const GridPosition& cell) const;

        // Containment by position, the outline being horizontal so height is disregarded.
        // A point enclosed by an odd number of outline crossings is within it
        bool contains(const point_type& position) const;

        void clear();

    private:

        id_type m_zone_id = nullid;
        id_type m_entity_id = nullid;
        outline_type m_outline;
        cells_type m_cells;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ROOM_HPP
