#if !defined(QUETZAL_STRATUM_SIM_REGION_TREE_HPP)
#define QUETZAL_STRATUM_SIM_REGION_TREE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionTree.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "Region.hpp"
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The regions of a worldspace: its top-level region, and the subregions of each region
    class RegionTree
    {
    public:

        using regions_type = std::vector<Region>;

        explicit RegionTree(Region root);

        // The top-level region first, and each region before its subregions
        const regions_type& regions() const;

        Region& root();
        const Region& root() const;

        // Adds the region as a subregion of the parent, and returns it. Requires a parent in the tree
        Region& add_subregion(PersistentId parent_id, Region subregion);

        bool contains(PersistentId region_id) const;

        // Requires a region in the tree
        Region& region(PersistentId region_id);
        const Region& region(PersistentId region_id) const;

        // Returns the name of each region from the top-level region down to the region, joined by '/'
        std::string extended_name(PersistentId region_id) const;

    private:

        regions_type m_regions;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_TREE_HPP
