#if !defined(QUETZAL_EXPRESSION_DATATYPE_HPP)
#define QUETZAL_EXPRESSION_DATATYPE_HPP
//------------------------------------------------------------------------------
// expression
// DataType.hpp
//------------------------------------------------------------------------------

namespace quetzal::expression
{

    // Null is the type of the literal null, which has no value
    enum class DataType
    {
        Null,
        Number,
        String
    };

} // namespace quetzal::expression

#endif // QUETZAL_EXPRESSION_DATATYPE_HPP
