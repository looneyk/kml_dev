# Building a GDExtension: what Godot requires of the library it loads, for every host that builds one
#
# Include this from a host's CMakeLists, then call quetzal_configure_gdextension on its shared
# library target. The root CMakeLists sets the CRT that godot-cpp caches, since it applies to every target

# The Godot release the extension runs against. GDExtension compatibility is version specific, so
# the release is set here and not taken from the first Godot on PATH
set(KML_DEV_GODOT_VERSION "4.7.2-stable" CACHE STRING "Godot release the host is run against")

# The console build first, for its output when debugging. HINTS adds the conventional install
# location, for a PATH that does not include it
find_program(KML_DEV_GODOT_EXECUTABLE
    NAMES "Godot_v${KML_DEV_GODOT_VERSION}_win64_console" "Godot_v${KML_DEV_GODOT_VERSION}_win64"
    HINTS "$ENV{LOCALAPPDATA}/Programs/Godot"
    DOC "Godot executable that Visual Studio launches to debug a GDExtension"
)

# Names the target as Godot expects and writes it into the project's bin directory, which the
# .gdextension file refers to, then sets Visual Studio's debugger to launch Godot, since the target
# is a DLL
# project_dir is the directory holding project.godot, and any further arguments are the user arguments that follow -- on
# the command line that the debugger launches Godot with
function(quetzal_configure_gdextension target project_dir)

    get_target_property(suffix godot-cpp GODOTCPP_SUFFIX)

    set(output_dir "${project_dir}/bin/")

    set_target_properties(${target} PROPERTIES
        CXX_VISIBILITY_PRESET ${GODOTCPP_SYMBOL_VISIBILITY}
        POSITION_INDEPENDENT_CODE ON

        # Wrapped in a generator expression so that the MSVC generator does not append its own per
        # configuration subdirectory, which would put the library where Godot cannot find it
        LIBRARY_OUTPUT_DIRECTORY "$<1:${output_dir}>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${output_dir}>"
        PDB_OUTPUT_DIRECTORY "$<1:${output_dir}>"

        # Godot looks for lib<name>.<platform>.<target>.<arch>, which GODOTCPP_SUFFIX supplies the
        # tail of
        PREFIX "lib"
        OUTPUT_NAME "${target}${suffix}"
    )

    if(NOT MSVC)
        return()
    endif()

    if(NOT KML_DEV_GODOT_EXECUTABLE)
        message(STATUS "${target}: Godot ${KML_DEV_GODOT_VERSION} not found, so F5 will not launch it")
        return()
    endif()

    set(debugger_arguments "--path \"${project_dir}\"")
    if(ARGN)
        list(JOIN ARGN " " user_arguments)
        string(APPEND debugger_arguments " -- ${user_arguments}")
    endif()

    # Runs the project's main scene, so F5 starts the game with breakpoints active in the extension
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_COMMAND "${KML_DEV_GODOT_EXECUTABLE}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "${debugger_arguments}"
        VS_DEBUGGER_WORKING_DIRECTORY "${project_dir}"
    )

endfunction()
