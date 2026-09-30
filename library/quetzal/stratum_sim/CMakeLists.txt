add_library(quetzal_stratum_sim STATIC
    ai.cpp
    attachments.cpp
    Character.cpp
    CharacterSpec.cpp
    Clock.cpp
    CommandQueue.cpp
    Config.cpp
    commands.cpp
    DiscreteLayout.cpp
    EventQueue.cpp
    file_components.cpp
    layout_entities.cpp
    Flat.cpp
    Game.cpp
    GridMapping.cpp
    IdAllocator.cpp
    load_layout.cpp
    properties.cpp
    RegionSpec.cpp
    SavedGameDetails.cpp
    Scheduler.cpp
    Session.cpp
    Settings.cpp
    Site.cpp
    World.cpp
    ZoneEntryIndex.cpp
    ai.hpp
    attachments.hpp
    Cell.hpp
    Character.hpp
    CharacterSpec.hpp
    Clock.hpp
    Command.hpp
    CommandQueue.hpp
    commands.hpp
    ContinuousLayout.hpp
    DirectionMask.hpp
    DiscreteLayout.hpp
    Event.hpp
    EventQueue.hpp
    file_components.hpp
    layout_entities.hpp
    Config.hpp
    Flat.hpp
    Game.hpp
    Generator.hpp
    GridMapping.hpp
    IdAllocator.hpp
    Intent.hpp
    Layout.hpp
    load_layout.hpp
    PersistentId.hpp
    RegionSpec.hpp
    properties.hpp
    SavedGameDetails.hpp
    Scheduler.hpp
    Session.hpp
    Settings.hpp
    Site.hpp
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
