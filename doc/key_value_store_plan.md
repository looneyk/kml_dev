# KeyValueStore --- Plan

Date: 2026-09-29

`quetzal::KeyValueStore`, in `library/quetzal/common`, is a map from string keys to string values. It becomes the one
name/value map in `common`, and `Properties` becomes an alias of it. stratum makes its creation requests, `RegionSpec`
and `CharacterSpec`, and its generator parameters of it (`session_plan.md`, `## World`, `## Character`). The item is
**`KeyValueStore` built up** in `work_plan.md`.

## Where it is now

- **`KeyValueStore`** is an `std::unordered_map<std::string, std::string>` with `contains`, `value`, which returns a
  `std::optional<std::string>`, `value_or`, `insert` and `remove`. It is constructed from an initializer list, a span of
  pairs, or a file of `key = value` lines, with `#` and `;` comments and whitespace trimmed.
- **`Properties`** derives from it, adding `|` and `|=`, a function template `extract<T>`, and `>>` and `<<`.
- **`Config`** derives from it the same way, adding an `operator[]` marked deprecated. The Vulkan applications and
  `quetzal/game` use it. It is deprecated, and becomes an alias of `KeyValueStore` as `Properties` does.
- **The header states that the class is immutable** and a base for `Config` and `Properties`, `Settings` and `Options`.
  It has `insert` and `remove`, and only `Config` and `Properties` derive from it.

## Problems

- **`insert` has two meanings.** `insert(pair)` keeps an existing value, and `insert(key, value)` overwrites it. The
  initializer list and span constructors keep the first of two duplicate keys, and the file constructor keeps the last.
- **`a | b` keeps `a`'s value** for a key both have, since `|=` inserts without overwriting. An overlay of overrides on
  defaults conventionally takes the right side's value, as Python's `dict` union does.
- **Iteration order is unspecified.** `<<` writes the entries in a different order from one build or library to the
  next, and a loop that applies each entry in turn depends on the order: `CylinderSpecification::apply` in
  `quetzal/model` resets a termination's names on its style key, so the result of a style key and a name key together
  depends on which comes first.
- **Failures are silent.** `to_type` ignores the result of `std::from_chars`, so a value that does not parse becomes
  zero. A file that cannot be opened makes an empty store, and a line with no `=` is skipped. Nothing reports a key that
  no consumer reads, so a misspelt key does nothing.
- **`extract<T>` does not compile once it is used,** since it passes a `std::optional<std::string>` to `to_type<T>`,
  which takes a `std::string`. Nothing uses it.
- **`>>` reads nothing** and leaves the stream good.
- **`<<` writes a JSON-like object without escaping,** so a value with a `"` or a `\` writes invalid JSON. Nothing reads
  that form, and it is a second format beside the file's `key = value` lines.
- **`Properties` and `Config` are subclasses that add operators,** which free functions on one type would provide.
- **Lookup takes a `const std::string&`,** so a key given as a `std::string_view` or a literal allocates a string.
- **The source departs from the conventions:** `bFirst`, and definitions in `KeyValueStore.cpp` written both inside the
  namespace block and qualified outside it.

## Changes

- **One type.** `|`, `|=`, `<<` and `>>` become free functions on `KeyValueStore`, and `Properties` becomes an alias:

  ```cpp
  using Properties = KeyValueStore;
  ```

- **An ordered map,** `std::map<std::string, std::string, std::less<>>`, so iteration and writing are in key order on
  every platform, and a `std::string_view` key is looked up without allocating.
- **One `insert` behavior: every overload overwrites,** and the constructors keep the last of two duplicate keys, as the
  file does.
  `a | b` takes `b`'s value for a key both have.
- **Typed access that reports failure.** `value<T>(key)` returns a `std::optional<T>`, empty if the key is absent, and
  throws `std::runtime_error`, with the key and the value, if the value does not parse in full. `value_or<T>(key,
  default_value)` does the same with a default. They replace `extract<T>`, and `to_type` checks `from_chars`.
- **Unread keys found.** `find_unknown_keys(known_keys)` returns the keys that are not among `known_keys`, so a consumer
  such as `World::create` reports a misspelt or unsupported key instead of ignoring it.
- **Reading a file reports failure.** The file constructor throws `std::runtime_error` if the file cannot be opened,
  and for a line that has no `=`, with its line number.
- **One text format.** `<<` writes the file's `key = value` lines and `>>` reads them, so a store written is read back
  equal. A JSON form belongs where JSON is: `stratum_format` converts a store to and from `raw_values_type` for
  `world.json`.
- **A value is one string.** A list, an interval or a vector is a token that its consumer parses, as dungen's
  parameters are now (`"40,30"`, `"1-1"`); a list a request needs is named, and defined in data.
- **The header states what the class is,** without the claim that it is immutable, and the source follows the
  conventions.

## Users

- **`quetzal/brep`** stores a `Properties` on each face, surface, submesh, half-edge, hole and seam, and copies it. Its
  dump writes each through `<<`, so the new format changes every dump line with properties: an empty map, written `{}`
  before, is written as nothing. No checked-in model reference has such a line.
- **`quetzal/model`'s specifications** construct a `Properties` from an initializer list and apply each entry in a loop.
  With the entries in key order, a name key comes before its style key (`axis_lower_name` before `axis_lower_style`),
  and the style would reset the name. Each `apply` applies the style keys before the loop. Each `*_upper_name` of an
  end termination writes `names[0]`, since a `Butt` style gives that termination one name.
- **`Config`'s users,** the Vulkan applications and `quetzal/game`, fill it with `insert` and read it with
  `operator[]`. A string setting is read with `value_or` and an empty default, as `operator[]` read it, and a number
  with `value<T>(key).value()`, which throws if the key is absent or its value does not parse.
- **stratum** makes `RegionSpec`'s generator parameters and region choices, and `CharacterSpec`, of `KeyValueStore`.
  The generator registry's functions take the parameters as a `const KeyValueStore&`.
