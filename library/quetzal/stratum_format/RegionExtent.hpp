#if !defined(QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP)
#define QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// RegionExtent.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/stratum_core/RegionExtent.hpp"
#include <string>

namespace quetzal::stratum_format
{

    namespace detail
    {

        // Throws std::runtime_error for an extent without exactly one of boxes and positions, an axis that is not an integer
        // or an interval between two, or an interval whose lower bound is above its upper bound
        stratum_core::RegionExtent read_region_extent(const raw_values_type& v, const std::string& path);

        // The extent on one line, with an axis of one position written as an integer
        std::string region_extent_text(const stratum_core::RegionExtent& extent);

    } // namespace detail

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REGIONEXTENT_HPP
