cmake_minimum_required(VERSION 3.25) # CXX26 dialect support
project(kml_dev C CXX)

# CMake has no CXX26 dialect flag for MSVC, so ask MSVC for the latest directly and let every other
# compiler be told C++26 the documented way
if(MSVC)
    set(CMAKE_CXX_STANDARD 23)
    add_compile_options(/std:c++latest /bigobj /W4) # heavily templated translation units exceed the default section count
else()
    set(CMAKE_CXX_STANDARD 26)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DEV_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARY3_DIR ${DEV_ROOT}/library3)

# Forced into the cache rather than merely set: godot-cpp's windows.cmake caches MultiThreaded, the
# static release CRT, even for a debug build, and quetzal's static libraries will not link into a
# GDExtension unless the two agree
set(CMAKE_MSVC_RUNTIME_LIBRARY "$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreaded>"
    CACHE STRING "MSVC runtime library for targets using the MSVC ABI" FORCE)

option(KML_DEV_GODOT "Build the Godot integration" ON)

find_package(Vulkan REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEV_ROOT}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEV_ROOT}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# add_subdirectory_ext: drop-in replacement for add_subdirectory that also registers the
# directory for a clean_<name> target running MSBuild's per-project Clean on the generated vcxproj.
# An optional second argument gives the target name explicitly, for directories whose
# target name doesn't match the directory name, quetzal libraries for example,
# otherwise defaults to the directory name.
# The targets themselves are created by support/, which keeps their generated projects and
# scratch directories together there instead of scattering three entries per target through the
# top of the build directory -- a function body runs in the scope of its caller, so creating them
# here would place every one of them at the root regardless of the directory being added
function(add_subdirectory_ext dir)
    add_subdirectory(${dir})
    if(ARGC GREATER 1)
        set(name ${ARGV1})
    else()
        get_filename_component(name ${dir} NAME)
    endif()
    set_property(GLOBAL APPEND PROPERTY KML_DEV_CLEAN_TARGET_NAMES ${name})
    set_property(GLOBAL APPEND PROPERTY KML_DEV_CLEAN_TARGET_DIRS ${dir})
endfunction()

add_subdirectory_ext(library/quetzal/brep quetzal_brep)
add_subdirectory_ext(library/quetzal/catagen quetzal_catagen)
add_subdirectory_ext(library/quetzal/common quetzal_common)
add_subdirectory_ext(library/quetzal/dungen quetzal_dungen)
add_subdirectory_ext(library/quetzal/expression quetzal_expression)
add_subdirectory_ext(library/quetzal/game quetzal_game)
add_subdirectory_ext(library/quetzal/geometry quetzal_geometry)
if(KML_DEV_GODOT)
    add_subdirectory_ext(library/quetzal/godot quetzal_godot)
endif()
add_subdirectory_ext(library/quetzal/math quetzal_math)
add_subdirectory_ext(library/quetzal/model quetzal_model)
add_subdirectory_ext(library/quetzal/pdf quetzal_pdf)
add_subdirectory_ext(library/quetzal/physics quetzal_physics)
add_subdirectory_ext(library/quetzal/scene_graph quetzal_scene_graph)
add_subdirectory_ext(library/quetzal/spatial_search quetzal_spatial_search)
add_subdirectory_ext(library/quetzal/stl quetzal_stl)
add_subdirectory_ext(library/quetzal/stratum_format quetzal_stratum_format)
add_subdirectory_ext(library/quetzal/stratum_sim quetzal_stratum_sim)
add_subdirectory_ext(library/quetzal/stratum_view quetzal_stratum_view)
add_subdirectory_ext(library/quetzal/svg quetzal_svg)
add_subdirectory_ext(library/quetzal/triangulation quetzal_triangulation)
add_subdirectory_ext(library/quetzal/vulkan quetzal_vulkan)
add_subdirectory_ext(library/quetzal/wavefront_obj quetzal_wavefront_obj)
add_subdirectory_ext(library/quetzal/xml quetzal_xml)

add_subdirectory(application/cubes)
add_subdirectory(application/gallery)
add_subdirectory(application/lightmatrix)
add_subdirectory(application/stratum_terminal)
add_subdirectory(application/toot1)

add_subdirectory_ext(model/acicula)
add_subdirectory_ext(model/cage)
add_subdirectory_ext(model/coils)
add_subdirectory_ext(model/helix_pyramid)
add_subdirectory_ext(model/laminaria)
add_subdirectory_ext(model/menger_sponge)
add_subdirectory_ext(model/mesh_operations)
add_subdirectory_ext(model/model_pattern)
add_subdirectory_ext(model/molds)
add_subdirectory_ext(model/primitives)
add_subdirectory_ext(model/spherical_harmonics)
add_subdirectory_ext(model/spikosahedron)
add_subdirectory_ext(model/sandbox)
add_subdirectory_ext(model/spinward)
add_subdirectory_ext(model/spoctahedron)
add_subdirectory_ext(model/trinket_box)
add_subdirectory_ext(model/wimble)

add_subdirectory(test/catagen_test)
add_subdirectory(test/dungen_test)
add_subdirectory(test/expression_test)
add_subdirectory(test/flecs_test)
add_subdirectory(test/geometry_test)
add_subdirectory(test/mapping_test)
add_subdirectory(test/mesh_diff_test)
add_subdirectory(test/projection_test)
add_subdirectory(test/stratum_format_test)
add_subdirectory(test/stratum_sim_test)
add_subdirectory(test/stratum_view_test)
add_subdirectory(test/string_util_test)
add_subdirectory(test/vector_point_test)
add_subdirectory(test/to_utf8_test)

add_subdirectory(tool/catagen)
add_subdirectory(tool/dungen)

add_subdirectory(utility/compare_precision_obj)
add_subdirectory(utility/dump_obj)
add_subdirectory(utility/mesh_diff)
add_subdirectory(utility/to_utf8)
add_subdirectory(utility/verify_model_output)

add_subdirectory(library3)

# After library3, unlike the other applications: configuring a GDExtension reads a property from
# the godot-cpp target, so that target has to exist by the time this directory is added
if(KML_DEV_GODOT)
    add_subdirectory(application/stratum_godot)
endif()

# Last, so that every add_subdirectory_ext above has registered by the time the targets are created
add_subdirectory(support)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT toot1)
