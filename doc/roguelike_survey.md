# Rooms and Corridors Across Roguelikes --- Survey

Status: reference, from general knowledge of these games rather than from their source
Date: 2026-09-08

How other roguelikes notate rooms and corridors, how they decide which a cell is, and what identity
a room carries once built. Gathered because stratum's level formats encode the distinction and its
loader reconstructs rooms from it, and because the same decision is about to be made a second time
for the wall format. `loader_plan.md` covers the structures that do the reconstructing.

Confidence varies by row. The Rogue and NetHack entries and the Angband room flag are solid; the
ToME and Qud entries are the weakest and are marked where they are inferred.

## Terms

Words the genre uses for these things, several of which collide with each other or with stratum's
own vocabulary.

**Room-and-corridor** --- generation that places discrete rooms and then digs passages between them,
as against **cave** or **cellular** generation, which carves open space out of rock and has no room
concept to begin with. The distinction under survey here only exists in the first kind.

**Corridor**, also **passage** --- floor connecting rooms, one or two cells wide, not part of any
room. Rogue's `#` glyph is called a passage. Note that stratum's `Passage` means something else
entirely: a place where two areas meet and a mover may cross between them, which in these games
would be a doorway.

**Vault** --- and this word carries two unrelated meanings:

- In Angband, DCSS and their descendants, a vault is authored content placed into a generated
  level: a fixed layout, written as an ASCII template with its own monsters and objects, usually
  more dangerous and better rewarded than the level around it. The generator treats the region as
  owned and will not dig through it. DCSS writes them in `.des` files, a Lua and ASCII DSL, and they
  range from a few decorative cells to an entire level. Angband grades them lesser and greater by
  size and danger.
- In NetHack, a vault is a specific level feature: a closed 2 by 2 room full of gold with no door,
  reachable only by digging or teleport, patrolled by a vault guard. Nothing to do with authored
  content.

**Machine** --- Brogue's term for a self-contained arrangement of terrain, items and monsters built
into a level after the base layout is dug: a reward room whose item is guarded, a flooded room with
a lever, a captive monster in a cage. A machine is instantiated from a **blueprint** by rules rather
than stamped from a fixed map, which is what separates it from a vault. Cells carry a machine number
so the game can ask which machine a cell belongs to.

**Subroom** --- NetHack's term for a room inside another room, such as a shop's back closet.

**Lit room** --- a room the player sees the whole of on entering, as against a dark one revealed a
cell at a time. A per-room property in the Rogue lineage and absent from games with a real light
model.

**Zone** --- Caves of Qud's term for one 80 by 25 chunk of the world, the unit that is generated and
loaded. stratum uses the same word for an independently loadable area, which is the same idea; it is
not a room in either.

## The survey

| Game | Corridor vs room on screen | How the distinction is stored | Room identity | Lighting |
|---|---|---|---|---|
| **Rogue** | `.` room, `#` corridor | `rooms[]`, nine slots in a 3 by 3 arrangement | flags only --- dark, maze, gone | whole room lit or dark on entry; corridors always dark |
| **NetHack** | `.` room, `#` corridor | `rooms[]` table **and** a room number per grid; distinct room and corridor terrain types | rich types --- shop, temple, zoo, morgue, barracks, throne room --- plus subrooms | `rlit` per room; a lit room reveals entirely on entry |
| **Angband** | `.` for both | a per-grid room flag, nothing else | none; vaults get a flag of their own | room lit by flood fill over the flag |
| **DCSS** | `.` for both | nothing for ordinary rooms; a vault mask per square | vaults only, named and tagged in `.des` | line of sight; no room lighting |
| **Brogue** | `.` for both | nothing for ordinary rooms; a machine number per cell | machines only | physical light model; no room rule |
| **ToME 4** | `.` for both, tiles by default | rooms are a generator-side concept; the grid keeps terrain *(inferred)* | vaults and greater vaults | lite radius, inherited from Angband |
| **Caves of Qud** | `.` for both, tiles by default | zone builders; ruins do carve rooms *(inferred)* | villages, sites and named structures, at and above zone scale | light radius |
| **stratum** | `' '` room, `'.'` corridor in the block format file; view glyphs are chosen separately | per-cell flag in the file; `Room` objects and `Cell::room_id` after load | `Room` with outline, cells, `zone_id` and `entity_id`; no type | none |

## Three axes, and they are independent

**Notation.** Distinguishing corridor from room floor on screen is a Rogue-lineage trait and nothing
else does it. Both games that do use `#` for the corridor and `.` for the room. Blank is solid rock
or unseen everywhere in the genre without exception, which is the one convention that is close to
universal.

**Determination.** Two settled answers. Keep what generation knew, as Rogue and NetHack do, since
the generator made a room and can simply record it. Or discard it and keep a per-cell flag, as
Angband does, recovering a region by flood fill whenever one is needed --- room lighting being the
case that needs it. Both work; they differ in what can be asked afterward.

**Identity.** Every game that discards room records still keeps identity for the rooms that carry
meaning: Angband and DCSS for vaults, Brogue for machines. What varies is where the line falls
between a room that is merely a shape and a room that is a thing. NetHack draws it furthest toward
"everything is a thing", giving ordinary rooms a type as well as shops and temples.

Qud belongs on the identity axis rather than the notation one. Its structures --- a village with
named inhabitants and a history, a named site --- are the equivalent of a NetHack room type in the
sense that matters, a region that is a thing rather than a shape. The differences are scale and
authorship: identity attaches at zone scale or above rather than to a rectangle of floor, and it
comes with generated history rather than a type tag. So it answers the identity question and has no
position on the corridor question, its zones not being built from rooms and passages.

## Where stratum sits

In neither camp on determination. The block format stores Angband's per-cell flag, and
`create_rooms` reconstructs NetHack's structure from it by flood fill --- `Room` objects with ids,
`Cell::room_id` being the room number equivalent. It is the recovery technique feeding the record
design.

That works while a room is only a shape, and its limits follow from the flag:

- Two rooms meeting with no wall between them come back as one room. A flag cannot say which.
- No room can carry a type, a name or anything else from the file, because there is nowhere in the
  format to write one. `Room` has a `zone_id` and an `entity_id` for exactly that kind of thing, and
  nothing can currently fill them from a level file.
- The wall format has one open-cell symbol and so states nothing about rooms at all.

stratum's `Room` is already more general than NetHack's in one respect: an outline and a cell list,
with no requirement that a room be one rectangle or even one contiguous run.

## What this bears on

The three axes are independent in the genre but are already coupled here. `Format::Block` and
`Format::Wall` name which format a file was written in, and a host picks its view from that ---
`Application` selects the cell view or the wall view on exactly that test. So format, what the
layout can express, and how it is drawn already move together.

Room identity is the decision the survey does not settle, because the genre does not settle it:
recording rooms at generation and reading them back, or flagging cells and recovering regions, are
both current practice in shipped games. What it does settle is that they are the only two answers,
and that every game choosing the second keeps records anyway for the rooms that mean something.
