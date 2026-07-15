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

get_libraries() {
    local name="$1"

    perl -0777 -ne 'while (/target_link_libraries\s*\([^)]*\)/gs) { my $block = $&; while ($block =~ /quetzal_[a-zA-Z_]+/g) { print "$&\n" } }' "$name/CMakeLists.txt"
}

get_dependencies() {
    local names=()
    for entry in "${applications[@]}"; do
        IFS=':' read -r name _ <<< "$entry"
        while IFS= read -r lib; do
            names+=("$lib")
        done < <(get_libraries "$name")
    done
    printf '%s\n' "${names[@]}" | sort -u
}

print_clean_progress() {
    local name="$1"
    local width=20
    local filled=$((width * clean_index / clean_total))
    local empty=$((width - filled))
    local bar
    bar=$(printf '%*s' "$filled" '' | tr ' ' '#')
    bar+=$(printf '%*s' "$empty" '' | tr ' ' '-')
    printf '\rCleaning [%s] %3d%% %-30s' "$bar" "$((100 * clean_index / clean_total))" "$name"
}

clear_progress() {
    printf '\r%*s\r' 60 ''
}

clean() {
    local name="$1"

    clean_index=$((clean_index + 1))
    print_clean_progress "$name"

    local output
    if ! output=$(cmake --build ../build --config "$configuration" --target "clean_$name" 2>&1); then
        clear_progress
        echo "$output"
    fi
}

build() {
    local application="$1"

    echo "$application"
    cmake --build ../build --config "$configuration" --target "$application" -- --nologo
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

dependencies=()
while IFS= read -r name; do
    dependencies+=("$name")
done < <(get_dependencies)

clean_index=0
clean_total=$(( ${#dependencies[@]} + ${#applications[@]} ))

for name in "${dependencies[@]}"; do
    clean "$name"
done

for entry in "${applications[@]}"; do
    IFS=':' read -r name _ <<< "$entry"
    clean "$name"
done

clear_progress

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
