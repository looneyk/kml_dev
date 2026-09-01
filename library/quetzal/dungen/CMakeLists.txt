# Bounds and Item sit in this directory but were not in dungen.vcxproj and are not built here
# either; they are the only files that reach outside dungen, to quetzal/math
add_library(quetzal_dungen STATIC
    Cell.cpp
    Cells.cpp
    CellType.cpp
    CellTypeProperties.cpp
    Door.cpp
    Level.cpp
    Pattern.cpp
    Room.cpp
    Shaft.cpp
    Wall.cpp
    Cell.hpp
    Cells.hpp
    CellType.hpp
    CellTypeProperties.hpp
    Door.hpp
    Level.hpp
    Pattern.hpp
    Room.hpp
    Shaft.hpp
    types.hpp
    Wall.hpp
)

set_property(TARGET quetzal_dungen PROPERTY FOLDER "library")

target_include_directories(quetzal_dungen PUBLIC
    ${DEV_ROOT}/library
)
