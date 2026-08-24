
# Compares the _ab and _ba output of mesh_operations. The two differ only in the order the submeshes are
# given, so every pair should describe the same solid. Run from the model directory.
#
# mesh_diff reports Different for a pair whose faces all match but whose surface names do not. union and
# intersection name the result submesh after the operation, so their surface names never match across a
# pair. Those are counted separately here, and only face differences set the exit code.

param (
    # Optional, limits the comparison to models whose name contains this string.
    [Parameter(Position = 0)]
    [string]$ModelName,

    # Only operations that give the same result whichever submesh comes first belong here. difference
    # does not, and the _clipped and _separated variations are excluded by the name match below.
    [string[]]$Operations = @('identity', 'union', 'intersection', 'symmetric_difference'),

    [string]$Configuration = 'Debug'
)

$dir_out = 'mesh_operations/out'
$path_mesh_diff = "../bin/$Configuration/mesh_diff.exe"

if (-not (Test-Path $path_mesh_diff)) {
    Write-Output "mesh_diff not found at $path_mesh_diff"
    exit 1
}

$dir_temp = Join-Path ([System.IO.Path]::GetTempPath()) 'mesh_diff_ab_ba'
$dir_ab = "$dir_temp/ab"
$dir_ba = "$dir_temp/ba"

Remove-Item $dir_temp -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $dir_ab, $dir_ba | Out-Null

# mesh_diff pairs files by name across the two directories, so each pair is copied in under a common name
$pair_count = 0
foreach ($operation in $Operations) {
    foreach ($file in Get-ChildItem "$dir_out/*_${operation}_ab.obj") {
        $model = $file.Name -replace ('_' + $operation + '_ab\.obj$'), ''
        if ($ModelName -and $model -notlike "*$ModelName*") {
            continue
        }

        $path_ba = "$dir_out/${model}_${operation}_ba.obj"
        if (-not (Test-Path $path_ba)) {
            continue
        }

        Copy-Item $file.FullName "$dir_ab/${model}_${operation}.obj"
        Copy-Item $path_ba "$dir_ba/${model}_${operation}.obj"
        ++$pair_count
    }
}

if ($pair_count -eq 0) {
    Write-Output 'No _ab/_ba pairs found'
    exit 1
}

$lines = & $path_mesh_diff $dir_ab $dir_ba

$identical = @()
$equivalent = @()
$naming = @()
$differing = @()

foreach ($line in $lines) {
    if ($line -notmatch '^(\S+\.obj)\t(Identical|Equivalent|Different): (.*)$') {
        continue
    }

    $name = $matches[1]
    $result = $matches[2]
    $detail = $matches[3]

    if ($result -eq 'Identical') {
        $identical += $name
    }
    elseif ($result -eq 'Equivalent') {
        $equivalent += "$name  $detail"
    }
    elseif ($detail -match '0 reversed, 0 different, 0 unmatched, 0 extra') {
        $naming += $name
    }
    else {
        $differing += "$name  $detail"
    }
}

Write-Output "$pair_count _ab/_ba pairs compared"
Write-Output "  identical             $($identical.Count)"
Write-Output "  equivalent            $($equivalent.Count)"
Write-Output "  surface names only    $($naming.Count)"
Write-Output "  face differences      $($differing.Count)"

if ($equivalent.Count -gt 0) {
    Write-Output ''
    Write-Output 'Equivalent, faces matched but differing in triangulation or vertex order:'
    foreach ($entry in $equivalent) { Write-Output "  $entry" }
}

if ($differing.Count -gt 0) {
    Write-Output ''
    Write-Output 'Face differences:'
    foreach ($entry in $differing) { Write-Output "  $entry" }
}

Remove-Item $dir_temp -Recurse -Force -ErrorAction SilentlyContinue

if ($differing.Count -gt 0) {
    exit 1
}

exit 0
