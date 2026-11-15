//------------------------------------------------------------------------------
// stratum_sim
// Site.cpp
//------------------------------------------------------------------------------

#include "Site.hpp"

//------------------------------------------------------------------------------
std::optional<quetzal::id_type> quetzal::stratum_sim::wall_id_of(const Site& site)
{
    if (!site.address.has_value())
    {
        return std::nullopt;
    }

    const WallId* address = std::get_if<WallId>(&*site.address);
    return address != nullptr ? std::optional<id_type>(address->wall_id) : std::nullopt;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::placement_type& quetzal::stratum_sim::placement_of(flecs::entity e)
{
    return e.get<Site>().placement;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::set_placement(flecs::entity e, const placement_type& placement)
{
    if (e.has<Site>())
    {
        e.get_mut<Site>().placement = placement;
        e.modified<Site>();
        return;
    }

    e.set<Site>(Site {Relation::In, {}, placement});
    return;
}
