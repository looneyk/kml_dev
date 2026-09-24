#if !defined(QUETZAL_STRATUM_SIM_PERSISTENTID_HPP)
#define QUETZAL_STRATUM_SIM_PERSISTENTID_HPP
//------------------------------------------------------------------------------
// stratum_sim
// PersistentId.hpp
//------------------------------------------------------------------------------

#include <flecs.h>
#include <compare>
#include <cstdint>
#include <functional>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // An entity's identity in a world, for as long as the entity exists
    struct PersistentId
    {
        using type = std::uint64_t;

        type id = 0;

        auto operator<=>(const PersistentId&) const = default;
    };

    //--------------------------------------------------------------------------
    // A reference to an entity that may be in another zone, or in none loaded
    struct EntityReference
    {
        PersistentId persistent_id;
        flecs::entity entity; // The runtime entity last resolved, not alive once its zone has been unloaded
    };

} // namespace quetzal::stratum_sim

namespace std
{

    //--------------------------------------------------------------------------
    template<>
    struct hash<quetzal::stratum_sim::PersistentId>
    {
        size_t operator()(const quetzal::stratum_sim::PersistentId& persistent_id) const noexcept
        {
            return hash<quetzal::stratum_sim::PersistentId::type>()(persistent_id.id);
        }
    };

} // namespace std

#endif // QUETZAL_STRATUM_SIM_PERSISTENTID_HPP
