#if !defined(QUETZAL_STRATUM_VIEW_CELLS_HPP)
#define QUETZAL_STRATUM_VIEW_CELLS_HPP
//------------------------------------------------------------------------------
// stratum_view
// cells.hpp
//------------------------------------------------------------------------------

#include "Buffer.hpp"
#include "types.hpp"
#include "quetzal/stratum_sim/attachments.hpp"
#include "quetzal/stratum_sim/DirectionMask.hpp"
#include "quetzal/stratum_sim/World.hpp"
#include <cstdint>
#include <optional>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // The fixed structure at a place
    // None is an edge with no wall or a corner no wall line reaches. Solid is a closed cell, or a lattice position whose
    // cells are all closed. StairsUp and StairsDown are a floor cell with an access point on it, or a stair block: the closed
    // cell beyond a wall with an access point on it
    enum class FeatureType : uint8_t
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
    // The directions in which the wall line continues from a position
    using stratum_sim::DirectionMask;

    //--------------------------------------------------------------------------
    // The contents of one position: the fixed structure, its access and its general detection state, the directions in which
    // the wall line continues, and whether an actor is on it
    struct Place
    {
        FeatureType type = FeatureType::None;
        std::optional<stratum_sim::AccessState> access;
        stratum_sim::DetectionState detection = stratum_sim::DetectionState::Visible;
        DirectionMask mask = DirectionMask::None;
        bool actor = false;

        friend bool operator==(const Place&, const Place&) = default;
    };

    using Cells = Buffer<Place>;
    using Lattice = Buffer<Place>;

    // The zone reduced to one Place per grid cell: Solid, Floor, a Door that occupies a cell, or stairs. A wall or door on the edge between two cells has no position here
    // The zone is as the viewer perceives it: a door the viewer finds concealed is drawn as though there were none
    Cells cells(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer);

    // The zone reduced onto the lattice of the wall notation: (2W+1) by (2H+1) for a W by H grid, with cell (i, j) at
    // (2i+1, 2j+1), the wall west of it at (2i, 2j+1), the wall south of it at (2i+1, 2j), and the corners at the even
    // positions in both
    // A corner is Wall if any of its four edges has a wall or a door. The zone is as the viewer perceives it
    Lattice lattice(const stratum_sim::World& world, flecs::entity zone, flecs::entity viewer);

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_CELLS_HPP
