add_library(quetzal_stratum_sim STATIC
    ai.cpp
    attachments.cpp
    Character.cpp
    Clock.cpp
    CommandQueue.cpp
    commands.cpp
    decode_components.cpp
    DiscreteLayout.cpp
    DiscretePlacementIndex.cpp
    DisplayName.cpp
    EventQueue.cpp
    file_components.cpp
    layout_entities.cpp
    Flat.cpp
    Game.cpp
    GridMapping.cpp
    IdAllocator.cpp
    load_character.cpp
    load_layout.cpp
    make_document.cpp
    properties.cpp
    SavedGameDetails.cpp
    Scheduler.cpp
    Session.cpp
    Settings.cpp
    Site.cpp
    specs.cpp
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
    decode_components.hpp
    DirectionMask.hpp
    DiscreteLayout.hpp
    DiscretePlacementIndex.hpp
    DisplayName.hpp
    Event.hpp
    EventQueue.hpp
    file_components.hpp
    layout_entities.hpp
    Config.hpp
    Flat.hpp
    Game.hpp
    Generation.hpp
    GridMapping.hpp
    IdAllocator.hpp
    Intent.hpp
    Layout.hpp
    load_character.hpp
    load_layout.hpp
    make_document.hpp
    PersistentId.hpp
    PlacementIndex.hpp
    properties.hpp
    SavedGameDetails.hpp
    Scheduler.hpp
    Session.hpp
    Settings.hpp
    Site.hpp
    specs.hpp
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
