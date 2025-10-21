//------------------------------------------------------------------------------
// roguelike
// main.cpp
//------------------------------------------------------------------------------

#include <time.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <cassert>

using namespace std;

#define PL_NSIZ 32 // name of player, ghost, shopkeeper
#define MAXULEV 30 // max character experience level

// size of terminal screen is (at least) (ROWNO+3) by COLNO
#define COLNO 80
#define ROWNO 21

#define MAXNROFROOMS 40 // max number of rooms per level
#define MAX_SUBROOMS 24 // max # of subrooms in a given room
#define DOORMAX 120     // max number of doors per level

#define LEFT 1
#define H_LEFT 2
#define CENTER 3
#define H_RIGHT 4
#define RIGHT 5

#define TOP 1
#define BOTTOM 5

#define SIZE(x) (int)(sizeof(x) / sizeof(x[0]))

typedef struct nhcoord {
    int x, y;
} coord;

#define MAXRECT 50
#define XLIM 4
#define YLIM 3

typedef struct nhrect {
    int lx, ly;
    int hx, hy;
} NhRect;

#include "dungeon.h"
#include "mkroom.h"
#include "rm.h"
#include "trap.h"

d_level s_nLevel;

struct dgn_topology { // special dungeon levels for speed
    d_level d_oracle_level;
    d_level d_bigroom_level; // unused
    d_level d_rogue_level;
    d_level d_medusa_level;
    d_level d_stronghold_level;
    d_level d_valley_level;
    d_level d_wiz1_level;
    d_level d_wiz2_level;
    d_level d_wiz3_level;
    d_level d_juiblex_level;
    d_level d_orcus_level;
    d_level d_baalzebub_level; // unused
    d_level d_asmodeus_level;  // unused
    d_level d_portal_level;    // only in goto_level() [do.c]
    d_level d_sanctum_level;
    d_level d_earth_level;
    d_level d_water_level;
    d_level d_fire_level;
    d_level d_air_level;
    d_level d_astral_level;
    int d_tower_dnum;
    int d_mines_dnum, d_quest_dnum;
    d_level d_qstart_level, d_qlocate_level, d_nemesis_level;
    d_level d_knox_level;
    d_level d_mineend_level;
    d_level d_sokoend_level;
} dungeon_topology;
#define oracle_level            (dungeon_topology.d_oracle_level)
#define bigroom_level           (dungeon_topology.d_bigroom_level)
#define rogue_level             (dungeon_topology.d_rogue_level)
#define medusa_level            (dungeon_topology.d_medusa_level)
#define stronghold_level        (dungeon_topology.d_stronghold_level)
#define valley_level            (dungeon_topology.d_valley_level)
#define wiz1_level              (dungeon_topology.d_wiz1_level)
#define wiz2_level              (dungeon_topology.d_wiz2_level)
#define wiz3_level              (dungeon_topology.d_wiz3_level)
#define juiblex_level           (dungeon_topology.d_juiblex_level)
#define orcus_level             (dungeon_topology.d_orcus_level)
#define baalzebub_level         (dungeon_topology.d_baalzebub_level)
#define asmodeus_level          (dungeon_topology.d_asmodeus_level)
#define portal_level            (dungeon_topology.d_portal_level)
#define sanctum_level           (dungeon_topology.d_sanctum_level)
#define earth_level             (dungeon_topology.d_earth_level)
#define water_level             (dungeon_topology.d_water_level)
#define fire_level              (dungeon_topology.d_fire_level)
#define air_level               (dungeon_topology.d_air_level)
#define astral_level            (dungeon_topology.d_astral_level)
#define tower_dnum              (dungeon_topology.d_tower_dnum)
#define mines_dnum              (dungeon_topology.d_mines_dnum)
#define quest_dnum              (dungeon_topology.d_quest_dnum)
#define qstart_level            (dungeon_topology.d_qstart_level)
#define qlocate_level           (dungeon_topology.d_qlocate_level)
#define nemesis_level           (dungeon_topology.d_nemesis_level)
#define knox_level              (dungeon_topology.d_knox_level)
#define mineend_level           (dungeon_topology.d_mineend_level)
#define sokoend_level           (dungeon_topology.d_sokoend_level)

stairway dnstair, upstair; // stairs up and down
#define xdnstair (dnstair.sx)
#define ydnstair (dnstair.sy)
#define xupstair (upstair.sx)
#define yupstair (upstair.sy)

stairway dnladder, upladder; // ladders up and down
#define xdnladder (dnladder.sx)
#define ydnladder (dnladder.sy)
#define xupladder (upladder.sx)
#define yupladder (upladder.sy)

stairway sstairs;

dest_area updest, dndest; // level-change destination areas

bool in_mklev = false;

// maze limits must be even; masking off lowest bit guarantees that
int x_maze_max = (COLNO - 1) & ~1;
int y_maze_max = (ROWNO - 1) & ~1;

const struct shclass shtypes[1] = {}; // ...


#define create_vault() create_room(-1, -1, 2, 2, -1, -1, VAULT, true)
static int vault_x, vault_y;

#define RND(x) (rand() % x)

// 0 <= rnd0(x) < x
int rnd0(int x)
{
    assert(x > 0);
    return RND(x);
}

#define rndplus(x, y) (rnd0(x) + (y))

// 1 <= rnd1(x) <= x
int rnd1(int x)
{
    assert(x > 0);
    x = RND(x) + 1;
    return x;
}

int nroom;
int nsubroom;
int doorindex;

static NhRect rect[MAXRECT + 1];
static int rect_cnt;

dungeon s_dungeon {};

int smeq[MAXNROFROOMS + 1] = {};

bool inside_room(struct mkroom* croom, int x, int y)
{
    return x >= croom->lx - 1 && x <= croom->hx + 1 && y >= croom->ly - 1 && y <= croom->hy + 1;
}

int isok(int x, int y)
{
    // x corresponds to curx, so x==1 is the first column. Ach. %%
    return x >= 1 && x <= COLNO - 1 && y >= 0 && y <= ROWNO - 1;
}

bool nexttodoor(int sx, int sy)
{
    int dx, dy;
    struct rm *lev;

    for (dx = -1; dx <= 1; dx++)
        for (dy = -1; dy <= 1; dy++) {
            if (!isok(sx + dx, sy + dy))
                continue;
            lev = &levl[sx + dx][sy + dy];
            if (IS_DOOR(lev->typ) || lev->typ == SDOOR)
                return true;
        }
    return false;
}

bool has_dnstairs(struct mkroom* sroom)
{
    if (sroom == dnstairs_room)
        return true;
    if (sstairs.sx && !sstairs.up)
        return (bool) (sroom == sstairs_room);
    return false;
}

bool has_upstairs(struct mkroom* sroom)
{
    if (sroom == upstairs_room)
        return true;
    if (sstairs.sx && sstairs.up)
        return (bool) (sroom == sstairs_room);
    return false;
}

int somex(struct mkroom* croom)
{
    return rndplus(croom->hx - croom->lx + 1, croom->lx);
}

int somey(struct mkroom* croom)
{
    return rndplus(croom->hy - croom->ly + 1, croom->ly);
}

bool somexy(struct mkroom* croom, coord *c)
{
    int try_cnt = 0;
    int i;

    if (croom->irregular) {
        i = (int) ((croom - rooms) + ROOMOFFSET);

        while (try_cnt++ < 100) {
            c->x = somex(croom);
            c->y = somey(croom);
            if (!levl[c->x][c->y].edge && (int) levl[c->x][c->y].roomno == i)
                return true;
        }
        // try harder; exhaustively search until one is found
        for (c->x = croom->lx; c->x <= croom->hx; c->x++)
            for (c->y = croom->ly; c->y <= croom->hy; c->y++)
                if (!levl[c->x][c->y].edge
                    && (int) levl[c->x][c->y].roomno == i)
                    return true;
        return false;
    }

    if (!croom->nsubrooms) {
        c->x = somex(croom);
        c->y = somey(croom);
        return true;
    }

    // Check that coords doesn't fall into a subroom or into a wall

    while (try_cnt++ < 100) {
        c->x = somex(croom);
        c->y = somey(croom);
        if (IS_WALL(levl[c->x][c->y].typ))
            continue;
        for (i = 0; i < croom->nsubrooms; i++)
            if (inside_room(croom->sbrooms[i], c->x, c->y))
                goto you_lose;
        break;
    you_lose:
        ;
    }
    if (try_cnt >= 100)
        return false;
    return true;
}

// find random point in generated corridors, so we don't create items in moats, bunkers, or walls
void mazexy(coord *cc)
{
    int cpt = 0;

    do {
        cc->x = 3 + 2 * rnd0((x_maze_max >> 1) - 1);
        cc->y = 3 + 2 * rnd0((y_maze_max >> 1) - 1);
        cpt++;
    } while (cpt < 100
             && levl[cc->x][cc->y].typ
                    != (level.flags.corrmaze ? CORR : ROOM));
    if (cpt >= 100) {
        int x, y;
        // last try
        for (x = 0; x < (x_maze_max >> 1) - 1; x++)
            for (y = 0; y < (y_maze_max >> 1) - 1; y++) {
                cc->x = 3 + 2 * x;
                cc->y = 3 + 2 * y;
                if (levl[cc->x][cc->y].typ
                    == (level.flags.corrmaze ? CORR : ROOM))
                    return;
            }
        assert(false); // panic("mazexy: can't find a place!");
    }
    return;
}

// are "lev1" and "lev2" actually the same?
bool on_level(d_level* lev1, d_level* lev2)
{
    return *lev1 == *lev2;
}

bool is_moat(int x, int y)
{
    int ltyp;

    if (!isok(x, y))
        return false;
    ltyp = levl[x][y].typ;
    if (!Is_juiblex_level(&s_nLevel)
        && (ltyp == MOAT
            || (ltyp == DRAWBRIDGE_UP
                && (levl[x][y].drawbridgemask & DB_UNDER) == DB_MOAT)))
        return true;
    return false;
}

bool is_pool(int x, int y)
{
    int ltyp;

    if (!isok(x, y))
        return false;
    ltyp = levl[x][y].typ;
    /* The ltyp == MOAT is not redundant with is_moat, because the
     * Juiblex level does not have moats, although it has MOATs. There
     * is probably a better way to express this. */
    if (ltyp == POOL || ltyp == MOAT || ltyp == WATER || is_moat(x, y))
        return true;
    return false;
}

bool  is_lava(int x, int y)
{
    int ltyp;

    if (!isok(x, y))
        return false;
    ltyp = levl[x][y].typ;
    if (ltyp == LAVAPOOL
        || (ltyp == DRAWBRIDGE_UP
            && (levl[x][y].drawbridgemask & DB_UNDER) == DB_LAVA))
        return true;
    return false;
}

struct trap * t_at(int x, int y)
{
    struct trap *trap = ftrap;

    while (trap) {
        if (trap->tx == x && trap->ty == y)
            return trap;
        trap = trap->ntrap;
    }
    return nullptr;
}

// are you in one of the Hell levels?
bool In_hell(d_level *lev)
{
    return s_dungeon.flags.hellish;
}

bool Invocation_lev(d_level *lev)
{
    return In_hell(lev) && *lev == s_dungeon.num_dunlevs - 1;
}

coord inv_pos = { 0, 0 };

bool invocation_pos(int x, int y)
{
    return Invocation_lev(&s_nLevel) && x == inv_pos.x && y == inv_pos.y;
}

bool isbig(struct mkroom* sroom)
{
    int area = (sroom->hx - sroom->lx + 1) * (sroom->hy - sroom->ly + 1);
    return area > 20;
}

// pick an unused room, preferably with only one door
struct mkroom* pick_room(bool strict)
{
    struct mkroom* sroom;
    int i = nroom;

    for (sroom = &rooms[rnd0(nroom)]; i--; sroom++) {
        if (sroom == &rooms[nroom])
            sroom = &rooms[0];
        if (sroom->hx < 0)
            return (struct mkroom*) 0;
        if (sroom->rtype != OROOM)
            continue;
        if (!strict) {
            if (has_upstairs(sroom) || (has_dnstairs(sroom) && rnd0(3)))
                continue;
        } else if (has_upstairs(sroom) || has_dnstairs(sroom))
            continue;
        if (sroom->doorct == 1 || !rnd0(5) /*|| wizard*/)
            return sroom;
    }
    return (struct mkroom*) 0;
}

void mkzoo(int type)
{
    struct mkroom* sroom;

    if ((sroom = pick_room(false)) != 0) {
        sroom->rtype = type;
//        fill_zoo(sroom);
    }
}

void mkswamp()
{
    struct mkroom* sroom;
    int sx, sy, i, eelct = 0;

    for (i = 0; i < 5; i++) { // turn up to 5 rooms swampy
        sroom = &rooms[rnd0(nroom)];
        if (sroom->hx < 0 || sroom->rtype != OROOM || has_upstairs(sroom)
            || has_dnstairs(sroom))
            continue;

        // satisfied; make a swamp
        sroom->rtype = SWAMP;
        for (sx = sroom->lx; sx <= sroom->hx; sx++)
            for (sy = sroom->ly; sy <= sroom->hy; sy++)
            {
//                if (!OBJ_AT(sx, sy) && !MON_AT(sx, sy) && !t_at(sx, sy)
//                    && !nexttodoor(sx, sy)) {
//                    if ((sx + sy) % 2) {
//                        levl[sx][sy].typ = POOL;
//                        if (!eelct || !rnd0(4)) {
//                            // mkclass() won't do, as we might get kraken
//                            makemon(rnd0(5)
//                                              ? &mons[PM_GIANT_EEL]
//                                              : rnd0(2)
//                                                 ? &mons[PM_PIRANHA]
//                                                 : &mons[PM_ELECTRIC_EEL],
//                                           sx, sy, NO_MM_FLAGS);
//                            eelct++;
//                        }
//                    } else if (!rnd0(4)) // swamps tend to be moldy
//                        makemon(mkclass(S_FUNGUS, 0), sx, sy,
//                                       NO_MM_FLAGS);
//                }
            }
        level.flags.has_swamp = 1;
    }
}

coord * shrine_pos(int roomno)
{
    static coord buf;
    int delta;
    struct mkroom* troom = &rooms[roomno - ROOMOFFSET];

    /* if width and height are odd, placement will be the exact center;
       if either or both are even, center point is a hypothetical spot
       between map locations and placement will be adjacent to that */
    delta = troom->hx - troom->lx;
    buf.x = troom->lx + delta / 2;
    if ((delta % 2) && rnd0(2))
        buf.x++;
    delta = troom->hy - troom->ly;
    buf.y = troom->ly + delta / 2;
    if ((delta % 2) && rnd0(2))
        buf.y++;
    return &buf;
}

void mktemple()
{
    struct mkroom* sroom;
    coord *shrine_spot;
    struct rm *lev;

    if (!(sroom = pick_room(true)))
        return;

    // set up Priest and shrine
    sroom->rtype = TEMPLE;
    /*
     * In temples, shrines are blessed altars
     * located in the center of the room */
    shrine_spot = shrine_pos((int) ((sroom - rooms) + ROOMOFFSET));
    lev = &levl[shrine_spot->x][shrine_spot->y];
    lev->typ = ALTAR;
//    lev->altarmask = induced_align(80);
//    priestini(&s_nLevel, sroom, shrine_spot->x, shrine_spot->y, false);
    lev->altarmask |= AM_SHRINE;
    level.flags.has_temple = 1;
}

void topologize(struct mkroom* croom, bool do_ordinary)
{
    int x, y, roomno = (int) ((croom - rooms) + ROOMOFFSET);
    int lowx = croom->lx, lowy = croom->ly;
    int hix = croom->hx, hiy = croom->hy;
    int rtype = croom->rtype;
    int subindex, nsubrooms = croom->nsubrooms;

    // skip the room if already done; i.e. a shop handled out of order
    // also skip if this is non-rectangular (it _must_ be done already)
    if ((int) levl[lowx][lowy].roomno == roomno || croom->irregular)
        return;
    if (Is_rogue_level(&s_nLevel))
        do_ordinary = true; // vision routine helper
    if ((rtype != OROOM) || do_ordinary)
        {
        // do innards first
        for (x = lowx; x <= hix; x++)
            for (y = lowy; y <= hiy; y++)
                if (rtype == OROOM)
                    levl[x][y].roomno = NO_ROOM;
                else
                    levl[x][y].roomno = roomno;
        // top and bottom edges
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                levl[x][y].edge = 1;
                if (levl[x][y].roomno)
                    levl[x][y].roomno = SHARED;
                else
                    levl[x][y].roomno = roomno;
            }
        // sides
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                levl[x][y].edge = 1;
                if (levl[x][y].roomno)
                    levl[x][y].roomno = SHARED;
                else
                    levl[x][y].roomno = roomno;
            }
    }
    // subrooms
    for (subindex = 0; subindex < nsubrooms; subindex++)
        topologize(croom->sbrooms[subindex], (bool) (rtype != OROOM));
}

void mkshop()
{
    struct mkroom* sroom;
    int i = -1;
    char *ep = (char *) 0;

    // first determine shoptype
//    if (wizard) {
//        ep = nh_getenv("SHOPTYPE");
        if (ep) {
            if (*ep == 'z' || *ep == 'Z') {
                mkzoo(ZOO);
                return;
            }
            if (*ep == 'm' || *ep == 'M') {
                mkzoo(MORGUE);
                return;
            }
            if (*ep == 'b' || *ep == 'B') {
                mkzoo(BEEHIVE);
                return;
            }
            if (*ep == 't' || *ep == 'T' || *ep == '\\') {
                mkzoo(COURT);
                return;
            }
            if (*ep == 's' || *ep == 'S') {
                mkzoo(BARRACKS);
                return;
            }
            if (*ep == 'a' || *ep == 'A') {
                mkzoo(ANTHOLE);
                return;
            }
            if (*ep == 'c' || *ep == 'C') {
                mkzoo(COCKNEST);
                return;
            }
            if (*ep == 'l' || *ep == 'L') {
                mkzoo(LEPREHALL);
                return;
            }
            if (*ep == '_') {
                mktemple();
                return;
            }
            if (*ep == '}') {
                mkswamp();
                return;
            }
            for (i = 0; shtypes[i].name; i++)
//                if (*ep == def_oc_syms[(int) shtypes[i].symb].sym)
                    goto gottype;
            if (*ep == 'g' || *ep == 'G')
                i = 0;
            else if (*ep == 'v' || *ep == 'V')
                i = FODDERSHOP - SHOPBASE; // veggy food
            else
                i = -1;
        }
//    }
gottype:
    for (sroom = &rooms[0];; sroom++) {
        if (sroom->hx < 0)
            return;
        if (sroom - rooms >= nroom) {
            // pline("rooms not closed by -1?");
            return;
        }
        if (sroom->rtype != OROOM)
            continue;
        if (has_dnstairs(sroom) || has_upstairs(sroom))
            continue;
        if ((/*wizard &&*/ ep && sroom->doorct != 0) || sroom->doorct == 1)
            break;
    }
    if (!sroom->rlit) {
        int x, y;

        for (x = sroom->lx - 1; x <= sroom->hx + 1; x++)
            for (y = sroom->ly - 1; y <= sroom->hy + 1; y++)
                levl[x][y].lit = 1;
        sroom->rlit = 1;
    }

    if (i < 0) { // shoptype not yet determined
        int j;

        // pick a shop type at random
        for (j = rnd1(100), i = 0; (j -= shtypes[i].prob) > 0; i++)
            continue;

        /* big rooms cannot be wand or book shops,
         * - so make them general stores */
//        if (isbig(sroom) && (shtypes[i].symb == WAND_CLASS || shtypes[i].symb == SPBOOK_CLASS))
//            i = 0;
    }
    sroom->rtype = SHOPBASE + i;

    // set room bits before stocking the shop
    topologize(sroom, false); // doesn't matter - this is a special room

    // stock the room with a shopkeeper and artifacts
//    stock_room(i, sroom);
}

// make and stock a room of a given type
void mkroom(int roomtype)
{
    if (roomtype >= SHOPBASE)
        mkshop(); // someday, we should be able to specify shop type
    else
        switch (roomtype) {
        case COURT:
            mkzoo(COURT);
            break;
        case ZOO:
            mkzoo(ZOO);
            break;
        case BEEHIVE:
            mkzoo(BEEHIVE);
            break;
        case MORGUE:
            mkzoo(MORGUE);
            break;
        case BARRACKS:
            mkzoo(BARRACKS);
            break;
        case SWAMP:
            mkswamp();
            break;
        case TEMPLE:
            mktemple();
            break;
        case LEPREHALL:
            mkzoo(LEPREHALL);
            break;
        case COCKNEST:
            mkzoo(COCKNEST);
            break;
        case ANTHOLE:
            mkzoo(ANTHOLE);
            break;
        default:
            assert(false); // impossible("Tried to make a room of type %d.", roomtype);
        }
}

bool bydoor(int x, int y)
{
    int typ;

    if (isok(x + 1, y)) {
        typ = levl[x + 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return true;
    }
    if (isok(x - 1, y)) {
        typ = levl[x - 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return true;
    }
    if (isok(x, y + 1)) {
        typ = levl[x][y + 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return true;
    }
    if (isok(x, y - 1)) {
        typ = levl[x][y - 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return true;
    }
    return false;
}

// see whether it is allowable to create a door at [x,y]
int okdoor(int x, int y)
{
    bool near_door = bydoor(x, y);

    return ((levl[x][y].typ == HWALL || levl[x][y].typ == VWALL)
            && doorindex < DOORMAX && !near_door);
}

bool Is_botlevel(d_level *lev)
{
    return *lev == s_dungeon.num_dunlevs;
}

bool Can_dig_down(d_level *lev)
{
    return !level.flags.hardfloor && !Is_botlevel(lev) && !Invocation_lev(lev);
}

/* returns the depth of a level, in floors below the surface (note levels in different dungeons can have the same depth) */
int depth(d_level *lev)
{
    return s_dungeon.depth_start + *lev - 1;
}

/* use instead of depth() wherever a degree of difficulty is made
 * dependent on the location in the dungeon (eg. monster creation). */
int level_difficulty()
{
    // ...
    return depth(&s_nLevel);
}

int do_comp(const void* vx, const void* vy)
{
    const struct mkroom* x, *y;

    x = (const struct mkroom*) vx;
    y = (const struct mkroom*) vy;
    if (x->lx < y->lx)
        return -1;
    return (x->lx > y->lx);
}

void finddpos(coord *cc, int xl, int yl, int xh, int yh)
{
    int x, y;

    x = rndplus(xh - xl + 1, xl);
    y = rndplus(yh - yl + 1, yl);
    if (okdoor(x, y))
        goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (okdoor(x, y))
                goto gotit;

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (IS_DOOR(levl[x][y].typ) || levl[x][y].typ == SDOOR)
                goto gotit;
    // cannot find something reasonable -- strange
    x = xl;
    y = yh;
gotit:
    cc->x = x;
    cc->y = y;
    return;
}

void sort_rooms()
{
    qsort((void*) rooms, nroom, sizeof(struct mkroom), do_comp);
}

/* clear out various globals that keep information on the current level.
 * some of this is only necessary for some types of levels (maze, normal,
 * special) but it's easier to put it all in one place than make sure
 * each type initializes what it needs to separately. */
void
clear_level_structures()
{
//    static struct rm zerorm = { cmap_to_glyph(S_stone), 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static struct rm zerorm = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // ...
    int x, y;
    struct rm *lev;

    for (x = 0; x < COLNO; x++) {
        lev = &levl[x][0];
        for (y = 0; y < ROWNO; y++) {
            *lev++ = zerorm;
            /*
             * These used to be '#if MICROPORT_BUG',
             * with use of memset(0) for '#if !MICROPORT_BUG' below,
             * but memset is not appropriate for initializing pointers,
             * so do these level.objects[][] and level.monsters[][]
             * initializations unconditionally. */
            level.objects[x][y] = nullptr;
            level.monsters[x][y] = nullptr;
        }
    }
    level.objlist = nullptr;
    level.buriedobjlist = nullptr;
    level.monlist = nullptr;
    level.damagelist = nullptr;
    level.bonesinfo = nullptr;

    level.flags.nfountains = 0;
    level.flags.nsinks = 0;
    level.flags.has_shop = 0;
    level.flags.has_vault = 0;
    level.flags.has_zoo = 0;
    level.flags.has_court = 0;
    level.flags.has_morgue = level.flags.graveyard = 0;
    level.flags.has_beehive = 0;
    level.flags.has_barracks = 0;
    level.flags.has_temple = 0;
    level.flags.has_swamp = 0;
    level.flags.noteleport = 0;
    level.flags.hardfloor = 0;
    level.flags.nommap = 0;
    level.flags.hero_memory = 1;
    level.flags.shortsighted = 0;
    level.flags.is_maze_lev = 0;
    level.flags.is_cavernous_lev = 0;
    level.flags.arboreal = 0;
    level.flags.wizard_bones = 0;
    level.flags.corrmaze = 0;

    nroom = 0;
    rooms[0].hx = -1;
    nsubroom = 0;
    subrooms[0].hx = -1;
    doorindex = 0;

    rect_cnt = 1;
    rect[0].lx = 0;
    rect[0].ly = 0;
    rect[0].hx = COLNO - 1;
    rect[0].hy = ROWNO - 1;

    vault_x = -1;
    xdnstair = ydnstair = xupstair = yupstair = 0;
    sstairs.sx = sstairs.sy = 0;
    xdnladder = ydnladder = xupladder = yupladder = 0;
}

// Search Index of one precise NhRect.
int get_rect_ind(NhRect *r)
{
    NhRect* rectp = &rect[0];
    for (int i = 0; i < rect_cnt; i++, rectp++)
        if (r->lx == rectp->lx && r->ly == rectp->ly && r->hx == rectp->hx && r->hy == rectp->hy)
            return i;
    return -1;
}

// Search a free rectangle that include the one given in arg
NhRect * get_rect(NhRect *r)
{
    NhRect* rectp = &rect[0];
    for (int i = 0; i < rect_cnt; i++, rectp++)
        if (r->lx >= rectp->lx && r->ly >= rectp->ly && r->hx <= rectp->hx && r->hy <= rectp->hy)
            return rectp;
    return 0;
}

NhRect * rnd_rect()
{
    return rect_cnt > 0 ? &rect[rnd0(rect_cnt)] : 0;
}

/* Search intersection between two rectangles (r1 & r2).
 * return true if intersection exist and put it in r3.
 * otherwise returns false */

bool intersect(NhRect *r1, NhRect *r2, NhRect *r3)
{
    if (r2->lx > r1->hx || r2->ly > r1->hy || r2->hx < r1->lx
        || r2->hy < r1->ly)
        return false;

    r3->lx = (r2->lx > r1->lx ? r2->lx : r1->lx);
    r3->ly = (r2->ly > r1->ly ? r2->ly : r1->ly);
    r3->hx = (r2->hx > r1->hx ? r1->hx : r2->hx);
    r3->hy = (r2->hy > r1->hy ? r1->hy : r2->hy);

    if (r3->lx > r3->hx || r3->ly > r3->hy)
        return false;
    return true;
}

// Remove a rectangle from the list of free NhRect.
void remove_rect(NhRect *r)
{
    int ind;

    ind = get_rect_ind(r);
    if (ind >= 0)
        rect[ind] = rect[--rect_cnt];
}

// Add a NhRect to the list.

void add_rect(NhRect *r)
{
    assert(rect_cnt < MAXRECT);

    // Check that this NhRect is not included in another one
    if (get_rect(r))
        return;
    rect[rect_cnt] = *r;
    rect_cnt++;
}

/* Okay, here we have two rectangles (r1 & r2).
 * r1 was already in the list and r2 is included in r1.
 * What we want is to allocate r2, that is split r1 into smaller rectangles then remove it. */
void split_rects(NhRect *r1, NhRect *r2)
{
    NhRect old_r = *r1;
    remove_rect(r1);

    // Walk down since rect_cnt & rect[] will change...
    for (int i = rect_cnt - 1; i >= 0; i--)
    {
        NhRect r;
        if (intersect(&rect[i], r2, &r))
        {
            split_rects(&rect[i], &r);
        }
    }

    if (r2->ly - old_r.ly - 1 > (old_r.hy < ROWNO - 1 ? 2 * YLIM : YLIM + 1) + 4)
    {
        NhRect r = old_r;
        r.hy = r2->ly - 2;
        add_rect(&r);
    }
    if (r2->lx - old_r.lx - 1 > (old_r.hx < COLNO - 1 ? 2 * XLIM : XLIM + 1) + 4)
    {
        NhRect r = old_r;
        r.hx = r2->lx - 2;
        add_rect(&r);
    }
    if (old_r.hy - r2->hy - 1 > (old_r.ly > 0 ? 2 * YLIM : YLIM + 1) + 4)
    {
        NhRect r = old_r;
        r.ly = r2->hy + 2;
        add_rect(&r);
    }
    if (old_r.hx - r2->hx - 1 > (old_r.lx > 0 ? 2 * XLIM : XLIM + 1) + 4)
    {
        NhRect r = old_r;
        r.lx = r2->hx + 2;
        add_rect(&r);
    }
}

// return the lowest level number for *this* dungeon
int dunlevs_in_dungeon(d_level *lev)
{
    return s_dungeon.num_dunlevs;
}

bool iswall(int x, int y)
{
    int type;

    if (!isok(x, y))
        return false;
    type = levl[x][y].typ;
    return IS_WALL(type) || IS_DOOR(type) || type == SDOOR || type == IRONBARS;
}

bool iswall_or_stone(int x, int y)
{
    int type;

    // out of bounds = stone
    if (!isok(x, y))
        return true;

    type = levl[x][y].typ;
    return type == STONE || IS_WALL(type) || IS_DOOR(type) || type == SDOOR || type == IRONBARS;
}

// return true if out of bounds, wall or rock
bool is_solid(int x, int y)
{
    return !isok(x, y) || IS_STWALL(levl[x][y].typ);
}

/* Return 1 (not true - we're doing bit vectors here) if we want to extend
 * a wall spine in the (dx,dy) direction.  Return 0 otherwise.
 *
 * To extend a wall spine in that direction, first there must be a wall there.
 * Then, extend a spine unless the current position is surrounded by walls
 * in the direction given by (dx,dy).  E.g. if 'x' is our location, 'W'
 * a wall, '.' a room, 'a' anything (we don't care), and our direction is
 * (0,1) - South or down - then:
 *
 *              a a a
 *              W x W           This would not extend a spine from x down
 *              W W W           (a corridor of walls is formed).
 *
 *              a a a
 *              W x W           This would extend a spine from x down.
 *              . W W
 */
int extend_spine(int locale[3][3], int wall_there, int dx, int dy)
{
    int spine, nx, ny;

    nx = 1 + dx;
    ny = 1 + dy;

    if (wall_there) { // wall in that direction
        if (dx) {
            if (locale[1][0] && locale[1][2]         // EW are wall/stone
                && locale[nx][0] && locale[nx][2]) { // diag are wall/stone
                spine = 0;
            } else {
                spine = 1;
            }
        } else { // dy
            if (locale[0][1] && locale[2][1]         // NS are wall/stone
                && locale[0][ny] && locale[2][ny]) { // diag are wall/stone
                spine = 0;
            } else {
                spine = 1;
            }
        }
    } else {
        spine = 0;
    }

    return spine;
}

/* Wall cleanup.
 (1) remove walls that are totally surrounded by stone - they are redundant.
 (2) correct the types so that they extend and connect to each other.
 */
void wallification(int x1, int y1, int x2, int y2)
{
    int type;
    int x, y;
    struct rm *lev;
    int bits;
    int locale[3][3]; // rock or wall status surrounding positions
    /*
     * Value 0 represents a free-standing wall.  It could be anything,
     * so even though this table says VWALL, we actually leave whatever typ was there alone. */
    static int spine_array[16] = { VWALL, HWALL,    HWALL,    HWALL,
                                   VWALL, TRCORNER, TLCORNER, TDWALL,
                                   VWALL, BRCORNER, BLCORNER, TUWALL,
                                   VWALL, TLWALL,   TRWALL,   CROSSWALL };

    assert(!(x1 < 0 || x2 >= COLNO || x1 > x2 || y1 < 0 || y2 >= ROWNO || y1 > y2)); // panic("wallification: bad bounds (%d,%d) to (%d,%d)", x1, y1, x2, y2);

    // Step 1: change walls surrounded by rock to rock.
    for (x = x1; x <= x2; x++)
        for (y = y1; y <= y2; y++) {
            lev = &levl[x][y];
            type = lev->typ;
            if (IS_WALL(type) && type != DBWALL) {
                if (is_solid(x - 1, y - 1) && is_solid(x - 1, y)
                    && is_solid(x - 1, y + 1) && is_solid(x, y - 1)
                    && is_solid(x, y + 1) && is_solid(x + 1, y - 1)
                    && is_solid(x + 1, y) && is_solid(x + 1, y + 1))
                    lev->typ = STONE;
            }
        }

    /*
     * Step 2: set the correct wall type.  We can't combine steps
     * 1 and 2 into a single sweep because we depend on knowing if
     * the surrounding positions are stone. */
    for (x = x1; x <= x2; x++)
        for (y = y1; y <= y2; y++) {
            lev = &levl[x][y];
            type = lev->typ;
            if (!(IS_WALL(type) && type != DBWALL))
                continue;

            // set the locations true if rock or wall or out of bounds
            locale[0][0] = iswall_or_stone(x - 1, y - 1);
            locale[1][0] = iswall_or_stone(x, y - 1);
            locale[2][0] = iswall_or_stone(x + 1, y - 1);

            locale[0][1] = iswall_or_stone(x - 1, y);
            locale[2][1] = iswall_or_stone(x + 1, y);

            locale[0][2] = iswall_or_stone(x - 1, y + 1);
            locale[1][2] = iswall_or_stone(x, y + 1);
            locale[2][2] = iswall_or_stone(x + 1, y + 1);

            // determine if wall should extend to each direction NSEW
            bits = (extend_spine(locale, iswall(x, y - 1), 0, -1) << 3)
                   | (extend_spine(locale, iswall(x, y + 1), 0, 1) << 2)
                   | (extend_spine(locale, iswall(x + 1, y), 1, 0) << 1)
                   | extend_spine(locale, iswall(x - 1, y), -1, 0);

            // don't change typ if wall is free-standing
            if (bits)
                lev->typ = spine_array[bits];
        }
}

void do_room_or_subroom(struct mkroom* croom, int lowx, int lowy, int hix, int hiy, bool lit, int rtype, bool special, bool is_room)
{
    int x, y;
    struct rm *lev;

    // locations might bump level edges in wall-less rooms
    // add/subtract 1 to allow for edge locations
    if (!lowx)
        lowx++;
    if (!lowy)
        lowy++;
    if (hix >= COLNO - 1)
        hix = COLNO - 2;
    if (hiy >= ROWNO - 1)
        hiy = ROWNO - 2;

    if (lit) {
        for (x = lowx - 1; x <= hix + 1; x++) {
            lev = &levl[x][max(lowy - 1, 0)];
            for (y = lowy - 1; y <= hiy + 1; y++)
                lev++->lit = 1;
        }
        croom->rlit = 1;
    } else
        croom->rlit = 0;

    croom->lx = lowx;
    croom->hx = hix;
    croom->ly = lowy;
    croom->hy = hiy;
    croom->rtype = rtype;
    croom->doorct = 0;
    /* if we're not making a vault, doorindex will still be 0
     * if we are, we'll have problems adding niches to the previous room
     * unless fdoor is at least doorindex */
    croom->fdoor = doorindex;
    croom->irregular = false;

    croom->nsubrooms = 0;
    croom->sbrooms[0] = (struct mkroom*) 0;
    if (!special) {
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                levl[x][y].typ = HWALL;
                levl[x][y].horizontal = 1; // For open/secret doors.
            }
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                levl[x][y].typ = VWALL;
                levl[x][y].horizontal = 0; // For open/secret doors.
            }
        for (x = lowx; x <= hix; x++) {
            lev = &levl[x][lowy];
            for (y = lowy; y <= hiy; y++)
                lev++->typ = ROOM;
        }
        if (is_room) {
            levl[lowx - 1][lowy - 1].typ = TLCORNER;
            levl[hix + 1][lowy - 1].typ = TRCORNER;
            levl[lowx - 1][hiy + 1].typ = BLCORNER;
            levl[hix + 1][hiy + 1].typ = BRCORNER;
        } else { // a subroom
            wallification(lowx - 1, lowy - 1, hix + 1, hiy + 1);
        }
    }
}

void add_room(int lowx, int lowy, int hix, int hiy, bool lit, int rtype, bool special)
{
    struct mkroom* croom;

    croom = &rooms[nroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special, true);
    croom++;
    croom->hx = -1;
    nroom++;
}

void add_subroom(struct mkroom* proom, int lowx, int lowy, int hix, int hiy, bool lit, int rtype, bool special)
{
    struct mkroom* croom;

    croom = &subrooms[nsubroom];
    do_room_or_subroom(croom, lowx, lowy, hix, hiy, lit, rtype, special, false);
    proom->sbrooms[proom->nsubrooms++] = croom;
    croom++;
    croom->hx = -1;
    nsubroom++;
}

bool check_room(int *lowx, int *ddx, int *lowy, int *ddy, bool vault)
{
    int x, y, hix = *lowx + *ddx, hiy = *lowy + *ddy;
    struct rm *lev;
    int xlim, ylim, ymax;

    xlim = XLIM + (vault ? 1 : 0);
    ylim = YLIM + (vault ? 1 : 0);

    if (*lowx < 3)
        *lowx = 3;
    if (*lowy < 2)
        *lowy = 2;
    if (hix > COLNO - 3)
        hix = COLNO - 3;
    if (hiy > ROWNO - 3)
        hiy = ROWNO - 3;
chk:
    if (hix <= *lowx || hiy <= *lowy)
        return false;

    // check area around room (and make room smaller if necessary)
    for (x = *lowx - xlim; x <= hix + xlim; x++) {
        if (x <= 0 || x >= COLNO)
            continue;
        y = *lowy - ylim;
        ymax = hiy + ylim;
        if (y < 0)
            y = 0;
        if (ymax >= ROWNO)
            ymax = (ROWNO - 1);
        lev = &levl[x][y];
        for (; y <= ymax; y++) {
            if (lev++->typ) {
                if (!vault) {
//                    debugpline2("strange area [%d,%d] in check_room.", x, y);
                }
                if (!rnd0(3))
                    return false;
                if (x < *lowx)
                    *lowx = x + xlim + 1;
                else
                    hix = x - xlim - 1;
                if (y < *lowy)
                    *lowy = y + ylim + 1;
                else
                    hiy = y - ylim - 1;
                goto chk;
            }
        }
    }
    *ddx = hix - *lowx;
    *ddy = hiy - *lowy;
    return true;
}

/* Create a new room. This is still very incomplete... */
bool create_room(int x, int y, int w, int h, int xal, int yal, int rtype, int rlit)
{
    int xabs, yabs;
    int wtmp, htmp, xaltmp, yaltmp, xtmp, ytmp;
    NhRect *r1 = 0, r2;
    int trycnt = 0;
    bool vault = false;
    int xlim = XLIM, ylim = YLIM;

    if (rtype == -1) // Is the type random ?
        rtype = OROOM;

    if (rtype == VAULT) {
        vault = true;
        xlim++;
        ylim++;
    }

    // on low levels the room is lit (usually)
    // some other rooms may require lighting

    // is light state random ?
    if (rlit == -1)
        rlit = (rnd1(1 + abs(depth(&s_nLevel))) < 11 && rnd0(77)) ? true : false;

    /* Here we will try to create a room. If some parameters are random we are willing to make several try before we give it up. */
    do {
        int xborder, yborder;
        wtmp = w;
        htmp = h;
        xtmp = x;
        ytmp = y;
        xaltmp = xal;
        yaltmp = yal;

        // First case : a totally random room

        if ((xtmp < 0 && ytmp < 0 && wtmp < 0 && xaltmp < 0 && yaltmp < 0)
            || vault) {
            int hx, hy, lx, ly, dx, dy;
            r1 = rnd_rect(); // Get a random rectangle

            if (!r1) { // No more free rectangles !
//                debugpline0("No more rects...");
                return false;
            }
            hx = r1->hx;
            hy = r1->hy;
            lx = r1->lx;
            ly = r1->ly;
            if (vault)
                dx = dy = 1;
            else {
                dx = 2 + rnd0((hx - lx > 28) ? 12 : 8);
                dy = 2 + rnd0(4);
                if (dx * dy > 50)
                    dy = 50 / dx;
            }
            xborder = (lx > 0 && hx < COLNO - 1) ? 2 * xlim : xlim + 1;
            yborder = (ly > 0 && hy < ROWNO - 1) ? 2 * ylim : ylim + 1;
            if (hx - lx < dx + 3 + xborder || hy - ly < dy + 3 + yborder) {
                r1 = 0;
                continue;
            }
            xabs = lx + (lx > 0 ? xlim : 3)
                   + rnd0(hx - (lx > 0 ? lx : 3) - dx - xborder + 1);
            yabs = ly + (ly > 0 ? ylim : 2)
                   + rnd0(hy - (ly > 0 ? ly : 2) - dy - yborder + 1);
            if (ly == 0 && hy >= (ROWNO - 1) && (!nroom || !rnd0(nroom))
                && (yabs + dy > ROWNO / 2)) {
                yabs = rndplus(3, 2);
                if (nroom < 4 && dy > 1)
                    dy--;
            }
            if (!check_room(&xabs, &dx, &yabs, &dy, vault)) {
                r1 = 0;
                continue;
            }
            wtmp = dx + 1;
            htmp = dy + 1;
            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp;
            r2.hy = yabs + htmp;
        } else { // Only some parameters are random
            int rndpos = 0;
            if (xtmp < 0 && ytmp < 0) { // Position is RANDOM
                xtmp = rnd1(5);
                ytmp = rnd1(5);
                rndpos = 1;
            }
            if (wtmp < 0 || htmp < 0) { // Size is RANDOM
                wtmp = rndplus(15, 3);
                htmp = rndplus(8, 2);
            }
            if (xaltmp == -1) // Horizontal alignment is RANDOM
                xaltmp = rnd1(3);
            if (yaltmp == -1) // Vertical alignment is RANDOM
                yaltmp = rnd1(3);

            // Try to generate real (absolute) coordinates here!

            xabs = (((xtmp - 1) * COLNO) / 5) + 1;
            yabs = (((ytmp - 1) * ROWNO) / 5) + 1;
            switch (xaltmp) {
            case LEFT:
                break;
            case RIGHT:
                xabs += (COLNO / 5) - wtmp;
                break;
            case CENTER:
                xabs += ((COLNO / 5) - wtmp) / 2;
                break;
            }
            switch (yaltmp) {
            case TOP:
                break;
            case BOTTOM:
                yabs += (ROWNO / 5) - htmp;
                break;
            case CENTER:
                yabs += ((ROWNO / 5) - htmp) / 2;
                break;
            }

            if (xabs + wtmp - 1 > COLNO - 2)
                xabs = COLNO - wtmp - 3;
            if (xabs < 2)
                xabs = 2;
            if (yabs + htmp - 1 > ROWNO - 2)
                yabs = ROWNO - htmp - 3;
            if (yabs < 2)
                yabs = 2;

            // Try to find a rectangle that fit our room !

            r2.lx = xabs - 1;
            r2.ly = yabs - 1;
            r2.hx = xabs + wtmp + rndpos;
            r2.hy = yabs + htmp + rndpos;
            r1 = get_rect(&r2);
        }
    } while (++trycnt <= 100 && !r1);
    if (!r1) { // creation of room failed ?
        return false;
    }
    split_rects(r1, &r2);

    if (!vault) {
        smeq[nroom] = nroom;
        add_room(xabs, yabs, xabs + wtmp - 1, yabs + htmp - 1, rlit != 0, rtype, false);
    } else {
        rooms[nroom].lx = xabs;
        rooms[nroom].ly = yabs;
    }
    return true;
}

void makerooms()
{
    bool tried_vault = false;

    // make rooms until satisfied
    // rnd_rect() will returns 0 if no more rects are available...
    while (nroom < MAXNROFROOMS && rnd_rect()) {
        if (nroom >= (MAXNROFROOMS / 6) && rnd0(2) && !tried_vault) {
            tried_vault = true;
            if (create_vault()) {
                vault_x = rooms[nroom].lx;
                vault_y = rooms[nroom].ly;
                rooms[nroom].hx = -1;
            }
        } else if (!create_room(-1, -1, -1, -1, -1, -1, OROOM, true))
            return;
    }
    return;
}

void add_door(int x, int y, struct mkroom* aroom)
{
    struct mkroom* broom;
    int tmp;
    int i;

    if (aroom->doorct == 0)
        aroom->fdoor = doorindex;

    aroom->doorct++;

    for (tmp = doorindex; tmp > aroom->fdoor; tmp--)
        doors[tmp] = doors[tmp - 1];

    for (i = 0; i < nroom; i++) {
        broom = &rooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }
    for (i = 0; i < nsubroom; i++) {
        broom = &subrooms[i];
        if (broom != aroom && broom->doorct && broom->fdoor >= aroom->fdoor)
            broom->fdoor++;
    }

    doorindex++;
    doors[aroom->fdoor].x = x;
    doors[aroom->fdoor].y = y;
}

void dosdoor(int x, int y, struct mkroom* aroom, int type)
{
//    bool shdoor = *in_rooms(x, y, SHOPBASE) ? true : false;
bool shdoor = false;

    if (!IS_WALL(levl[x][y].typ)) // avoid SDOORs on already made doors
        type = DOOR;
    levl[x][y].typ = type;
    if (type == DOOR) {
        if (!rnd0(3)) { // is it a locked door, closed, or a doorway?
            if (!rnd0(5))
                levl[x][y].doormask = D_ISOPEN;
            else if (!rnd0(6))
                levl[x][y].doormask = D_LOCKED;
            else
                levl[x][y].doormask = D_CLOSED;

            if (levl[x][y].doormask != D_ISOPEN && !shdoor && level_difficulty() >= 5 && !rnd0(25))
                levl[x][y].doormask |= D_TRAPPED;
        } else {
            levl[x][y].doormask = (shdoor ? D_ISOPEN : D_NODOOR);
        }

        /* also done in roguecorr(); doing it here first prevents making mimics in place of trapped doors on rogue level */
        if (Is_rogue_level(&s_nLevel))
            levl[x][y].doormask = D_NODOOR;

//        if (levl[x][y].doormask & D_TRAPPED) {
//            struct monst *mtmp;
//
//            if (level_difficulty() >= 9 && !rnd0(5)
//                && !((mvitals[PM_SMALL_MIMIC].mvflags & G_GONE)
//                     && (mvitals[PM_LARGE_MIMIC].mvflags & G_GONE)
//                     && (mvitals[PM_GIANT_MIMIC].mvflags & G_GONE))) {
//                // make a mimic instead
//                levl[x][y].doormask = D_NODOOR;
//                mtmp = makemon(mkclass(S_MIMIC, 0), x, y, NO_MM_FLAGS);
//                if (mtmp)
//                    set_mimic_sym(mtmp);
//            }
//        }
        // newsym(x,y);
    } else { // SDOOR
        if (shdoor || !rnd0(5))
            levl[x][y].doormask = D_LOCKED;
        else
            levl[x][y].doormask = D_CLOSED;

        if (!shdoor && level_difficulty() >= 4 && !rnd0(20))
            levl[x][y].doormask |= D_TRAPPED;
    }

    add_door(x, y, aroom);
}

void dodoor(int x, int y, struct mkroom* aroom)
{
    if (doorindex >= DOORMAX) {
        assert(false); // impossible("DOORMAX exceeded?");
        return;
    }

    dosdoor(x, y, aroom, rnd0(8) ? DOOR : SDOOR);
}

// Dig a corridor between two points.
bool dig_corridor(coord *org, coord *dest, bool nxcor, int ftyp, int btyp)
{
    int dx = 0, dy = 0, dix, diy, cct;
    struct rm *crm;
    int tx, ty, xx, yy;

    xx = org->x;
    yy = org->y;
    tx = dest->x;
    ty = dest->y;
    if (xx <= 0 || yy <= 0 || tx <= 0 || ty <= 0 || xx > COLNO - 1
        || tx > COLNO - 1 || yy > ROWNO - 1 || ty > ROWNO - 1) {
        assert(false); // debugpline4("dig_corridor: bad coords <%d,%d> <%d,%d>.", xx, yy, tx, ty);
        return false;
    }
    if (tx > xx)
        dx = 1;
    else if (ty > yy)
        dy = 1;
    else if (tx < xx)
        dx = -1;
    else
        dy = -1;

    xx -= dx;
    yy -= dy;
    cct = 0;
    while (xx != tx || yy != ty) {
        // loop: dig corridor at [xx,yy] and find new [xx,yy]
        if (cct++ > 500 || (nxcor && !rnd0(35)))
            return false;

        xx += dx;
        yy += dy;

        if (xx >= COLNO - 1 || xx <= 0 || yy <= 0 || yy >= ROWNO - 1)
            return false; // impossible

        crm = &levl[xx][yy];
        if (crm->typ == btyp) {
            if (ftyp != CORR || rnd0(100)) {
                crm->typ = ftyp;
//                if (nxcor && !rnd0(50))
//                    mksobj_at(BOULDER, xx, yy, true, false);
            } else {
                crm->typ = SCORR;
            }
        } else if (crm->typ != ftyp && crm->typ != SCORR) {
            // strange ...
            return false;
        }

        // find next corridor position
        dix = abs(xx - tx);
        diy = abs(yy - ty);

        if ((dix > diy) && diy && !rnd0(dix-diy+1)) {
            dix = 0;
        } else if ((diy > dix) && dix && !rnd0(diy-dix+1)) {
            diy = 0;
        }

        // do we have to change direction ?
        if (dy && dix > diy) {
            int ddx = (xx > tx) ? -1 : 1;

            crm = &levl[xx + ddx][yy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dx = ddx;
                dy = 0;
                continue;
            }
        } else if (dx && diy > dix) {
            int ddy = (yy > ty) ? -1 : 1;

            crm = &levl[xx][yy + ddy];
            if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR) {
                dy = ddy;
                dx = 0;
                continue;
            }
        }

        // continue straight on?
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;

        // no, what must we do now??
        if (dx) {
            dx = 0;
            dy = (ty < yy) ? -1 : 1;
        } else {
            dy = 0;
            dx = (tx < xx) ? -1 : 1;
        }
        crm = &levl[xx + dx][yy + dy];
        if (crm->typ == btyp || crm->typ == ftyp || crm->typ == SCORR)
            continue;
        dy = -dy;
        dx = -dx;
    }
    return true;
}

void join(int a, int b, bool nxcor)
{
    coord cc, tt, org, dest;
    int tx, ty, xx, yy;
    struct mkroom* croom, *troom;
    int dx, dy;

    croom = &rooms[a];
    troom = &rooms[b];

    /* find positions cc and tt for doors in croom and troom
       and direction for a corridor between them */

    if (troom->hx < 0 || croom->hx < 0 || doorindex >= DOORMAX)
        return;
    if (troom->lx > croom->hx) {
        dx = 1;
        dy = 0;
        xx = croom->hx + 1;
        tx = troom->lx - 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else if (troom->hy < croom->ly) {
        dy = -1;
        dx = 0;
        yy = croom->ly - 1;
        finddpos(&cc, croom->lx, yy, croom->hx, yy);
        ty = troom->hy + 1;
        finddpos(&tt, troom->lx, ty, troom->hx, ty);
    } else if (troom->hx < croom->lx) {
        dx = -1;
        dy = 0;
        xx = croom->lx - 1;
        tx = troom->hx + 1;
        finddpos(&cc, xx, croom->ly, xx, croom->hy);
        finddpos(&tt, tx, troom->ly, tx, troom->hy);
    } else {
        dy = 1;
        dx = 0;
        yy = croom->hy + 1;
        ty = troom->ly - 1;
        finddpos(&cc, croom->lx, yy, croom->hx, yy);
        finddpos(&tt, troom->lx, ty, troom->hx, ty);
    }
    xx = cc.x;
    yy = cc.y;
    tx = tt.x - dx;
    ty = tt.y - dy;
    if (nxcor && levl[xx + dx][yy + dy].typ)
        return;
    if (okdoor(xx, yy) || !nxcor)
        dodoor(xx, yy, croom);

    org.x = xx + dx;
    org.y = yy + dy;
    dest.x = tx;
    dest.y = ty;

    if (!dig_corridor(&org, &dest, nxcor, level.flags.arboreal ? ROOM : CORR,
                      STONE))
        return;

    // we succeeded in digging the corridor
    if (okdoor(tt.x, tt.y) || !nxcor)
        dodoor(tt.x, tt.y, troom);

    if (smeq[a] < smeq[b])
        smeq[b] = smeq[a];
    else
        smeq[a] = smeq[b];
}

void makecorridors()
{
    int a, b, i;
    bool any = true;

    for (a = 0; a < nroom - 1; a++) {
        join(a, a + 1, false);
        if (!rnd0(50))
            break; // allow some randomness
    }
    for (a = 0; a < nroom - 2; a++)
        if (smeq[a] != smeq[a + 2])
            join(a, a + 2, false);
    for (a = 0; any && a < nroom; a++) {
        any = false;
        for (b = 0; b < nroom; b++)
            if (smeq[a] != smeq[b]) {
                join(a, b, false);
                any = true;
            }
    }
    if (nroom > 2)
        for (i = rnd0(nroom) + 4; i; i--) {
            a = rnd0(nroom);
            b = rnd0(nroom - 2);
            if (b >= a)
                b += 2;
            join(a, b, true);
        }
}

bool place_niche(struct mkroom* aroom, int *dy, int *xx, int *yy)
{
    coord dd;

    if (rnd0(2)) {
        *dy = 1;
        finddpos(&dd, aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1);
    } else {
        *dy = -1;
        finddpos(&dd, aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1);
    }
    *xx = dd.x;
    *yy = dd.y;
    return (bool) ((isok(*xx, *yy + *dy)
                       && levl[*xx][*yy + *dy].typ == STONE)
                      && (isok(*xx, *yy - *dy)
                          && !IS_POOL(levl[*xx][*yy - *dy].typ)
                          && !IS_FURNITURE(levl[*xx][*yy - *dy].typ)));
}

/* Like Can_dig_down (above), but also allows falling through on the
 * stronghold level.  Normally, the bottom level of a dungeon resists
 * both digging and falling. */
bool Can_fall_thru(d_level *lev)
{
    return Can_dig_down(lev) || Is_stronghold(lev);
}

struct trap *maketrap(int x, int y, int typ)
{
    // ...
    return nullptr;
}

// there should be one of these per trap, in the same order as trap.h
static const char *trap_engravings[TRAPNUM] = {
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0,
    // 14..16: trap door, teleport, level-teleport
    "Vlad was here", "ad aerarium", "ad aerarium", (char *) 0, (char *) 0,
    (char *) 0,      (char *) 0,    (char *) 0,    (char *) 0, (char *) 0,
};

void makeniche(int trap_type)
{
    struct mkroom* aroom;
    struct rm *rm;
    int vct = 8;
    int dy, xx, yy;
    struct trap *ttmp;

    if (doorindex < DOORMAX) {
        while (vct--) {
            aroom = &rooms[rnd0(nroom)];
            if (aroom->rtype != OROOM)
                continue; // not an ordinary room
            if (aroom->doorct == 1 && rnd0(5))
                continue;
            if (!place_niche(aroom, &dy, &xx, &yy))
                continue;

            rm = &levl[xx][yy + dy];
            if (trap_type || !rnd0(4)) {
                rm->typ = SCORR;
                if (trap_type) {
                    if ((trap_type == HOLE || trap_type == TRAPDOOR)
                        && !Can_fall_thru(&s_nLevel))
                        trap_type = ROCKTRAP;
                    ttmp = maketrap(xx, yy + dy, trap_type);
                    if (ttmp) {
                        if (trap_type != ROCKTRAP)
                            ttmp->once = 1;
//                        if (trap_engravings[trap_type]) {
//                            make_engr_at(xx, yy - dy,
//                                         trap_engravings[trap_type], 0L,
//                                         DUST);
//                            wipe_engr_at(xx, yy - dy, 5,
//                                         false); // age it a little
//                        }
                    }
                }
                dosdoor(xx, yy, aroom, SDOOR);
            } else {
                rm->typ = CORR;
                if (rnd0(7))
                    dosdoor(xx, yy, aroom, rnd0(5) ? SDOOR : DOOR);
                else {
                    // inaccessible niches occasionally have iron bars
                    if (!rnd0(5) && IS_WALL(levl[xx][yy].typ)) {
                        levl[xx][yy].typ = IRONBARS;
//                        if (rnd0(3))
//                            mkcorpstat(CORPSE, nullptr, mkclass(S_HUMAN, 0), xx, yy + dy, true);
                    }
//                    if (!level.flags.noteleport)
//                        mksobj_at(SCR_TELEPORTATION, xx, yy + dy, true, false);
//                    if (!rnd0(3))
//                        mkobj_at(0, xx, yy + dy, true);
                }
            }
            return;
        }
    }
}

void make_niches()
{
    int ct = rnd1((nroom >> 1) + 1), dep = depth(&s_nLevel);
    bool ltptr = (!level.flags.noteleport && dep > 15),
            vamp = (dep > 5 && dep < 25);

    while (ct--) {
        if (ltptr && !rnd0(6)) {
            ltptr = false;
            makeniche(LEVEL_TELEP);
        } else if (vamp && !rnd0(6)) {
            vamp = false;
            makeniche(TRAPDOOR);
        } else
            makeniche(NO_TRAP);
    }
}

void
makevtele()
{
    makeniche(TELEP_TRAP);
}

bool occupied(int x, int y)
{
    return (bool) (t_at(x, y) || IS_FURNITURE(levl[x][y].typ)
                      || is_lava(x, y) || is_pool(x, y)
                      || invocation_pos(x, y));
}

// return the lowest level explored in the game
int deepest_lev_reached(bool noquest)
{
    /* this function is used for three purposes: to provide a factor
     * of difficulty in monster generation; to provide a factor of
     * difficulty in experience calculations (botl.c and end.c); and
     * to insert the deepest level reached in the game in the topten
     * display.  the 'noquest' arg switch is required for the latter.
     *
     * from the player's point of view, going into the Quest is _not_
     * going deeper into the dungeon -- it is going back "home", where
     * the dungeon starts at level 1.  given the setup in dungeon.def,
     * the depth of the Quest (thought of as starting at level 1) is
     * never lower than the level of entry into the Quest, so we exclude
     * the Quest from the topten "deepest level reached" display
     * calculation.  _However_ the Quest is a difficult dungeon, so we
     * include it in the factor of difficulty calculations.
     */
    d_level tmp;

    // each dungeon ...
    tmp = s_dungeon.dunlev_ureached;
    if (tmp == 0)
        return 1;
    return depth(&tmp);
}

// make a trap somewhere (in croom if mazeflag = 0 && !tm)
// if tm != null, make trap at that location
void mktrap(int num, int mazeflag, struct mkroom* croom, coord *tm)
{
    int kind;
    coord m;

    // no traps in pools
    if (tm && is_pool(tm->x, tm->y))
        return;

    if (num > 0 && num < TRAPNUM) {
        kind = num;
    } else if (Is_rogue_level(&s_nLevel)) {
        switch (rnd0(7)) {
        default:
            kind = BEAR_TRAP;
            break; // 0
        case 1:
            kind = ARROW_TRAP;
            break;
        case 2:
            kind = DART_TRAP;
            break;
        case 3:
            kind = TRAPDOOR;
            break;
        case 4:
            kind = PIT;
            break;
        case 5:
            kind = SLP_GAS_TRAP;
            break;
        case 6:
            kind = RUST_TRAP;
            break;
        }
    } else if (Inhell && !rnd0(5)) {
        // bias the frequency of fire traps in Gehennom
        kind = FIRE_TRAP;
    } else {
        unsigned lvl = level_difficulty();

        do {
            kind = rnd1(TRAPNUM - 1);
            // reject "too hard" traps
            switch (kind) {
            case MAGIC_PORTAL:
            case VIBRATING_SQUARE:
                kind = NO_TRAP;
                break;
            case ROLLING_BOULDER_TRAP:
            case SLP_GAS_TRAP:
                if (lvl < 2)
                    kind = NO_TRAP;
                break;
            case LEVEL_TELEP:
                if (lvl < 5 || level.flags.noteleport)
                    kind = NO_TRAP;
                break;
            case SPIKED_PIT:
                if (lvl < 5)
                    kind = NO_TRAP;
                break;
            case LANDMINE:
                if (lvl < 6)
                    kind = NO_TRAP;
                break;
            case WEB:
                if (lvl < 7)
                    kind = NO_TRAP;
                break;
            case STATUE_TRAP:
            case POLY_TRAP:
                if (lvl < 8)
                    kind = NO_TRAP;
                break;
            case FIRE_TRAP:
                if (!Inhell)
                    kind = NO_TRAP;
                break;
            case TELEP_TRAP:
                if (level.flags.noteleport)
                    kind = NO_TRAP;
                break;
            case HOLE:
                // make these much less often than other traps
                if (rnd0(7))
                    kind = NO_TRAP;
                break;
            }
        } while (kind == NO_TRAP);
    }

    if ((kind == TRAPDOOR || kind == HOLE) && !Can_fall_thru(&s_nLevel))
        kind = ROCKTRAP;

    if (tm)
        m = *tm;
    else {
        int tryct = 0;
        bool avoid_boulder = (kind == PIT || kind == SPIKED_PIT || kind == TRAPDOOR || kind == HOLE);

//        do {
//            if (++tryct > 200)
//                return;
//            if (mazeflag)
//                mazexy(&m);
//            else if (!somexy(croom, &m))
//                return;
//        } while (occupied(m.x, m.y) || (avoid_boulder && sobj_at(BOULDER, m.x, m.y)));
    }

//    maketrap(m.x, m.y, kind);
//    if (kind == WEB)
//        makemon(&mons[PM_GIANT_SPIDER], m.x, m.y, NO_MM_FLAGS);
}

void mkstairs(int x, int y, char up, struct mkroom* croom)
{
    if (!x) {
//        impossible("mkstairs:  bogus stair attempt at <%d,%d>", x, y);
        return;
    }

    /*
     * We can't make a regular stair off an end of the dungeon.  This
     * attempt can happen when a special level is placed at an end and
     * has an up or down stair specified in its description file. */
    if ((s_nLevel == 1 && up) || (s_nLevel == dunlevs_in_dungeon(&s_nLevel) && !up))
        return;

    if (up) {
        xupstair = x;
        yupstair = y;
        upstairs_room = croom;
    } else {
        xdnstair = x;
        ydnstair = y;
        dnstairs_room = croom;
    }

    levl[x][y].typ = STAIRS;
    levl[x][y].ladder = up ? LA_UP : LA_DOWN;
}

void mkfount(int mazeflag, struct mkroom* croom)
{
    coord m;
    int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (mazeflag)
            mazexy(&m);
        else if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    // Put a fountain at m.x, m.y
    levl[m.x][m.y].typ = FOUNTAIN;
    // Is it a "blessed" fountain? (affects drinking from fountain)
    if (!rnd0(7))
        levl[m.x][m.y].blessedftn = 1;

    level.flags.nfountains++;
}

void mksink(struct mkroom* croom)
{
    coord m;
    int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    // Put a sink at m.x, m.y
    levl[m.x][m.y].typ = SINK;

    level.flags.nsinks++;
}

void mkaltar(struct mkroom* croom)
{
    coord m;
    int tryct = 0;
//    aligntyp al;

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

    // Put an altar at m.x, m.y
    levl[m.x][m.y].typ = ALTAR;

    // -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL
//    al = rnd0((int) A_LAWFUL + 2) - 1;
//    levl[m.x][m.y].altarmask = Align2amask(al);
}

static void mkgrave(struct mkroom* croom)
{
    coord m;
    int tryct = 0;
//    struct obj *otmp;
    bool dobell = !rnd0(10);

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(croom, &m))
            return;
    } while (occupied(m.x, m.y) || bydoor(m.x, m.y));

//    // Put a grave at m.x, m.y
//    make_grave(m.x, m.y, dobell ? "Saved by the bell!" : (char *) 0);
//
//    // Possibly fill it with objects
//    if (!rnd0(3))
//        mkgold(0L, m.x, m.y);
//    for (tryct = rnd0(5); tryct; tryct--) {
//        otmp = mkobj(RANDOM_CLASS, true);
//        if (!otmp)
//            return;
//        curse(otmp);
//        otmp->ox = m.x;
//        otmp->oy = m.y;
//        add_to_buried(otmp);
//    }
//
//    // Leave a bell, in case we accidentally buried someone alive
//    if (dobell)
//        mksobj_at(BELL, m.x, m.y, true, false);
    return;
}

//------------------------------------------------------------------------------
void makelevel()
{
    struct mkroom* croom, *troom;
//    int tryct;
//    int x, y;
int x;
//    struct monst *tmonst; // always put a web with a spider
    int room_threshold;

    clear_level_structures();
/*
    {
        s_level *slev = Is_special(&s_nLevel);

        // check for special levels
        if (slev && !Is_rogue_level(&s_nLevel)) {
            makemaz(slev->proto);
            return;
        } else if (s_dungeon.proto[0]) {
            makemaz("");
            return;
        } else if (In_mines(&s_nLevel)) {
            makemaz("minefill");
            return;
        } else if (In_quest(&s_nLevel)) {
            char fillname[9];
            s_level *loc_lev;

            Sprintf(fillname, "%s-loca", urole.filecode);
            loc_lev = find_level(fillname);

            Sprintf(fillname, "%s-fil", urole.filecode);
            Strcat(fillname,
                   (s_nLevel < loc_lev->dlevel) ? "a" : "b");
            makemaz(fillname);
            return;
//        } else if (In_hell(&s_nLevel) || (rnd0(5) && s_nLevel.dnum == medusa_level.dnum && depth(&s_nLevel) > depth(&medusa_level))) {
//            makemaz("");
//            return;
        }
    }

    // otherwise, fall through - it's a "regular" level.

    if (Is_rogue_level(&s_nLevel)) {
        makeroguerooms();
        makerogueghost();
    } else
*/
        makerooms();
    sort_rooms();

    // construct stairs (up and down in different rooms if possible)
    croom = &rooms[rnd0(nroom)];
    if (!Is_botlevel(&s_nLevel))
        mkstairs(somex(croom), somey(croom), 0, croom); // down
    if (nroom > 1) {
        troom = croom;
        croom = &rooms[rnd0(nroom - 1)];
        if (croom == troom)
            croom++;
    }

    if (s_nLevel != 1) {
        int sx, sy;
        do {
            sx = somex(croom);
            sy = somey(croom);
        } while (occupied(sx, sy));
        mkstairs(sx, sy, 1, croom); // up
    }

room_threshold = 3;
cout << "x" << endl;
//    if (Is_rogue_level(&s_nLevel))
//        goto skip0;
cout << "y" << endl;
    makecorridors();
    make_niches();

    // make a secret treasure vault, not connected to the rest
//    if (do_vault()) {
//        int w, h;
//        w = 1;
//        h = 1;
//        if (check_room(&vault_x, &w, &vault_y, &h, true)) {
//        fill_vault:
//            add_room(vault_x, vault_y, vault_x + w, vault_y + h, true, VAULT,
//                     false);
//            level.flags.has_vault = 1;
//            ++room_threshold;
//            fill_room(&rooms[nroom - 1], false);
//            mk_knox_portal(vault_x + w, vault_y + h);
//            if (!level.flags.noteleport && !rnd0(3))
//                makevtele();
//        } else if (rnd_rect() && create_vault()) {
//            vault_x = rooms[nroom].lx;
//            vault_y = rooms[nroom].ly;
//            if (check_room(&vault_x, &w, &vault_y, &h, true))
//                goto fill_vault;
//            else
//                rooms[nroom].hx = -1;
//        }
//    }

    {
        int u_depth = depth(&s_nLevel);

//        if (wizard && nh_getenv("SHOPTYPE"))
//            mkroom(SHOPBASE);
//        else if (u_depth > 1 && u_depth < depth(&medusa_level)
if (u_depth > 1 && u_depth < depth(&medusa_level)
                 && nroom >= room_threshold && rnd0(u_depth) < 3)
            mkroom(SHOPBASE);
        else if (u_depth > 4 && !rnd0(6))
            mkroom(COURT);
        else if (u_depth > 5 && !rnd0(8)
)//                 && !(mvitals[PM_LEPRECHAUN].mvflags & G_GONE))
            mkroom(LEPREHALL);
        else if (u_depth > 6 && !rnd0(7))
            mkroom(ZOO);
        else if (u_depth > 8 && !rnd0(5))
            mkroom(TEMPLE);
        else if (u_depth > 9 && !rnd0(5)
)//                 && !(mvitals[PM_KILLER_BEE].mvflags & G_GONE))
            mkroom(BEEHIVE);
        else if (u_depth > 11 && !rnd0(6))
            mkroom(MORGUE);
        else if (u_depth > 12 && !rnd0(8)
)//                 && antholemon())
            mkroom(ANTHOLE);
        else if (u_depth > 14 && !rnd0(4)
)//                 && !(mvitals[PM_SOLDIER].mvflags & G_GONE))
            mkroom(BARRACKS);
        else if (u_depth > 15 && !rnd0(6))
            mkroom(SWAMP);
        else if (u_depth > 16 && !rnd0(8)
)//                 && !(mvitals[PM_COCKATRICE].mvflags & G_GONE))
            mkroom(COCKNEST);
    }

skip0:

    // for each room: put things inside
    for (croom = rooms; croom->hx > 0; croom++) {
        if (croom->rtype != OROOM)
            continue;

        // put a sleeping monster inside
        /* Note: monster may be on the stairs. This cannot be
           avoided: maybe the player fell through a trap door
           while a monster was on the stairs. Conclusion:
           we have to check for monsters on the stairs anyway. */

//        if (u.uhave.amulet || !rnd0(3)) {
//            x = somex(croom);
//            y = somey(croom);
//            tmonst = makemon(nullptr, x, y, NO_MM_FLAGS);
//            if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER]
//                && !occupied(x, y))
//                maketrap(x, y, WEB);
//        }
        // put traps and mimics inside
        x = 8 - (level_difficulty() / 6);
        if (x <= 1)
            x = 2;
        while (!rnd0(x))
            mktrap(0, 0, croom, (coord *) 0);
//        if (!rnd0(3))
//            mkgold(0L, somex(croom), somey(croom));
        if (Is_rogue_level(&s_nLevel))
            goto skip_nonrogue;
        if (!rnd0(10))
            mkfount(0, croom);
        if (!rnd0(60))
            mksink(croom);
        if (!rnd0(60))
            mkaltar(croom);
        x = 80 - (depth(&s_nLevel) * 2);
        if (x < 2)
            x = 2;
        if (!rnd0(x))
            mkgrave(croom);

        // put statues inside
//        if (!rnd0(20))
//            mkcorpstat(STATUE, nullptr, somex(croom), somey(croom), CORPSTAT_INIT);
        /* put box/chest inside;
         *  40% chance for at least 1 box, regardless of number
         *  of rooms; about 5 - 7.5% for 2 boxes, least likely
         *  when few rooms; chance for 3 or more is negligible. */
//        if (!rnd0(nroom * 5 / 2))
//            mksobj_at((rnd0(3)) ? LARGE_BOX : CHEST, somex(croom), somey(croom), true, false);

        // maybe make some graffiti
//        if (!rnd0(27 + 3 * abs(depth(&s_nLevel)))) {
//            char buf[BUFSZ];
//            const char *mesg = random_engraving(buf);
//            if (mesg) {
//                do {
//                    x = somex(croom);
//                    y = somey(croom);
//                } while (levl[x][y].typ != ROOM && !rnd0(40));
//                if (!(IS_POOL(levl[x][y].typ)
//                      || IS_FURNITURE(levl[x][y].typ)))
//                    make_engr_at(x, y, mesg, 0L, MARK);
//            }
//        }

    skip_nonrogue:
;
//        if (!rnd0(3)) {
//            mkobj_at(0, somex(croom), somey(croom), true);
//            tryct = 0;
//            while (!rnd0(5)) {
//                if (++tryct > 100) {
//                    impossible("tryct overflow4");
//                    break;
//                }
//                mkobj_at(0, somex(croom), somey(croom), true);
//            }
//        }
    }
}

/* put a non-diggable boundary around the initial portion of a level map.
 * assumes that no level will initially put things beyond the isok() range.
 *
 * we can't bound unconditionally on the last line with something in it,
 * because that something might be a niche which was already reachable,
 * so the boundary would be breached
 *
 * we can't bound unconditionally on one beyond the last line, because
 * that provides a window of abuse for wallified special levels */
void
bound_digging()
{
    int x, y;
    unsigned typ;
    struct rm *lev;
    bool found, nonwall;
    int xmin, xmax, ymin, ymax;

    if (Is_earthlevel(&s_nLevel))
        return; // everything diggable here

    found = nonwall = false;
    for (xmin = 0; !found && xmin <= COLNO; xmin++) {
        lev = &levl[xmin][0];
        for (y = 0; y <= ROWNO - 1; y++, lev++) {
            typ = lev->typ;
            if (typ != STONE) {
                found = true;
                if (!IS_WALL(typ))
                    nonwall = true;
            }
        }
    }
    xmin -= (nonwall || !level.flags.is_maze_lev) ? 2 : 1;
    if (xmin < 0)
        xmin = 0;

    found = nonwall = false;
    for (xmax = COLNO - 1; !found && xmax >= 0; xmax--) {
        lev = &levl[xmax][0];
        for (y = 0; y <= ROWNO - 1; y++, lev++) {
            typ = lev->typ;
            if (typ != STONE) {
                found = true;
                if (!IS_WALL(typ))
                    nonwall = true;
            }
        }
    }
    xmax += (nonwall || !level.flags.is_maze_lev) ? 2 : 1;
    if (xmax >= COLNO)
        xmax = COLNO - 1;

    found = nonwall = false;
    for (ymin = 0; !found && ymin <= ROWNO; ymin++) {
        lev = &levl[xmin][ymin];
        for (x = xmin; x <= xmax; x++, lev += ROWNO) {
            typ = lev->typ;
            if (typ != STONE) {
                found = true;
                if (!IS_WALL(typ))
                    nonwall = true;
            }
        }
    }
    ymin -= (nonwall || !level.flags.is_maze_lev) ? 2 : 1;

    found = nonwall = false;
    for (ymax = ROWNO - 1; !found && ymax >= 0; ymax--) {
        lev = &levl[xmin][ymax];
        for (x = xmin; x <= xmax; x++, lev += ROWNO) {
            typ = lev->typ;
            if (typ != STONE) {
                found = true;
                if (!IS_WALL(typ))
                    nonwall = true;
            }
        }
    }
    ymax += (nonwall || !level.flags.is_maze_lev) ? 2 : 1;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            if (y <= ymin || y >= ymax || x <= xmin || x >= xmax) {
                levl[x][y].wall_info |= W_NONDIGGABLE;
            }
}

/*      Place deposits of minerals (gold and misc gems) in the stone
 *      surrounding the rooms on the map.
 *      Also place kelp in water.
 *      mineralize(-1, -1, -1, -1, false); => "default" behaviour */
void mineralize(int kelp_pool, int kelp_moat, int goldprob, int gemprob, bool skip_lvl_checks)
{
//    s_level *sp;
//    struct obj *otmp;
    int x, y, cnt;

    if (kelp_pool < 0)
        kelp_pool = 10;
    if (kelp_moat < 0)
        kelp_moat = 30;

    // Place kelp, except on the plane of water
//    if (!skip_lvl_checks && In_endgame(&s_nLevel))
//        return;
//    for (x = 2; x < (COLNO - 2); x++)
//        for (y = 1; y < (ROWNO - 1); y++)
//            if ((kelp_pool && levl[x][y].typ == POOL && !rnd0(kelp_pool))
//                || (kelp_moat && levl[x][y].typ == MOAT && !rnd0(kelp_moat)))
//                mksobj_at(KELP_FROND, x, y, true, false);

    // determine if it is even allowed; almost all special levels are excluded
//    if (!skip_lvl_checks
//        && (In_hell(&s_nLevel) || In_V_tower(&s_nLevel) || Is_rogue_level(&s_nLevel)
//            || level.flags.arboreal
//            || ((sp = Is_special(&s_nLevel)) != 0 && !Is_oracle_level(&s_nLevel)
//                && (!In_mines(&s_nLevel) || sp->flags.town))))
//        return;

    // basic level-related probabilities
    if (goldprob < 0)
        goldprob = 20 + depth(&s_nLevel) / 3;
    if (gemprob < 0)
        gemprob = goldprob / 4;

    // mines have ***MORE*** goodies - otherwise why mine?
//    if (!skip_lvl_checks) {
//        if (In_mines(&s_nLevel)) {
//            goldprob *= 2;
//            gemprob *= 3;
//        } else if (In_quest(&s_nLevel)) {
//            goldprob /= 4;
//            gemprob /= 6;
//        }
//    }

    /*
     * Seed rock areas with gold and/or gems.
     * We use fairly low level object handling to avoid unnecessary
     * overhead from placing things in the floor chain prior to burial. */
    for (x = 2; x < (COLNO - 2); x++)
        for (y = 1; y < (ROWNO - 1); y++)
            if (levl[x][y + 1].typ != STONE) { // <x,y> spot not eligible
                y += 2; // next two spots aren't eligible either
            } else if (levl[x][y].typ != STONE) { // this spot not eligible
                y += 1; // next spot isn't eligible either
            } else if (!(levl[x][y].wall_info & W_NONDIGGABLE)
                       && levl[x][y - 1].typ == STONE
                       && levl[x + 1][y - 1].typ == STONE
                       && levl[x - 1][y - 1].typ == STONE
                       && levl[x + 1][y].typ == STONE
                       && levl[x - 1][y].typ == STONE
                       && levl[x + 1][y + 1].typ == STONE
                       && levl[x - 1][y + 1].typ == STONE) {
                if (rnd0(1000) < goldprob) {
//                    if ((otmp = mksobj(GOLD_PIECE, false, false)) != 0) {
//                        otmp->ox = x, otmp->oy = y;
//                        otmp->quan = 1L + rnd1(goldprob * 3);
//                        otmp->owt = weight(otmp);
//                        if (!rnd0(3))
//                            add_to_buried(otmp);
//                        else
//                            place_object(otmp, x, y);
//                    }
                }
                if (rnd0(1000) < gemprob) {
                    for (cnt = rnd1(2 + s_nLevel / 3); cnt > 0; cnt--)
                    {
//                        if ((otmp = mkobj(GEM_CLASS, false)) != 0) {
//                            if (otmp->otyp == ROCK) {
//                                dealloc_obj(otmp); // discard it
//                            } else {
//                                otmp->ox = x, otmp->oy = y;
//                                if (!rnd0(3))
//                                    add_to_buried(otmp);
//                                else
//                                    place_object(otmp, x, y);
//                            }
//                        }
                    }
                }
            }
}



// -------------------------------------------------------------------------
// Wall Angle --------------------------------------------------------------

#ifdef WA_VERBOSE

static const char *FDECL(type_to_name, (int));
static void FDECL(error4, (int, int, int, int, int, int));

static int bad_count[MAX_TYPE]; // count of positions flagged as bad
static const char *type_names[MAX_TYPE] = {
    "STONE", "VWALL", "HWALL", "TLCORNER", "TRCORNER", "BLCORNER", "BRCORNER",
    "CROSSWALL", "TUWALL", "TDWALL", "TLWALL", "TRWALL", "DBWALL", "TREE",
    "SDOOR", "SCORR", "POOL", "MOAT", "WATER", "DRAWBRIDGE_UP", "LAVAPOOL",
    "IRON_BARS", "DOOR", "CORR", "ROOM", "STAIRS", "LADDER", "FOUNTAIN",
    "THRONE", "SINK", "GRAVE", "ALTAR", "ICE", "DRAWBRIDGE_DOWN", "AIR",
    "CLOUD"
};

static const char *type_to_name(int type)
{
    return (type < 0 || type >= MAX_TYPE) ? "unknown" : type_names[type];
}

static void
error4(x, y, a, b, c, dd)
int x, y, a, b, c, dd;
{
    pline("set_wall_state: %s @ (%d,%d) %s%s%s%s",
          type_to_name(levl[x][y].typ), x, y,
          a ? "1" : "", b ? "2" : "", c ? "3" : "", dd ? "4" : "");
    bad_count[levl[x][y].typ]++;
}
#endif // WA_VERBOSE

/* Return 'which' if position is implies an unfinished exterior.  Return
 * zero otherwise.  Unfinished implies outer area is rock or a corridor.
 *
 * Things that are ambiguous: lava */
int check_pos(int x, int y, int which)
{
    int type;
    if (!isok(x, y))
        return which;
    type = levl[x][y].typ;
    if (IS_ROCK(type) || type == CORR || type == SCORR)
        return which;
    return 0;
}

// Return true if more than one is non-zero.
#ifdef WA_VERBOSE
bool
more_than_one(x, y, a, b, c)
int x, y, a, b, c;
{
    if ((a && (b | c)) || (b && (a | c)) || (c && (a | b))) {
        error4(x, y, a, b, c, 0);
        return true;
    }
    return false;
}
#else
#define more_than_one(x, y, a, b, c) \
    (((a) && ((b) | (c))) || ((b) && ((a) | (c))) || ((c) && ((a) | (b))))
#endif

// Return the wall mode for a T wall.
int set_twall(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3)
{
    int wmode, is_1, is_2, is_3;

//    nhUse(x0);
//    nhUse(y0);

    is_1 = check_pos(x1, y1, WM_T_LONG);
    is_2 = check_pos(x2, y2, WM_T_BL);
    is_3 = check_pos(x3, y3, WM_T_BR);
    if (more_than_one(x0, y0, is_1, is_2, is_3)) {
        wmode = 0;
    } else {
        wmode = is_1 + is_2 + is_3;
    }
    return wmode;
}

// Return wall mode for a horizontal or vertical wall.
int set_wall(int x, int y, int horiz)
{
    int wmode, is_1, is_2;

    if (horiz) {
        is_1 = check_pos(x, y - 1, WM_W_TOP);
        is_2 = check_pos(x, y + 1, WM_W_BOTTOM);
    } else {
        is_1 = check_pos(x - 1, y, WM_W_LEFT);
        is_2 = check_pos(x + 1, y, WM_W_RIGHT);
    }
    if (more_than_one(x, y, is_1, is_2, 0)) {
        wmode = 0;
    } else {
        wmode = is_1 + is_2;
    }
    return wmode;
}

// Return a wall mode for a corner wall. (x4,y4) is the "inner" position.
int set_corn(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
    int wmode, is_1, is_2, is_3, is_4;

    is_1 = check_pos(x1, y1, 1);
    is_2 = check_pos(x2, y2, 1);
    is_3 = check_pos(x3, y3, 1);
    is_4 = check_pos(x4, y4, 1); // inner location

    /*
     * All 4 should not be true.  So if the inner location is rock,
     * use it.  If all of the outer 3 are true, use outer.  We currently
     * can't cover the case where only part of the outer is rock, so
     * we just say that all the walls are finished (if not overridden
     * by the inner section). */
    if (is_4) {
        wmode = WM_C_INNER;
    } else if (is_1 && is_2 && is_3)
        wmode = WM_C_OUTER;
    else
        wmode = 0; // finished walls on all sides

    return wmode;
}

// Return mode for a crosswall.
int set_crosswall(int x, int y)
{
    int wmode, is_1, is_2, is_3, is_4;

    is_1 = check_pos(x - 1, y - 1, 1);
    is_2 = check_pos(x + 1, y - 1, 1);
    is_3 = check_pos(x + 1, y + 1, 1);
    is_4 = check_pos(x - 1, y + 1, 1);

    wmode = is_1 + is_2 + is_3 + is_4;
    if (wmode > 1) {
        if (is_1 && is_3 && (is_2 + is_4 == 0)) {
            wmode = WM_X_TLBR;
        } else if (is_2 && is_4 && (is_1 + is_3 == 0)) {
            wmode = WM_X_BLTR;
        } else {
#ifdef WA_VERBOSE
            error4(x, y, is_1, is_2, is_3, is_4);
#endif
            wmode = 0;
        }
    } else if (is_1)
        wmode = WM_X_TL;
    else if (is_2)
        wmode = WM_X_TR;
    else if (is_3)
        wmode = WM_X_BR;
    else if (is_4)
        wmode = WM_X_BL;

    return wmode;
}

// Called from mklev.  Scan the level and set the wall modes.
void set_wall_state()
{
    int x, y;
    int wmode;
    struct rm *lev;

#ifdef WA_VERBOSE
    for (x = 0; x < MAX_TYPE; x++)
        bad_count[x] = 0;
#endif

    for (x = 0; x < COLNO; x++)
        for (lev = &levl[x][0], y = 0; y < ROWNO; y++, lev++) {
            switch (lev->typ) {
            case SDOOR:
                wmode = set_wall(x, y, (int) lev->horizontal);
                break;
            case VWALL:
                wmode = set_wall(x, y, 0);
                break;
            case HWALL:
                wmode = set_wall(x, y, 1);
                break;
            case TDWALL:
                wmode = set_twall(x, y, x, y - 1, x - 1, y + 1, x + 1, y + 1);
                break;
            case TUWALL:
                wmode = set_twall(x, y, x, y + 1, x + 1, y - 1, x - 1, y - 1);
                break;
            case TLWALL:
                wmode = set_twall(x, y, x + 1, y, x - 1, y - 1, x - 1, y + 1);
                break;
            case TRWALL:
                wmode = set_twall(x, y, x - 1, y, x + 1, y + 1, x + 1, y - 1);
                break;
            case TLCORNER:
                wmode =
                    set_corn(x - 1, y - 1, x, y - 1, x - 1, y, x + 1, y + 1);
                break;
            case TRCORNER:
                wmode =
                    set_corn(x, y - 1, x + 1, y - 1, x + 1, y, x - 1, y + 1);
                break;
            case BLCORNER:
                wmode =
                    set_corn(x, y + 1, x - 1, y + 1, x - 1, y, x + 1, y - 1);
                break;
            case BRCORNER:
                wmode =
                    set_corn(x + 1, y, x + 1, y + 1, x, y + 1, x - 1, y - 1);
                break;
            case CROSSWALL:
                wmode = set_crosswall(x, y);
                break;

            default:
                wmode = -1; // don't set wall info
                break;
            }

            if (wmode >= 0)
                lev->wall_info = (lev->wall_info & ~WM_MASK) | wmode;
        }

#ifdef WA_VERBOSE
    // check if any bad positions found
    for (x = y = 0; x < MAX_TYPE; x++)
        if (bad_count[x]) {
            if (y == 0) {
                y = 1; // only print once
                pline("set_wall_type: wall mode problems with: ");
            }
            pline("%s %d;", type_names[x], bad_count[x]);
        }
#endif // WA_VERBOSE
}

void mklev()
{
//    init_mapseen(&s_nLevel);

    in_mklev = true;
    makelevel();
    bound_digging();
    mineralize(-1, -1, -1, -1, false);
    in_mklev = false;
    /* has_morgue gets cleared once morgue is entered; graveyard stays
       set (graveyard might already be set even when has_morgue is clear
       [see fixup_special()], so don't update it unconditionally) */
    if (level.flags.has_morgue)
        level.flags.graveyard = 1;
    if (!level.flags.is_maze_lev) {
        for (struct mkroom* croom = &rooms[0]; croom != &rooms[nroom]; croom++)
            topologize(croom, false);
    }
    set_wall_state();
    // for many room types, rooms[].rtype is zeroed once the room has been entered; rooms[].orig_rtype always retains original rtype value
    for (int ridx = 0; ridx < SIZE(rooms); ridx++)
        rooms[ridx].orig_rtype = rooms[ridx].rtype;
}

time_t getnow()
{
    time_t t = 0;
    time(&t);
    return t;
}

void setrandom()
{
    unsigned int seed = getnow();
    srand(seed);
}

char glyphs[] =
{
    ' ', '|', '-', '+', '+', '+', '+', '7',
    '8', '9', 'a', 'b', 'c', 'd', '\\', '=',
    '0', '1', '2', '3', '4', '5', '/', '#',
    '.', '<', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q'
};

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    setrandom();

//    context.botlx = 1;
//    context.ident = 1;
//    context.stethoscope_move = -1;
//    context.warnlevel = 1;
//    context.next_attrib_check = 600;  // arbitrary first setting
//    context.tribute.enabled = true;   // turn on 3.6 tributes
//    context.tribute.tributesz = sizeof(struct tribute_info);
//
//    init_artifacts(); // before u_init() in case $WIZKIT specifies any artifacts
//    u_init();
//
//    load_qtlist();          // load up the quest text info

    mklev();

//    u_on_upstairs();
//    vision_reset();          // set up internals for level (after mklev)
//    check_special_room(false);
//
//    docrt();
//
//    flush_screen(1);

    ofstream ofs("x.txt");

    for (int y = 0; y < ROWNO; ++y)
    {
        for (int x = 0; x < COLNO; ++x)
        {
            struct rm& lev = levl[x][y];
//            ofs << setw(3) << lev.typ;
            cout << glyphs[lev.typ];
            ofs << glyphs[lev.typ];
        }
        cout << endl;
        ofs << endl;
    }

    return 0;
}
