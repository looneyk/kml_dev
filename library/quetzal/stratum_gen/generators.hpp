#if !defined(QUETZAL_STRATUM_GEN_GENERATORS_HPP)
#define QUETZAL_STRATUM_GEN_GENERATORS_HPP
//------------------------------------------------------------------------------
// stratum_gen
// generators.hpp
//------------------------------------------------------------------------------

#include "quetzal/stratum_format/Document.hpp"
#include "quetzal/stratum_sim/Generation.hpp"
#include <functional>
#include <string>
#include <string_view>

namespace quetzal::stratum_gen
{

    using warning_function_type = std::function<void(const std::string& warning)>;

    // The name the dungen generator is registered under
    inline constexpr std::string_view dungen_generator_name = "dungen";

    // The ZoneSpec key of the zone's display name, which a generator writes as the zone's name
    inline constexpr std::string_view name_key = "name";

    //--------------------------------------------------------------------------
    // Returns the document of a zone that dungen generates with its default parameters, at the ZoneSpec's level if it has
    // one, with the ZoneSpec's name, and with a seed from the platform's entropy source. Passes dungen's warnings to warning
    stratum_format::Document generate_dungen(const stratum_sim::ZoneSpec& zone_spec, const warning_function_type& warning);

    // Registers each generator under its name
    void register_generators(stratum_sim::Generation& generation, const warning_function_type& warning);

} // namespace quetzal::stratum_gen

#endif // QUETZAL_STRATUM_GEN_GENERATORS_HPP
