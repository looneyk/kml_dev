---
paths:
  - "**/*.{hpp,cpp,h,inl}"
  - "**/CMakeLists.txt"
---

# Code

These rules apply to C++ throughout the repository, alongside the Conventions in `CLAUDE.md` and `writing.md`.

## Design

- **Build general support from the start.** When a mechanism applies to one member of a group, build it for every
  member, even when one needs it now: if one `FormatSpec` setting is optional with a `resolved_*` function, every
  setting is. The minimal version is never the default choice.
- **Fold a concept into an existing mechanism** where it is an instance of one, case by case: `style` is an ordinary
  inherited component, not a key beside `components` with its own table, struct, and reader checks. Before a local fix,
  consider whether the problem belongs in a structure, a spec, or a mechanism already there.
- **Remove code that adds no value.** A nicety the output does not need, such as `number_text` forcing a `.0` on JSON
  reals, and a wrapper around one call, such as `skip_bracket` over `skip_delimiter`, are removed, not kept as an
  option. If the output changes, the references are regenerated and the docs updated.
- **Temporary defaults go at the highest level,** a host or the top-level game object, from a named value marked
  temporary. A library function takes the value as a required parameter, never as a default argument or a convenience
  overload.
- **A general library never refers to its client.** A `library/quetzal` library without the `stratum_` prefix has no
  rule, scope, message, or comment example about the game. The client's policy, such as which settings are modifiable,
  is in the client (`stratum_sim`'s `insert_definitions`), and the library offers the general mechanism in its own
  terms: a scope, a setting, a file, a base path.
- **Settings are generic:** no header, enumeration, registered type, or special code for one setting. A placeholder
  setting such as `gameplay.reseed_on_open` only proves generic access and has no further design.
- **Code is mathematically precise.** A value is a size along x, y, or z, a signed coordinate, or an origin: "The size
  of a region in zones in the x direction", not "Region width" or "east to west". Compass directions, levels, up and
  down, width, and depth come from data files, and an axis is never mapped to a game concept such as level unless the
  design does.
- **Follow what major open source projects do** when choosing between options, and say what they do. One shared `detail`
  namespace per library is the norm (Boost, Eigen, fmt, folly, range-v3, Qt), with collisions managed by naming: a name
  that is the bare noun of its namespace, or a bare generic verb, gets a qualifier (`compare` becomes `compare_models`).

## Values and state

- **No pointers, `std::variant`, or `std::any`** unless absolutely necessary. Objects are held by value and cleared and
  refilled in place (`session.clear()`, `create()`, `load()`), not replaced through a `std::unique_ptr`. The existing
  uses in stratum (`Command` and `Event` over `std::any`, `ZoneLayout`'s `std::unique_ptr<Layout>`, and others) are not
  extended.
- **No valid value is reserved as a marker.** "Unset" or "none" is `std::optional`, and validation checks that it is
  engaged: a seed that rejects 0 as "never assigned" costs `--seed 0`. `quetzal::id_type` with `nullid` is the one
  exception and is no precedent.
- **Optional means absent, never "set later".** A member every instance receives soon after construction is a
  constructor argument. A `std::optional` is dereferenced with `*` only where something has just proved it engaged, with
  a comment saying so, and with `.value()` elsewhere.
- **A domain invariant is an assert,** and the branch handling the excluded case is removed and never described in
  comments or docs as possible. A degenerate input is not special-cased for a tidier answer: a negative extent produces
  a negative volume, and checking is the caller's. If the assert fires, the code that produced the data is fixed.
- **During development, failures are loud.** An assert or an exception, never a silent null return or a no-op; graceful
  handling is added for release.
- **Every throw is preceded by an error log entry with the same message,** at the throw site, built once into a
  `const std::string`: `log::logger().error(category, message)` with the library's category, such as `"stratum_sim"` or
  `log_category` in stratum_format. The same applies to a throwing helper such as stratum_format's `detail::fail`.

## Form

- **Constrained template parameters:** `template<std::integral T>` and `template<Concept T>`, not a `requires` clause on
  a trait. A `requires` clause is kept for a conjunction, an expression, or a constraint on a nested type.
- **Range-for variables are `const auto&`,** or `auto&`. An explicit type is kept only where it converts or slices on
  purpose.
- **Every function ends with an explicit return,** `return;` in a void function, a doctest `TEST_CASE` body, and a void
  lambda included, and after a final loop or if/else. Constructors and destructors are the exception. A return that
  cannot be reached, as after a `while (true)` loop, is MSVC's C4702 at /W4; the loop has a condition instead.
- **Header layout:** the public interface first, then everything else in one `detail` namespace at the end, with
  out-of-line definitions in declaration order. `detail` may depend on public types and never the reverse. A type used
  as a by-value parameter default must be complete before its use, and a concept cannot be forward declared, so a
  concept constraining a public overload is public and precedes it.
- **Files:** a significant class, with behavior, invariants, or lifetime, has its own `.hpp` and `.cpp`, named for it.
  Simple structs and a small set of related free functions may share a file named for its content. A document's reader
  and writer go with the document, never in `read_x` and `write_x` files, and a small isolated feature is one pair: a
  file format is `XDocument.hpp/.cpp` with `format_name` and `format_version` as static members. A file is named for
  what it contains, never for where its data is stored.

## Portability

- **Cross-platform code by default,** in files that build only on Windows too: portable standard C++ or a portable
  library, not `WideCharToMultiByte` and other `<Windows.h>` calls. An unavoidable platform call is guarded with
  `#if defined(_WIN32)`, with its headers inside the guard.
- **File streams open in binary mode,** for input and output, and lines are read through `quetzal::read_line` in
  `library/quetzal/common/file_util.hpp`, which drops a trailing `\r`. Text mode strips CRLF only on Windows, so only an
  explicit strip behaves the same everywhere. `std::stol` ignores trailing garbage and `std::from_chars` with a
  full-consumption check does not, so a stricter parser can expose CRLF input. A platform call inside `assert(...)`
  vanishes under `NDEBUG`.
- **Dates and times are ISO 8601:** `YYYY-MM-DD`, 24-hour `HH:MM:SS`, and a `T` separator and offset in a full
  timestamp, in logs, file names, reports, and test output.

## Tests

- **A test executable reports each test case's outcome,** through doctest and CTest (`doc/testing_plan.md`), never as one
  failure counter returned as the exit code.
