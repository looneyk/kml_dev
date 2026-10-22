# Stratum Testing --- Review

Status: review; `testing_plan.md` is the plan that follows it
Date: 2026-09-28

How stratum is tested --- the five test executables `stratum_format_test`, `stratum_sim_test`, `stratum_view_test`, `dungen_test` and
`catagen_test` --- how they are run, and how well that scales and can be automated. The random engine is designed in
`random_engine.md`; this document refers to that design rather than restating it.

## Summary

The tests catch regressions well for one developer on one Windows machine. They are not registered with a test runner,
their reference files do not reproduce with another standard library, and the largest test executable is not organized to keep
growing.

## Strengths

- **Every test executable writes one line per case and returns 0 or 1,** so any test runner or CI system can drive it unchanged.
- **The generator tests compare zone files to their references as equivalent,** parsed rather than byte for byte, so a
  formatting change is not a mismatch.
- **Seed sweeps check invariants over many seeds** ("writes a door entity at each door over N seeds"), not one sample.
- **`stratum_format_test`'s mutation cases assert the exact text of each read error,** where format regressions usually
  go unnoticed.
- **The build is `/W4` and warning-free.**

## Findings

Most severe first.

### 1. Reference files depend on the standard library

`math::Random<>` uses `std::default_random_engine` --- `mt19937` in MSVC, `minstd_rand0` in libstdc++, `minstd_rand` in
libc++ --- and the standard distributions, whose algorithms are implementation-defined. Built with GCC or Clang and
their own libraries:

- every dungen and catagen reference file mismatches;
- the seed-dependent cases in `stratum_sim_test` fail ("At this seed the door is in the top row of the shared wall");
- an update to MSVC's standard library is free to change the distributions, with the same result on Windows.

`random_engine.md` (`## The random engine type`) specifies the random engine type explicitly, which fixes the sequence of raw
values, and makes the distributions and the shuffle the project's own, since reference files must match across standard
libraries. A CI job on a second platform and a seed quoted in a bug report both depend on it, and the
distributions of the project's own are short: a bounded integer by Lemire's method, a real from the top 53 bits of a
64-bit value. Building them with the generation runs as objects (`random_engine.md`, `## Ownership`) regenerates the reference files once rather than twice, and
each file checked in before then is one more to regenerate.

### 2. The tests are not registered with CTest

Neither the root `CMakeLists.txt` nor any test executable's calls `enable_testing()` or `add_test`. Each test executable
reads `data/` and
writes `out/` relative to the working directory, so running one needs a manual `cd`; `VS_DEBUGGER_WORKING_DIRECTORY`
sets it only under the debugger. Registering each test executable with
`add_test(NAME ... COMMAND ... WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})` and a `stratum` label lets
`ctest --test-dir build -C Debug -L stratum -j` run all five, from the right directories, in parallel.

### 3. No CI, and the headless tests need Vulkan and Godot to configure

`.github/` contains only `copilot-instructions.md`. The root `CMakeLists.txt` calls `find_package(Vulkan REQUIRED)`
unconditionally, and `KML_DEV_GODOT` defaults to ON, so configuring the build for the stratum tests needs the Vulkan
SDK and godot-cpp, neither of which they use. An option for the Vulkan projects, as `KML_DEV_GODOT` is for Godot, lets a
plain runner configure and build them.

A Windows job with MSVC is the first step. A Linux job with GCC or Clang is the one that catches finding 1 and any
MSVC-only code, and it follows once finding 1 is resolved.

### 4. Cases in `stratum_sim_test` share random state

`g_suite_random(1)` is one global random engine, and every `SuiteWorld` takes it by reference. Nothing draws from it
yet, but once the simulation does, a case that draws from it shifts the draws of every case after it:

- a case added in the middle of the run can change the outcome of an unrelated case later in it;
- a subset of the cases, once they can be filtered, draws differently from the full run.

The test executable should seed the process-wide random engines before each case, as `random_engine.md`
(`## Ownership`) describes for the tests. Separately, `Game` seeds from `std::random_device` and cannot be given a seed (`random_engine.md`,
`## Ownership`, not yet built), so the cases that construct a `Game` draw unseeded values, and a failure through one cannot be
replayed.

### 5. `stratum_sim_test` is one file with no filtering or isolation

`main.cpp` is 4,397 lines, with about 620 cases in 44 functions that `main` calls in a fixed order.

- No function or case can be run by itself from the command line.
- An assert, an uncaught exception, or a flecs abort ends the process, and every result after it is lost.
- Every feature adds cases to it, so its compile time grows, and any change recompiles all of it.

Splitting it by area into several translation units --- layout, movement, zones and relocation, access points,
persistent ids --- and adding a name filter solves the first and third. Registering each area as its own CTest test,
as an executable per area or one executable run with a filter per test, runs each in its own process and solves the
second.

### 6. The harness is copied into eleven test executables

`report_group`, `report_case`, `report_case_equal`, `report_results` and their counters are duplicated in the `main.cpp`
of each of stratum's test executables and of six others, and the copies have diverged: `stratum_view_test`'s `report_case_equal` is a template, and the others take
`size_t`. Two remedies:

- **A small shared test library,** which keeps the current output unchanged.
- **A test framework,** as most open source C++ projects use: doctest, Catch2, or GoogleTest. Each provides name
  filtering, a line per case (passing cases included, with `--success` in doctest and Catch2), JUnit XML for CI, and CTest
  discovery (`doctest_discover_tests`, `catch_discover_tests`, `gtest_discover_tests`) that registers each case as a
  test of its own. doctest is closest to the current style: one header, fast to compile, `CHECK_MESSAGE` for the
  failure detail, and subcases for scenarios.

### 7. References are regenerated by hand

An intended output change is accepted by copying a test executable's `out/` over its `reference/`. An `--update-references`
argument on each test executable makes it one command, and by clearing `reference/` first it also removes the file of a scenario
that no longer exists.

### 8. Untested areas

- **The hosts.** `stratum_terminal` and `stratum_godot` have no tests. Godot's console build runs `main.gd` headless, so
  a smoke script --- load the extension, start a game, move, relocate once, exit with a status --- can be a CTest test.
- **Scale.** Nothing measures loading, zone creation, or relocation at size, and real worlds will be several orders of
  magnitude larger than the tests' data. A stress case --- a large generated zone, the maximum number of resident
  zones --- run in Release with its timings logged detects a quadratic pass in the change that introduces it.
- **Build configurations.** Only Debug is run. A Release run exposes undefined behavior that the optimizer exploits and
  Debug does not, and `/fsanitize=address` in MSVC, or ASan and UBSan in the Linux job, detects memory errors.

### 9. Commits combine unrelated changes

A single commit often combines several features: 6d1c70f adds `StratumGame`, persistent ids, the access point
overhaul, the zone loading cleanup, and a catagen terminology change. With no CI, nothing shows which change first broke
a case, and `git bisect` ends at a commit too large to find it in.

## Suggested order

1. A named random engine and distributions of the project's own, with the reference files regenerated once (finding 1).
2. CTest registration and an option for the Vulkan projects (findings 2 and 3).
3. The process-wide random engines seeded before each case, and a seed that `Game` can be given (finding 4).
4. doctest or a shared harness, and `stratum_sim_test` split by area (findings 5 and 6).
5. CI: a Windows job, then a Linux job (finding 3).
6. A stress case and a Godot headless smoke test (finding 8).
