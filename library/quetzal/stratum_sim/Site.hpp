#if !defined(QUETZAL_STRATUM_SIM_SITE_HPP)
#define QUETZAL_STRATUM_SIM_SITE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Site.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <optional>
#include <variant>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Whether an entity is within what it is sited on or against its surface
    enum class Relation
    {
        In,
        On
    };

    //--------------------------------------------------------------------------
    // A wall of the zone's layout, whose id also provides its normal
    struct WallAddress
    {
        id_type wall_id = nullid;
    };

    //--------------------------------------------------------------------------
    // One cell of a flat, a flat being the whole floor or ceiling of the zone. For an entity fixed
    // to that surface: what rests on a floor has a placement and no address, a cell being a position
    // and an address to one a position written a second way
    struct FlatAddress
    {
        id_type flat_id = nullid;
        discrete_point_type cell_position;
    };

    using address_type = std::variant<WallAddress, FlatAddress>;

    //--------------------------------------------------------------------------
    // Where an entity is. The placement is in the frame the address resolves to, and in the parent's
    // own where there is none -- a zone's child is placed in the zone. An address is an id rather
    // than a relationship because a wall is a record and not an entity; the reverse direction is a
    // query filtered by that id
    struct Site
    {
        Relation relation = Relation::In;
        std::optional<address_type> address;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    // The wall an entity is sited on, and none where it is sited on a flat cell or on its parent
    std::optional<id_type> wall_id_of(const Site& site);

    //--------------------------------------------------------------------------
    // An entity's placement, and where it is set the rest of its site is left as it was
    const placement_type& placement_of(flecs::entity e);
    void set_placement(flecs::entity e, const placement_type& placement);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SITE_HPP
