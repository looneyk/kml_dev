#if !defined(QUETZAL_STRATUM_VIEW_CELLS_HPP)
#define QUETZAL_STRATUM_VIEW_CELLS_HPP
//------------------------------------------------------------------------------
// stratum_view
// cells.hpp
//------------------------------------------------------------------------------

#include "Buffer.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <cstdint>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // What stands at a place, which is as far as the simulation is reduced before a view has
    // any say. Every view consumes these; what each turns them into is its own business
    // One entry per type of thing and none per state it can be in: what state it is in
    // travels beside it, so a door that can be shut, locked or concealed is one Kind
    // Rock is nothing standing here at all, which is what a closed cell and an edge with no
    // wall on it are alike. Which of the two a position is, the position itself says
    enum class Kind : uint8_t
    {
        Rock,
        Floor,
        Actor,
        Wall,
        Door
    };

    //--------------------------------------------------------------------------
    // The states a view draws apart, one flag each. Named after the component data they are
    // read from -- whichever of a component or its field reads as the state itself -- so
    // that a glyph set names what the simulation already calls it
    // Only what changes the drawing belongs here: a lock enters when a locked door is drawn
    // differently and not before
    enum class State : uint16_t
    {
        None = 0,
        Open = 1 << 0, // Openable::open
        Passable = 1 << 1, // the Passable tag
        Concealed = 1 << 2, // the Concealed tag, its found field being the inverse

        // Every cell this position touches is rock, which is what tells rock from an edge
        // with no wall on it. The two are one absence and this is the context that says
        // which, so nothing needs a second way of standing for nothing
        // Read from the layout rather than from a component, there being no entity here
        Solid = 1 << 3
    };

    constexpr State operator|(State a, State b);
    constexpr State operator&(State a, State b);
    constexpr State& operator|=(State& a, State b);
    constexpr bool any(State state);

    // Whether a state can stand on a kind at all, which is what rejects a glyph set naming
    // one that never applies rather than leaving it a line that can never match
    bool applies(Kind kind, State state);

    //--------------------------------------------------------------------------
    // Where the wall line continues from a position, one bit per direction. Sixteen patterns
    // is the whole of a box drawing set; a glyph set that can say less maps many of them onto
    // one glyph, which is the shape of a set with less to say rather than a key too wide
    enum class Mask : uint8_t
    {
        None = 0,
        East = 1 << 0,
        North = 1 << 1,
        West = 1 << 2,
        South = 1 << 3
    };

    constexpr Mask operator|(Mask a, Mask b);
    constexpr Mask operator&(Mask a, Mask b);
    constexpr Mask& operator|=(Mask& a, Mask b);
    constexpr bool any(Mask mask);

    //--------------------------------------------------------------------------
    // What stands at one position, the state it is in, and what reaches it
    struct Place
    {
        Kind kind = Kind::Rock;
        State state = State::None;
        Mask mask = Mask::None;

        friend bool operator==(const Place&, const Place&) = default;
    };

    using Cells = Buffer<Place>;
    using Lattice = Buffer<Place>;

    // The whole zone reduced to what occupies each cell, shaped by the layout's grid. One
    // Place per grid position and no more: what stands on the edges between cells is not in
    // it, so a wall or a door between two open cells reduces to Floor on both sides
    // Emits Rock, Floor, Actor and the Door that stands in a cell of its own, which is what
    // a doorway is in the block format. A door on an edge has no position here
    Cells cells(const stratum_sim::World& world, flecs::entity zone);

    // The whole zone reduced onto the lattice the wall format writes: the cells and what
    // stands on the edges between them. (2W+1) by (2H+1) for a W by H grid, with cell (i, j)
    // at (2i+1, 2j+1), the wall west of it at (2i, 2j+1), the wall south of it at (2i+1, 2j),
    // and the corners at the even positions in both
    // A corner answers Wall where any of the four edges touching it holds a wall or a door.
    // What a view makes of that -- a box drawing character, a column covering the joint -- is
    // its own, and the four edges are beside it in the buffer to decide from
    Lattice lattice(const stratum_sim::World& world, flecs::entity zone);

    //--------------------------------------------------------------------------
    namespace detail
    {

        constexpr State state_of(Kind kind)
        {
            return kind == Kind::Door ? State::Open | State::Passable | State::Concealed
                : kind == Kind::Rock ? State::Solid
                : State::None;
        }

    } // namespace detail

    //--------------------------------------------------------------------------
    constexpr State operator|(State a, State b)
    {
        return static_cast<State>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
    }

    constexpr State operator&(State a, State b)
    {
        return static_cast<State>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
    }

    constexpr State& operator|=(State& a, State b)
    {
        a = a | b;
        return a;
    }

    constexpr bool any(State state)
    {
        return static_cast<uint16_t>(state) != 0;
    }

    inline bool applies(Kind kind, State state)
    {
        return (state & detail::state_of(kind)) == state;
    }

    //--------------------------------------------------------------------------
    constexpr Mask operator|(Mask a, Mask b)
    {
        return static_cast<Mask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    constexpr Mask operator&(Mask a, Mask b)
    {
        return static_cast<Mask>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    constexpr Mask& operator|=(Mask& a, Mask b)
    {
        a = a | b;
        return a;
    }

    constexpr bool any(Mask mask)
    {
        return static_cast<uint8_t>(mask) != 0;
    }

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_CELLS_HPP
