cmake_minimum_required(VERSION 3.20)
project(kml_dev C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DEV_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARY3_DIR ${DEV_ROOT}/library3)

set(CMAKE_MSVC_RUNTIME_LIBRARY "$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreaded>")

find_package(Vulkan REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEV_ROOT}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEV_ROOT}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# add_subdirectory_ext: drop-in replacement for add_subdirectory that also defines a
# clean_<name> target running MSBuild's per-project Clean on the generated vcxproj.
# Visual Studio generator only: `cmake --build --target clean` cleans the whole
# solution, and there is no generator-agnostic equivalent for a single project.
function(add_subdirectory_ext dir)
    add_subdirectory(${dir})
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        get_filename_component(name ${dir} NAME)
        add_custom_target(clean_${name}
            COMMAND MSBuild ${CMAKE_BINARY_DIR}/${dir}/${name}.vcxproj /t:Clean /p:Configuration=$<CONFIG> /p:Platform=${CMAKE_VS_PLATFORM_NAME}
        )
    endif()
endfunction()

add_subdirectory(library/quetzal/brep)
add_subdirectory(library/quetzal/common)
add_subdirectory(library/quetzal/expression)
add_subdirectory(library/quetzal/game)
add_subdirectory(library/quetzal/geometry)
add_subdirectory(library/quetzal/math)
add_subdirectory(library/quetzal/model)
add_subdirectory(library/quetzal/pdf)
add_subdirectory(library/quetzal/scene_graph)
add_subdirectory(library/quetzal/spatial_search)
add_subdirectory(library/quetzal/stl)
add_subdirectory(library/quetzal/svg)
add_subdirectory(library/quetzal/triangulation)
add_subdirectory(library/quetzal/vulkan)
add_subdirectory(library/quetzal/wavefront_obj)
add_subdirectory(library/quetzal/xml)

add_subdirectory(library3)

add_subdirectory(application/cubes)
add_subdirectory(application/gallery)
add_subdirectory(application/lightmatrix)
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

add_subdirectory(test/expression_test)
add_subdirectory(test/to_utf8_test)

add_subdirectory(utility/compare_precision_obj)
add_subdirectory(utility/dump_obj)
add_subdirectory(utility/mesh_diff)
add_subdirectory(utility/to_utf8)
add_subdirectory(utility/verify_model_output)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT toot1)
