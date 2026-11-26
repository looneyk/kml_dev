
## New access point proposal

**The class**

access point, one end of a translocation pair
    type: staircase, door, ladder, rope, portal, waypoint, ...
    placement (position, egress direction)
    optional directionality
    (calculated landing position)
    destination: the opposing access point of the same type, arrival at landing, identified by:
        access point id (an existing access point)
        region name (a newly created access point in a new region/worldspace, dungeon/tower/...)
        zone extended, absolute or relative name (see notes about tracking unused access points below)

**Specific usages**

staircase, door (one end, adjacent to its landing)
    placement, position is staircase block, direction is into space
    directionality - rise, vertical offset, see below
    destination

ladder, rope
    placement, position is shaft block, direction is toward wall rope hangs from or ladder resting on (may be (0, 0) for center)
    directionality - rise, vertical offset, see below
    destination

portal, waypoint
    placement, direction is up (floor) or out into space (wall)
    directionality - none
    destination

shaft
    placement, position is shaft block, direction is un
    directionality - rise, vertical offset, see below

Block notation

Interact with any access point type from the block cell position itself or an adjacent cell. Landing cells are this fixed set of position and adjacent cells.

Wall notation

|type|rise|pos|ori|egress|lpos|ingress|
|---|---|---|---|---|---|---|
|stair|down|0|1|-1|-1|1|
|stair|up|0|1|-1|-1|1|
|door|null|0|1|-1|-1|1|
|ladder|down|0|1|-1|-1|1|
|ladder|up|0|1|0|0|0|
|rope|down|0|1|-1|-1|1|
|rope|up|0|1|0|0|0|
|portal|null|0|null|0|0|0|
|shaft|down|0|null|0|0|0|
|shaft|up|0|null|0|0|0|

pos - position is the cell position of the access point, the block cell for stair/door, the shaft cell for ladder/rope, and the flat cell for portal.
ori - orientation is the horizontal component of the direction that one would walk into a staircase/door or enter a shaft from (the normal of the wall that a ladder/rope is attached to). It can be calculated using the egress direction.
egress is the direction of travel emerging from a staircase/door or shaft

landing position lpos = pos + egress
ingress = -egress

ori = -egress

Stairs in both notations are a full cell. (Changed from previous where wall notation stairs were attached to a wall)
Stair orientation is the horizontal component of the direction that one would walk into the staircase.
Door here only applies to doors with access points, normal doors do not use the access point procedure.
ladder/rope is the actual access point attached to the top and bottom of a shaft.
A shaft access point attached to the top is one way down and the bottom access point is invisible.
Numbers are x coordinates where the access point and landing are aligned along the x-axis.
Example shows values where the access point is on the right and the landing is to the left of it.
With the exception of ladder/rope where the bottom landing and the access point positions coincide.
ladder/rope are shafts with an attached access point.
Entering a shaft from the bottom is not possible without a lift/aid and/or climbing gear, or levitation.

**Notes**

enter from placement position
arrive at landing position, in opposite landing direction

destination zone coding
+|-x real value, a relative distance
+|-[n] [] integer value, a relative count of floors, levels, cells, ...
= prefix indicates an absolute distance or count

destination must agree with directionality (an up staircase cannot lead lower zone)

destination may be invisible, so it looks like a just a regular location
destination may not have a destination, one-way

stair, ladder, rope cells can be interactive or automatic
shaft/rope top cells walked on, if automatic, the rope will be used if there, otherwise fall
notification state controls whether it is interactive or automatic

transporting to a portal, they are not active until stepped off

In both notations, all access points are full block cells with the exception of wall notation doors which are attached to a wall.
Wall notation should support both wall attachment and full block cell doorways where the door is in the cell midway between the opposite walls.
The former is Grimrock style, the latter is Dungeon Master style.

When a zone/level is created, a number of down staircases wil be created. they will typically have a relative zone level offset of -1, but can be any lower value as well.
When each new zone/level is created, a number of up staircases will be created, one for each down staircase above it, and similarly for down staircases.
Entering the first staircase leading to each new zone/level will trigger its creation
So each empty zone in a dungeon will store a list of up and down staircases that it will have to link to on creation.
That would become the minimum number of staircases for the generator parameters. Any additional staircases betond that minimum would need to link to other new zones/levels.
This is written in terms of staircases, but it applies to other type of access points as well.
If unconstrained, this can create an unmanageable web of access points.


**Responses**

The proposal is mostly coherent, and two of its ideas are real improvements. The weak spots are that placement and landing duplicate what the site already encodes, that directionality and destination both say where the access point leads, and that the linking scheme builds a different world depending on which zone the player enters first.

**Strong points**
- **One type plus a signed directionality.** A single `staircase` type with a rise of +1 or -1 replaces separate up and down types joined by an arrival-type mapping. "Destination must agree with directionality" then becomes a validation rule rather than a type relation.
- **Landing as a separate concept** from where the access point is. It makes the arrival rule explicit for each type instead of special-casing walls and flat cells.

**Coherency issues**
1. **Placement duplicates the site.** Each case maps onto a site variant that already exists:
   - A staircase is a wall placement whose normal points into space. That is today's wall site, seen from the block instead of the floor cell.
   - A ladder is a wall site in the shaft cell.
   - A center rope is a ceiling flat cell.
   - A floor portal is a floor flat cell, and a door is a door facing's wall.

   Keeping placement as the entity's site avoids two sources of truth. The direction field also mixes a surface normal (portal "up") with a facing (staircase "into space"). The site already keeps these apart: a wall placement has a normal, and a flat cell has an orientation.
2. **Landing should be a rule for each type, not a stored field.** Every case in the proposal derives it from the placement. It is either the site's cell (portal, rope, ladder bottom) or the cell one step across the wall (ladder top, staircase). A data-driven rule of the form "same cell, or across the wall, facing such-and-such" keeps the existing plan's property that arrival cannot disagree with the site, and it extends to mod types. A stored landing would only be an override.
3. **Directionality and destination overlap.** A relative count of levels in directionality and a relative zone name in the destination are two ways to say "one level down". I'd split them:
   - Directionality is the physical offset.
   - The destination says which zone.
   - The agreement rule checks one against the other.

   Directionality is also written as vertical only, while zones have x, y, z worldspace positions. A passage at a zone's edge, or a door into an interior zone, has a horizontal offset. A signed worldspace offset vector would cover both.
4. **"The opposing access point of the same type" fails for one-way passages.** A pit lands you in the zone below, and there is no pit there to arrive at. A one-way passage needs either an arrival-only landing with no type, or an arrival type it can name. As written it is neither.
5. **Rope and ladder belong as attachments to the shaft, not as access point types of their own.** A shaft top with nothing in it is a pit (exit only). A rope or ladder changes the shaft from falling to climbing. This follows the component rule: cut the rope, and the same shaft becomes a fall with no change of type.
6. **Automatic versus interactive** mixes two properties:
   - Whether moving into the cell triggers the passage is a property of the access point: a pit or a slide, yes; a staircase, usually no.
   - Whether to confirm first is a player setting.

   "Notification state" sounds like the second, but the falling behavior depends on the first.
7. **"Portals are not active until stepped off"** needs no state on the actor if triggering is defined as moving into the cell. Arriving by relocation is not a move, so it never triggers.
8. **The linking scheme depends on the order zones are generated.** The minimum counts come from whichever neighbor was generated first. Suppose zone B below is first reached some other way, such as through a portal. Its up staircases are then fixed before zone A above decides its down count. So the connections, and the world, change with the order the player explores, which works against the seed integrity that `world_structure_plan.md` relies on. The fix is to decide the connection graph when the worldspace or region is created: how many access points of each type join each pair of zones. Each empty zone stores its list, and the generator places exactly those. That also answers the "unmanageable web" concern, because the graph's size is a region spec parameter instead of something that grows during play. Regions created on the fly, such as a dungeon entered from the surface, decide their own graph at creation.

If the proposal replaces the earlier sections, items 1 and 2 are where it meets `## The arrival placement`. With landing derived from the site, that section's main property carries over unchanged.


All zones in a worldspace are generated empty at once, so order is not an issue.
This is my stage 1 of getting rid of site, replacing its usage in access points first. so you site comments are mostly moot.
rope and ladder are the access points then, and we'll create a one way invisible access point for the bottom. Unless you can levitate. Directionality is neded for drawing, destination is the actual amount.
landing as a position offset is probably fine.

Agreed on site, directionality, and landing. I'll drop those comments. One point still stands, and I want to make sure the first reply was clear about it.

**Fill order, not creation order.** I agree that creating every empty zone at once removes any ordering problem in creation. My concern was the counts. Under the notes, a zone's staircase counts are chosen when its content is generated, and the neighbor generated later takes them as its minimum. Take zone A above zone B:
- If A is generated first, B gets at least as many up staircases as A chose down staircases.
- If B is generated first, A gets at least as many down staircases as B chose up staircases.

So the same seed can give different connections depending on which zone the player reaches first. That only matters if determinism across different play orders is a goal. If it is, the counts for each pair of zones can be chosen and stored when the empty zones are created. Generation then places exactly those counts.

**The rest**
- **Directionality for drawing, destination for the amount** separates them cleanly. The only link left is the check that they agree in sign.
- **Rope and ladder as access points, with an invisible one-way arrival at the bottom,** fits the "no destination" case already in the notes. The levitation exception makes the test of whether an actor can go through depend on the actor. That matches how `crossable()` and perception already work for each actor.
- **Landing as an offset** works best measured relative to the placement's direction: one cell along it for a staircase, zero for a portal. It is then the same for every instance of a type, and can be data for each type rather than a field on each access point.


The seed pool idea can be used here for the counts.

Yes, that works. Every empty zone already has its seed and its resolved `ZoneSpec`. A seed pool is drawn from the zone's seed before the rest of its generation (`world_structure_plan.md`, step 8). So a zone's access point counts can be computed from its pool without generating the zone, and either zone of a pair can compute the other's. The connections then come out the same whichever zone is filled first.

Two things make it hold:

- **One zone of each pair draws the count.** The other zone takes that count as its minimum. For example, the upper zone draws its count of staircases leading down to each lower zone, and each lower zone computes it from the upper zone's pool. If both zones drew, they could disagree. A staircase that skips levels is still a single pair, with the same owner rule.
- **The count comes only from the pool and the spec.** A count that depends on what generation produces, such as how many staircases fit, can't be computed for a zone that hasn't been filled. If space runs out, generation has to fail rather than silently place fewer.
