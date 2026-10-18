add_library(quetzal_settings STATIC
    Settings.cpp
    SettingsGroup.cpp
    SettingsGroups.cpp
    Setting.hpp
    Settings.hpp
    SettingsGroup.hpp
    SettingsGroups.hpp
)

set_property(TARGET quetzal_settings PROPERTY FOLDER "library")

target_include_directories(quetzal_settings PUBLIC
    ${DEV_ROOT}/library
)

target_link_libraries(quetzal_settings PUBLIC quetzal_common)
