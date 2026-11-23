#if !defined(QUETZAL_STRATUM_CORE_ZONE_HPP)
#define QUETZAL_STRATUM_CORE_ZONE_HPP
//------------------------------------------------------------------------------
// stratum_core
// Zone.hpp
//------------------------------------------------------------------------------

#include "PersistentId.hpp"
#include "types.hpp"
#include <optional>
#include <string>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    struct Zone
    {
        PersistentId zone_id;
        std::optional<zone_position_type> position;
        std::optional<std::string> name;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_ZONE_HPP
