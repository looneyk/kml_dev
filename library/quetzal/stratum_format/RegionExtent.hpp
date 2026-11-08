#if !defined(QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP)
#define QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// RegionExtent.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <optional>
#include <string>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The positions of a region in its worldspace: the union of boxes, for the ranges arrangement, or a list of zone
    // positions
    struct RegionExtent
    {
        enum class Arrangement
        {
            Ranges,
            Positions
        };

        // Each axis is an inclusive range, and an absent axis spans the parent's whole range on that axis
        struct Box
        {
            std::optional<math::Interval<coord_type>> x;
            std::optional<math::Interval<coord_type>> y;
            std::optional<math::Interval<coord_type>> z;
        };

        Arrangement arrangement = Arrangement::Ranges;
        std::vector<Box> boxes;
        std::vector<zone_position_type> positions;
    };

    namespace detail
    {

        // Throws std::runtime_error for an arrangement that is neither, a member the arrangement does not have, an axis
        // that is not an integer or a range of two, or a range whose lower bound is above its upper bound
        RegionExtent read_region_extent(const raw_values_type& v, const std::string& path);

        // The extent on one line, with an axis of one position written as an integer
        std::string region_extent_text(const RegionExtent& extent);

    } // namespace detail

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP
