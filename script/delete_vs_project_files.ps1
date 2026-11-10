# Deletes the Visual Studio solution and project files, and the .vs directory, of each application, model, test, tool and
# utility that the root CMakeLists.txt adds. Library project files are kept, as standalone solutions outside the CMake
# build still reference them.

param (
    # Lists what would be deleted without deleting it
    [switch]$DryRun
)

$root_directory = Join-Path $PSScriptRoot '..'
$cmake_lists = Get-Content (Join-Path $root_directory 'CMakeLists.txt') -Raw

$directories = [regex]::Matches($cmake_lists, '(?m)^\s*add_subdirectory(?:_ext)?\(\s*([^\s)]+)') |
    ForEach-Object { $_.Groups[1].Value } |
    Where-Object { $_ -match '^(application|model|test|tool|utility)/' } |
    Select-Object -Unique

$patterns = @('*.sln', '*.slnx', '*.vcxproj', '*.vcxproj.filters', '*.vcxproj.user')

$deleted_count = 0

foreach ($directory in $directories) {
    $path = Join-Path $root_directory $directory
    $targets = @(Get-ChildItem -Path (Join-Path $path '*') -File -Include $patterns)
    $ide_directory = Join-Path $path '.vs'
    if (Test-Path $ide_directory) {
        $targets += Get-Item $ide_directory -Force
    }

    foreach ($target in $targets) {
        $relative_path = "$directory/$($target.Name)"
        if ($DryRun) {
            Write-Output "would delete $relative_path"
        }
        else {
            Remove-Item $target.FullName -Recurse -Force
            Write-Output "deleted $relative_path"
        }
        $deleted_count++
    }
}

Write-Output "$deleted_count files and directories in $($directories.Count) project directories"
