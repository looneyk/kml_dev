
$configuration = 'Debug'

$applications = @(
    @{ Name = 'primitives';      Verify = $true }
    @{ Name = 'helix_pyramid';   Verify = $true }
    @{ Name = 'menger_sponge';   Verify = $true }
    @{ Name = 'spikosahedron';   Verify = $true }
    @{ Name = 'spinward';        Verify = $true }
    @{ Name = 'spoctahedron';    Verify = $true }
    @{ Name = 'trinket_box';     Verify = $true }
    @{ Name = 'acicula';         Verify = $true }
    @{ Name = 'cage';            Verify = $true }
    @{ Name = 'coils';           Verify = $true }
    @{ Name = 'laminaria';       Verify = $true }
    @{ Name = 'molds';           Verify = $true }
    @{ Name = 'wimble';          Verify = $true }
    @{ Name = 'mesh_operations'; Verify = $false }
)

function executable_path {
    param (
        $name
    )

    return "../../bin/$configuration/$name.exe"
}

function build {
    param (
        $name
    )

    Write-Output $name
    cmake --build ../build --config $configuration --target $name
}

function run {
    param (
        $name
    )

    Set-Location $name
    & (executable_path $name)
    Set-Location ..
}

function verify {
    param (
        $name
    )

    Set-Location $name
    & (executable_path 'verify_model_output') (executable_path $name) out reference $name
    Set-Location ..
}

cmake --build ../build --config $configuration --target clean

build 'verify_model_output'

foreach ($application in $applications) {
    build $application.Name
}

foreach ($application in $applications) {
    if ($application.Verify) {
        verify $application.Name
    }
}
