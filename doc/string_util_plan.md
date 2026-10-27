# string_util --- Plan

Date: 2026-10-25

This document describes how format specifications used in `std::format`, custom stream manipulators, and `to_chars` and
`from_chars` can be expanded to support custom delimiters for array, series, and composite geometry types. Parts of this may be
implemented in `quetzal/common/string_util` as well as the array, series, and composite geometry types themselves.

## Overview

Array types (composite types consisting of a series of scalar elements)
    quetzal::math::DiscreteVector
    quetzal::math::Vector
    quetzal::geometry::DiscretePoint
    quetzal::geometry::Point
Series types (composite types consisting of a series of a single array element type)
    quetzal::math::Interval (a series of two scalars)
    quetzal::geometry::AxisAlignedBoundingBox
    quetzal::geometry::Box
    quetzal::geometry::DiscreteBox
    quetzal::geometry::Polygon
    quetzal::geometry::Segment
Composite geometry types (composite types consisting of a combination of different array and/or scalar element types)
    quetzal::geometry::BoundingSphere
    quetzal::geometry::Circle
    quetzal::geometry::DirectedPoint
    quetzal::geometry::DiscreteDirectedPoint
    quetzal::geometry::HalfPlane
    quetzal::geometry::HalfSpace
    quetzal::geometry::Line
    quetzal::geometry::Orientation
    quetzal::geometry::OrientedPoint
    quetzal::geometry::Plane
    quetzal::geometry::Ray
    quetzal::geometry::Sphere
Composite composite geometry types (composite types consisting of an arbitrary combination of other geometry element types)
    quetzal::geometry::Intersection

## Settings

The text of a value has a coordinate form and up to four delimiters:

| Prefix | Setting | Effect |
| --- | --- | --- |
| `/h` | Homogeneous coordinate form | A homogeneous type writes and reads every stored value, the weight included |
| `/s` | Separator | Between an array type's elements |
| `/b` | Opening and closing bracket | Two single characters, before and after an array type's elements |
| `/o` | Opening bracket | Before an array type's elements |
| `/c` | Closing bracket | After an array type's elements |
| `/n` | Series delimiter | Between the elements of a series type: a lower and an upper bound or a series of points |

The separator and the brackets take their names from C++23's `std::range_formatter`, whose `set_separator(separator)`
and `set_brackets(opening, closing)` set them. The standard has no counterpart to the series delimiter. The opening and
closing brackets are set separately; the closing one is not derived from the opening one.

The default series delimiter is `", "`. `AxisAlignedBoundingBox`, `Box`, `DiscreteBox`, and `quetzal::math::Interval` override it with
`" .. "`, and every other type keeps it. For now, the composite geometry types and `Intersection` also write the series
delimiter between their components. A stream has no series delimiter until `series_delim` sets one, and then it overrides
the type's default.

The Cartesian coordinate form is the default. A homogeneous type then writes a point's components divided by its weight
and reads the components with the weight set to 1, or a vector's components with the weight set to 0. A Cartesian type
writes and reads the same values in either form.

`FormatSpec` is the struct of these settings. Each path sets them the same way: a format spec's prefixes, a stream's
manipulators, and the members of the `FormatSpec` argument of `to_chars` and `from_chars`. Every member is optional, and
a member that is not set selects the type's own default, which `resolved_homogeneous`, `resolved_separator`,
`resolved_brackets`, and `resolved_series_delimiter` apply. A formatter base takes a type's defaults as a `FormatSpec`
in its constructor.

## Format spec

A spec that begins with `/` has a delimiter part, which ends at the first unescaped `:`, or at the end of the spec, and
the element type's own spec follows the `:`. A spec that does not begin with `/` is the element type's spec alone, so
`{:.2f}` is unchanged, and a spec that begins with `:` has an empty delimiter part. The standard range formatter
introduces its element spec with a `:` the same way (`{::x}`).

The delimiter part is a sequence of components, each a prefix from the table above followed by its delimiter. `/h` has
no delimiter, so text after it is an error. `/b` is followed by exactly two characters, the opening and the closing
bracket, so `/b[]` is `/o[/c]`.

- A delimiter runs from its prefix to the next unescaped `/` or `:`, or to the end of the spec. Every character in it
  is literal, spaces included, so in `/s, /o[` the separator is `", "`.
- A delimiter may be empty.
- The components are in any order. A component that is left out keeps its default, and a setting given twice, as by a
  repeated prefix or by `/b` with `/o` or `/c`, is an error.
- A type ignores the components that do not apply to it, so an array type ignores `/n` and a Cartesian type ignores
  `/h`. A series type passes the coordinate form, the separator, and the brackets to its elements.
- `/` followed by a character other than a prefix letter is that character: `//` is `/`, `/:` is `:`, `/{` is `{`, and
  `/}` is `}`.

For example, `{:/s, /o[/c]:.2f}` writes `[1.50, 2.25, 3.00]`, and `{:/n..:}` writes a bounding box as
`1 2 3..4 5 6`.

A malformed delimiter part, one with an unknown prefix, a setting given twice, text after `/h`, text after `/b` that is
not two characters, or a `/` at its end, throws `std::format_error`.

`operator<<` sets the stream's `FormatSpec`, which `stream_format_spec` returns, on the formatter directly with
`set_format_spec`, so a stream writes any delimiter without prefixes or escapes.

## Stream manipulators

`cartesian` and `homogeneous` set the coordinate form, as `std::dec` and `std::hex` set a base. `separator_delim` takes
the separator, `brackets_delim` the opening and the closing bracket, and `series_delim` the series delimiter. The
manipulators and the formatters' `set_separator`, `set_brackets`, and `set_series_delimiter` each take a delimiter as a
literal string. Each manipulator sets the same state on an input and an output stream.

In the code, the declarations, members, and parameters for the settings are in order of scope, smallest first:
homogeneous, separator, brackets, series.

## Work

- [x] **Conversions for `Polygon` and the composite types.** `Polygon`, each composite geometry type, and `Intersection`
      write the series delimiter between their elements or components, and each has stream extraction, `to_chars`, and
      `from_chars` as well as its formatter and stream insertion.
- [x] **An intersection's value in text.** `Intersection`'s stream extraction, `to_chars`, and `from_chars` are implemented.
      The locus and its (virtual) variant value are formatted as a series, and the value may itself be an embedded series
      if it is a composite type.
- [x] **`DiscreteBox` as a series type.** `DiscreteBox` writes its lower and upper bounds with the series delimiter
      between them, `" .. "` by default, and reads them the same way.
- [x] **The homogeneous weight in text.** `cartesian` and `homogeneous` stream manipulators set the coordinate form of
      the text for both reading and writing, with a matching format-spec prefix and `to_chars` and `from_chars`
      parameter, ignored for Cartesian types. `cartesian`, the default, writes a point's components divided by its
      weight and reads the components with the weight set to 1, or a vector's components with the weight set to 0.
      `homogeneous` writes and reads every stored value, the weight included, so a weight that is not exactly 1 or 0
      is visible.
- [x] **A `/b` prefix for a pair of single-character brackets.** `/b[]` sets the opening bracket to `[` and the closing
      bracket to `]`.

## Types without full conversions

These math and geometry types have no formatter, `to_chars`, or `from_chars`. They are not part of the current work.

| Type | Current conversions |
| --- | --- |
| `quetzal::geometry::Polyline` | `<<` writes the vertices with the vector inserter, and there is no `>>`. It is a series type. |
| `quetzal::geometry::PolygonWithHoles` | None. It is a polygon and a series of hole polygons. |
| `quetzal::geometry::Path` | None. It is an alias of `std::vector<OrientedPoint<Traits>>`, so it has no type of its own to convert. |
| `quetzal::geometry::Curve` | None. It is an abstract parametric curve, and `CurveNURBS` derives from it. |
| `quetzal::geometry::Points` | `<<` writes a vector of points, one per line. |
| `quetzal::geometry::BoundingVolume` | `<<` calls the virtual `print`. It is the abstract base of `AxisAlignedBoundingBox` and `BoundingSphere`. |
| `quetzal::geometry::Attributes` | `<<` only, for `Position`, `PositionNormal`, `PositionNormalColor`, and `PositionNormalTexture`. They are composite types. |
| `quetzal::math::Quaternion` | `<<` and `>>`, as four numbers separated by spaces. It is an array type. |
| `quetzal::math::Rotation` | `<<` and `>>`, as the axis and then the angle. It is a composite type. |
| `quetzal::math::Matrix` | `<<` only, with a tab between columns and a newline after each row. Its rows need a delimiter of their own. |
