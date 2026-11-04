# Development Setup

This document lists what a new machine needs to build and run everything in the root `CMakeLists.txt`, in the order it
is set up.

## Tools

| Tool | Why |
| --- | --- |
| Visual Studio 2022 or later, with the Desktop development with C++ workload | MSVC, which compiles the tree at `/std:c++latest` |
| CMake 3.25 or later | The root `CMakeLists.txt` requires it; the one Visual Studio installs is enough |
| Git for Windows | Cloning, and the `bash` that the Claude hooks run under |
| The Vulkan SDK | The Vulkan applications and their libraries require it; the rest of the tree configures and builds without it |
| Godot 4.7.2-stable, the win64 console executable, in `%LOCALAPPDATA%/Programs/Godot` | The Godot host runs in it, and Visual Studio launches it to debug the extension. godot-cpp is built against the 4.7 API |
| Python 3 on the `PATH` | The Claude wording hook, `.claude/hooks/check_wording.py` |

Without Godot, the build configures with `-DKML_DEV_GODOT=OFF`, which leaves out godot-cpp, `quetzal_godot` and
`stratum_godot`.

`KML_DEV_VULKAN` defaults on only where CMake finds the Vulkan SDK. Off, it leaves out `quetzal_vulkan`,
`quetzal_scene_graph`, `quetzal_game`, `lightmatrix`, `gallery`, `cubes` and `toot1`, and glfw, fastgltf and
KTX-Software in `library3/`, which only they use.

## Third-party repositories

The build uses fourteen repositories cloned into `library3/`, which the root repository does not track.
`script/library3_repositories.psd1` lists each one's URL, branch and the commit the tree is built against.

- **`script/clone_library3.ps1`** clones each that is not cloned yet, on its branch and reset to its commit. A clone that
  exists is left as it is, and reported if its commit differs. `-DryRun` lists what it would clone.
- **`script/update_library3.ps1`** pulls each, fast-forward only, and writes each one's new commit into
  `library3_repositories.psd1`, so the change appears in that file's diff. `-DryRun` fetches and reports how many
  commits each is behind, without pulling.

## Building

1. `pwsh script/clone_library3.ps1`
2. `cmake -S . -B build`
3. Build the solution, `build/kml_dev.slnx`, or a target of it (`CLAUDE.md`, `## Build`).
4. Import the Godot project once, after the extension is built:
   `Godot_v4.7.2-stable_win64_console.exe --headless --path application/stratum_godot/project --import`.
   Godot does not find the extension in a project that has not been imported.

## Building the startup project from Visual Studio

Visual Studio's Build.BuildSelection builds the project of the document with editor focus, not the Startup Project.
`script/build_startup_project.ps1` builds the Startup Project instead: it finds the running Visual Studio through the
running object table, reads `SolutionBuild.StartupProjects` through DTE automation, and runs MSBuild on that project.

To bind it to Ctrl+B, add it under Tools > External Tools, with `pwsh.exe` as the command and
`-File <repository>/script/build_startup_project.ps1 -SolutionPath "$(SolutionDir)$(SolutionFileName)"` as the
arguments, and bind the external tool to Ctrl+B under Tools > Options > Keyboard. External Tools has no
`$(SolutionPath)` macro; that name is only an MSBuild property.

- **Solution folders are recognized by their `Project.Kind`,** `{66A26720-8FB5-11D2-AA7E-00C04F688DDE}` in the CMake-generated
  `kml_dev.slnx`. The often-cited `{66A2671D-8FB5-11D2-AA7E-00C04F688DDE}` is the `Project.Kind` of Miscellaneous Files.
- **The running object table is read in C#,** in the script's `Add-Type` block. `IRunningObjectTable`, `IEnumMoniker`,
  `IMoniker`, and `IBindCtx` are vtable-only COM interfaces, and a PowerShell `[Type]` cast does not perform the
  `QueryInterface` a compiled cast does, so only the `IDispatch` DTE object is returned to PowerShell.
- **The script runs under PowerShell 7,** `pwsh.exe`.

The user directory, `%LOCALAPPDATA%/KML/Stratum`, is created when a host first runs, and the build copies the terminal
host's development `config.json` beside its executable, so neither needs setting up.
