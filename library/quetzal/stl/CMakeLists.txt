add_custom_target(quetzal_stl SOURCES
    binary_util.hpp
    Reader.hpp
    symbols.hpp
    Writer.hpp
    WriterBinary.hpp
    WriterText.hpp
)

set_property(TARGET quetzal_stl PROPERTY FOLDER "library")
