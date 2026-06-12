add_library(quetzal_wavefront_obj STATIC
    Material.cpp
    MaterialLibrary.cpp
    Material.hpp
    MaterialLibrary.hpp
    Reader.hpp
    reader_util.hpp
    symbols.hpp
    Writer.hpp
)

set_property(TARGET quetzal_wavefront_obj PROPERTY FOLDER "library")

set_target_properties(quetzal_wavefront_obj PROPERTIES OUTPUT_NAME wavefront_obj)

target_include_directories(quetzal_wavefront_obj PRIVATE
    ${DEV_ROOT}/library
)
