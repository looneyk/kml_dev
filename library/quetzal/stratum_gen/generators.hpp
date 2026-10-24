#if !defined(QUETZAL_STRATUM_GEN_GENERATORS_HPP)
#define QUETZAL_STRATUM_GEN_GENERATORS_HPP
//------------------------------------------------------------------------------
// stratum_gen
// generators.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/math/Random.hpp"
#include "quetzal/stratum_format/Document.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace quetzal::stratum_gen
{

    using zone_position_type = stratum_format::zone_position_type;
    using seed_type = math::portable_random_engine_type::result_type;

    //--------------------------------------------------------------------------
    struct ZoneRequest
    {
        std::string name; // Display name, empty for none
        std::optional<zone_position_type> position; // The zone's position in its region
        seed_type seed = 0;
        KeyValueStore parameters; // Generator specific
    };

    using generator_function_type = std::function<stratum_format::Document(const ZoneRequest& request)>;

    // The name the dungen generator is registered under
    inline constexpr std::string_view dungen_generator_name = "dungen";

    // Returns the document of a zone that dungen generates
    stratum_format::Document generate_dungen(const ZoneRequest& request);

    // <generator name, function>
    std::map<std::string, generator_function_type, std::less<>> generators();

} // namespace quetzal::stratum_gen

#endif // QUETZAL_STRATUM_GEN_GENERATORS_HPP
