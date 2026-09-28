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
    Parameters.cpp
    Pattern.cpp
    place_staircases.cpp
    remove_doors.cpp
    Room.cpp
    Staircase.cpp
    Wall.cpp
    write_zone.cpp
    Zone.cpp
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
    Parameters.hpp
    Pattern.hpp
    place_staircases.hpp
    remove_doors.hpp
    Room.hpp
    Staircase.hpp
    types.hpp
    Wall.hpp
    write_zone.hpp
    Zone.hpp
)

set_property(TARGET quetzal_dungen PROPERTY FOLDER "library")

target_include_directories(quetzal_dungen PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_dungen PUBLIC quetzal_common quetzal_math quetzal_stratum_format)
