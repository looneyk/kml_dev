//------------------------------------------------------------------------------
// stratum_sim
// Flat.cpp
//------------------------------------------------------------------------------

#include "Flat.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_sim::Flat::Flat(value_type height, bool ceiling) :
    m_height(height),
    m_ceiling(ceiling),
    m_sampling(),
    m_heights()
{
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Flat::ceiling() const
{
    return m_ceiling;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_ceiling(bool b)
{
    m_ceiling = b;
    return;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_sim::value_type> quetzal::stratum_sim::Flat::height(const point_type& position) const
{
    if (!sampled())
    {
        return m_height;
    }

    discrete_point_type grid_position = m_sampling->grid_position_at(position);
    if (!m_sampling->contains(grid_position))
    {
        return {}; // Off the sampling grid is off the surface
    }

    return sample(grid_position);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::value_type quetzal::stratum_sim::Flat::height() const
{
    return m_height;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_height(value_type height)
{
    m_height = height;
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_sampling(const GridMapping& grid)
{
    m_sampling = grid;
    m_heights.assign(static_cast<size_t>(grid.extent.x()) * static_cast<size_t>(grid.extent.y()), m_height);

    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::Flat::sampled() const
{
    return m_sampling.has_value();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::GridMapping& quetzal::stratum_sim::Flat::sampling() const
{
    assert(sampled());
    return *m_sampling;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_sim::value_type> quetzal::stratum_sim::Flat::sample(const discrete_point_type& grid_position) const
{
    assert(sampled());
    assert(m_sampling->contains(grid_position));

    return m_heights[static_cast<size_t>(grid_position.j()) * static_cast<size_t>(m_sampling->extent.x()) + static_cast<size_t>(grid_position.i())];
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_sample(const discrete_point_type& grid_position, std::optional<value_type> height)
{
    assert(sampled());
    assert(m_sampling->contains(grid_position));

    m_heights[static_cast<size_t>(grid_position.j()) * static_cast<size_t>(m_sampling->extent.x()) + static_cast<size_t>(grid_position.i())] = height;

    return;
}
