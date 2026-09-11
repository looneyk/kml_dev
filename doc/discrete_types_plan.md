# Discrete Coordinate Types --- Plan

Status: not started. Two changes, the first landing on its own before the second begins
Date: 2026-09-10

The last of the conversion to `geometry::DiscretePoint` and `math::DiscreteVector`: settling what the
shared coordinate type is, and then converting the view, which is the one place still addressing a
grid with loose `size_t`. `loader_plan.md` covers the loader structures; what is built goes to
`game_system_reference.md`.

## What and why

Everything upstream of the view is already on the discrete types --- `GridMapping::extent`,
`DiscreteLayout`, both generators, the loader, both hosts. `stratum_view::Buffer` is not, and the cost
is concentrated in one file. `cells.cpp` carries eight `static_cast<size_t>` calls whose only job is
to cross the boundary, and a pair of adapter lambdas whose only job is to hide those casts from the
twenty call sites in `lattice()` --- sites already written in the grid's own coordinate type.
`Buffer`'s `size_t` API is the only reason that code cannot address the buffer directly.

Converting the view pays off only if the view and the sim share one coordinate type, since the whole
of the win is that a grid extent travels into a buffer whole, with no conversion. That makes the
coordinate type itself the first question, and today it is `long` --- 32 bits under MSVC, 64 on
Linux and macOS, so the same code has different overflow points and different serialized widths
depending on where it was built.

Nothing about behavior changes in either change. Every case in the four stratum suites passes as it
did and every checked-in level and frame file stays byte for byte what it was.

## Change 1 --- coord_type

`coord_type` becomes `std::int32_t`. A fixed width states what the type is rather than leaving it to
the data model; 32 rather than 64 because capacity was never the question --- the largest thing in
the tree is a 63 by 63 lattice --- and because that is what grid and tile code generally uses.

`dungen/types.hpp` and `catagen/types.hpp` already declare the alias, so only its definition moves.
`stratum_sim/types.hpp` has no `coord_type` at all: it spells the width directly into
`discrete_vector_type` and `discrete_point_type`. It gains one, and both aliases rebase onto it.

The sweep is the rest: 77 bare `long` spellings across `stratum_sim`, `stratum_view`, `dungen`,
`catagen`, both tools and the four stratum suites. Each becomes `coord_type` or `auto` according to
what it is, never a bare width --- the width belongs in the alias declarations and nowhere else.

| Shape | Becomes |
|---|---|
| Loop induction, `for (long j = 0; j < map.extent.y(); ++j)` | `coord_type` |
| Lambda and function parameters, `[&](long i, long j)` | `coord_type` |
| A local whose initializer already has the type, `long di = direction.x();` | `auto` |
| A local declared and then assigned, `long nx = 0;` | `coord_type` |
| `static_cast<long>` | `static_cast<coord_type>` |

Two things stay on `long`. The `DiscreteVector<long, 3>` aliases in `string_util_test` and
`vector_point_test` exercise the template over an arbitrary signed integral and are testing the math
library rather than naming a stratum coordinate. And `to_integer` in both tool mains is a general
number parser feeding seeds and counts as well as coordinates; it follows only if every caller turns
out to want a coordinate.

On Windows this change is a no-op, `long` being 32 bits there already. On Linux and macOS it narrows
from 64, and the index arithmetic ceiling becomes about 46340 square everywhere rather than only
under MSVC. Both are far outside anything the generators produce.

## Change 2 --- Buffer, Span and Size

`Buffer` gains a `types.hpp` of its own alongside dungen's and catagen's, and its API moves to the
point and vector:

    Buffer(const vector_type& extent, T fill = T());

    const vector_type& extent() const;
    bool contains(const point_type& position) const;

    T at(const point_type& position) const;
    void set(const point_type& position, T value);

`width()` and `height()` go, rather than joining `extent()`. What that buys is that the extent stays
one value the whole way: `Cells cells(grid.extent, Place {PlaceType::Solid})` needs no unpacking, and
comparing two buffers is one `==` rather than four terms.

There is no two-coordinate overload beside the point form. `DiscretePoint`'s two-argument constructor
is not explicit, so a call site reads `at({i, j})`, and a loose pair sitting beside a point-taking
function is the shape this conversion exists to remove rather than to add.

A private `offset()` is the one conversion in the class, and holds the assert:

    assert(contains(position));
    return static_cast<size_t>(position.j() * m_extent.x() + position.i());

`contains()` is what supplies the lower bound. Today's `assert(i < m_width && j < m_height)` gets it
free from unsignedness, and a signed coordinate would lose it silently --- `at(-1, 0)` would pass an
upper-bound-only check and index before the start of the vector.

`Span` becomes a position and a length, `point_type position` with a `coord_type count` beside it.
`count` is a run along the same axis as the extent, so it shares the coordinate type rather than
staying a `size_t` that has to be converted to be compared against one.

`stratum_terminal::Size` goes entirely, replaced by the shared vector ordered x then y. It is the
last transposed extent in the codebase: every other one is x first, and `Size` is `(rows, columns)`.
The transposition does not disappear, it moves to the three places the protocol actually asks for it
--- the two platform reads, and the XTWINOPS escape, which is natively height first.

`dungen::Cells` is the already-converted sibling and settles the shape of all of this: the
extent-taking constructor, the point-taking accessor, `assert(contains(position))`, and the
descending row print that is also the answer for `to_string()`.

## Order of work

Change 1 is one step, verified across every suite before Change 2 starts.

Change 2 goes a file at a time, each step building warning-free in both configurations and leaving
the five reference frames byte for byte equal:

1. `stratum_view/types.hpp`.
2. `Buffer.hpp` internals and the new API, the old surface kept as forwarders so nothing downstream
   breaks yet. A frame that moves here implicates `offset()`, `to_string()` or `difference()` and
   nothing else.
3. `cells.cpp`, the type conversion: both adapter lambdas and all eight casts go.
4. `cells.cpp`, `line` and `solid` taking a point, the four neighbor offsets becoming a loop over
   `cardinal_directions`, and the lattice mapping named once rather than spelled out at six sites.
5. `ascii.cpp`, then `StratumWorld.cpp`, then `stratum_view_test`.
6. `Span`, atomically across `Buffer.hpp`, `Terminal.cpp` and the test.
7. Delete the transitional API. Whatever still calls it fails to compile, which is the completeness
   check on the sweep.
8. `Size`.

Two guards in `cells.cpp` stay exactly as they are. `line`'s `x < 0 || y < 0` is what keeps a
left-edge position from reaching the buffer with a negative, and `at` asserts rather than returning a
sentinel. `solid`'s bounds guard `layout.open()` rather than the buffer, and its `(x - 1) / 2` relies
on truncation toward zero.

The incremental path in `Terminal::draw` is the one part of this with no automated coverage: the test
exercises `difference` and `Span` directly but never through `Terminal`. The `Span` step and the
`Size` step are each checked by hand against a running terminal.
