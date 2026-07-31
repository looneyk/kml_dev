# Per-project clean targets, one clean_<name> per directory registered by add_subdirectory_ext.
# BuildProjectReferences=false requires dependencies to be cleaned separately, and explicitly for speed
# (MSBuild's Clean cascades into every library the project links, which can be redundant
# when cleaning many projects that share the same libraries.)
# Visual Studio generator only: `cmake --build --target clean` cleans the whole
# solution, and there is no generator-agnostic equivalent for a single project.
# Defined here rather than at the root so that the generated project, filters and scratch
# directory for each target land under this one build subdirectory

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    return()
endif()

get_property(clean_target_names GLOBAL PROPERTY KML_CLEAN_TARGET_NAMES)
get_property(clean_target_dirs GLOBAL PROPERTY KML_CLEAN_TARGET_DIRS)

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
