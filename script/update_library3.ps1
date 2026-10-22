# Pulls each third-party repository in library3 that library3_repositories.psd1 lists, fast-forward only, and writes
# each one's new commit into that file, so clone_library3.ps1 clones what the tree is then built against. The file is in
# git, so the update shows in its diff.

param (
    # Fetches and reports how far behind each repository is, without pulling or writing anything
    [switch]$DryRun
)

$library3_directory = Join-Path $PSScriptRoot '../library3'
$repositories_filepath = Join-Path $PSScriptRoot 'library3_repositories.psd1'
$repositories = (Import-PowerShellDataFile $repositories_filepath).Repositories

$failure_count = 0

foreach ($repository in $repositories) {
    $name = $repository.Name
    $directory = Join-Path $library3_directory $name

    if (-not (Test-Path (Join-Path $directory '.git'))) {
        Write-Output "$name`: not cloned; clone_library3.ps1 clones it"
        $failure_count++
        continue
    }

    if ($DryRun) {
        git -C $directory fetch --quiet
        $behind_count = git -C $directory rev-list --count 'HEAD..@{upstream}'
        if ($LASTEXITCODE -ne 0) {
            Write-Output "$name`: failed"
            $failure_count++
            continue
        }
        Write-Output "$name`: $behind_count commits behind"
        continue
    }

    $commit_before = git -C $directory rev-parse HEAD
    git -C $directory pull --ff-only --quiet
    if ($LASTEXITCODE -ne 0) {
        Write-Output "$name`: failed"
        $failure_count++
        continue
    }

    $commit = git -C $directory rev-parse HEAD
    $repository.Commit = $commit
    if ($commit -eq $commit_before) {
        Write-Output "$name`: current"
    }
    else {
        Write-Output "$name`: updated from $commit_before to $commit"
    }
}

if (-not $DryRun) {
    $lines = @('@{', '    Repositories = @(')
    foreach ($repository in $repositories) {
        $lines += "        @{ Name = '$($repository.Name)'; Url = '$($repository.Url)'; Branch = '$($repository.Branch)'; Commit = '$($repository.Commit)' }"
    }
    $lines += @('    )', '}')
    [System.IO.File]::WriteAllText($repositories_filepath, ($lines -join "`n") + "`n")
}

if ($failure_count -gt 0) {
    Write-Output "$failure_count of $($repositories.Count) failed"
    exit 1
}
