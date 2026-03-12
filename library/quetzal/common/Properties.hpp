#if !defined(QUETZAL_PROPERTIES_HPP)
#define QUETZAL_PROPERTIES_HPP
//------------------------------------------------------------------------------
// quetzal
// Properties.hpp
//------------------------------------------------------------------------------

#include "string_util.hpp"
#include <iosfwd>
#include <map>
#include <string>

namespace quetzal
{

    // Should be a class, map type is too common, but for now, operator usage is specific enough to prevent conflict ...
    using Properties = std::map<std::string, std::string>;

    Properties& operator|=(Properties& lhs, const Properties& rhs);
    Properties operator|(Properties lhs, const Properties& rhs);

    template<typename T>
    T extract(const Properties& properties, const std::string& key);

    std::istream& operator>>(std::istream& is, Properties& properties);
    std::ostream& operator<<(std::ostream& os, const Properties& properties);

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename T>
T quetzal::extract(const Properties& properties, const std::string& key)
{
    return properties.contains(key) ? to_type<T>(properties.at(key)) : T();
}

#endif // QUETZAL_PROPERTIES_HPP
