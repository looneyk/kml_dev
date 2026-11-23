#if !defined(QUETZAL_STRATUM_CORE_REGION_TREE_HPP)
#define QUETZAL_STRATUM_CORE_REGION_TREE_HPP
//------------------------------------------------------------------------------
// stratum_core
// RegionTree.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "Region.hpp"
#include <string>
#include <vector>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // The regions of a worldspace
    class RegionTree
    {
    public:

        using regions_type = std::vector<Region>;

        explicit RegionTree(Region root);

        // The node names in the order of tree traversal, starting at the root, regions precede their subregions
        const regions_type& regions() const;

        Region& root();
        const Region& root() const;

        // Requires the region to be in the tree
        // Returns the newly inserted region
        Region& add_subregion(PersistentId parent_id, Region subregion);

        bool contains(PersistentId region_id) const;

        // Requires the region to be in the tree
        Region& region(PersistentId region_id);
        const Region& region(PersistentId region_id) const;

        std::string extended_name(PersistentId region_id) const;

    private:

        regions_type m_regions;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_REGION_TREE_HPP
