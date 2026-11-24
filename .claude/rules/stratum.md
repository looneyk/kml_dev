---
paths:
  - "library/quetzal/stratum_*/**"
  - "library/quetzal/{dungen,catagen,godot}/**"
  - "application/stratum_*/**"
  - "tool/{dungen,catagen}/**"
  - "test/{stratum,dungen,catagen}*/**"
  - "doc/game_system_*.md"
---

# Stratum

These rules apply to the stratum game system, its views, its zone format, and its generators. `doc/game_system_reference.md`
describes what is built, and `doc/game_system_principles.md` the principles.

## Design

- **Every game supports mods and custom content:** objects, geometry, styles, and behavior, added as data and script
  without recompiling and with no vocabulary known in advance. No closed enums, C++-only registration, or hardcoded type
  cases for content; `FeatureType` is a known blocker. JSON stops at loading (`ordered_json` in `stratum_format::ZoneDocument`),
  and components are typed in the world with schemas from C++ or data (principle 8, `doc/modding_plan.md`).
- **A component encodes the state, never an interpretation of it.** A smashed door is `Access` open, off its hinges, or
  closed, a blocking pile of rubble; there is no `Crossable` tag. A conclusion such as passable or visible is derived in
  a query, `crossable()`, and perception is per actor: the view draws the player's, and each denizen acts on its own perception.
- **Scale is several orders of magnitude beyond the tree.** The checked-in zones, references, and tests are test-stage
  data, so performance, memory, and data structures are judged against far larger worlds, with costs per operation.
- **An access point's destination is not checked against its `Directionality`.** Agreement is the designer's
  responsibility, and leaving it unchecked allows non-Euclidean layouts.
- **A new game is wholly generated.** The host calls `Game::create` with the starting specs. A host loading a zone file
  is scaffolding until generation is complete, not a design case, and loading saved zones is a separate flow.
- **`ContinuousLayout` is separate from `DiscreteLayout`** because the two will work differently. Nothing done to
  `DiscreteLayout` is applied to `ContinuousLayout` or its doc text, and `ContinuousLayout` is not changed for now.
- **`procgen` is a general library,** `quetzal::procgen`, not `stratum_procgen`, and is never designed against
  `stratum_sim` types. Its passes, seeding, and validation are parameterized over what they produce, and roguelike terms
  such as "vault", "room", and "biome" are one application's.
- **The `stratum_` prefix marks what is bound to the layout model,** regions extruded vertically with one floor and one
  ceiling each, the sector model of the Doom lineage. Broadly useful libraries, such as `procgen`, `content`, and `godot`,
  have no prefix. `stratum_pattern` is the game's working title, and `pattern` is a placeholder that is not used
  in new names.

## Zone files

- **A zone file is machine-written,** by generators, tools, and saves, and is already longer and more complex than
  anyone writes by hand. Ease of hand-writing is never a reason for a format choice, an id scheme, nesting, or a key
  name, and readability as text is limited to inspecting a file or a diff. The uniform form, one entities list with
  relationships as components, is preferred to a nested or readable one.
- **The JSON entities are the source of truth.** The staircase glyphs `<` and `>` are visual aids: nothing semantic, such
  as an access point's type or order, is derived from them, since `<` is used for staircases up, waypoints, and portals
  alike.
- **A format's version changes only for a structural change.** A new field, preferably optional with a defined meaning
  when absent, is part of the current version, and so are regenerated references. A format change goes into the version
  in development unless it is independent of it.
- **An intended change to generated output,** such as the dungen and catagen reference zones or the view frames, that
  passes the test executable's structural checks is accepted by rerunning after the last code change, copying `out/` over
  `reference/`, and rerunning to confirm. Output that changed for no intended reason is a finding to raise.

## Terms

- **Zone, not level:** a zone file, which is a template or a saved zone. A zone's level is a signed index from the surface,
  0 at the surface, -1, -2 going down, and 1, 2 going up, shown to players as an absolute value. Level is metadata on the
  zone, never an object or entity, and nothing is keyed by it; a region may have many zones at one level. Depth is
  separate metadata and unused. Established identifiers such as `level_down` stay.
- **Denizens** are a zone's population. "Monster" is only a specifically hostile creature used as an example.
- **Staircase,** in code names, comments, and docs. "Stairs" stays in command-line option tags (`--stairs-down`) and
  their zone file parameter tokens (`stairs_down`).
- **Access point,** not entry or relocator. An access point is the component that moves an actor to its destination, not an
  entity type. A staircase, a portal, and a waypoint are entities with an access point, and entity `type` values are
  prefabs in `World`. "Portal" is only the non-ordinary or instantaneous type. Checks look for the access point
  component, not for a staircase. Every zone has at least one, listed in `layout.components.access_points`. An access
  point has its type, an order counting from 0 among the access points of that type, an egress, and an
  optional destination. A staircase down can be a tower's entrance, entered from below.
- **A staircase is sited on the floor of its cell** with an orientation: a floor cell in the block notation, or in the
  wall notation a stair block, a closed cell written `<` or `>`, with an egress toward the floor it opens
  onto. `check_access_points` in `stratum_format/read_zone.cpp` and the generators' `write_zone.cpp` are the source of
  truth.
- **Edge and wall.** An edge is the zone file's one glyph for both sides, and a wall is one side. An actor interacts with
  the wall on its side, and the wall on the other side is separate.
- **Doors** are an entity list in the zone with the shared state (open, locked, ...). A door has two door facings as
  children and links to them; each door facing links to its wall, whose id supplies the normal, and is that wall's
  attachment, so nothing searches `OnWall`. A door facing contains what is accessible from its side, such as a keyhole.
  `attachment_plan.md`, `sim_zone_model.md`, and `game_system_principles.md` are checked before proposing entity
  structure for walls, doors, or attachments.
- **"Facing" is always qualified:** a door facing (`DoorFacing`), or a facing direction, the direction of a staircase's
  or an actor's placement, written `orientation` in a file. "Toward" is the verb ("a wall toward the cell it bounds").
- **Orientation** is the directional property. **Child,** not part: a part is one type of child.
- **Component** is the flecs ECS component; a "file component" and a "code component" are qualified.
- **A door is not `fills`;** `relationship` is written only where it distinguishes, as `contained_in` from `on`.
- **A cell is not a cell position.** A cell is the square of the grid, and its cell position `[i, j]` is where it is.
  Every key and code name for one is `cell_position`.
- **A wall placement is a cell position and a unit normal,** kept separate, `{ "cell_position": [i, j], "normal": [ni, nj] }`,
  and combined only by `encode_wall_id(cell_position, normal)`. `wall_toward` is the movement query.
- **Junction `[u, v]`** is the lower-left (SW) corner of the cell at `[u, v]`.

## Views

- **Wall notation in a cell view** (the terminal's cell view, Godot's tile view) is allowed and never rejected, but only
  for a zone that a block notation zone could have been converted to. Walls or doors on a cell edge, rooms back to back,
  are not drawn reliably there and are not bugs, and the combination is the lowest priority.
