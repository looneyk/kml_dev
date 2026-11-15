# Testing --- Plan

Date: 2026-10-21

This plan describes how the tests are written, registered and run: doctest in place of the hand-written harness, CTest
to run every test executable or any subset of one, and reference files that one option updates. Stratum's five test executables ---
`stratum_format_test`, `stratum_sim_test`, `stratum_view_test`, `dungen_test` and `catagen_test` --- come first, then
the other test executables in the root `CMakeLists.txt`. `stratum_testing_review.md` is the review behind it, and the
open items are in `work_plan.md` (`### Testing`).

## Terms

| Term | What it is | Who uses it |
| --- | --- | --- |
| **test executable** | One program of tests that CMake builds, such as `stratum_sim_test` | *test executable* in CMake's and CTest's documentation; *test program* in GoogleTest's; *test binary* in Rust's `cargo test` |
| **test suite** | The test cases of a test executable that are filtered together, a doctest `TEST_SUITE`; the harness's `report_group` | *test suite* in doctest, GoogleTest (the first name of `TEST(Suite, Name)`) and JUnit; a *tag* in Catch2 |
| **test case** | One named test, a doctest `TEST_CASE`, and the unit that CTest runs | *test case* in doctest, Catch2 and JUnit; *test* in GoogleTest and CTest |
| **check** | One condition a test case asserts: `CHECK` continues past a failure, and `REQUIRE` ends the test case | *assertion*; `EXPECT_` and `ASSERT_` in GoogleTest |
| **subcase** | A branch of a test case, which runs the test case's code before it again for each branch | `SECTION` in Catch2; GoogleTest has none and uses a fixture |
| **fixture** | The state a test case starts from, a class the test case derives from | *fixture* in every framework; `SetUp` and `TearDown` in GoogleTest |
| **reference file** | Output checked in beside a test executable, which a test case's output must match | *golden file* in Go; *snapshot* in Jest and Insta; *approved file* in ApprovalTests |
| **test driver** | The program that runs the test executables and reports each test case: CTest | *test runner* in most frameworks |
| **test discovery** | Registering each test case of a built test executable with CTest, from the list the executable prints | `doctest_discover_tests`, `catch_discover_tests`, `gtest_discover_tests` |
| **label** | A name CTest selects tests by, such as `stratum` | *label* in CTest; *tag* in Catch2; *category* in NUnit |

## The framework

doctest, vendored at `library3/doctest` with its `doctest.cmake`. Godot's unit tests and nlohmann/json's are written in
it, and LLVM, Chromium, Abseil and fmt use GoogleTest. Catch2 works as doctest does, and has been a compiled library
since version 3. doctest is one header, compiles the fastest of the three, and has test suites and subcases, which match
the harness's groups and cases.

**Test cases live only in the test executables, never in production code.** doctest can compile test cases written
beside the code they test and strip them with `DOCTEST_CONFIG_DISABLE`; that is not used. Every test case is in a test
executable's own source files under `test/`, `doctest.h` is included only by those files and by `quetzal_testing`, and
no library under `library/quetzal` other than `quetzal_testing` includes doctest or links to it.

## A test executable on doctest

- **The harness is removed.** `report_group`, `report_case`, `report_case_equal`, `report_results` and their counters
  leave every test executable.
- **A harness group becomes a test case** of the same name, and **a harness case a named check** in it, `CHECK_CASE`
  or `CHECK_CASE_EQUAL`, which reports the case's name and its detail when it fails. Most cases build on the ones
  before them, such as a game created, saved and then opened, so a test case for each would repeat their setup.
- **A test suite is a set of related test cases,** such as a source file of `stratum_sim_test`.
- **A sweep over seeds or files is a loop in one test case,** with `CAPTURE` of the seed or the file, so a failure
  reports which one failed.
- **A case repeated for each scenario is a test case with a subcase for each scenario,** with the harness case's name, such as
  "shipped is equivalent to its reference". doctest registers no test case at run time, and its subcases are how it
  repeats a test case over values, as GoogleTest's value-parameterized tests and Catch2's `GENERATE` do. `-sc` selects
  a subcase. CTest runs the subcases of one test case in one process.
- **A failure's detail is the check's message,** through `CHECK_CASE`'s detail, `CHECK_MESSAGE` or `INFO`, where
  `report_case` took a detail.
- **A test case that builds a world starts from a fixture,** which loads the setting definitions once in the process,
  returns every setting to its default, and seeds the test executable's random engine, and the process-wide random
  engines once they exist (`random_engine.md`, `## Ownership`), so a test case runs the same alone, filtered, or in the
  full run.

## quetzal_testing

What every test executable shares, in `library/quetzal/testing/`, the target `quetzal_testing` and the namespace
`quetzal::testing`. It links doctest and `quetzal_common`.

```cpp
namespace quetzal::testing
{

    // Returns nothing if the file a test case wrote matches its reference file, or a description of the difference
    using comparison_function_type = std::function<std::optional<std::string>(const std::filesystem::path& out_filepath,
        const std::filesystem::path& reference_filepath)>;

    // Returns nothing if the two files are identical, byte for byte, or the first line on which they differ
    std::optional<std::string> compare_identical(const std::filesystem::path& out_filepath, const std::filesystem::path& reference_filepath);

    // Returns true if the run was given --update-references
    bool updating_references();

    // Checks that the file a test case wrote to out/ matches the reference file at the same relative path in reference/,
    // or, when updating references, copies it over the reference file
    void check_reference(const std::filesystem::path& relative_filepath, const comparison_function_type& compare = compare_identical);

} // namespace quetzal::testing

// Checks a condition, reported under a name, and with a detail if one is given, when it fails
#define CHECK_CASE(name, ...)

// Checks that a value equals the value expected, reported under a name with both values when it fails
#define CHECK_CASE_EQUAL(name, value, value_expected)
```

- **`main` is defined once, in `quetzal_testing`,** as `gtest_main` defines it for GoogleTest and `Catch2WithMain` for
  Catch2. A test executable's sources are only its test cases, and linking the library supplies `main`, which reads
  `--update-references`, passes every other argument to doctest, runs the test cases, and returns doctest's result.
- **A test executable whose reference files are equivalent rather than identical passes its own comparison.** The
  generator test executables compare zone files as parsed JSON, and report the JSON patch between them.
- **`FileValidator` is unchanged,** for the model programs.

## Registering and running

- **The root `CMakeLists.txt` calls `enable_testing()`,** and each test executable calls `doctest_discover_tests` with
  its own directory as the working directory, its name and a colon as the prefix of each test case's name, and its
  label: `stratum` for stratum's test executables, `mesh_operations` for the geometry and mesh file tests, and `quetzal`
  for the other libraries' tests.
- **Test cases that share output files run one at a time,** through a CTest resource lock, as `stratum_sim_test`'s
  share `out/saves` and `out/working`.
- **CTest runs each test case in a process of its own,** so an assert, an uncaught exception or a flecs abort fails that
  test case alone.
- **CTest writes a line for each test case,** passed or failed, and the output of every test case to
  `build/Testing/Temporary/LastTest.log`. `--output-on-failure` prints a failed test case's checks, and `--output-junit`
  writes JUnit XML.

```
ctest --test-dir build -C Debug -L stratum -j 8           every stratum test case
ctest --test-dir build -C Debug -R "^stratum_sim_test:"    one test executable
ctest --test-dir build -C Debug -R "door"                  the test cases whose names match
```

From a test executable's own directory, doctest's filters select a test suite, test cases by name, or the test cases
of a source file, and `--success` prints every passing check:

```
../../bin/Debug/stratum_sim_test.exe -ts="loading"
../../bin/Debug/stratum_sim_test.exe -tc="*locked*" --success
../../bin/Debug/stratum_sim_test.exe -sf="*relocation*"
../../bin/Debug/dungen_test.exe --update-references
```

## stratum_sim_test by area

`main.cpp` becomes a source file for each area, all compiled into the one test executable `stratum_sim_test`, with the
fixture and the helpers they share in `test_support.hpp` and `test_support.cpp`. doctest registers a test case from whichever source
file defines it, so no file lists another's test cases. Each file is a test suite named for its area.

| File | Test cases |
| --- | --- |
| `scheduling.cpp` | command queue, event queue, scheduler, clock, driver, commands |
| `layout.cpp` | layout hierarchy, zone layout, rooms, grid, flats, blocking, movement, adjacency and sight, walk, polygon containment |
| `occupancy.cpp` | proximity index, zone proximity index, occupancy, world move |
| `loading.cpp` | load layout, load walls, doors, door entities, loaded entities, decoded components, properties, layout entities |
| `world.cpp` | world, persistent ids, residency, saved zone documents, ai |
| `relocation.cpp` | specs, regions, level targets, access points, relocating through, entering access points, interacting with access points |
| `game.cpp` | game, persistent settings, insert definitions, config |

## Outside this plan

- mesh_operations and the model programs that `model/test.ps1` runs.
- CI. What this plan builds runs under CI as it is: CTest's exit status, `--output-junit` for the CI system's report,
  and working directories that CTest sets. CI adds the job itself, and an option to configure the build without the
  Vulkan SDK.
- A stress case and a Godot headless smoke test (`stratum_testing_review.md`, finding 8).

## Open

The open items are tracked in `work_plan.md`, under `### Testing`.
