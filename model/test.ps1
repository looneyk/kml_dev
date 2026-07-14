
$configuration = 'Debug'

# application list, loop through in build and run ...

function executable_path {
    param (
        $application
    )

    return "../../bin/$configuration/$application.exe"
}

function build {
    param (
        $application
    )

    Write-Output $application

    $application_exe = executable_path $application
    if (Test-Path $application_exe) {
        Remove-Item $application_exe
    }

    cmake --build ../build --config $configuration --target $application
}

function run {
    param (
        $application
    )

    Set-Location $application
    & (executable_path $application)
    Set-Location ..
}

function verify {
    param (
        $application
    )

    Set-Location $application
    & (executable_path "verify_model_output") (executable_path $application) out reference $application
    Set-Location ..
}

build -application verify_model_output
build -application primitives
build -application helix_pyramid
build -application menger_sponge
build -application spikosahedron
build -application spinward
build -application spoctahedron
build -application trinket_box
build -application acicula
build -application cage
build -application coils
build -application laminaria
build -application molds
build -application wimble
build -application mesh_operations

verify -application primitives
verify -application helix_pyramid
verify -application menger_sponge
verify -application spikosahedron
verify -application spinward
verify -application spoctahedron
verify -application trinket_box
verify -application acicula
verify -application cage
verify -application coils
verify -application molds
verify -application laminaria
verify -application wimble
#run -application mesh_operations
