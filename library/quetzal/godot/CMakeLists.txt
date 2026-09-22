# A custom target, as for the other header only quetzal libraries, puts the sources in the solution with nothing to link
add_custom_target(quetzal_godot SOURCES
    gdextension.cmake
)

set_property(TARGET quetzal_godot PROPERTY FOLDER "library")
