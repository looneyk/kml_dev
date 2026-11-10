# Reports the releases of each third-party repository in library3_repositories.psd1 that are newer than its pin, and with
# -Name moves one repository's pin: to a release tag, or for a repository that publishes no releases, to the head of its
# branch. The new pin is written into that file, which is in git, so the change shows in its diff, and checked out in
# the repository's clone.

param (
    # The repository whose pin to move
    [string]$Name,

    # The release tag to move it to; the newest release newer than its pin if not given
    [string]$Tag
)

$library3_directory = Join-Path $PSScriptRoot '../library3'
$repositories_filepath = Join-Path $PSScriptRoot 'library3_repositories.psd1'
$repositories = (Import-PowerShellDataFile $repositories_filepath).Repositories

# A tag whose name marks a release candidate, beta, alpha, or preview
$prerelease_pattern = '(?i)(rc|alpha|beta|pre|dev)'

# Returns the key a tag sorts by: its version, a release after the pre-releases of its version, and then the pre-release's number
function Get-TagKey([string]$tag) {
    $numbers = [regex]::Match($tag, '\d+(\.\d+){0,3}').Value
    $suffix = $tag.Substring($tag.IndexOf($numbers) + $numbers.Length)
    $prerelease_number = [regex]::Match($suffix, '\d+').Value
    [pscustomobject]@{
        Version = if ($numbers) { [version]($numbers -replace '^(\d+)$', '$1.0') } else { [version]'0.0' }
        Release = $tag -notmatch $prerelease_pattern
        PrereleaseNumber = if ($prerelease_number) { [int]$prerelease_number } else { 0 }
    }
}

# Returns true if the first tag sorts after the second
function Test-TagNewer([string]$tag, [string]$pin) {
    $key = Get-TagKey $tag
    $pin_key = Get-TagKey $pin
    if ($key.Version -ne $pin_key.Version) { return $key.Version -gt $pin_key.Version }
    if ($key.Release -ne $pin_key.Release) { return $key.Release }
    return $key.PrereleaseNumber -gt $pin_key.PrereleaseNumber
}

# Returns the tags of the repository newer than the pin, oldest first: its releases, and its pre-releases too if the pin is one
function Get-NewerTags($repository) {
    $pin_is_release = $repository.Tag -notmatch $prerelease_pattern
    git ls-remote --tags --refs $repository.Url | ForEach-Object { ($_ -split 'refs/tags/')[1] } |
        Where-Object { ($_ -notmatch $prerelease_pattern -or -not $pin_is_release) -and (Test-TagNewer $_ $repository.Tag) } |
        Sort-Object { (Get-TagKey $_).Version }, { (Get-TagKey $_).Release }, { (Get-TagKey $_).PrereleaseNumber }
}

# Returns the commit that a tag of the repository points to
function Get-TagCommit($repository, [string]$tag) {
    $lines = git ls-remote --tags $repository.Url "refs/tags/$tag" "refs/tags/$tag^{}"
    $peeled = $lines | Where-Object { $_ -match '\^\{\}$' }
    (($peeled ?? $lines) | Select-Object -First 1) -split '\s+' | Select-Object -First 1
}

function Write-Repositories {
    $lines = @('@{', '    Repositories = @(')
    foreach ($repository in $repositories) {
        $pin = if ($repository.Tag) { "Tag = '$($repository.Tag)'" } else { "Branch = '$($repository.Branch)'" }
        $lines += "        @{ Name = '$($repository.Name)'; Url = '$($repository.Url)'; $pin; Commit = '$($repository.Commit)' }"
    }
    $lines += @('    )', '}')
    [System.IO.File]::WriteAllText($repositories_filepath, ($lines -join "`n") + "`n")
}

if (-not $Name) {
    foreach ($repository in $repositories) {
        if ($repository.Tag) {
            $newer = @(Get-NewerTags $repository)
            $report = if ($newer.Count -gt 0) { "newer: $($newer -join ', ')" } else { 'no newer release' }
            Write-Output "$($repository.Name) at $($repository.Tag): $report"
        }
        else {
            $head = (git ls-remote $repository.Url "refs/heads/$($repository.Branch)") -split '\s+' | Select-Object -First 1
            $report = if ($head -eq $repository.Commit) { 'at the head' } else { "the head is $head" }
            Write-Output "$($repository.Name) on $($repository.Branch) at $($repository.Commit): $report"
        }
    }
    exit 0
}

$repository = $repositories | Where-Object { $_.Name -eq $Name }
if (-not $repository) {
    Write-Output "$Name is not in library3_repositories.psd1"
    exit 1
}

if ($repository.Tag -or $Tag) {
    $new_tag = if ($Tag) { $Tag } else { @(Get-NewerTags $repository) | Select-Object -Last 1 }
    if (-not $new_tag) {
        Write-Output "$Name`: no newer release than $($repository.Tag)"
        exit 0
    }
    $commit = Get-TagCommit $repository $new_tag
    if (-not $commit) {
        Write-Output "$Name`: $new_tag is not a tag of $($repository.Url)"
        exit 1
    }
    $repository.Remove('Branch')
    $repository.Tag = $new_tag
    $pin = $new_tag
}
else {
    $commit = (git ls-remote $repository.Url "refs/heads/$($repository.Branch)") -split '\s+' | Select-Object -First 1
    $pin = $commit
}

$repository.Commit = $commit
Write-Repositories

$directory = Join-Path $library3_directory $Name
if (Test-Path (Join-Path $directory '.git')) {
    git -C $directory fetch --quiet --tags
    git -C $directory checkout --quiet $pin
    if ($LASTEXITCODE -ne 0 -or (git -C $directory rev-parse HEAD) -ne $commit) {
        Write-Output "$Name`: pinned to $pin, but the clone could not be checked out at it"
        exit 1
    }
}

Write-Output "$Name`: pinned to $pin, commit $commit"
