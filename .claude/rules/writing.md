# Writing

These rules apply to code names, comments, documents, commit messages, and replies. They extend the Conventions in
`CLAUDE.md`, and the wording check in `.claude/hooks/` flags the watch-list words and clipped openings in added text.

## Names

Name code so that it reads as a sentence at every use, `if (staircase_faces(zone, cell))`, and choose names when they
are introduced, not after.

- **One reading.** A name describes what the code does in a way that cannot be read as something else; accurate is not
  enough. List what else it could mean, pick a name that rules those out, and check it in every context where it is
  used. `build_error(world, document)` reads as building game objects; it checks whether the world can build from the
  document, so it is `check_buildable`.
- **The plainest name for the role.** `discrete` for a `DiscreteLayout` adds nothing, and `layout_to_populate` inside
  `populate` restates the function. Start from the plainest noun and lengthen it only to resolve a real collision, with
  the name the surrounding code already uses (`zone_layout` beside `World::layout`). Prefer a helper call inline to a
  variable.
- **One name, one meaning.** Two different objects or values never share a name, however closely related: a parent and a child, a
  container and an element, or a configured value and a derived one. `Config::working_directory` contains every game
  in progress, and one game's directory inside it is `game_directory`.
- **No technology.** A name is for the role, not the technology behind it: `raw_values_type`, not `json_type`.
  Third-party names used as they are, such as `flecs::entity`, are unaffected.
- **A function per operation.** When an existing function computes the right result only after its arguments are
  transformed, write one named for the operation: `encode_wall_id(cell_position, normal)`, not
  `wall_toward(address.grid_position, -address.normal)`.
- **Functions** are mostly verb phrases for an operation (`find_`, `create_staircase`, `extract_keys`) and often noun
  phrases for what is returned: a count (`room_count()`), a getter without `get_` (`name()`), or a derived value
  (`encoded_entity(e)`). A name describes what the function does or returns, not how the result is used: `extract_keys`, not
  `keys_to_compare`.
- **Booleans and predicates** are adjective phrases, never `is_`: `modifiable`, `restart_required`, `examined(e)`. A
  phrase that reads as the condition, such as `staircase_faces(zone, cell)`, is fine. `has_` is a last resort, after an
  adjective the domain already uses (`bound(relocator)`) or a phrase that reads as the condition.
- **Counts** take `_count`: `room_count`, not `n_rooms`.
- **Type aliases** end in `_type`, are named for their role, never for the aliased type, and are declared in the class
  whose interface uses them (`Settings::listener_id_type`). dungen aliases `math::DiscreteVector<coord_type, 2>` as
  `vector_type`, `position_type`, and `dimensions_type`.
- **Type names** have a qualifier that tells them from their siblings: `DirectionMask`, not `Mask`. A prefix that every
  type in reach shares qualifies nothing, so stratum has `Config`, not `GameConfig`. Variables and enum values stay
  unqualified.
- **`index`** is for lookup structures, such as a map of files by name. A position in a sequence is a `slot` or an `id`,
  following the surrounding code. Element identifiers use `quetzal::id_type` and `quetzal::nullid`; counts, limits, and
  loop ordinals are `size_t`.
- **Members** spell the full name of their own class (`m_surface_resources`). A third-party class's member has a name
  matching its accessor: `flecs::world m_entities` beside `entities()`.

## Comments

- **The default is no comment.** A comment that restates a name, paraphrases a signature, or narrates the next lines is
  a failure to name. Rename, extract a named value or function, or choose a clearer type first.
- **No comment repeats what the code beside it already shows,** such as a check and the message it fails with.
- **A surviving comment describes what the code is or does,** plus a genuinely unexpected caveat, at half the length of the
  first draft. It does not narrate the mechanism, the trade-off, or the debugging behind it. A predicate whose name
  cannot describe it fully gets "Returns true if ...", and a bool member "True if ...".

  > Which cell of the grid, as a coordinate rather than a stored record. The grid it sits on is recorded at load rather
  > than derived from the wall geometry afterwards

  becomes

  > The coordinates of a cell in the grid

- **A new class or struct may have a concise comment** saying what it is.
- **No comparisons, no alternatives, no callers.** "Does what `set_values` does" sends the reader to another function
  and goes stale when it changes. An alternative not taken belongs in `doc/decision_rationale.md`, and what a function is
  used for belongs at the caller.
- **No possessive "own" without its noun.** "The game's own" leaves the reader asking "own what?"; "a line of its own"
  is fine.
- **A container whose types say nothing about its contents** has the names its key and value would have:
  `// <entity_id, zone_id>` above `std::unordered_map<PersistentId, PersistentId> m_entity_zones`. An alias such as
  `using door_walls_type = std::vector<std::vector<id_type>>` has a line saying what the containers represent.
- **Existing comments are never added to or extended.** An obvious one in code being changed anyway is deleted. A
  comment ending in `...` is the author's TODO marker: the unfinished work it describes is acted on, and the comment is
  never polished, moved, or copied.

## Wording

- **Specific words.** The watch list in `CLAUDE.md` applies everywhere, and so do `rock` for a closed cell, `shape` for
  form or structure (a room's shape is fine), `step` for movement or synchronizing (a move of one cell, or a numbered
  plan step, is fine), and `states` for what a spec or a key contains. Sweep site by site, and never replace one
  watch-list word with another.
- **Plain phrasing.** Subject, verb, and object, in the order the idea runs: "Parts are entities that share the same
  parent", not "Parts are the entities whose parent refers to a whole". Say what happens, not what doesn't, with the
  simplest direct definition: "Junction [u, v] is the lower-left (SW) corner of cell [u, v]".
- **Headings and labels are noun phrases:** `## Current state`, not `## Where it is now`. An open item is a direct
  question: `**Does a wall side without a door merit an entity?**`.
- **A sentence opens with its subject:** "Mod additions are unpredictable", not "What a mod adds cannot be predicted".
  An imperative, a "whether" after its verb, and an introductory phrase ("On a new machine, run it"), are fine.
- **A document opens with its subject** and what it is, does, or contains: "A `RegionSpec` contains the keys that
  `World::create` needs ...", never "This plan ..." or "This document ...". A sentence that would start "This ..." uses
  the noun instead.
- **No padding.** No exclusion clause ("and nothing else"), no qualifier for a case that cannot arise, and the
  collective term rather than its members: "its variant value", not "the value of a point, line, ray, ...". In stratum,
  "the world" means what is loaded, so unloaded zones are never listed as exclusions.
- **US spelling and the Oxford comma** in new text: center, behavior, neighbor, gray, color, catalog, license.
- **The 150-column wrap is soft** for documents and comments, and does not apply to data files. A line runs past by a
  word or two rather than breaking before a short remainder, and existing lines are never split to fit.

## Terminology

- **A general word that collides with a domain term is replaced** by one the domain leaves free. Entity nesting in
  stratum is "nested", not "depth" or "level", which are geometric there. The same applies to edge, wall, facing, room,
  cell, zone, flat, and type in stratum, and face, loop, trace, and segment in the model suite.
- **A bare container word is the concept:** "map" is an associative container, and `std::map` is the ordered tree.
- **No coined or redefined terms.** Use the identifiers and wording of the code, or standard terms, and describe what
  the code does rather than inventing a verb for it. A new term is proposed before it is used. Vocabulary comes from
  identifiers and from what the author writes, not from comment prose.
- **Standard terms come first.** A new concept is introduced with what other libraries, games, and engines call it, and
  who uses which: "logger" (spdlog, log4j, Rust's `log::logger()`); "key" and "section" for a setting's name (Qt, ini,
  Unreal, Godot's `ConfigFile`).
- **A library type's state is described in the words of its interface:** an optional "has a value", from
  `has_value()`, and a container is "empty", from `empty()`. Jargon from proposals, papers, and implementers'
  discussions stays out, accurate or not.
- **Settled terms:**
  - "Kind" only as a last resort, after "type" or the specific relation.
  - A test executable is one program that CMake builds, and a test suite is a doctest `TEST_SUITE`, then test case and
    check (`doc/testing_plan.md`). "The tests" for the whole is fine.
  - Identical is equal byte for byte, and equivalent is equal as parsed (mesh_diff's `FileResult::identical` and
    `equivalent`). Never "by value".
  - "Random engine", never "engine" alone.
  - In stratum, `actor`, not `mover`.
  - Never `slit`, `carries over` for "stays the same", or `standing` for being at a position.

## Limits that are only today's arrangement

- **The current arrangement is not a constraint.** `doc/` is the document directory, not "stratum's documents, plus the
  other two". "Largely independent projects share this repository", not "Three largely independent projects". A count
  is a fact with an expiry date.
- **The same census in code** designs for exactly what exists: one enum value, one format, one caller. A comment that
  mentions the one current caller or the one case a branch handles today is the same mistake.
- **"That exists" is the tell.** "The only attachment that exists", "nothing uses it yet": an appeal to the current
  inventory is a reason from a census, not from the design. It is fine as a cost estimate ("this touches four call
  sites"), never as a reason to build for less, and it is never a point on the narrow side of a comparison.
- **The narrow option is never offered** as a choice: drop, skip, and defer-until-used are not alternatives.
- **Size and performance** are written as a cost per operation, with how they scale, never from the data seen so far.
- **Real constraints are still written down:** the build scope rule, `notes` files being the author's, git being
  read-only. The test is whether the statement would need editing merely because something was added elsewhere.
