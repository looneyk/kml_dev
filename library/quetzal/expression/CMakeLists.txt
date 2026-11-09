add_library(quetzal_expression STATIC
    Parser.cpp
    SymbolTable.cpp
    DataType.hpp
    Evaluator.hpp
    Expression.hpp
    Node.hpp
    ParseError.hpp
    Parser.hpp
    SymbolTable.hpp
)

set_property(TARGET quetzal_expression PROPERTY FOLDER "library")

set_target_properties(quetzal_expression PROPERTIES OUTPUT_NAME expression)

target_include_directories(quetzal_expression PRIVATE
    ${DEV_ROOT}/library
)
