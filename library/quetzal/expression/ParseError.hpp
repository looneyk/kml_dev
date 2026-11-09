#if !defined(QUETZAL_EXPRESSION_PARSEERROR_HPP)
#define QUETZAL_EXPRESSION_PARSEERROR_HPP
//------------------------------------------------------------------------------
// expression
// ParseError.hpp
//------------------------------------------------------------------------------

#include <cstddef>
#include <string>

namespace quetzal::expression
{

    //--------------------------------------------------------------------------
    // Why text did not parse, and the position in the text where parsing stopped
    struct ParseError
    {
        enum class Code
        {
            InvalidCharacter,
            UnterminatedString,
            UnexpectedToken,
            UnknownIdentifier,
            ArgumentCount,
            ArgumentType,
            MixedTypes,
            StringOperand,
            NullOperand
        };

        Code code;
        std::size_t position = 0;
        std::string message;
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_PARSEERROR_HPP
