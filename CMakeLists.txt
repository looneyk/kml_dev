cmake_minimum_required(VERSION 3.20)
project(kml_dev CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(DEV_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBRARY3_DIR ${DEV_ROOT}/library3)

set(CMAKE_MSVC_RUNTIME_LIBRARY "$<IF:$<CONFIG:Debug>,MultiThreadedDebug,MultiThreaded>")

find_package(Vulkan REQUIRED)

add_library(glfw3_mt STATIC IMPORTED GLOBAL)
set_target_properties(glfw3_mt PROPERTIES
    IMPORTED_LOCATION ${LIBRARY3_DIR}/glfw/bin/lib-vc2022/glfw3_mt.lib
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEV_ROOT}/out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${DEV_ROOT}/lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(library/quetzal/common)
add_subdirectory(library/quetzal/game)
add_subdirectory(library/quetzal/vulkan)
add_subdirectory(library/quetzal/scene_graph)

add_subdirectory(application/toot1)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT toot1)
