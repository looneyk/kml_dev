# Layout properties plan

The rooms, walls, flats, flats' overrides for rooms, and flat cells of a `DiscreteLayout` have their properties in
tables in the layout, keyed by their layout ids. A layout part has no entity. The ECS is for actors and items: many
objects with varying sets of components, updated by systems each turn. Geometry is dense, changes little during play,
and is already identified by the layout, so an entity for each layout part adds a second identity and the code to keep
the two consistent.

The zone file stays data-driven. The components written for a room, a wall, a flat, and a flat's override for a room
or a cell are read into that part's properties, and a mod adds properties, behavior, textures, and models as data and
script without recompiling.

## Precedent

- **Quake `.map` and Hammer.** Geometry is brushes, and an entity is a list of key-value pairs: `classname`, `target`,
  `targetname`, `model`. The `classname` selects the code that runs it.
- **Doom.** A linedef has a special and a tag, and a sector has a special. A door is a sector with a special, and its
  open or closed state is kept with the sector.
- **Source I/O.** A map object has outputs, such as `OnOpen` and `OnTrigger`, each connected to an input of a
  target object, so behavior is wired up in data.
- **Tiled, LDtk, and Godot metadata.** A tile, a layer, a level, or an object has custom properties as a typed
  key-value list.

## Property tables

| Table | Keyed by | Entries |
| --- | --- | --- |
| Rooms | Room id | One per room |
| Flats | Flat id | The floor, and the ceiling if there is one |
| A flat's overrides for rooms | Flat id and room id | One per room with an override |
| Walls | Wall id | One per wall with properties |
| Flat cells | Flat id and cell position | One per cell with properties |

`layout.wall_properties(wall_id)` and its counterparts for the other tables return the entry, or an empty optional if
there is none. An entry contains:

- **Typed fields that the engine reads,** such as `style`, `terrain`, a flat's `height`, and the texture and model keys.
- **A key-value map of the other properties,** from a property's name to its value. A value is typed by a schema
  registered from C++ or from data, so a mod adds properties without recompiling. Scripts and hooks read these
  properties, and the engine reads none of them.
- **Hooks.**

## Inheritance

A wall or a flat cell inherits from the room that contains it, and from the layout where no room contains it. A flat's
values win over the room's or the layout's at each level, as `discrete_layout.md` (`### Inheritance`) specifies.
`resolved_wall_properties(wall_id)` and its counterparts merge an entry with the entries above it along that chain,
field by field.

## Hooks

A hook binds an event to a handler: a Lua function run in the `ScriptSandbox`, or a C++ function registered under a
name. The zone file and mods bind handlers by name. The events of a layout part include an actor entering a room or a
cell, crossing a wall, and interacting with a wall or a cell.

## Assets

A texture or a model in an entry is a key into an asset catalog, which maps the key to files. A mod replaces a
catalog entry to change every use of it, or an entry's key to change one layout part.

## Entities fixed to walls and flat cells

An entity fixed to a wall, such as a door facing or a torch, has the wall's id, its relation to the wall (`in` or
`on`), and its placement on the wall's surface. An entity fixed to a flat cell, such as a staircase or a shaft, has the
flat, the cell position, its relation, and its placement. The zone has a map from each wall id, and from each flat
cell, to the entities fixed to it, beside the zone's proximity index. Crossing a wall and facing a wall look up the
entities fixed to it through that map.

## Zones

A zone is not an entity either. Its record in its worldspace, a `stratum_core::Zone`, contains its persistent id,
position, and name, with its zone spec, seed, and layout, and is the one source of truth for them. A region and every
other structure that lists zones has only zone ids. The zone file and `world.json` are both written from the record.

Each entity in a zone has its zone's persistent id, as `InZone` does for an entity owned by the game, and `World` keeps
an index from each zone id to the entities in that zone. `zone_of` reads that one component, a zone's contents are found
through the index, and unloading a zone deletes the entities the index lists for it.

## Removed

- `LayoutEntity`, `EntityIndex<T>`, `layout_entities.hpp`, `for_each_layout_entity`, and `AttachedEntities`.
- The zone entity, with its `Zone` tag and its `RegionId`, `WorldspaceId`, and `zone_position_type` components.
- The `RoomId`, `WallId`, `FlatId`, and `FlatCellPosition` components. The layout ids remain as plain values.
- The `property<T>` overloads for layout parts, replaced by resolution over the tables.
- The exclusions of layout entities from persistent ids, saving, and unloading.

## Saving

A saved zone writes each table's current entries as the components of its `rooms`, `walls`, and `flats`, where the
loader reads them. The zone file format does not change.

## Open questions

- **Is a door a layout part with an entry, or an entity?**
- **Is a staircase, a shaft, or a portal an entity with an access point, or fields and hooks in a wall's or a flat
  cell's entry?**
- **Is inheritance resolved at load and stored in each entry, or merged on each read?**
- **What is the value type of the key-value map?**
- **Is a container entity for each zone's contents more efficient than a zone id on each entity?** An entity with no
  data, as the flecs parent of the zone's contents, lets flecs group the contents by zone and delete them in one call
  when the zone unloads. A zone id on each entity needs `World`'s index for both.
