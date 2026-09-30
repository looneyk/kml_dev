#if !defined(QUETZAL_CONFIG_HPP)
#define QUETZAL_CONFIG_HPP
//------------------------------------------------------------------------------
// quetzal common
// Config.hpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"

namespace quetzal
{

    // Temporary, so that its users need not change until they move to KeyValueStore
    using Config = KeyValueStore;

} // namespace quetzal

#endif // QUETZAL_CONFIG_HPP
