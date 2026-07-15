
param (
    [Parameter(Mandatory = $true)]
    [string]$Dir,
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [string]$Configuration = 'Debug',
    [string]$BuildRoot = '../build'
)

$platform = 'x64'

$vcxproj = "$BuildRoot/$Dir/$Name/$Name.vcxproj"
cmd.exe /c "MSBuild $vcxproj /t:Clean /p:Configuration=$Configuration /p:Platform=$platform"
