# Bounds and Item sit in this directory but were not in dungen.vcxproj and are not built here
# either; they are the only files that reach outside dungen, to quetzal/math
add_library(quetzal_dungen STATIC
    Algorithm.cpp
    Cell.cpp
    Cells.cpp
    CellType.cpp
    CellTypeProperties.cpp
    Door.cpp
    generate.cpp
    generate_cave1.cpp
    generate_grid1.cpp
    generate_maze1.cpp
    Level.cpp
    Parameters.cpp
    Pattern.cpp
    place_stairs.cpp
    Room.cpp
    Stair.cpp
    StairPlacements.cpp
    Wall.cpp
    write_level.cpp
    Algorithm.hpp
    Cell.hpp
    Cells.hpp
    CellType.hpp
    CellTypeProperties.hpp
    Door.hpp
    generate.hpp
    generate_cave1.hpp
    generate_grid1.hpp
    generate_maze1.hpp
    Level.hpp
    Parameters.hpp
    Pattern.hpp
    place_stairs.hpp
    Room.hpp
    Stair.hpp
    StairPlacements.hpp
    types.hpp
    Wall.hpp
    write_level.hpp
)

set_property(TARGET quetzal_dungen PROPERTY FOLDER "library")

target_include_directories(quetzal_dungen PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_dungen PUBLIC quetzal_common quetzal_math)
