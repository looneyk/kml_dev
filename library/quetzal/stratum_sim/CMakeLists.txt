add_library(quetzal_stratum_sim STATIC
    ai.cpp
    attachments.cpp
    Clock.cpp
    CommandQueue.cpp
    commands.cpp
    DiscreteLayout.cpp
    EventQueue.cpp
    Flat.cpp
    GridMapping.cpp
    load_layout.cpp
    Scheduler.cpp
    Session.cpp
    Site.cpp
    World.cpp
    ai.hpp
    attachments.hpp
    Clock.hpp
    Cell.hpp
    Command.hpp
    CommandQueue.hpp
    commands.hpp
    ContinuousLayout.hpp
    DirectionMask.hpp
    DiscreteLayout.hpp
    Event.hpp
    EventQueue.hpp
    Flat.hpp
    Generator.hpp
    GridMapping.hpp
    Intent.hpp
    Layout.hpp
    load_layout.hpp
    Scheduler.hpp
    Session.hpp
    Site.hpp
    types.hpp
    World.hpp
)

set_property(TARGET quetzal_stratum_sim PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_sim PUBLIC
    ${DEV_ROOT}/library
)

# Plain name rather than flecs::flecs_static: library3 is added after library/ in the root
# CMakeLists, and a namespaced target must already exist when it is referenced
target_link_libraries(quetzal_stratum_sim PUBLIC quetzal_common quetzal_stratum_format flecs_static)
