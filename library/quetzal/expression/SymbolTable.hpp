#if !defined(QUETZAL_EXPRESSION_SYMBOLTABLE_HPP)
#define QUETZAL_EXPRESSION_SYMBOLTABLE_HPP
//------------------------------------------------------------------------------
// expression
// SymbolTable.hpp
//------------------------------------------------------------------------------

#include "DataType.hpp"
#include <concepts>
#include <format>
#include <functional>
#include <map>
#include <string>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    struct Symbol
    {
        Symbol() = default;
        Symbol(DataType type, std::string value);
        Symbol(const char* value);
        Symbol(const std::string& value);

        template<typename T> requires ((std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool> && !std::same_as<T, char>)
        Symbol(T value);

        DataType type = DataType::String;
        std::string value;
    };

    using SymbolTable = std::map<std::string, Symbol, std::less<>>;

} // namespace quetzal::expression

//------------------------------------------------------------------------------
template<typename T> requires ((std::integral<T> || std::floating_point<T>) && !std::same_as<T, bool> && !std::same_as<T, char>)
quetzal::expression::Symbol::Symbol(T value) :
    type(DataType::Number),
    value(std::format("{}", value))
{
}

#endif // QUETZAL_EXPRESSION_SYMBOLTABLE_HPP
