#if !defined(QUETZAL_STRATUM_CORE_PERSISTENTID_HPP)
#define QUETZAL_STRATUM_CORE_PERSISTENTID_HPP
//------------------------------------------------------------------------------
// stratum_core
// PersistentId.hpp
//------------------------------------------------------------------------------

#include <compare>
#include <cstdint>
#include <functional>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // An entity's identity in a world, for as long as the entity exists
    struct PersistentId
    {
        using type = std::uint64_t;

        type id = 0;

        auto operator<=>(const PersistentId&) const = default;
    };

} // namespace quetzal::stratum_core

namespace std
{

    //--------------------------------------------------------------------------
    template<>
    struct hash<quetzal::stratum_core::PersistentId>
    {
        size_t operator()(const quetzal::stratum_core::PersistentId& persistent_id) const noexcept
        {
            return hash<quetzal::stratum_core::PersistentId::type>()(persistent_id.id);
        }
    };

} // namespace std

#endif // QUETZAL_STRATUM_CORE_PERSISTENTID_HPP
