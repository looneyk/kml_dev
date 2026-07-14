#!/bin/bash

configuration=Debug

# application list, loop through in build and run ...

executable_path() {
    local application="$1"

    echo "../../bin/$configuration/$application.exe"
}

build() {
    local application="$1"

    echo "$application"

    local application_exe=$(executable_path "$application")
    if [ -f "$application_exe" ]; then
        rm "$application_exe"
    fi

    cmake --build ../build --config "$configuration" --target "$application"
}

run() {
    local application="$1"

    cd "$application"
    "$(executable_path "$application")"
    cd ..
}

verify() {
    local application="$1"

    cd "$application"
    "$(executable_path verify_model_output)" "$(executable_path "$application")" out reference "$application"
    cd ..
}

build verify_model_output
build primitives
build helix_pyramid
build menger_sponge
build spikosahedron
build spinward
build spoctahedron
build trinket_box
build acicula
build cage
build coils
build laminaria
build molds
build wimble
build mesh_operations

verify primitives
verify helix_pyramid
verify menger_sponge
verify spikosahedron
verify spinward
verify spoctahedron
verify trinket_box
verify acicula
verify cage
verify coils
verify molds
verify laminaria
verify wimble
#run mesh_operations
