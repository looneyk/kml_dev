add_library(quetzal_stratum_format STATIC
    AccessPointTarget.cpp
    checked_values.cpp
    GameManifestDocument.cpp
    grid.cpp
    json_util.cpp
    read_character.cpp
    read_config.cpp
    read_game.cpp
    read_world.cpp
    read_zone.cpp
    references.cpp
    RegionExtent.cpp
    RegionSpec.cpp
    RegionSpecsDocument.cpp
    SettingDefinitionsDocument.cpp
    SettingsDocument.cpp
    WorldDefinitionDocument.cpp
    write_character.cpp
    write_config.cpp
    write_game.cpp
    write_world.cpp
    write_zone.cpp
    ZoneDocument.cpp
    AccessPointTarget.hpp
    CharacterDocument.hpp
    checked_values.hpp
    config_format.hpp
    ConfigDocument.hpp
    GameDocument.hpp
    GameManifestDocument.hpp
    Notation.hpp
    WorldDefinitionDocument.hpp
    WorldDocument.hpp
    grid.hpp
    json_util.hpp
    read_character.hpp
    read_config.hpp
    read_game.hpp
    read_world.hpp
    read_zone.hpp
    references.hpp
    RegionExtent.hpp
    RegionSpec.hpp
    RegionSpecsDocument.hpp
    saved_game_format.hpp
    SettingDefinitionsDocument.hpp
    SettingsDocument.hpp
    types.hpp
    write_character.hpp
    write_config.hpp
    write_game.hpp
    write_world.hpp
    write_zone.hpp
    ZoneDocument.hpp
)

set_property(TARGET quetzal_stratum_format PROPERTY FOLDER "library")

target_include_directories(quetzal_stratum_format PUBLIC
    ${DEV_ROOT}/library
)

# Not nlohmann_json::nlohmann_json, as library3 is added after library/ and a namespaced target must already exist
target_link_libraries(quetzal_stratum_format PUBLIC quetzal_common quetzal_log quetzal_settings nlohmann_json)
