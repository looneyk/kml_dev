# Scripting Language --- Plan

The scripting language and its environment are what a game's and a mod's scripts and patterns are written in and run
by: the language, its implementation, and the binding between it and the simulation. The candidates are compared
against the requirements below, and the selection and its reasons go in `decision_rationale.md`. The selection is the open item
**The scripting language** in `work_plan.md`, chosen before Phase 4 (`game_system_principles.md`).

## Uses

- **Patterns** (`pattern_resolution_plan.md`): a value in a spec that is evaluated when the spec is resolved. A pattern
  is usually a single line that builds a string or calculates a number, and in a scripting language it can be a
  sequence of statements that returns the value. Any spec can be generated, so each spec, and each zone's, can have pattern text of its own,
  and the cost of a pattern is a compile and a call for each one.
- **Behavior** (`modding_plan.md`, `## Scripting`): event hooks on types (interacting, entering, a turn passing), AI
  writing `Intent`, rules, and commands. A hook runs for each event on each entity that has it.
- **Generators and placement** (`modding_plan.md`, `## Geometry`): a script that writes a zone's geometry and content,
  calling the passes and inner loops in C++.

## Requirements

- **Sandboxed.** A script reaches only what the game exposes, with no file system or process access, and its
  instruction count and memory are limited, so a mod's script cannot stop the game or exhaust its memory.
- **Deterministic.** A script given the same seed and inputs produces the same result on every platform and every run:
  arithmetic, the text of a number, the iteration order of a table, and random draws, which come only from the
  simulation's seeded random source.
- **Bound through reflection.** A script reads and writes any registered component, a mod's included, through flecs'
  meta cursor, with no binding written per component. Component types are registered at runtime from data.
- **Stateless between calls.** A script's state is stored in components, so saving a zone saves everything and no
  interpreter state is saved.
- **Costs per operation** that scale to worlds orders of magnitude larger than the test zones: compiling a script,
  calling it, crossing between the script and C++, and reading or writing a component field.
- **Embeddable in this build:** source that builds with CMake and MSVC in the solution's static runtime, a permissive
  license, and an implementation that is maintained.
- **Authoring:** a language that mod authors know or learn quickly, with editor support (a language server) and a
  debugger.

## Candidates

| Candidate | Language | License | Sandbox | Typing | Used by |
| --- | --- | --- | --- | --- | --- |
| Lua 5.5 (PUC-Rio) | Lua | MIT | `_ENV` environments, count hooks (`lua_sethook`), custom allocator | Dynamic | Factorio, Legend of Grimrock, DCSS |
| LuaJIT 2.1 | Lua 5.1 | MIT | As Lua, with the FFI excluded | Dynamic | Noita, Garry's Mod |
| Luau | Lua 5.1 derived | MIT | Built in: read-only globals, interrupt callback | Gradual, with a type checker | Roblox |
| AngelScript | C++-like | zlib | Only registered functions and types are reachable; line callback | Static | Amnesia, SOMA, Overgrowth |
| Squirrel | Lua-like, with classes | MIT | Only registered functions are reachable | Dynamic | Left 4 Dead 2, Portal 2, OpenTTD |
| Wren | Class-based | MIT | Only provided modules are reachable | Dynamic | --- |
| QuickJS-ng | JavaScript | MIT | Interrupt handler, memory limit | Dynamic | --- |
| A WebAssembly runtime (wasmtime, WAMR) | Any that compiles to WebAssembly | Apache 2.0 | By design: linear memory, imported functions only, fuel | That of the source language | Microsoft Flight Simulator |

- **CPython and .NET are not candidates.** Neither has a supported in-process sandbox: CPython removed `rexec` and
  `Bastion`, and .NET Core has no code access security.
- **GDScript is not a candidate.** Nothing from Godot's scripting layer is used by the core
  (`game_system_principles.md`).
- **LuaJIT's FFI reaches any C function,** so a sandbox excludes it, and LuaJIT is released as a rolling branch rather
  than as versions.
- **A WebAssembly runtime is an environment rather than a language.** A mod author needs a compiler toolchain, and a
  pattern of one expression would need a language compiled at load.

## Questions each candidate is tested on

- **Determinism:** whether the iteration order of a table depends on a per-run hash seed (Lua 5.5's `lua_newstate` takes
  the seed, and `luaL_newstate` draws one from an address and the time), how a number is written as text, and whether the
  built-in random source can be removed or replaced.
- **Reflection binding:** a component exposed through one generic type whose field access calls the meta cursor (Lua's
  `__index` and `__newindex`, Squirrel's delegates, a JavaScript `Proxy`), or, for a static language, types registered
  at runtime before scripts are compiled.
- **Cost per operation,** measured: compile a pattern, call it, call a C++ function from a script, read and write a
  component field, and create an environment for a spec.
- **Build:** warning-free at `/W4` with the library's own warnings suppressed, under `/MTd` and `/MT`.

## Lua 5.5 prototype

`spike/lua` (`lua_spike`) embeds Lua 5.5.1 from `library3/lua`, and its test cases resolve patterns, check the sandbox
and its limits, check determinism, and read and write a component registered at runtime through the meta cursor.

- **Lua is compiled as C++,** so a Lua error is a C++ exception and unwinds C++ frames with their destructors. A Lua
  error raised by `longjmp` in a C build skips them. A client includes `lua.h`, `lualib.h`, and `lauxlib.h`, and
  the library builds warning-free in Debug and Release.
- **A pattern is compiled as `return <text>`,** and as a sequence of statements if that fails, in load mode `"t"`, so
  a binary chunk is rejected. Its environment's globals fall through a metatable to a sandbox with the safe functions of
  the base library, read-only copies of `math` (without `random` and `randomseed`), `string` (without `dump`), `table`,
  and `utf8`, and `random`, with the interface of Lua's `math.random`. A global set by a pattern is set in its own environment,
  and a global that neither the environment nor the sandbox has is an error. A table such as `region` is a plain table.
- **The result is written as text by C++:** an integer as an integer, and a float as `std::format` writes it, so `6 / 2`
  is `3` where Lua's `tostring` writes `3.0`. A boolean is written as `true` or `false`. A pattern's value is a
  boolean, a number, a string, or nil.
- **The limits:** a count hook stops a call after an instruction limit, and the allocator refuses an allocation over a
  memory limit. With the collector stopped, a refused allocation still runs Lua's emergency full collection, and the
  state is usable after either error.
- **Determinism:** `lua_newstate` is given a hash seed derived from the world seed, and two states of one world visit a
  table's string keys in one order. A table whose keys are tables or functions is hashed by address, so its `pairs`
  order can differ between runs. `random` draws from the state's `Random`, and two states with one seed draw one
  sequence.
- **Reflection binding:** a component is a userdata whose `__index` and `__newindex` move a meta cursor to the member
  and read or write it as a boolean, an integer, a float, or a string, so a component registered at runtime needs no
  binding of its own.
- **Not yet covered:** the string metatable's `__index` is the unrestricted `string` table, so `('').dump` is reachable,
  though without `load` its output cannot run.

The costs, measured in Release on the development machine, 20,000 distinct patterns or 1,000,000 iterations in a loop:

| Operation | Cost |
| --- | --- |
| Create a sandboxed state | 19 us |
| Create a zone environment of 8 symbols | 0.5 us |
| Compile a distinct pattern | 2.5 us |
| Compile and call a distinct pattern | 3.0 us |
| Call a compiled pattern | 0.2 us |
| Call a C++ function from Lua | 0.035 us |
| Read and write a component member | 0.27 us |
| Memory left by a compiled pattern until a collection | 900 bytes |
| A full collection of 20,000 compiled patterns | 6.6 ms, 0.33 us per pattern |
| `quetzal::expression`: parse and evaluate a pattern | 1.8 us |

## Steps

- [x] 1. **Requirements settled.** The requirements and the questions above reviewed and revised.
- [x] 2. **Shortlist.** Lua 5.5.
- [x] 3. **Prototypes.** Each shortlisted candidate embedded in a test executable: a pattern resolved, a hook reading
  and writing a component through the meta cursor, the sandbox and its limits, and the determinism questions, with
  the costs per operation measured.
- [x] 4. **Selection.** Lua 5.5, with an entry in `decision_rationale.md` ("Lua for scripts and patterns"),
  `modding_plan.md`'s `## Scripting` revised, and `parser_function_plan.md` archived.
- [x] 5. **Plans revised.** `pattern_resolution_plan.md` and `region_spec_plan.md` revised for patterns in Lua.

## Open questions

- **Can a script suspend?** A coroutine or fiber has state between calls, which saving does not cover.
- **Is one interpreter state shared by the game,** with an environment for each script and spec, or is there one for
  each mod?
