
param (
    [Parameter(Mandatory = $true)]
    [string]$Dir,
    [Parameter(Mandatory = $true)]
    [string]$Name
)

$configuration = 'Debug'
$platform = 'x64'

$vcxproj = "../build/$Dir/$Name/$Name.vcxproj"
cmd.exe /c "MSBuild $vcxproj /t:Clean /p:Configuration=$configuration /p:Platform=$platform"
