add_library(quetzal_stratum_format STATIC
    checked_values.cpp
    Document.cpp
    grid.cpp
    read_character.cpp
    read_game.cpp
    read_world.cpp
    read_zone.cpp
    references.cpp
    RelocatorTarget.cpp
    write_character.cpp
    write_game.cpp
    write_world.cpp
    write_zone.cpp
    written_form.cpp
    CharacterDocument.hpp
    checked_values.hpp
    Document.hpp
    GameDocument.hpp
    Notation.hpp
    WorldDocument.hpp
    grid.hpp
    read_character.hpp
    read_game.hpp
    read_world.hpp
    read_zone.hpp
    references.hpp
    RelocatorTarget.hpp
    saved_game_format.hpp
    types.hpp
    write_character.hpp
    write_game.hpp
    write_world.hpp
    write_zone.hpp
    written_form.hpp
)

set_property(TARGET quetzal_stratum_format PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_format PUBLIC
    ${DEV_ROOT}/library
)

# Not nlohmann_json::nlohmann_json, as library3 is added after library/ and a namespaced target must already exist
target_link_libraries(quetzal_stratum_format PUBLIC quetzal_common nlohmann_json)
