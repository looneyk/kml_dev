
param (
    # Optional, limits the run to a single model application. Named ApplicationName instead
    # of just Application to prevent collision with the $application loop variable below.
    [Parameter(Position = 0)]
    [string]$ApplicationName,

    # Skip the clean pass, allowing for an incremental build.
    [switch]$NoClean
)

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

if ($ApplicationName) {
    $selected = @($applications | Where-Object { $_.Name -eq $ApplicationName })
    if ($selected.Count -eq 0) {
        Write-Output "Unknown application '$ApplicationName'"
        Write-Output "Available: $(($applications | ForEach-Object { $_.Name }) -join ', ')"
        exit 1
    }

    $applications = $selected
}

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

if (-not $NoClean) {
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
}

build 'verify_model_output'

foreach ($application in $applications) {
    build $application.Name
}

# verify_model_output exits with the FileValidator result value, ordered least to most severe, so the
# status is both the severity and the index of the color that result was reported with
$status_different = 3
$pass_colors = @(34, 76, 148)

$passed = 0
$failed = 0
$worst = 0

foreach ($application in $applications) {
    if ($application.Verify) {
        verify $application.Name

        if ($LASTEXITCODE -ge $status_different) {
            ++$failed
        }
        else {
            ++$passed
            if ($LASTEXITCODE -gt $worst) {
                $worst = $LASTEXITCODE
            }
        }
    }
}

$total = $passed + $failed
if ($passed -gt 0) {
    Write-Output "`e[38;5;$($pass_colors[$worst])m$passed of $total Passed`e[0m"
}

if ($failed -gt 0) {
    Write-Output "`e[38;5;196m$failed of $total Failed`e[0m"
}

# Otherwise the status of whichever application happened to run last would be the script's
exit ($failed -gt 0 ? 1 : 0)
