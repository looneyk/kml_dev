#if !defined(QUETZAL_EXPRESSION_SYMBOLTABLE_HPP)
#define QUETZAL_EXPRESSION_SYMBOLTABLE_HPP
//------------------------------------------------------------------------------
// SymbolTable.hpp
//
//------------------------------------------------------------------------------

#include <map>
#include <string>

namespace quetzal::expression
{

    using SymbolTable = std::map<std::string, std::string>;

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_SYMBOLTABLE_HPP
