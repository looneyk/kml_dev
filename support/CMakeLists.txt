# Per-project clean targets, one clean_<name> per directory registered by add_subdirectory_ext
# BuildProjectReferences=false keeps a clean from cascading into every library the project links, so dependencies are
# cleaned separately
# Visual Studio generator only, as there is no generator-agnostic clean for a single project
# Defined here so the generated project, filters and scratch directory of each target are under this build
# subdirectory

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    return()
endif()

get_property(clean_target_names GLOBAL PROPERTY KML_DEV_CLEAN_TARGET_NAMES)
get_property(clean_target_dirs GLOBAL PROPERTY KML_DEV_CLEAN_TARGET_DIRS)

list(LENGTH clean_target_names clean_target_count)
if(clean_target_count EQUAL 0)
    return()
endif()

math(EXPR clean_target_last "${clean_target_count} - 1")
foreach(i RANGE ${clean_target_last})
    list(GET clean_target_names ${i} name)
    list(GET clean_target_dirs ${i} dir)

    add_custom_target(clean_${name}
        COMMAND MSBuild ${CMAKE_BINARY_DIR}/${dir}/${name}.vcxproj /t:Clean /p:Configuration=$<CONFIG> /p:Platform=${CMAKE_VS_PLATFORM_NAME} /p:BuildProjectReferences=false
    )
    set_property(TARGET clean_${name} PROPERTY FOLDER "CleanTargets")
endforeach()
