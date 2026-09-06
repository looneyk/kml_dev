- Do not create new comments
- Do not auto-complete, extend, or add to comments at any time

## Project Overview
Modern C++ geometry, graphics and simulation libraries (namespace `quetzal::`) with applications,
test projects and command-line utilities. Header-heavy, template-based, targeting Windows with MSVC.

Three largely independent projects share the repository and the libraries under `library/quetzal/`.
Work in one rarely touches another; establish which one a request belongs to before searching.

- **stratum** --- headless dungeon-crawler simulation, its ASCII and tile views, and two level
  generators. `library/quetzal/stratum_sim`, `stratum_view`, `dungen`, `catagen`;
  `application/stratum_terminal`, `stratum_godot`; `tool/dungen`, `tool/catagen`
- **mesh_operations** --- boundary-representation solid modelling: intersection, splitting, boolean
  operations. `library/quetzal/brep`, `geometry`, `math`; `model/*`; `utility/mesh_diff`
- **Vulkan applications** --- real-time 3D rendering. `library/quetzal/vulkan`, `scene_graph`,
  `game`; `application/lightmatrix`, `gallery`, `cubes`, `toot1`

## Build System
- Primary: CMake. The generated tree is `build/`, mirroring the source tree; the solution is
  `build/kml_dev.slnx`. Regenerate with `cmake -S . -B build`
- Build a named target rather than the whole solution --- seconds instead of minutes:
  `MSBuild build/library/quetzal/brep/quetzal_brep.vcxproj -p:Configuration=Debug -p:Platform=x64 -m`
- Use dashed switches (`-p:`, `-m`), not slashes, and call MSBuild directly rather than through
  `cmd /c`
- Toolchain: MSVC v145 (VS2022), C++23/C++26 (`/std:c++latest`), Level 4 warnings, SDL checks
  enabled. The build is warning-free; keep it that way
- Output: binaries in `bin/{Configuration}/`, libraries in `lib/{Configuration}/`
- Standalone Visual Studio solutions also exist for `application/*_d3d11` and
  `model/mesh_operations`, separate from the CMake build

## Scope
Only targets wired into the root `CMakeLists.txt`, or driven by the `model/*.ps1` scripts, are
supported. Other directories are reference or legacy; do not analyse them or report them broken.
Flag a file only when it looks like it should be in the build and is not.

## Running Tests
Several suites read data by relative path and must run from their own directory, not the repository
root:

    cd test/stratum_sim_test && ../../bin/Debug/stratum_sim_test.exe
    cd test/stratum_view_test && ../../bin/Debug/stratum_view_test.exe
    cd model/mesh_operations && ../../bin/Debug/mesh_operations.exe

`stratum_view_test`, `dungen_test` and `catagen_test` compare output byte for byte against
checked-in reference files, so an unintended change appears as a file mismatch rather than a failed
assertion.

## Architecture Decisions
- Modular library in `library/quetzal/` with 30+ modules: math, geometry, model, brep, xml,
  wavefront_obj, vulkan, direct3d11, scene_graph, gui, io, stratum_sim, stratum_view
- Consistent namespace: `quetzal::{module}::{entity}`
- Half-edge mesh with full polygon support (holes, submeshes, surfaces)
- stratum stores one wall record per side of a cell edge, wound counterclockwise about the open
  space it faces; an opening is the absence of a wall rather than a flag on one

## Code Conventions
- Braces on their own line (Allman), 4-space indentation, pointers aligned to type (`int* ptr`),
  operators and control-flow keywords padded, switch cases indented
- [.editorconfig](../.editorconfig) sets UTF-8, LF, trimmed trailing whitespace and a final newline
  for every file. Its `cpp_*` keys are Visual Studio's own formatter settings and are read by
  nothing else; it sets no indent size, so the 4-space rule is convention rather than enforced
- [astylerc](../astylerc) is an Artistic Style option file that nothing in the build invokes. It
  applies only when astyle is run by hand with `--options=astylerc`, and it does not name a brace
  style, so it will not impose one
- LF line endings in every file, on Windows included
- ASCII only in source; `--` is the em dash in comments, `---` in Markdown
- Forward slashes in paths, on Windows included
- No absolute paths in code; use paths relative to a runtime root
- `snake_case` variables. Type aliases end in `_type` and are never named after the type they alias.
  Member variables spell the full class name (`m_surface_resources`, not `m_surface`)
- Prefer names over IDs for long-lived references
- Be specific in prose and in names: no vague terms, and no one word stretched across several
  related meanings

## Potential Pitfalls
- Surface normals deprecated in favor of vertex/face normals
- `wall_toward` scans every wall, so a cell step is O(walls) and a sight line O(cells x walls);
  an edge index is planned and not built
- `notes` files anywhere in the tree are the author's: read them, never edit them

## Key Files and Directories
- [CLAUDE.md](../CLAUDE.md): the same orientation as this file, for a different assistant --- keep
  the two in agreement
- [build/PropertySheet.props](../build/PropertySheet.props): global compiler settings for the
  standalone solutions
- [library/quetzal/model/primitives.hpp](../library/quetzal/model/primitives.hpp): core geometry API
- [library/quetzal/vulkan/CLAUDE.md](../library/quetzal/vulkan/CLAUDE.md): the rendering engine in
  its own right
- [notes](../notes): active design notes and TODOs for mesh operations
- `doc/`: stratum's documents; the other two projects keep their notes with their code
- `application/`, `test/`, `tool/`, `utility/`, `model/`: apps, suites, side programs, CLI tools,
  and the model programs

## Documentation
- `doc/game_system_reference.md` describes what stratum is and how it is built --- read it first.
  `doc/work_plan.md` is the work queue and holds the open items from every plan
- `*_plan.md` files are intent; `*_reference.md` files are what exists
- README.md is minimal; mesh_operations design thoughts are in [notes](../notes)
