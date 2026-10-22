# Clones the third-party repositories that the CMake build uses into library3, each at its commit in
# library3_repositories.psd1, as on a new machine. A repository already cloned is left as it is, and reported if its
# commit differs.

param (
    # Lists what would be cloned without cloning it
    [switch]$DryRun
)

$library3_directory = Join-Path $PSScriptRoot '../library3'
$repositories = (Import-PowerShellDataFile (Join-Path $PSScriptRoot 'library3_repositories.psd1')).Repositories

$failure_count = 0

foreach ($repository in $repositories) {
    $name = $repository.Name
    $directory = Join-Path $library3_directory $name

    if (Test-Path (Join-Path $directory '.git')) {
        $commit = git -C $directory rev-parse HEAD
        if ($commit -eq $repository.Commit) {
            Write-Output "$name`: cloned at its commit"
        }
        else {
            Write-Output "$name`: cloned at $commit, the tree is built against $($repository.Commit)"
        }
        continue
    }

    if (Test-Path $directory) {
        Write-Output "$name`: $directory exists and is not a clone"
        $failure_count++
        continue
    }

    if ($DryRun) {
        Write-Output "$name`: would clone $($repository.Url) at $($repository.Commit)"
        continue
    }

    # Cloning the branch and then resetting it keeps the branch tracking its remote, so a later pull updates it
    git clone --branch $repository.Branch $repository.Url $directory
    if ($LASTEXITCODE -eq 0) {
        git -C $directory reset --hard $repository.Commit
    }

    if ($LASTEXITCODE -ne 0) {
        Write-Output "$name`: failed"
        $failure_count++
        continue
    }

    Write-Output "$name`: cloned"
}

if ($failure_count -gt 0) {
    Write-Output "$failure_count of $($repositories.Count) failed"
    exit 1
}
