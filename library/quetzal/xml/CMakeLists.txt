add_library(quetzal_xml STATIC
    Attribute.cpp
    Declaration.cpp
    Document.cpp
    Element.cpp
    Token.cpp
    Attribute.hpp
    Declaration.hpp
    Document.hpp
    Element.hpp
    Token.hpp
)

set_property(TARGET quetzal_xml PROPERTY FOLDER "library")

set_target_properties(quetzal_xml PROPERTIES OUTPUT_NAME xml)

target_include_directories(quetzal_xml PRIVATE
    ${DEV_ROOT}/library
)
