/* Sector operation appears both as a public and inside callers. In each
 * context sn identifies the region and source identifies its trigger sprite. */
{
    long i, j, goal, middle;
    int animation_result;
    short a, b;

#ifdef SECTOR_INLINE_HELPERS
#define SET_ANIMATION(p, g, v) SET_ANIMATION_INLINE(sn, p, g, v, animation_result)
#define QUERY_ANIMATION(p) getanimationgoal(p)
#define FIND_PLAYER(s) checkcursectnums(s)
#define WARP_ELEVATORS(i, d) WARP_ELEVATORS_INLINE(i, d)
#else
#define SET_ANIMATION(p, g, v) (animation_result = setanimation(sn, p, g, v))
#define QUERY_ANIMATION(p) query_animation(p)
#define FIND_PLAYER(s) player_in_sector(s)
#define WARP_ELEVATORS(i, d) activatewarpelevators(i, d)
#endif

    switch ((short)(sector[sn].lotag & 32767)) {
    case 26:
        if (QUERY_ANIMATION(&sector[sn].floorz) == -1) {
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 22;
            operatesectors(sn, source);
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 9;
            operatesectors(sn, source);
            sector[sn].lotag &= 0xff00;
            sector[sn].lotag |= 26;
        }
        break;
    case 9: {
        /* The sliding-door geometry follows Ken's September 1994 routine. */
        long cx, cy, first, last, speed, dx, dy;
        long door[2];
        short w, prev;

        first = sector[sn].wallptr;
        last = first + sector[sn].wallnum - 1;
        cx = cy = 0;
        speed = sector[sn].extra >> 4;
        for (w = first; w <= last; w++) {
            cx += wall[w].x;
            cy += wall[w].y;
        }
        cx /= last - first + 1;
        cy /= last - first + 1;
        door[0] = -1;
        door[1] = -1;
        for (w = first; w <= last; w++)
            if (cx == wall[w].x || cy == wall[w].y) {
                if (door[0] == -1)
                    door[0] = w;
                else
                    door[1] = w;
            }

        for (j = 0; j < 2; j++) {
            if (cx == wall[door[j]].x && cy == wall[door[j]].y) {
                prev = door[j] - 1;
                if (prev < first)
                    prev = last;
                dx = ((wall[prev].x + wall[wall[door[j]].point2].x) >> 1) - wall[door[j]].x;
                dy = ((wall[prev].y + wall[wall[door[j]].point2].y) >> 1) - wall[door[j]].y;
                if (dx != 0) {
                    dx = wall[wall[wall[door[j]].point2].point2].x;
                    dx -= wall[wall[door[j]].point2].x;
                    SET_ANIMATION(&wall[door[j]].x, wall[door[j]].x + dx, speed);
                    SET_ANIMATION(&wall[prev].x, wall[prev].x + dx, speed);
                    SET_ANIMATION(&wall[wall[door[j]].point2].x, wall[wall[door[j]].point2].x + dx, speed);
                    spritesound(6, source);
                } else if (dy != 0) {
                    dy = wall[wall[wall[door[j]].point2].point2].y;
                    dy -= wall[wall[door[j]].point2].y;
                    SET_ANIMATION(&wall[door[j]].y, wall[door[j]].y + dy, speed);
                    SET_ANIMATION(&wall[prev].y, wall[prev].y + dy, speed);
                    SET_ANIMATION(&wall[wall[door[j]].point2].y, wall[wall[door[j]].point2].y + dy, speed);
                    spritesound(6, source);
                }
            } else {
                prev = door[j] - 1;
                if (prev < first)
                    prev = last;
                dx = ((wall[prev].x + wall[wall[door[j]].point2].x) >> 1) - wall[door[j]].x;
                dy = ((wall[prev].y + wall[wall[door[j]].point2].y) >> 1) - wall[door[j]].y;
                if (dx != 0) {
                    SET_ANIMATION(&wall[door[j]].x, cx, speed);
                    SET_ANIMATION(&wall[prev].x, cx + dx, speed);
                    SET_ANIMATION(&wall[wall[door[j]].point2].x, cx + dx, speed);
                    spritesound(6, source);
                } else if (dy != 0) {
                    SET_ANIMATION(&wall[door[j]].y, cy, speed);
                    SET_ANIMATION(&wall[prev].y, cy + dy, speed);
                    SET_ANIMATION(&wall[wall[door[j]].point2].y, cy + dy, speed);
                    spritesound(6, source);
                }
            }
        }
        break;
    }
    case 15:
        for (a = headspritesect[sn]; a >= 0; a = nextspritesect[a])
            if (sprite[a].picnum == SECTOREFFECTOR && sprite[a].lotag == 17)
                break;
        b = FIND_PLAYER(sn);
        if (b == -1) {
            if (sector[sn].floorz > sprite[a].z)
                WARP_ELEVATORS(a, -1);
            else
                WARP_ELEVATORS(a, 1);
        } else {
            if (player_bits[b] & 1)
                WARP_ELEVATORS(a, -1);
            if (player_bits[b] & 2)
                WARP_ELEVATORS(a, 1);
        }
        break;
    case 17:
lift_up:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, -1);
        if (a != -1) {
            SET_ANIMATION(&sector[sn].floorz, sector[a].floorz, sector[sn].extra);
            spritesound(SOUND_LIFT_START, source);
            break;
        }
        sector[sn].lotag ^= 1;
        /* Fall through to the opposite direction. */
    case 16:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, 1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto lift_up;
        }
        SET_ANIMATION(&sector[sn].floorz, sector[a].floorz, sector[sn].extra);
        spritesound(SOUND_LIFT_START, source);
        break;
    case 19:
elevator_up:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, -1);
        if (a != -1) {
            goal = sector[a].floorz;
            SET_ANIMATION(&sector[sn].floorz, goal, sector[sn].extra);
            SET_ANIMATION(&sector[sn].ceilingz, goal + sector[sn].ceilingz - sector[sn].floorz, sector[sn].extra);
            spritesound(SOUND_LIFT_START, source);
            break;
        }
        sector[sn].lotag ^= 1;
        /* Fall through to the opposite direction. */
    case 18:
        a = nextsectorneighborz(sn, sector[sn].floorz, 1, 1);
        if (a == -1) {
            sector[sn].lotag ^= 1;
            goto elevator_up;
        }
        goal = sector[a].floorz;
        SET_ANIMATION(&sector[sn].floorz, goal, 256);
        SET_ANIMATION(&sector[sn].ceilingz, goal + sector[sn].ceilingz - sector[sn].floorz, sector[sn].extra);
        spritesound(SOUND_LIFT_START, source);
        break;
    case 29:
        if (sector[sn].lotag & 0x8000) {
            a = nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1);
            goal = sector[a].floorz;
        } else {
            a = nextsectorneighborz(sn, sector[sn].ceilingz, -1, -1);
            goal = sector[a].ceilingz;
        }
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == 22 && sprite[i].hitag == sector[sn].hitag) {
                if (sector[sn].lotag & 0x8000)
                    sprite[i].yvel = sector[sn].extra;
                else
                    sprite[i].yvel = -sector[sn].extra;
                hittype[i][0] = sn;
                hittype[i][1] = 1;
            }
        sector[sn].lotag ^= 0x8000;
        SET_ANIMATION(&sector[sn].ceilingz, goal, sector[sn].extra);
        spritesound(6, source);
        break;
    case 20:
        if (sector[sn].lotag & 0x8000) {
            for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
                if (sprite[i].sectnum == sn && sprite[i].lotag == 9)
                    break;
            goal = i == -1 ? sector[sn].floorz : sprite[i].z;
        } else {
            a = nextsectorneighborz(sn, sector[sn].ceilingz, -1, -1);
            goal = sector[a].ceilingz;
        }
        sector[sn].lotag ^= 0x8000;
        SET_ANIMATION(&sector[sn].ceilingz, goal, sector[sn].extra);
        spritesound(6, source);
        break;
    case 21:
        j = QUERY_ANIMATION(&sector[sn].floorz);
        if (j >= 0) {
            if (animategoal[sn] == sector[sn].ceilingz) {
                a = nextsectorneighborz(sn, animategoal[sn], 1, 1);
                animategoal[j] = sector[a].floorz;
            } else
                animategoal[j] = sector[sn].ceilingz;
        } else {
            goal = sector[sn].ceilingz;
            if (goal == sector[sn].floorz) {
                a = nextsectorneighborz(sn, goal, 1, 1);
                goal = sector[a].floorz;
            }
            sector[sn].lotag ^= 0x8000;
            SET_ANIMATION(&sector[sn].floorz, goal, sector[sn].extra);
            if (animation_result >= 0)
                spritesound(6, source);
        }
        break;
    case 22:
        middle = (sector[sn].floorz + sector[sn].ceilingz) >> 1;
        j = QUERY_ANIMATION(&sector[sn].ceilingz);
        if (j >= 0) {
            if (middle == animategoal[sn]) {
                a = nextsectorneighborz(sn, sector[sn].floorz, -1, -1);
                animategoal[j] = sector[a].ceilingz;
            } else
                animategoal[j] = middle;
        } else {
            goal = middle;
            if (middle == sector[sn].ceilingz) {
                a = nextsectorneighborz(sn, sector[sn].floorz, -1, -1);
                goal = sector[a].ceilingz;
            }
            SET_ANIMATION(&sector[sn].ceilingz, goal, sector[sn].extra);
            if (animation_result >= 0) {
                sector[sn].lotag ^= 0x8000;
                spritesound(6, source);
            }
        }
        j = QUERY_ANIMATION(&sector[sn].floorz);
        if (j >= 0) {
            if (middle == animategoal[sn]) {
                a = nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1);
                animategoal[j] = sector[a].floorz;
            } else
                animategoal[j] = middle;
        } else {
            goal = middle;
            if (middle == sector[sn].floorz) {
                a = nextsectorneighborz(sn, sector[sn].ceilingz, 1, 1);
                goal = sector[a].floorz;
            }
            SET_ANIMATION(&sector[sn].floorz, goal, sector[sn].extra);
            if (animation_result >= 0)
                spritesound(6, source);
        }
        break;
    case 23:
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == 11 && sprite[i].sectnum == sn && hittype[i][4] == 0)
                break;
        if (i < 0)
            break;
        sector[sn].lotag ^= 0x8000;
        hittype[i][4] = 1;
        hittype[i][3] = -hittype[i][3];
        spritesound(6, source);
        for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
            if (j != i && sprite[j].lotag == 11 && sprite[j].hitag == sprite[i].hitag && hittype[j][4] == 0) {
                sector[sprite[j].sectnum].lotag ^= 0x8000;
                hittype[j][4] = 1;
                hittype[j][3] = -hittype[j][3];
            }
        break;
    case 25:
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == 15 && sprite[i].sectnum == sn)
                break;
        if (i >= 0)
            spritesound(6, source);
        for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
            if (sprite[j].hitag == sprite[i].hitag) {
                if (sprite[j].lotag == 18)
                    hittype[j][0] ^= 1;
                if (sprite[j].lotag == 15) {
                    sector[sprite[j].sectnum].lotag ^= 0x8000;
                    sprite[j].ang += 1024;
                    hittype[j][4] = (sector[sprite[j].sectnum].lotag & 0x8000) ? 1 : 2;
                }
            }
        break;
    case 27:
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if ((sprite[i].lotag & 255) == 20 && sprite[i].sectnum == sn) {
                sector[sn].lotag ^= 0x8000;
                hittype[i][0] = (sector[sn].lotag & 0x8000) ? 1 : 2;
                spritesound(6, source);
                break;
            }
        break;
    case 28:
        for (i = headspritesect[sn]; i >= 0; i = nextspritesect[i])
            if (sprite[i].statnum == 3 && (sprite[i].lotag & 32767) == 21)
                break;
        goal = sprite[i].hitag;
        for (j = headspritestat[3]; j >= 0; j = nextspritestat[j])
            if ((sprite[j].lotag & 32767) == 21 && hittype[j][0] == 0 && sprite[j].hitag == goal)
                hittype[j][0] = 1;
        break;
    }
#undef SET_ANIMATION
#undef QUERY_ANIMATION
#undef FIND_PLAYER
#undef WARP_ELEVATORS
}
