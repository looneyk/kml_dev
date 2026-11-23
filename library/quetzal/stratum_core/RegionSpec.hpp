#if !defined(QUETZAL_STRATUM_CORE_REGIONSPEC_HPP)
#define QUETZAL_STRATUM_CORE_REGIONSPEC_HPP
//------------------------------------------------------------------------------
// stratum_core
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "RegionExtent.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <optional>
#include <string>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // The parameters that a region is created from
    struct RegionSpec
    {
        std::optional<std::string> name;
        std::optional<std::string> display_name;
        std::optional<std::string> parent;
        std::optional<std::string> region_spec;
        std::optional<seed_type> seed;

        std::optional<RegionExtent> extent;
        std::optional<discrete_vector_type> zone_size;

        KeyValueStore store;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_REGIONSPEC_HPP
