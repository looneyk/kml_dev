# kml_dev

Largely independent projects share this repository and the `quetzal` libraries under
`library/quetzal/`. Work in one rarely touches another; check which one a request belongs to before
searching, and stay inside it.

| Project | What it is | Where |
| --- | --- | --- |
| **stratum** | A headless dungeon-crawler simulation, its ASCII and tile views, its level file format, and its level generators | `library/quetzal/stratum_sim`, `stratum_view`, `stratum_format`, `stratum_gen`, `dungen`, `catagen`, `godot`; `application/stratum_terminal`, `stratum_godot`; `tool/dungen`, `tool/catagen` |
| **Model suite** | Procedurally generated 3D models: the model programs, and the boundary-representation solid modeling and supporting libraries they are built with. `model/mesh_operations` tests the CSG feature: intersection, splitting, and the boolean operations | `library/quetzal/brep`, `model`, `geometry`, `math`, `triangulation`, `wavefront_obj`, `svg`, `xml`, `pdf`; `model/*`; `utility/mesh_diff`, `verify_model_output`, `dump_obj`, `compare_precision_obj` |
| **Vulkan applications** | Real-time 3D rendering | `library/quetzal/vulkan`, `scene_graph`, `game`; `application/lightmatrix`, `gallery`, `cubes`, `toot1` |

`doc/vulkan.md` covers the rendering engine in its own right.

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

Several test executables read data by relative path, so they must run from their own directory, not the
repository root:

    cd test/stratum_sim_test && ../../bin/Debug/stratum_sim_test.exe
    cd test/stratum_view_test && ../../bin/Debug/stratum_view_test.exe
    cd model/mesh_operations && ../../bin/Debug/mesh_operations.exe

`stratum_view_test` checks that rendered frames are identical to checked-in reference files, byte for
byte, and the generator test executables that level files are equivalent to theirs, so an unintended change shows
up as a mismatch rather than a failed assertion.

Every test executable is written in doctest (`doc/testing_plan.md`) and registered with CTest, which runs each test case
from the test executable's directory. The labels are `stratum`, `mesh_operations` and `quetzal`:

    ctest --test-dir build -C Debug -j 8
    ctest --test-dir build -C Debug -L stratum -j 8
    ctest --test-dir build -C Debug -R "^catagen_test:"

Run directly, a test executable takes doctest's filters, such as `-ts="loading"` for a test suite and `-tc="*seed*"` for
test cases, and `--update-references` copies each file it writes to `out/` over its reference.

## Conventions

- **Braces on their own line** (Allman), 4-space indentation, pointers aligned to type (`int* ptr`).
- **Wrap at 150 columns**, code and comments alike. Fill the line before breaking it; the more of a class, function, or file fits on screen, the more of its context is visible at once.
- **LF line endings, UTF-8 with no byte order mark, and a final newline** in every file, on Windows included.
- **ASCII only** in source. In comments, `--` is the em dash. Markdown uses `---`.
- **Forward slashes** in paths, on Windows included.
- **No absolute paths in code.** Use paths relative to a runtime root. Absolute paths are fine in
  shell commands.
- **Names**: `snake_case` variables; type aliases end in `_type` and are never named after the type
  they alias; member variables spell the full class name (`m_surface_resources`, not `m_surface`).
- **Comments are a last resort.** Prefer code that reads without one --- a named value, a smaller
  function, a clearer type. Write one only where the code is the way it is for a legitimately
  unusual reason, and then give that reason in a line or two. Not the decision process, not a
  restatement of what the code does, not a reference to callers or to other modules. Nothing an
  experienced engineer already knows, and nothing that is an invariant or a common practice in
  this codebase --- those get no comment at all. Only on code added entirely by Claude: do not
  auto-complete, extend, or add to comments on pre-existing code at any time.
- **Be specific in prose and in names.** No vague terms, and no one word stretched across several
  related meanings. The words below are a watch list, not a blacklist. Every one of them
  has a legitimate usage according to its conventional definition: what is wrong is the reach for a word loose
  enough to fit anywhere in place of the one that says what is meant. Ordinary use, fixed
  idioms included, is not the problem and is not to be edited out.

  | Word | Ordinary use, which is fine | The reach to watch for |
  | --- | --- | --- |
  | **stands** | an actor stands on a place; solve as it stands; what stands in for an exact zero; the offer stands | a general verb of place where a real relation exists --- a doorway *is one cell of* a wall line, it does not stand in one |
  | **carries** | a person carries a load; an addition carries a digit | a general verb of having --- a struct *has* a field or *tracks* a count |
  | **names** | naming a variable; a message mentions a file by its name | referring or selecting --- a parameter *refers to* a cell |
  | **lands** | a plane lands; a blow lands | where a result ends up, on completion --- a value *is stored in* a slot |
  | **answers** | a question or a message is answered | returning or reporting --- a function *returns* a distance |
  | **record** (verb) | recording audio, video, or a vote; `record` as a noun for a struct | storing or noting, referring to --- a pass *stores* a count |
  | **holds** | an invariant holds; a hand holds a cup | storing, containing, owning --- a vector *contains* cells |
  | **says** | speech; a quoted message | declaring or encoding --- a flag *indicates* a state, a field *encodes* a direction |
  | **wants** | a person wants something | needing --- a function *requires* an argument |
  | **knows** | a person knows a fact; dependency direction, as a deliberate idiom | having or referring --- an object *has* a pointer |
  | **sits** | physical placement --- a cell sits on the ground | being declared or stored --- a function *is declared in* a header |
  | **gives** | gives up; gives way; an equation gives a result | returning or producing --- a function *returns* a value |
  | **serves** | a server serves a request; serves a purpose | sufficing or representing --- a value *represents* an empty cell |
  | **takes in** | a hull takes in water; a household takes in a stray | enclosing or including --- a bound *encloses* a region |
  | **reaches for** | physical reach | depending on or using --- a pass *uses* a table |
  | **covers** | area covered; a document covers a topic | distance or handling --- a mover *travels* a distance, a case *handles* an input |
  | **gate** | a gate in a fence; blocking progress --- this gates stage 5 | a test that compares output --- a validator *compares* files |
  | **list** | a list of ids; a table lists the keys of a section | membership --- a zone *is in* a region, a region's `zone_ids` *has* its id |
  | **thing** | fixed idioms --- "one more thing", "the whole thing" | a placeholder noun where a specific one exists --- a *cell*, a *wall*, an *actor* |

- **Choose the words when writing a comment or a document, not after.** A word that would fit a
  dozen unrelated sentences is a fallback rather than a choice. Stop on it, work out what the
  specific relation is --- what the thing is, does, holds, or is part of --- and name that. The
  general word is right only where nothing narrower is true, and then it stays. This is the check
  the list above is a record of, and it applies to words the list does not include.
- **Write comments and documents in plain, grammatical English.** A comment can be a noun phrase or an imperative. Short means saying
  less, never dropping grammar: cut a clause that adds nothing, not the words that make the rest
  readable. Name the thing and say what it is or does. Starting with "What a", "Whether" or "On a
  ...:" is a warning sign, and so is an elliptical clause such as "null where none". End a sentence at the
  first point where it is true and complete. An urge to add to a finished sentence is the signal to stop.

  | Clipped | Plain |
  | --- | --- |
  | What a flat states for one room | The address of a flat's override for one room |
  | On a zone: its geometry and topology | The zone's geometry and topology |
  | Whether the entity has an address | Returns true if the entity has an address |
  | Null where none has one | Returns a null entity if none has one |
  | The layout's own, access_points aside | The layout's other components, excluding access_points |

## Documents

`doc/` is the document directory. The `*_plan.md` files are intent; the `*_reference.md` files are
what is built. For stratum, read `doc/game_system_reference.md` first --- it describes what exists,
and `doc/work_plan.md` is the work queue holding the open items from every plan. `notes` files
anywhere in the tree belong to the author: read them, never edit them.

## Git

Read-only. `git status`, `git log` and `git diff` are fine. Never stage, commit, branch, or
otherwise modify anything --- history is the author's alone.
