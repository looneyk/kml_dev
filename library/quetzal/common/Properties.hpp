#if !defined(QUETZAL_PROPERTIES_HPP)
#define QUETZAL_PROPERTIES_HPP
//------------------------------------------------------------------------------
// quetzal
// Properties.hpp
//------------------------------------------------------------------------------

#include <iosfwd>
#include <map>
#include <string>

namespace quetzal
{

    using Properties = std::map<std::string, std::string>;

    Properties& operator|=(Properties& lhs, const Properties& rhs);
    Properties operator|(Properties lhs, const Properties& rhs);

    std::istream& operator>>(std::istream& is, Properties& properties);
    std::ostream& operator<<(std::ostream& os, const Properties& properties);

} // namespace quetzal

#endif // QUETZAL_PROPERTIES_HPP
