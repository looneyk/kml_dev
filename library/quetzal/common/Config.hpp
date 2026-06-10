#if !defined(QUETZAL_CONFIG_HPP)
#define QUETZAL_CONFIG_HPP
//------------------------------------------------------------------------------
// quetzal common
// Config.hpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"
#include <map>
#include <string>

namespace quetzal
{

    class Config : public KeyValueStore
    {
    public:
        using KeyValueStore::KeyValueStore;

        // deprecated ...
        std::string operator[](const std::string& key) const
        {
            return value_or(key, std::string{});
        }
    };

} // namespace quetzal

#endif // QUETZAL_CONFIG_HPP
