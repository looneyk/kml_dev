add_library(quetzal_stratum_sim STATIC
    ai.cpp
    Clock.cpp
    CommandQueue.cpp
    commands.cpp
    DiscreteLayout.cpp
    EventQueue.cpp
    load_layout.cpp
    load_layout_catagen.cpp
    load_layout_dungen.cpp
    Room.cpp
    Scheduler.cpp
    Session.cpp
    World.cpp
    ai.hpp
    Clock.hpp
    Command.hpp
    CommandQueue.hpp
    commands.hpp
    ContinuousLayout.hpp
    DiscreteLayout.hpp
    Event.hpp
    EventQueue.hpp
    Intent.hpp
    Layout.hpp
    load_layout.hpp
    load_layout_catagen.hpp
    load_layout_dungen.hpp
    Room.hpp
    Scheduler.hpp
    Session.hpp
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
