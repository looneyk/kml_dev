# Rooms and Corridors Across Roguelikes --- Survey

Status: reference, from general knowledge of these games rather than from their source
Date: 2026-09-08, revised 2026-09-22

This survey covers how other roguelikes notate rooms and corridors, how they decide whether a cell belongs to a room or
a corridor, and what identity a room has once it is built.

Confidence varies by row. The Rogue and NetHack entries and the Angband room flag are solid; the ADOM, ToME and Qud
entries are the weakest and are marked where they are inferred.

## Terms

These are the words the genre uses, several of which collide with each other or with stratum's own vocabulary.

**Room-and-corridor** --- generation that places discrete rooms and then digs passages between them, as against
**cave** or **cellular** generation, which carves open space out of solid ground and has no room concept to begin
with. The distinction under survey here exists only in the first kind.

**Corridor**, also **passage** --- floor connecting rooms, one or two cells wide, not part of any room. Rogue's `#`
glyph is called a passage.

**Vault** --- a word with two unrelated meanings:

- In Angband, DCSS and their descendants, a vault is authored content placed into a generated level: a fixed layout,
  written as an ASCII template with its own monsters and objects, usually more dangerous and better rewarded than the
  level around it. The generator treats the region as owned and will not dig through it. DCSS writes them in `.des`
  files, a Lua and ASCII DSL, and they range from a few decorative cells to an entire level. Angband grades them lesser
  and greater by size and danger.
- In NetHack, a vault is a specific level feature: a closed 2 by 2 room full of gold with no door, reachable only by
  digging or teleport, patrolled by a vault guard.

**Machine** --- Brogue's term for a self-contained arrangement of terrain, items and monsters built into a level after
the base layout is dug: a reward room whose item is guarded, a flooded room with a lever, a captive monster in a cage. A
machine is instantiated from a **blueprint** by rules rather than stamped from a fixed map, which is what separates it
from a vault. Each cell has a machine number, so the game can ask which machine a cell belongs to.

**Subroom** --- NetHack's term for a room inside another room, such as a shop's back closet.

**Lit room** --- a room the player sees the whole of on entering, as against a dark one revealed a cell at a time. A
per-room property in the Rogue lineage and absent from games with a real light model.

**Zone** --- Caves of Qud's term for one 80 by 25 chunk of the world, the unit that is generated and loaded. stratum
uses the same word for an independently loadable area, which is the same idea; it is not a room in either.

## The survey

| Game | Corridor vs room on screen | How the distinction is stored | Room identity | Lighting |
|---|---|---|---|---|
| **Rogue** | `.` room, `#` corridor | `rooms[]`, nine slots in a 3 by 3 arrangement | flags only --- dark, maze, gone | whole room lit or dark on entry; corridors always dark |
| **NetHack** | `.` room, `#` corridor | `rooms[]` table **and** a room number per grid; distinct room and corridor terrain types | rich types --- shop, temple, zoo, morgue, barracks, throne room --- plus subrooms | `rlit` per room; a lit room reveals entirely on entry |
| **Angband** | `.` for both | a per-grid room flag, nothing else | none; vaults get a flag of their own | room lit by flood fill over the flag |
| **DCSS** | `.` for both | nothing for ordinary rooms; a vault mask per square | vaults only, named and tagged in `.des` | line of sight; no room lighting |
| **Brogue** | `.` for both | nothing for ordinary rooms; a machine number per cell | machines only | physical light model; no room rule |
| **ADOM** | `.` for both *(uncertain)* | generated rooms have a kind; whole levels are authored outright | special room types --- shop, kitchen, treasure chamber, fungus and swamp rooms, big room | dark rooms and dark levels; light sources matter |
| **ToME 4** | `.` for both, tiles by default | rooms are a generator-side concept; the grid keeps terrain *(inferred)* | vaults and greater vaults | lite radius, inherited from Angband |
| **Caves of Qud** | `.` for both, tiles by default | zone builders; ruins do carve rooms *(inferred)* | villages, sites and named structures, at and above zone scale | light radius |
| **stratum** | `.` for both; view glyphs are chosen separately | `rooms` in the zone file; `Cell::room_id` after load | a room id, with components from the file such as a name and a style | none |

## Three axes, and they are independent

**Notation.** Distinguishing corridor from room floor on screen is a Rogue-lineage trait and nothing else does it. Both
games that do use `#` for the corridor and `.` for the room. Blank is solid or unseen everywhere in the genre, the one
convention that is close to universal.

**Determination.** Two settled answers. Keep what generation knew, as Rogue and NetHack do, since the generator made a
room and can store it. Or discard it and keep a per-cell flag, as Angband does, recovering a region by flood fill
whenever one is needed --- room lighting being the case that needs it. Both work; they differ in what can be asked
afterward.

**Identity.** Every game that discards room records still keeps identity for the rooms that carry meaning: Angband and
DCSS for vaults, Brogue for machines. What varies is where the line falls between a room that is merely an area and a
room with identity. NetHack and ADOM draw it furthest toward identity for all, giving ordinary rooms a kind as well as
shops and temples.

### Persistence decides determination

Whether a level survives being left tracks closely with whether room records are kept.

Angband keeps the least identity and is also the game that discards the level: leave and return and it is generated
afresh, so nothing is lost by discarding what generation knew. NetHack and ADOM both persist levels and both keep room
tables. DCSS and Brogue persist levels too, and keep records for the regions that carry meaning while letting ordinary
rooms go.

Flood fill recovers a region from the level as it is now, not as it was generated. Against a level that will never be
seen again that is free. Against a persistent level a player digs, burns and blasts through, it is not: two rooms become
one the moment a hole is knocked between them, and no recovery can tell that they were ever two. A room stored at
generation survives what happens to the terrain afterward; a flag does not.

Qud belongs on the identity axis rather than the notation one. Its structures --- a village with named inhabitants and
a history, a named site --- are the equivalent of a NetHack room type in the sense that matters, a region with identity
rather than an area. The differences are scale and authorship: identity is at zone scale or above rather than on a
rectangle of floor, and it comes with generated history rather than a type tag. So it answers the identity question and
has no position on the corridor question, its zones not being built from rooms and passages.

Recording rooms at generation and reading them back, or flagging cells and recovering regions, are the only two answers
in shipped games. Every game choosing the second keeps records anyway for the rooms that mean something, and the games
choosing it are the ones whose levels do not have to survive being left.

## Where stratum is

stratum keeps what generation knew. A zone file states each room, by perimeter or by a cell in it with barriers closing
the fill (`discrete_layout.md`, `## rooms`), with components such as a name and a style, and loading sets `room_id` on
each of its cells. Two rooms meeting with no wall between them stay two, and a room need not be a rectangle.
