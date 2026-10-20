add_library(quetzal_settings STATIC
    Settings.cpp
    SettingGroup.cpp
    SettingGroups.cpp
    SettingsSnapshot.cpp
    Setting.hpp
    Settings.hpp
    SettingGroup.hpp
    SettingGroups.hpp
    SettingsSnapshot.hpp
)

set_property(TARGET quetzal_settings PROPERTY FOLDER "library")

target_include_directories(quetzal_settings PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_settings PUBLIC quetzal_common)
