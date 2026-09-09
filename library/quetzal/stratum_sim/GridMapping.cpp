//------------------------------------------------------------------------------
// stratum_sim
// GridMapping.cpp
//------------------------------------------------------------------------------

#include "GridMapping.hpp"
#include <cmath>

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::GridMapping::contains(const discrete_point_type& grid_position) const
{
    return grid_position.i() >= 0 && grid_position.j() >= 0 && grid_position.i() < extent.x() && grid_position.j() < extent.y();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::discrete_point_type quetzal::stratum_sim::GridMapping::grid_position_at(const point_type& position) const
{
    auto [px, py, pz] = position.rep();
    auto [ox, oy, oz] = origin.rep();

    value_type i = std::floor((px - ox) / scale);
    value_type j = std::floor((py - oy) / scale);

    return {static_cast<long>(i), static_cast<long>(j)};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::GridMapping::position(const discrete_point_type& grid_position) const
{
    auto [ox, oy, oz] = origin.rep();

    value_type x = ox + (static_cast<value_type>(grid_position.i()) + value_type(0.5)) * scale;
    value_type y = oy + (static_cast<value_type>(grid_position.j()) + value_type(0.5)) * scale;

    return point_type(x, y, oz);
}
