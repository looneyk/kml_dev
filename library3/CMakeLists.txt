set(CMAKE_FOLDER "library3")

# The root sets /W4 for quetzal's own code. Third party sources are not ours to fix, and a later /W on the command line wins
if(MSVC)
    add_compile_options(/W1)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

add_subdirectory(simdjson)

set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_SHARED OFF CACHE BOOL "" FORCE) # Defaults ON, independently of BUILD_SHARED_LIBS
set(FLECS_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(flecs)

# flecs headers are heavily inlined and templated, so they compile as part of whatever includes them and warn at that file's level.
# Marking them SYSTEM drops those to /external:W1, which is what the compiler already applies to the rest of library3.
# The cost is that a warning provoked by our own use but reported inside a flecs header is silenced with them; the alternative is a pragma at every include site
get_target_property(flecs_include_directories flecs_static INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(flecs_static PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${flecs_include_directories}")

set(UTF8CPP_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(UTF8CPP_ENABLE_BENCHMARKS OFF CACHE BOOL "" FORCE)
add_subdirectory(utfcpp)

set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
set(FASTGLTF_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(fastgltf)

# basis_universal's SSE kernel does not compile with this MSVC: cppspmd_sse.h returns a temporary as a non-const reference. KTX only enables it on x86_64, so it is off here
set(BASISU_SSE FALSE)

set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS "OFF" CACHE STRING "" FORCE)
add_subdirectory(KTX-Software)

if(KML_DEV_GODOT)
    set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API version the bindings target")
    add_subdirectory(godot-cpp)

    # Same treatment as flecs above and for the same reason: these headers compile as part of
    # whatever includes them, so at /W4 they report against the extension's own sources
    get_target_property(godot_cpp_include_directories godot-cpp INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(godot-cpp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${godot_cpp_include_directories}")

    set_property(TARGET godot-cpp PROPERTY FOLDER "library3")
endif()

set_property(TARGET flecs_static PROPERTY FOLDER "library3")
set_property(TARGET glfw PROPERTY FOLDER "library3")
set_property(TARGET update_mappings PROPERTY FOLDER "library3")
set_property(TARGET simdjson PROPERTY FOLDER "library3")
set_property(TARGET fastgltf PROPERTY FOLDER "library3")
set_property(TARGET ktx PROPERTY FOLDER "library3")
set_property(TARGET utf8cpp PROPERTY FOLDER "library3")

add_library(glfw3_mt ALIAS glfw)

unset(CMAKE_FOLDER)
