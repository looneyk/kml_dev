#if !defined(QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP)
#define QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP
//------------------------------------------------------------------------------
// stratum_format
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/stratum_core/RegionSpec.hpp"
#include <string>

namespace quetzal::stratum_format
{

    namespace detail
    {

        // Throws std::runtime_error for a member that is not of its type, a seed whose text is not a seed, or a key under a
        // member's name, such as zone_size.x
        stratum_core::RegionSpec read_region_spec(const raw_values_type& v, const std::string& path);

        // The spec on one line: its typed members, then its store
        std::string region_spec_text(const stratum_core::RegionSpec& region_spec);

    } // namespace detail

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP
