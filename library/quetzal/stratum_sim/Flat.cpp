//------------------------------------------------------------------------------
// stratum_sim
// Flat.cpp
//------------------------------------------------------------------------------

#include "Flat.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_sim::Flat::Flat(value_type height, bool ceiling) :
    m_height(height),
    m_ceiling(ceiling),
    m_room_heights(),
    m_cell_heights()
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
std::optional<quetzal::stratum_sim::value_type> quetzal::stratum_sim::Flat::room_height(id_type room_id) const
{
    auto i = m_room_heights.find(room_id);
    return i != m_room_heights.end() ? std::optional<value_type>(i->second) : std::nullopt;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_room_height(id_type room_id, value_type height)
{
    m_room_heights[room_id] = height;
    return;
}

//------------------------------------------------------------------------------
std::optional<quetzal::stratum_sim::value_type> quetzal::stratum_sim::Flat::cell_height(const discrete_point_type& cell_position) const
{
    auto i = m_cell_heights.find(cell_position);
    return i != m_cell_heights.end() ? std::optional<value_type>(i->second) : std::nullopt;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Flat::set_cell_height(const discrete_point_type& cell_position, value_type height)
{
    m_cell_heights[cell_position] = height;
    return;
}
