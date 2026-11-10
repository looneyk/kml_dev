set(CMAKE_FOLDER "library3")

# The root sets /W4 for quetzal's own code. Third party sources are not ours to fix, and a later /W on the command line wins
if(MSVC)
    add_compile_options(/W1)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(KML_DEV_VULKAN)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(glfw)
endif()

add_subdirectory(simdjson)

# JSON_SystemInclude marks the headers SYSTEM, which is the same treatment flecs gets below and for
# the same reason: they compile as part of whatever includes them and would report at /W4
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
set(JSON_SystemInclude ON CACHE BOOL "" FORCE)
add_subdirectory(json)

set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_SHARED OFF CACHE BOOL "" FORCE) # Defaults ON, independently of BUILD_SHARED_LIBS
set(FLECS_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(flecs)

# flecs headers are heavily inlined and templated, so they compile as part of whatever includes them and warn at that file's level.
# Marking them SYSTEM drops those to /external:W1, which is what the compiler already applies to the rest of library3.
# The cost is that a warning provoked by our own use but reported inside a flecs header is silenced with them; the alternative is a pragma at every include site
get_target_property(flecs_include_directories flecs_static INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(flecs_static PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${flecs_include_directories}")

add_subdirectory(CDT/CDT)

# Same treatment as flecs above: CDT is header-only, so its headers compile as part of quetzal_triangulation at /W4
get_target_property(cdt_include_directories CDT INTERFACE_INCLUDE_DIRECTORIES)
set_target_properties(CDT PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${cdt_include_directories}")

# Lua is released with a makefile and no CMake build. lua.c and luac.c are the standalone interpreter and compiler, each with a main
file(GLOB lua_sources ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/*.c)
list(REMOVE_ITEM lua_sources ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lua.c ${CMAKE_CURRENT_SOURCE_DIR}/lua/src/luac.c)
# Compiled as C++, so that a Lua error is a C++ exception, which runs the destructors of the C++ frames it unwinds where longjmp
# would skip them. A client includes lua.h, lualib.h, and lauxlib.h, not lua.hpp, whose extern "C" is for Lua compiled as C
set_source_files_properties(${lua_sources} PROPERTIES LANGUAGE CXX)
add_library(lua_static STATIC ${lua_sources})
target_include_directories(lua_static SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lua/src)

set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(doctest)

set(UTF8CPP_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(UTF8CPP_ENABLE_BENCHMARKS OFF CACHE BOOL "" FORCE)
add_subdirectory(utfcpp)

if(KML_DEV_VULKAN)
    set(FASTGLTF_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
    set(FASTGLTF_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(fastgltf)

    # Same treatment as flecs above: fastgltf's containers are templates in its headers, which compile at /W4 in quetzal_scene_graph
    get_target_property(fastgltf_include_directories fastgltf INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(fastgltf PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${fastgltf_include_directories}")

    # basis_universal's SSE kernel does not compile with this MSVC: cppspmd_sse.h returns a temporary as a non-const reference. KTX only enables it on x86_64, so it is off here
    set(BASISU_SSE FALSE)

    set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
    set(KTX_FEATURE_LOADTEST_APPS "OFF" CACHE STRING "" FORCE)
    add_subdirectory(KTX-Software)
endif()

if(KML_DEV_GODOT)
    set(GODOTCPP_API_VERSION "4.7" CACHE STRING "Godot API version the bindings target")
    # The extension catches the exceptions the quetzal libraries throw, and with exceptions disabled MSVC's std::exception
    # has another layout, which keeps a pointer to the message rather than a copy
    set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
    add_subdirectory(godot-cpp)

    # Same treatment as flecs above and for the same reason: these headers compile as part of
    # whatever includes them, so at /W4 they report against the extension's own sources
    get_target_property(godot_cpp_include_directories godot-cpp INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(godot-cpp PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${godot_cpp_include_directories}")

    set_property(TARGET godot-cpp PROPERTY FOLDER "library3")
endif()

set_property(TARGET flecs_static PROPERTY FOLDER "library3")
set_property(TARGET lua_static PROPERTY FOLDER "library3")
set_property(TARGET simdjson PROPERTY FOLDER "library3")
set_property(TARGET utf8cpp PROPERTY FOLDER "library3")

if(KML_DEV_VULKAN)
    set_property(TARGET glfw PROPERTY FOLDER "library3")
    set_property(TARGET update_mappings PROPERTY FOLDER "library3")
    set_property(TARGET fastgltf PROPERTY FOLDER "library3")
    set_property(TARGET ktx PROPERTY FOLDER "library3")

    add_library(glfw3_mt ALIAS glfw)
endif()

unset(CMAKE_FOLDER)
