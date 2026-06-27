#if !defined(QUETZAL_PROPERTIES_HPP)
#define QUETZAL_PROPERTIES_HPP
//------------------------------------------------------------------------------
// quetzal
// Properties.hpp
//------------------------------------------------------------------------------

#include "KeyValueStore.hpp"
#include "string_util.hpp"
#include <iosfwd>
#include <optional>
#include <string>

namespace quetzal
{

    class Properties : public KeyValueStore
    {
    public:
        using KeyValueStore::KeyValueStore;

        Properties& operator|=(const Properties& rhs);
    };

    Properties operator|(Properties lhs, const Properties& rhs);

    template<typename T>
    std::optional<T> extract(const Properties& properties, const std::string& key);

    std::istream& operator>>(std::istream& is, Properties& properties);
    std::ostream& operator<<(std::ostream& os, const Properties& properties);

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename T>
std::optional<T> quetzal::extract(const Properties& properties, const std::string& key)
{
    return properties.contains(key) ? std::make_optional(to_type<T>(properties.value(key))) : std::nullopt;
}

#endif // QUETZAL_PROPERTIES_HPP
