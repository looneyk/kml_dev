#if !defined(QUETZAL_STRATUM_VIEW_CELLS_HPP)
#define QUETZAL_STRATUM_VIEW_CELLS_HPP
//------------------------------------------------------------------------------
// stratum_view
// cells.hpp
//------------------------------------------------------------------------------

#include "Buffer.hpp"
#include "types.hpp"
#include "quetzal/stratum_sim/DirectionMask.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <cstdint>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // The fixed structure at a place, whatever its state
    // None is an edge with no wall or a corner no wall line reaches. Solid is a closed cell, or a lattice position whose
    // cells are all closed. StairsUp and StairsDown are a floor cell with a relocator on it, or a stair block: the closed cell
    // beyond a wall with a relocator on it
    enum class PlaceType : uint8_t
    {
        None,
        Solid,
        Floor,
        Wall,
        Door,
        StairsUp,
        StairsDown
    };

    //--------------------------------------------------------------------------
    // The states that a view draws differently, one flag each, named after the component data they are read from
    enum class State : uint16_t
    {
        None = 0,
        Open = 1 << 0, // Access is open
        Concealed = 1 << 1 // Detection conceals it from actors in general, though not from the viewer
    };

    constexpr State operator|(State a, State b);
    constexpr State operator&(State a, State b);
    constexpr State& operator|=(State& a, State b);
    constexpr bool any(State state);

    // Returns true if a place type can be in the state
    bool applies(PlaceType type, State state);

    //--------------------------------------------------------------------------
    // The directions in which the wall line continues from a position
    using stratum_sim::DirectionMask;

    //--------------------------------------------------------------------------
    // The contents of one position: the fixed structure, its state, the directions in which the wall line continues, and
    // whether an actor is on it
    struct Place
    {
        PlaceType type = PlaceType::None;
        State state = State::None;
        DirectionMask mask = DirectionMask::None;
        bool actor = false;

        friend bool operator==(const Place&, const Place&) = default;
    };

    using Cells = Buffer<Place>;
    using Lattice = Buffer<Place>;

    // The zone reduced to one Place per grid cell: Solid, Floor, a Door that occupies a cell, as a doorway does in the
    // block notation, or stairs. A wall or door on the edge between two cells has no position here
    // The zone is as the viewer perceives it: a door the viewer finds concealed is drawn as though there were none
    Cells cells(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer);

    // The zone reduced onto the lattice of the wall notation: (2W+1) by (2H+1) for a W by H grid, with cell (i, j) at
    // (2i+1, 2j+1), the wall west of it at (2i, 2j+1), the wall south of it at (2i+1, 2j), and the corners at the even
    // positions in both
    // A corner is Wall if any of its four edges has a wall or a door. The viewer is used as in cells
    Lattice lattice(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer);

    //--------------------------------------------------------------------------
    namespace detail
    {

        constexpr State state_of(PlaceType type)
        {
            return type == PlaceType::Door ? State::Open | State::Concealed : State::None;
        }

    } // namespace detail

    //--------------------------------------------------------------------------
    constexpr State operator|(State a, State b)
    {
        return static_cast<State>(std::to_underlying(a) | std::to_underlying(b));
    }

    constexpr State operator&(State a, State b)
    {
        return static_cast<State>(std::to_underlying(a) & std::to_underlying(b));
    }

    constexpr State& operator|=(State& a, State b)
    {
        a = a | b;
        return a;
    }

    constexpr bool any(State state)
    {
        return state != State::None;
    }

    inline bool applies(PlaceType type, State state)
    {
        return (state & detail::state_of(type)) == state;
    }

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_CELLS_HPP
