# Header only so far, so a custom target as with the other header only quetzal libraries: it puts
# the sources in the solution without producing anything to link
add_custom_target(quetzal_godot SOURCES
    gdextension.cmake
)

set_property(TARGET quetzal_godot PROPERTY FOLDER "library")
