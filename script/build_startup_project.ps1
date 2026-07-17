param(
    [Parameter(Mandatory = $true)]
    [string]$SolutionPath
)

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

public static class VsRotHelper
{
    [DllImport("ole32.dll")]
    private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable prot);

    [DllImport("ole32.dll")]
    private static extern int CreateBindCtx(int reserved, out IBindCtx ppbc);

    // All ROT/moniker interfaces here are vtable-only (non-IDispatch); a real compiled cast is
    // required to invoke them at all, which is why this lives in C# rather than PowerShell.
    public static object[] GetRunningDteInstances()
    {
        IRunningObjectTable rot;
        GetRunningObjectTable(0, out rot);
        IBindCtx bindCtx;
        CreateBindCtx(0, out bindCtx);

        IEnumMoniker enumMoniker;
        rot.EnumRunning(out enumMoniker);
        enumMoniker.Reset();

        IMoniker[] monikers = new IMoniker[1];
        List<object> results = new List<object>();

        while (enumMoniker.Next(1, monikers, IntPtr.Zero) == 0)
        {
            string displayName = null;
            try
            {
                monikers[0].GetDisplayName(bindCtx, null, out displayName);
            }
            catch
            {
                continue;
            }

            if (displayName != null && displayName.StartsWith("!VisualStudio.DTE"))
            {
                try
                {
                    object obj;
                    rot.GetObject(monikers[0], out obj);
                    results.Add(obj);
                }
                catch
                {
                }
            }
        }

        return results.ToArray();
    }
}
'@

function Get-RunningDte {
    param([string]$TargetSolutionPath)

    foreach ($dte in [VsRotHelper]::GetRunningDteInstances()) {
        try {
            if ($dte.Solution.FullName -and ($dte.Solution.FullName -ieq $TargetSolutionPath)) {
                return $dte
            }
        } catch { }
    }
    return $null
}

function Get-AllProjects {
    param($projects)
    $result = @()
    foreach ($project in $projects) {
        if ($null -eq $project) { continue }
        # Solution folders show up as projects of this kind; recurse into their nested sub-projects.
        # Kind's braces/casing aren't guaranteed, so compare on the bare hex GUID.
        $isSolutionFolder = $project.Kind -and ($project.Kind.Trim('{}') -ieq '66A26720-8FB5-11D2-AA7E-00C04F688DDE')
        if ($isSolutionFolder) {
            try {
                foreach ($item in $project.ProjectItems) {
                    if ($item.SubProject) {
                        $result += Get-AllProjects @($item.SubProject)
                    }
                }
            } catch { }
        }
        else {
            $result += $project
        }
    }
    return $result
}

$dte = Get-RunningDte -TargetSolutionPath $SolutionPath
if (-not $dte) {
    Write-Error "Could not find a running Visual Studio instance with solution '$SolutionPath' open."
    exit 1
}

$dte.Documents.SaveAll()

$allProjects = Get-AllProjects $dte.Solution.Projects
$startupUniqueNames = @($dte.Solution.SolutionBuild.StartupProjects)
$activeConfig = $dte.Solution.SolutionBuild.ActiveConfiguration

if ($startupUniqueNames.Count -eq 0) {
    Write-Error "No startup project is set for solution '$SolutionPath'."
    exit 1
}

$exitCode = 0

foreach ($uniqueName in $startupUniqueNames) {
    # UniqueName separators can come back as / or \ depending on how the solution was generated (e.g. CMake);
    # normalize both sides so a slash-direction difference alone doesn't break the match.
    $normalizedUniqueName = $uniqueName -replace '\\', '/'
    $project = $allProjects | Where-Object { ($_.UniqueName -replace '\\', '/') -eq $normalizedUniqueName } | Select-Object -First 1
    if (-not $project) {
        Write-Warning "Could not resolve startup project '$uniqueName' in the solution."
        Write-Warning "Projects found (count=$($allProjects.Count)): $(($allProjects | ForEach-Object { $_.UniqueName }) -join ' | ')"
        $exitCode = 1
        continue
    }

    $configuration = $null
    $platform = $null
    foreach ($context in $activeConfig.SolutionContexts) {
        if ($context.ProjectName -eq $uniqueName -or $context.ProjectName -eq $project.Name) {
            $configuration = $context.ConfigurationName
            $platform = $context.PlatformName
            break
        }
    }
    if (-not $configuration) { $configuration = $activeConfig.Name }
    if (-not $platform) { $platform = "x64" }

    Write-Host "Building startup project: $($project.FullName) [$configuration|$platform]"
    & MSBuild.exe $project.FullName "-p:Configuration=$configuration" "-p:Platform=$platform" -m -verbosity:minimal
    if ($LASTEXITCODE -ne 0) { $exitCode = $LASTEXITCODE }
}

exit $exitCode
