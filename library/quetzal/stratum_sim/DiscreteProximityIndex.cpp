//------------------------------------------------------------------------------
// stratum_sim
// DiscreteProximityIndex.cpp
//------------------------------------------------------------------------------

#include "DiscreteProximityIndex.hpp"
#include "DiscreteLayout.hpp"
#include <algorithm>
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_sim::DiscreteProximityIndex::DiscreteProximityIndex(const DiscreteLayout& discrete_layout) :
    ProximityIndexBase(),
    m_discrete_layout(discrete_layout),
    m_entity_ids_by_slot(),
    m_placements_by_entity_id()
{
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteProximityIndex::place(id_type entity_id, const placement_type& placement)
{
    size_t slot = slot_at(placement);

    if (auto it = m_placements_by_entity_id.find(entity_id); it != m_placements_by_entity_id.end())
    {
        size_t slot_from = slot_at(it->second);
        if (slot_from != slot)
        {
            erase_entity_id(slot_from, entity_id);
            insert_entity_id(slot, entity_id);
        }

        it->second = placement;
        return;
    }

    insert_entity_id(slot, entity_id);
    m_placements_by_entity_id.emplace(entity_id, placement);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteProximityIndex::remove(id_type entity_id)
{
    auto it = m_placements_by_entity_id.find(entity_id);
    assert(it != m_placements_by_entity_id.end());

    erase_entity_id(slot_at(it->second), entity_id);
    m_placements_by_entity_id.erase(it);
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::DiscreteProximityIndex::contains(id_type entity_id) const
{
    return m_placements_by_entity_id.contains(entity_id);
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteProximityIndex::entity_count() const
{
    return m_placements_by_entity_id.size();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ProximityIndexBase::entity_ids_type quetzal::stratum_sim::DiscreteProximityIndex::entities_at(const placement_type& placement) const
{
    return entities_at(m_discrete_layout.cell_position_at(placement.position()));
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ProximityIndexBase::entity_ids_type quetzal::stratum_sim::DiscreteProximityIndex::entities_at(const discrete_point2_type& cell_position) const
{
    auto it = m_entity_ids_by_slot.find(m_discrete_layout.slot(cell_position));
    return it != m_entity_ids_by_slot.end() ? it->second : entity_ids_type();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ProximityIndexBase::entity_ids_type quetzal::stratum_sim::DiscreteProximityIndex::entities_within(const placement_type& placement, value_type distance) const
{
    assert(distance >= value_type(0));

    auto [x, y, z] = placement.position().rep();
    value_type distance2 = distance * distance;

    auto proximate = [&](const placement_type& other)
    {
        auto [ox, oy, oz] = other.position().rep();
        value_type dx = ox - x;
        value_type dy = oy - y;
        value_type dz = oz - z;

        return dx * dx + dy * dy + dz * dz <= distance2;
    };

    const discrete_vector2_type& extent = m_discrete_layout.grid().extent;
    discrete_point2_type lower = m_discrete_layout.cell_position_at(point_type(x - distance, y - distance, z));
    discrete_point2_type upper = m_discrete_layout.cell_position_at(point_type(x + distance, y + distance, z));
    coord_type i_begin = std::max(lower.x(), coord_type(0));
    coord_type j_begin = std::max(lower.y(), coord_type(0));
    coord_type i_end = std::min(upper.x() + 1, extent.x());
    coord_type j_end = std::min(upper.y() + 1, extent.y());

    entity_ids_type entity_ids;

    if (i_begin >= i_end || j_begin >= j_end)
    {
        return entity_ids;
    }

    size_t cell_count = static_cast<size_t>(i_end - i_begin) * static_cast<size_t>(j_end - j_begin);

    if (cell_count <= m_placements_by_entity_id.size())
    {
        for (coord_type j = j_begin; j < j_end; ++j)
        {
            for (coord_type i = i_begin; i < i_end; ++i)
            {
                auto it = m_entity_ids_by_slot.find(m_discrete_layout.slot(discrete_point2_type{i, j}));
                if (it == m_entity_ids_by_slot.end())
                {
                    continue;
                }

                for (id_type entity_id : it->second)
                {
                    if (proximate(m_placements_by_entity_id.at(entity_id)))
                    {
                        entity_ids.push_back(entity_id);
                    }
                }
            }
        }
    }
    else
    {
        for (const auto& [entity_id, other] : m_placements_by_entity_id)
        {
            if (proximate(other))
            {
                entity_ids.push_back(entity_id);
            }
        }
    }

    std::sort(entity_ids.begin(), entity_ids.end());
    return entity_ids;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::DiscreteProximityIndex::slot_at(const placement_type& placement) const
{
    return m_discrete_layout.slot(m_discrete_layout.cell_position_at(placement.position()));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteProximityIndex::insert_entity_id(size_t slot, id_type entity_id)
{
    entity_ids_type& entity_ids = m_entity_ids_by_slot[slot];
    entity_ids.insert(std::lower_bound(entity_ids.begin(), entity_ids.end(), entity_id), entity_id);
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::DiscreteProximityIndex::erase_entity_id(size_t slot, id_type entity_id)
{
    auto it = m_entity_ids_by_slot.find(slot);
    assert(it != m_entity_ids_by_slot.end());

    entity_ids_type& entity_ids = it->second;
    auto position = std::lower_bound(entity_ids.begin(), entity_ids.end(), entity_id);
    assert(position != entity_ids.end() && *position == entity_id);
    entity_ids.erase(position);

    if (entity_ids.empty())
    {
        m_entity_ids_by_slot.erase(it);
    }

    return;
}
