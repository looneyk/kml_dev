add_library(quetzal_stratum_sim STATIC
    CommandQueue.cpp
    commands.cpp
    EventQueue.cpp
    Layout.cpp
    load_layout.cpp
    Scheduler.cpp
    World.cpp
    Command.hpp
    CommandQueue.hpp
    commands.hpp
    Event.hpp
    EventQueue.hpp
    Layout.hpp
    load_layout.hpp
    Scheduler.hpp
    types.hpp
    World.hpp
)

set_property(TARGET quetzal_stratum_sim PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_sim PUBLIC
    ${DEV_ROOT}/library
)

# Plain name rather than flecs::flecs_static: library3 is added after library/ in the root
# CMakeLists, and a namespaced target must already exist when it is referenced
target_link_libraries(quetzal_stratum_sim PUBLIC quetzal_common flecs_static)
