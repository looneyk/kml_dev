
$configuration = 'Debug'

# application list, call on each ...

function mesh_diff {
    $path = "../bin/$configuration/mesh_diff.exe"
    & $path $args[0] $args[1] $args[2]

    if ($LASTEXITCODE -ne 0) {
        echo "Command failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE 
    }
}

mesh_diff primitives reference out
exit 0

mesh_diff primitives reference out
mesh_diff helix_pyramid reference out
mesh_diff menger_sponge reference out
mesh_diff spikosahedron reference out
mesh_diff spinward reference out
mesh_diff spoctahedron reference out
mesh_diff trinket_box reference out
mesh_diff acicula reference out
mesh_diff cage reference out
mesh_diff coils reference out
mesh_diff laminaria reference out
mesh_diff molds reference out
mesh_diff wimble reference out
