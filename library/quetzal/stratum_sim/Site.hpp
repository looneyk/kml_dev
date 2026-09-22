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
    // Whether an entity is inside what it is sited on or on its surface
    enum class Relation
    {
        In,
        On
    };

    //--------------------------------------------------------------------------
    // A wall of the zone's layout; its id encodes its normal
    struct WallAddress
    {
        id_type wall_id = nullid;
    };

    //--------------------------------------------------------------------------
    // One cell of a flat, the whole floor or ceiling of the zone, for an entity fixed to that surface. An entity resting on
    // a floor has a placement and no address
    struct FlatAddress
    {
        id_type flat_id = nullid;
        discrete_point_type cell_position;
    };

    using address_type = std::variant<WallAddress, FlatAddress>;

    //--------------------------------------------------------------------------
    // Where an entity is. The placement is relative to the address, or to the parent where there is none
    struct Site
    {
        Relation relation = Relation::In;
        std::optional<address_type> address;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    // Empty for a site on a flat cell or on the parent
    std::optional<id_type> wall_id_of(const Site& site);

    //--------------------------------------------------------------------------
    // set_placement leaves the rest of the site unchanged
    const placement_type& placement_of(flecs::entity e);
    void set_placement(flecs::entity e, const placement_type& placement);

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SITE_HPP
