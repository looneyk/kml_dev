add_library(stratum_godot SHARED
    register_types.cpp
    StratumWorld.cpp
    register_types.hpp
    StratumWorld.hpp
)

set_property(TARGET stratum_godot PROPERTY FOLDER "application")

# flecs_static has this as a PUBLIC define, but godot-cpp's headers are compiled into this
# target too and flecs.h would otherwise declare everything dllimport
target_compile_definitions(stratum_godot PRIVATE flecs_STATIC)

target_link_libraries(stratum_godot PRIVATE
    quetzal_log
    quetzal_stratum_gen
    quetzal_stratum_view
    godot-cpp
)

include(${DEV_ROOT}/library/quetzal/godot/gdextension.cmake)
quetzal_configure_gdextension(stratum_godot "${CMAKE_CURRENT_SOURCE_DIR}/project" "manifest=data/manifests/godot_block.json")
