add_library(stratum_godot SHARED
    register_types.cpp
    StratumWorld.cpp
    register_types.hpp
    StratumWorld.hpp
)

set_property(TARGET stratum_godot PROPERTY FOLDER "application")

# Shared data lives outside any one application, so its location is configured in rather than
# found relative to wherever Godot happens to be started from
target_compile_definitions(stratum_godot PRIVATE KML_DEV_ASSETS_ROOT="${DEV_ROOT}/assets")

# flecs_static carries this as a PUBLIC define, but godot-cpp's headers are compiled into this
# target too and flecs.h would otherwise declare everything dllimport
target_compile_definitions(stratum_godot PRIVATE flecs_STATIC)

target_link_libraries(stratum_godot PRIVATE
    quetzal_stratum_view
    godot-cpp
)

include(${DEV_ROOT}/library/quetzal/godot/gdextension.cmake)
quetzal_configure_gdextension(stratum_godot "${CMAKE_CURRENT_SOURCE_DIR}/project")
