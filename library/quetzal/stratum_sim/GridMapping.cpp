//------------------------------------------------------------------------------
// stratum_sim
// GridMapping.cpp
//------------------------------------------------------------------------------

#include "GridMapping.hpp"
#include <cmath>

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::GridMapping::contains(const discrete_point2_type& cell_position) const
{
    return cell_position.x() >= 0 && cell_position.y() >= 0 && cell_position.x() < extent.x() && cell_position.y() < extent.y();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::discrete_point2_type quetzal::stratum_sim::GridMapping::cell_position_at(const point_type& position) const
{
    auto [px, py, pz] = position.rep();
    auto [ox, oy, oz] = origin.rep();

    value_type i = std::floor((px - ox) / scale);
    value_type j = std::floor((py - oy) / scale);

    return {static_cast<coord_type>(i), static_cast<coord_type>(j)};
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::point_type quetzal::stratum_sim::GridMapping::position(const discrete_point2_type& cell_position) const
{
    auto [ox, oy, oz] = origin.rep();

    value_type x = ox + (static_cast<value_type>(cell_position.x()) + value_type(0.5)) * scale;
    value_type y = oy + (static_cast<value_type>(cell_position.y()) + value_type(0.5)) * scale;

    return point_type(x, y, oz);
}
