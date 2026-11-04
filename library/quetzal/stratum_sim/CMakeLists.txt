add_library(quetzal_stratum_sim STATIC
    actors.cpp
    ai.cpp
    attachments.cpp
    Character.cpp
    Clock.cpp
    CommandQueue.cpp
    commands.cpp
    Config.cpp
    decode_components.cpp
    DiscreteLayout.cpp
    DiscretePlacementIndex.cpp
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
    Name.cpp
    positions.cpp
    properties.cpp
    RandomStream.cpp
    SavedGameDetails.cpp
    seeds.cpp
    Scheduler.cpp
    Session.cpp
    PersistentSettings.cpp
    setting_definitions.cpp
    Site.cpp
    specs.cpp
    World.cpp
    ZoneEntryIndex.cpp
    actors.hpp
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
    Name.hpp
    PersistentId.hpp
    PlacementIndex.hpp
    positions.hpp
    properties.hpp
    RandomStream.hpp
    SavedGameDetails.hpp
    seeds.hpp
    Scheduler.hpp
    Session.hpp
    PersistentSettings.hpp
    setting_definitions.hpp
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
target_link_libraries(quetzal_stratum_sim PUBLIC quetzal_common quetzal_settings quetzal_stratum_format flecs_static)
target_link_libraries(quetzal_stratum_sim PRIVATE quetzal_log)
