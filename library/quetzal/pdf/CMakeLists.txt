add_library(quetzal_pdf STATIC
    Document.cpp
    Font.cpp
    Document.hpp
    Font.hpp
)

set_property(TARGET quetzal_pdf PROPERTY FOLDER "library")

set_target_properties(quetzal_pdf PROPERTIES OUTPUT_NAME pdf)

target_include_directories(quetzal_pdf PRIVATE
    ${DEV_ROOT}/library
)
