#if !defined(QUETZAL_PROPERTIES_HPP)
#define QUETZAL_PROPERTIES_HPP
//------------------------------------------------------------------------------
// quetzal
// Properties.hpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"

namespace quetzal
{

    // Temporary, so that its users need not change until they move to KeyValueStore
    using Properties = KeyValueStore;

} // namespace quetzal

#endif // QUETZAL_PROPERTIES_HPP
