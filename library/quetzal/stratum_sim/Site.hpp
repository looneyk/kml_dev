#if !defined(QUETZAL_STRATUM_SIM_SITE_HPP)
#define QUETZAL_STRATUM_SIM_SITE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Site.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <flecs.h>
#include <compare>
#include <optional>
#include <variant>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The relation of an entity to the object it is sited on: inside it, or on its surface
    enum class Relation
    {
        In,
        On
    };

    //--------------------------------------------------------------------------
    // A wall of the zone's layout; its id encodes its normal
    struct WallId
    {
        id_type wall_id = nullid;

        auto operator<=>(const WallId&) const = default;
    };

    //--------------------------------------------------------------------------
    // The position of one cell of a flat, which is the zone's whole floor or ceiling. It is for an entity fixed to that
    // surface; an entity resting on a floor has a placement and no address
    struct FlatCellPosition
    {
        id_type flat_id = nullid;
        discrete_point_type cell_position;

        auto operator<=>(const FlatCellPosition&) const = default;
    };

    using address_type = std::variant<WallId, FlatCellPosition>;

    //--------------------------------------------------------------------------
    // Where an entity is. The placement is relative to the address, or to the parent if there is no address
    struct Site
    {
        Relation relation = Relation::In;
        std::optional<address_type> address;
        placement_type placement;
    };

    //--------------------------------------------------------------------------
    // The entities sited on a wall or a flat cell, kept on the wall's or cell's entity
    struct AttachedEntities
    {
        std::vector<flecs::entity> entities;
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
