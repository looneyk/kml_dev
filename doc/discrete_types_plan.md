# Discrete Coordinate Types --- Plan

Status: complete
Date: 2026-09-10, revised 2026-09-22

Complete the conversion to `geometry::DiscretePoint` and `math::DiscreteVector`: settle the shared coordinate type, then
convert the view, which addressed a grid with `size_t`. `game_system_reference.md` describes what is built.

The view and the simulation share one coordinate type, so a grid extent goes into a buffer whole, with no conversion.
Behavior does not change: every case in the stratum suites passes, and every checked-in zone and frame file stays byte
for byte the same.

## Change 1 --- coord_type

`coord_type` is `std::int32_t`, a fixed width rather than `long`, which is 32 bits under MSVC and 64 on Linux and
macOS. 32 bits, as grid and tile code generally uses.

`dungen/types.hpp`, `catagen/types.hpp` and `stratum_sim/types.hpp` declare `coord_type`, and `discrete_vector_type`
and `discrete_point_type` are defined over it. Every bare `long` in `stratum_sim`, `stratum_view`, `dungen`, `catagen`,
both tools and the stratum suites becomes `coord_type` or `auto`, the width stated only in the alias declarations:

| Form | Becomes |
|---|---|
| Loop variable, `for (long j = 0; j < map.extent.y(); ++j)` | `coord_type` |
| Lambda and function parameters, `[&](long i, long j)` | `coord_type` |
| A local whose initializer has the type, `long di = direction.x();` | `auto` |
| A local declared and then assigned, `long nx = 0;` | `coord_type` |
| `static_cast<long>` | `static_cast<coord_type>` |

The `DiscreteVector<long, 3>` aliases in `string_util_test` and `vector_point_test`, which test the math library over an
arbitrary signed integral, stay `long`, as does the general number parser in the tools.

## Change 2 --- Buffer, Span and Size

`stratum_view` has a `types.hpp`, and `Buffer` takes points and vectors:

    Buffer(const vector_type& extent, T fill = T());

    const vector_type& extent() const;
    bool contains(const point_type& position) const;

    T at(const point_type& position) const;
    void set(const point_type& position, T value);

`width()` and `height()` are removed, so the extent is one value throughout. There is no two-coordinate overload; a call
reads `at({i, j})`.

A private `offset()` converts a position to a subscript and asserts `contains(position)`, which supplies the lower bound
an unsigned index had.

`Span` is a position and a `coord_type count` along its row.

`stratum_terminal::Size`, in rows by columns, is replaced by the shared vector in x then y; the three platform and
escape calls that are height first swap there.

## Steps

- [x] **1. `coord_type`**, verified across every suite.
- [x] **2. `stratum_view/types.hpp`.**
- [x] **3. `Buffer`'s point and vector API.**
- [x] **4. `cells.cpp`** addressing the buffer directly, with the lattice mapping written once.
- [x] **5. `ascii.cpp`, `StratumWorld.cpp` and `stratum_view_test`.**
- [x] **6. `Span`.**
- [x] **7. The transitional API removed.**
- [x] **8. `Size` removed.**
