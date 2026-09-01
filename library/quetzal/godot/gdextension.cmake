# Building a GDExtension: what Godot requires of the library it loads, which is enough of it to be
# worth keeping in one place rather than in each host that builds one
#
# Include this from a host's CMakeLists, then call quetzal_configure_gdextension on its shared
# library target. The CRT match godot-cpp would otherwise cache over is handled by the root
# CMakeLists, since it applies to every target rather than only to these

# Which Godot the extension is run against, stated rather than discovered: GDExtension
# compatibility is version specific, and picking up whichever Godot happened to be first on PATH
# could launch a release the bindings were not generated for
set(KML_DEV_GODOT_VERSION "4.7.2-stable" CACHE STRING "Godot release the host is run against")

# The console build first, its output being the point when debugging. HINTS covers the
# conventional install location for a PATH that has not been set or a shell that predates it
find_program(KML_DEV_GODOT_EXECUTABLE
    NAMES "Godot_v${KML_DEV_GODOT_VERSION}_win64_console" "Godot_v${KML_DEV_GODOT_VERSION}_win64"
    HINTS "$ENV{LOCALAPPDATA}/Programs/Godot"
    DOC "Godot executable that Visual Studio launches to debug a GDExtension"
)

# Names the target as Godot expects and writes it into the project's bin directory, where the
# .gdextension file refers to it from, then points Visual Studio's debugger at Godot so that F5
# on a target that is only ever a DLL launches something
# project_dir is the directory holding project.godot
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

    # Runs the project's main scene, so F5 gives a running game with breakpoints live in the
    # extension it loaded
    set_target_properties(${target} PROPERTIES
        VS_DEBUGGER_COMMAND "${KML_DEV_GODOT_EXECUTABLE}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "--path \"${project_dir}\""
        VS_DEBUGGER_WORKING_DIRECTORY "${project_dir}"
    )

endfunction()
