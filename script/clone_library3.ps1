# Clones the third-party repositories that the CMake build uses into library3, each at its pin in library3_repositories.psd1,
# as on a new machine: a release tag, or a commit on its branch for a repository that publishes no releases. A repository
# already cloned is left as it is, and reported if it is not at its pin, unless -Checkout is given.

param (
    # Lists what would be cloned or checked out without doing it
    [switch]$DryRun,

    # Checks out its pin in a repository already cloned that is not at it
    [switch]$Checkout
)

$library3_directory = Join-Path $PSScriptRoot '../library3'
$repositories = (Import-PowerShellDataFile (Join-Path $PSScriptRoot 'library3_repositories.psd1')).Repositories

$failure_count = 0

foreach ($repository in $repositories) {
    $name = $repository.Name
    $directory = Join-Path $library3_directory $name
    $pin = if ($repository.Tag) { $repository.Tag } else { $repository.Commit }

    if (Test-Path (Join-Path $directory '.git')) {
        $commit = git -C $directory rev-parse HEAD
        if ($commit -eq $repository.Commit) {
            Write-Output "$name`: cloned at $pin"
            continue
        }

        if (-not $Checkout) {
            Write-Output "$name`: cloned at $commit, the tree is built against $pin"
            continue
        }

        if ($DryRun) {
            Write-Output "$name`: would check out $pin"
            continue
        }

        git -C $directory fetch --quiet --tags
        if ($LASTEXITCODE -eq 0) {
            git -C $directory checkout --quiet $pin
        }
    }
    elseif (Test-Path $directory) {
        Write-Output "$name`: $directory exists and is not a clone"
        $failure_count++
        continue
    }
    elseif ($DryRun) {
        Write-Output "$name`: would clone $($repository.Url) at $pin"
        continue
    }
    elseif ($repository.Tag) {
        git clone --quiet --branch $repository.Tag $repository.Url $directory
    }
    else {
        # Cloning the branch and then resetting it keeps the branch tracking its remote, so a later pull updates it
        git clone --quiet --branch $repository.Branch $repository.Url $directory
        if ($LASTEXITCODE -eq 0) {
            git -C $directory reset --quiet --hard $repository.Commit
        }
    }

    if ($LASTEXITCODE -ne 0 -or (git -C $directory rev-parse HEAD) -ne $repository.Commit) {
        Write-Output "$name`: failed, or $pin is not commit $($repository.Commit)"
        $failure_count++
        continue
    }

    Write-Output "$name`: at $pin"
}

if ($failure_count -gt 0) {
    Write-Output "$failure_count of $($repositories.Count) failed"
    exit 1
}
