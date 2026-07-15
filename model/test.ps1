
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

function get_libraries {
    param (
        $name
    )

    $content = Get-Content "$name/CMakeLists.txt" -Raw
    $libraries = @()
    foreach ($call in [regex]::Matches($content, 'target_link_libraries\s*\([^)]*\)')) {
        $libraries += [regex]::Matches($call.Value, 'quetzal_[a-zA-Z_]+') | ForEach-Object { $_.Value }
    }
    return $libraries
}

function get_dependencies {
    param (
        $applications
    )

    $names = @()
    foreach ($application in $applications) {
        $names += get_libraries $application.Name
    }
    $names = $names | Select-Object -Unique

    return $names | ForEach-Object { @{ Name = $_ } }
}

function clean {
    param (
        $name
    )

    $script:cleaned_count++
    Write-Progress -Activity 'Cleaning' -Status $name -PercentComplete (100 * $script:cleaned_count / $script:clean_total)

    $output = cmake --build ../build --config $configuration --target "clean_$name" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Progress -Activity 'Cleaning' -Completed
        Write-Output $output
    }
}

function build {
    param (
        $name
    )

    Write-Output $name
    cmake --build ../build --config $configuration --target $name -- --nologo
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

$dependencies = get_dependencies $applications

$cleaned_count = 0
$clean_total = $dependencies.Count + $applications.Count

foreach ($dependency in $dependencies) {
    clean $dependency.Name
}

foreach ($application in $applications) {
    clean $application.Name
}

Write-Progress -Activity 'Cleaning' -Completed

build 'verify_model_output'

foreach ($application in $applications) {
    build $application.Name
}

foreach ($application in $applications) {
    if ($application.Verify) {
        verify $application.Name
    }
}
