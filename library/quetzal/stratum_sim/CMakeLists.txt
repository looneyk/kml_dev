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
    IdAllocator.cpp
    load_layout.cpp
    Player.cpp
    Scheduler.cpp
    Session.cpp
    Site.cpp
    StratumGame.cpp
    World.cpp
    ZoneEntryIndex.cpp
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
    IdAllocator.hpp
    Intent.hpp
    Layout.hpp
    load_layout.hpp
    PersistentId.hpp
    Player.hpp
    Scheduler.hpp
    Session.hpp
    Site.hpp
    StratumGame.hpp
    types.hpp
    World.hpp
    Zone.hpp
    ZoneEntryIndex.hpp
)

set_property(TARGET quetzal_stratum_sim PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_sim PUBLIC
    ${DEV_ROOT}/library
)

# Not flecs::flecs_static, as library3 is added after library/ and a namespaced target must already exist
target_link_libraries(quetzal_stratum_sim PUBLIC quetzal_common quetzal_stratum_format flecs_static)
