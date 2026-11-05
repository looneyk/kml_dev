//------------------------------------------------------------------------------
// stratum_sim
// RegionTree.cpp
//------------------------------------------------------------------------------

#include "RegionTree.hpp"
#include <algorithm>
#include <format>
#include <utility>
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_sim::RegionTree::RegionTree(Region root) :
    m_regions()
{
    assert(!root.parent_id.has_value() && root.subregion_ids.empty());
    m_regions.push_back(std::move(root));
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::RegionTree::regions_type& quetzal::stratum_sim::RegionTree::regions() const
{
    return m_regions;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Region& quetzal::stratum_sim::RegionTree::root()
{
    return m_regions.front();
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Region& quetzal::stratum_sim::RegionTree::root() const
{
    return m_regions.front();
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Region& quetzal::stratum_sim::RegionTree::add_subregion(PersistentId parent_id, Region subregion)
{
    assert(!contains(subregion.region_id) && subregion.subregion_ids.empty());

    region(parent_id).subregion_ids.push_back(subregion.region_id);
    subregion.parent_id = parent_id;
    m_regions.push_back(std::move(subregion));

    return m_regions.back();
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::RegionTree::contains(PersistentId region_id) const
{
    return std::ranges::contains(m_regions, region_id, &Region::region_id);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::Region& quetzal::stratum_sim::RegionTree::region(PersistentId region_id)
{
    auto i = std::ranges::find(m_regions, region_id, &Region::region_id);
    assert(i != m_regions.end());
    return *i;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::Region& quetzal::stratum_sim::RegionTree::region(PersistentId region_id) const
{
    auto i = std::ranges::find(m_regions, region_id, &Region::region_id);
    assert(i != m_regions.end());
    return *i;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::RegionTree::extended_name(PersistentId region_id) const
{
    const Region& named = region(region_id);
    return named.parent_id.has_value() ? std::format("{}/{}", extended_name(named.parent_id.value()), named.name) : named.name;
}
