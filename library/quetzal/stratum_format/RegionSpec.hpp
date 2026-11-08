#if !defined(QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP)
#define QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP
//------------------------------------------------------------------------------
// stratum_format
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include "RegionExtent.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/math/Random.hpp"
#include <optional>
#include <string>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The parameters that a region is created from
    // subregions inherit store values
    struct RegionSpec // type, terrain, competency level ...
    {
        using seed_type = math::portable_random_engine_type::result_type;

        std::optional<std::string> name;
        std::optional<std::string> display_name;
        std::optional<std::string> parent;
        std::optional<std::string> region_spec;
        std::optional<seed_type> seed;

        std::optional<RegionExtent> extent;

        // The size of each zone of a grid worldspace, for a top-level spec
        std::optional<discrete_vector_type> zone_size;

        KeyValueStore store;
    };

    namespace detail
    {

        // Throws std::runtime_error for a member that is not of its type, a seed whose text is not a seed, or a key under a
        // member's name, such as zone_size.x
        RegionSpec read_region_spec(const raw_values_type& v, const std::string& path);

        // The spec on one line: its typed members, then its store
        std::string region_spec_text(const RegionSpec& region_spec);

    } // namespace detail

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_REGIONSPEC_HPP
