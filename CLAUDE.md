# kml_dev

Three largely independent projects share this repository and the `quetzal` libraries under
`library/quetzal/`. Work in one rarely touches another; check which one a request belongs to before
searching, and stay inside it.

| Project | What it is | Where |
| --- | --- | --- |
| **stratum** | A headless dungeon-crawler simulation, its ASCII and tile views, and two level generators | `library/quetzal/stratum_sim`, `stratum_view`, `dungen`, `catagen`; `application/stratum_terminal`, `stratum_godot`; `tool/dungen`, `tool/catagen` |
| **mesh_operations** | Boundary-representation solid modelling --- intersection, splitting, boolean operations --- and the model programs that exercise it | `library/quetzal/brep`, `geometry`, `math`; `model/*`; `utility/mesh_diff` |
| **Vulkan applications** | Real-time 3D rendering | `library/quetzal/vulkan`, `scene_graph`, `game`; `application/lightmatrix`, `gallery`, `cubes`, `toot1` |

`library/quetzal/vulkan/CLAUDE.md` covers the rendering engine in its own right.

## Scope

Only targets wired into the root `CMakeLists.txt`, or driven by the `model/*.ps1` scripts, are
supported. Plenty of other directories exist and are reference or legacy; do not analyse them or
report them broken. Flag a file only when it looks like it *should* be in the build and is not.

`application/*_d3d11` and `model/mesh_operations` also carry standalone Visual Studio solutions,
which are separate from the CMake build.

## Build

The CMake build tree is `build/`, mirroring the source tree, with binaries in `bin/<Configuration>/`.
Build a named target rather than the whole solution --- it is the difference between seconds and
minutes:

    MSBuild build/library/quetzal/stratum_sim/quetzal_stratum_sim.vcxproj -p:Configuration=Debug -p:Platform=x64 -m
    MSBuild build/test/stratum_view_test/stratum_view_test.vcxproj -p:Configuration=Debug -p:Platform=x64 -m
    MSBuild build/application/lightmatrix/lightmatrix.vcxproj -p:Configuration=Debug -p:Platform=x64 -m

The whole solution is `build/kml_dev.slnx`. Regenerate with `cmake -S . -B build`.

Use dashed switches (`-p:`, `-m`), not slashes, and call MSBuild directly rather than through
`cmd /c`. The build is `/W4` and warning-free; keep it that way.

## Running tests

Several suites read data by relative path, so they must run from their own directory, not the
repository root:

    cd test/stratum_sim_test && ../../bin/Debug/stratum_sim_test.exe
    cd test/stratum_view_test && ../../bin/Debug/stratum_view_test.exe
    cd model/mesh_operations && ../../bin/Debug/mesh_operations.exe

`stratum_view_test` and the generator suites compare rendered output byte for byte against
checked-in reference files, so an unintended change shows up as a file mismatch rather than a
failed assertion.

## Conventions

- **Braces on their own line** (Allman), 4-space indentation, pointers aligned to type (`int* ptr`).
- **LF line endings** in every file, on Windows included.
- **ASCII only** in source. In comments, `--` is the em dash. Markdown uses `---`.
- **Forward slashes** in paths, on Windows included.
- **No absolute paths in code.** Use paths relative to a runtime root. Absolute paths are fine in
  shell commands.
- **Names**: `snake_case` variables; type aliases end in `_type` and are never named after the type
  they alias; member variables spell the full class name (`m_surface_resources`, not `m_surface`).
- **Comments** may be added sparingly, and only for non-obvious conditions, to code blocks or lines
  added entirely by Claude. Do not auto-complete, extend, or add to comments at any time on
  pre-existing code.
- **Be specific in prose and in names.** No vague terms, and no one word stretched across several
  related meanings.

## Documents

`doc/` is stratum's; the other two projects keep their notes with their code. Read
`doc/game_system_reference.md` first --- it describes what exists. `doc/work_plan.md` is the work
queue and holds the open items from every plan. The `*_plan.md` files are intent; the `*_reference.md`
files are what is built. `notes` files anywhere in the tree belong to the author: read them, never
edit them.

## Git

Read-only. `git status`, `git log` and `git diff` are fine. Never stage, commit, branch, or
otherwise modify anything --- history is the author's alone.
