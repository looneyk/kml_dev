add_library(quetzal_expression STATIC
    Expression.cpp
    OpAdd.cpp
    OpBinary.cpp
    OpDivide.cpp
    OpFunctionCall.cpp
    OpIdentifier.cpp
    OpLiteral.cpp
    OpMultiply.cpp
    OpNegate.cpp
    OpSubtract.cpp
    OpUnary.cpp
    Parser.cpp
    Expression.hpp
    OpAdd.hpp
    OpBinary.hpp
    OpDivide.hpp
    OpFunctionCall.hpp
    OpIdentifier.hpp
    OpLiteral.hpp
    OpMultiply.hpp
    OpNegate.hpp
    OpSubtract.hpp
    OpUnary.hpp
    Parser.hpp
    SymbolTable.hpp
)

set_property(TARGET quetzal_expression PROPERTY FOLDER "library")

set_target_properties(quetzal_expression PROPERTIES OUTPUT_NAME expression)

target_include_directories(quetzal_expression PRIVATE
    ${DEV_ROOT}/library
)
