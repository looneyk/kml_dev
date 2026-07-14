#!/bin/bash

configuration=Debug

applications=(
    "primitives:true"
    "helix_pyramid:true"
    "menger_sponge:true"
    "spikosahedron:true"
    "spinward:true"
    "spoctahedron:true"
    "trinket_box:true"
    "acicula:true"
    "cage:true"
    "coils:true"
    "laminaria:true"
    "molds:true"
    "wimble:true"
    "mesh_operations:false"
)

executable_path() {
    local application="$1"

    echo "../../bin/$configuration/$application.exe"
}

build() {
    local application="$1"

    echo "$application"
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

cmake --build ../build --config "$configuration" --target clean

build verify_model_output

for entry in "${applications[@]}"; do
    IFS=':' read -r name _ <<< "$entry"
    build "$name"
done

for entry in "${applications[@]}"; do
    IFS=':' read -r name should_verify <<< "$entry"
    if [ "$should_verify" = "true" ]; then
        verify "$name"
    fi
done
