/* Post-script sprite movement. The includer supplies i and p. */
short flags, owner, goal;
long *movement;

flags = sprite[i].hitag;
if (!hittype[i].move || !flags)
    return;

hittype[i].movecount++;
movement = hittype[i].move;
if (flags & ON_OWNER) {
    owner = sprite[i].owner;
    if (owner >= 0 && sprite[owner].statnum == 1) {
        sprite[i].x = sprite[sprite[i].owner].x;
        sprite[i].y = sprite[sprite[i].owner].y;
        sprite[i].z = sprite[sprite[i].owner].z;
    }
}
if (flags & SLOW_TO_STOP)
    sprite[i].xvel >>= 1;
if (flags & FACE_PLAYER)
    sprite[i].ang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
if (flags & GET_H)
    sprite[i].xvel += (movement[0] - sprite[i].xvel) >> 1;
if (flags & GET_V)
    sprite[i].zvel += ((movement[1] << 4) - sprite[i].zvel) >> 1;
if (flags & HOVER) {
    if (sprite[i].z + 8192 < sector[sprite[i].sectnum].floorz)
        sprite[i].zvel += movement[1] << 4;
    else if (sprite[i].z + 8192 > sector[sprite[i].sectnum].floorz)
        sprite[i].zvel -= movement[1] << 4;
}
if (flags & STICK_TO_CEILING)
    sprite[i].z = sector[sprite[i].sectnum].ceilingz + 2048;
if (flags & STICK_TO_FLOOR)
    sprite[i].z = sector[sprite[i].sectnum].floorz - 2048;
if (flags & FACE_TO_PLAYER) {
    goal = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
    sprite[i].ang += getincangle_shift(sprite[i].ang, goal, 1);
}
if (sprite[i].xvel | sprite[i].zvel) {
    if (!ssp(i, 1) && (flags & RHR) && (sector[sprite[i].sectnum].lotag & 32767) != 1) {
        sprite[i].ang += 256;
        ssp(i, 1);
    }
}
