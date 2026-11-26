/* Shared opcode bodies. The caller supplies its continuation and exit. */
switch (*insptr) {
case CON_IFRND:
    insptr++;
    PARSE_IF((krand() >> 8) >= 255 - *insptr);
    CON_NEXT();

case CON_IFCANSEE:
    if (cached_cansee == -1) {
        angle = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
        difference = getincangle_shift(sprite[i].ang, angle, 0);
        if (difference > -128 && difference < 128)
            cached_cansee = cansee(sprite[i].x, sprite[i].y,
                sprite[i].z - (sprite[i].yrepeat << 7), sprite[i].sectnum,
                ps[p].posx, ps[p].posy, ps[p].posz - (16 << 8), ps[p].cursectnum);
        else
            cached_cansee = 0;
    }
    PARSE_IF(cached_cansee);
    CON_NEXT();

case CON_IFHITWEAPON:
    hit_sprite = ifhitbyweapon(i);
    PARSE_IF(hit_sprite >= 0);
    CON_NEXT();

case CON_IFSQUISHED:
    PARSE_IF(ifsquished(i, p) >= 0);
    CON_NEXT();

case CON_IFDEAD:
    if (sprite[i].picnum == APLAYER)
        j = ps[sprite[i].yvel].health - 1;
    else
        j = sprite[i].extra;
    PARSE_IF(j < 0);
    CON_NEXT();

case CON_ACTION:
    hittype[i].actioncount = 0;
    hittype[i].actionframe = 0;
    insptr++;
    hittype[i].action = (long *)*insptr;
    insptr++;
    CON_NEXT();

case CON_IFPDISTL:
    insptr++;
    PARSE_IF(x < *insptr);
    CON_NEXT();

case CON_IFPDISTG:
    insptr++;
    PARSE_IF(x > *insptr);
    CON_NEXT();

case CON_ELSE:
    insptr = (long *)insptr[1];
    CON_NEXT();

case CON_STRENGTH:
    insptr++;
    if (sprite[i].picnum == APLAYER)
        ps[sprite[i].yvel].health = *insptr;
    else
        sprite[i].extra = *insptr;
    insptr++;
    CON_NEXT();

case CON_CACTOR:
    insptr++;
    sprite[i].picnum = *insptr++;
    CON_NEXT();

case CON_SHOOT:
    insptr++;
    shoot(i, *insptr);
    insptr++;
    CON_NEXT();

case CON_ADDSCORE:
    insptr++;
    ps[p].score += *insptr;
    insptr++;
    CON_NEXT();

case CON_SOUND:
    insptr++;
    spritesound(*insptr, i);
    insptr++;
    CON_NEXT();

case CON_FALL:
    insptr++;
    if (sprite[i].z < sector[sprite[i].sectnum].floorz) {
        if (sprite[i].zvel < 2048)
            sprite[i].zvel += 256;
        sprite[i].z += sprite[i].zvel;
    } else {
        sprite[i].zvel = 0;
        sprite[i].z = sector[sprite[i].sectnum].floorz;
    }
    CON_NEXT();

case CON_RBRACE:
    insptr++;
    CON_STOP();

case CON_ADDBATTERYAMMO:
    insptr++;
    ps[p].batteryammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_NUKE:
    insptr++;
    nuke(sprite[i].x, sprite[i].y, sprite[i].z, i);
    CON_NEXT();

case CON_KILLIT:
    killit_flag = 1;
    insptr++;
    CON_NEXT();

case CON_ADDWEAPON:
    insptr++;
    addweapon(p, *insptr, 0);
    insptr++;
    CON_NEXT();

case CON_WEAPON:
    insptr += 2;
    CON_NEXT();

case CON_ADDHEALTH:
    insptr++;
    if (!disable_armor && ps[p].armor)
        ps[p].armor -= 4;
    else if (ps[p].health > 0)
        ps[p].health += *insptr;
    else
        ps[p].health = 0;
    insptr++;
    CON_NEXT();

case CON_STATE:
    resume = insptr + 2;
    insptr = (long *)insptr[1];
    while (!parse(i, p, x))
        ;
    insptr = resume;
    CON_NEXT();

case CON_LBRACE:
    insptr++;
    while (!parse(i, p, x))
        ;
    CON_NEXT();

case CON_MOVE:
    hittype[i].movecount = 0;
    insptr++;
    hittype[i].move = (long *)*insptr++;
    sprite[i].hitag = *insptr++;
    if (sprite[i].hitag & FACE_EXIT)
        sprite[i].ang = furthestangle(i, 8);
    CON_NEXT();

case CON_SPAWN:
    insptr++;
    spawn(i, *insptr);
    insptr++;
    CON_NEXT();

case CON_IFWASWEAPON:
    insptr++;
    PARSE_IF(hit_sprite >= 0 && sprite[hit_sprite].picnum == *insptr);
    CON_NEXT();

case CON_IFACTION:
    insptr++;
    PARSE_IF((long)hittype[i].action == *insptr);
    CON_NEXT();

case CON_IFACTIONCOUNT:
    insptr++;
    PARSE_IF(hittype[i].actioncount >= *insptr);
    CON_NEXT();

case CON_RESETACTIONCOUNT:
    insptr++;
    hittype[i].actioncount = 0;
    CON_NEXT();

case CON_DEBRIS:
    insptr++;
    for (j = 0; j < *insptr; j++)
        EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z, 0,
            SCRAP1 + krand() % 11, -8, 32, 32, krand() & 2047,
            64 + (krand() & 128), -512 - (krand() & 511), i, 5);
    insptr++;
    CON_NEXT();

case CON_IFSKILL:
    insptr++;
    PARSE_IF(ps[p].skill < *insptr);
    CON_NEXT();

case CON_CSTAT:
    insptr++;
    sprite[i].cstat = *insptr++;
    CON_NEXT();

case CON_IFMOVE:
    insptr++;
    PARSE_IF((long)hittype[i].move == *insptr);
    CON_NEXT();

case CON_RESETPLAYER:
    insptr++;
    resetplayer(p);
    CON_NEXT();

case CON_IFONWATER:
    PARSE_IF((sector[sprite[i].sectnum].lotag & 32767) == 1 &&
        sector[sprite[i].sectnum].floorz - sprite[i].z < (sprite[i].yrepeat << 7));
    CON_NEXT();

case CON_IFINWATER:
    PARSE_IF((sector[sprite[i].sectnum].lotag & 32767) == 2);
    CON_NEXT();

case CON_IFPLAYERSWEAPON:
    insptr++;
    PARSE_IF(ps[p].curr_weapon == *insptr);
    CON_NEXT();

case CON_IFMOVECOUNT:
    insptr++;
    PARSE_IF(hittype[i].movecount >= *insptr);
    CON_NEXT();

case CON_RESETMOVECOUNT:
    insptr++;
    hittype[i].movecount = 0;
    CON_NEXT();

case CON_ADDINVENTORY:
    insptr++;
    switch (*insptr) {
    case 0: ps[p].inventory[2] = 1; break;
    case 1: ps[p].inventory[3] = 1; break;
    case 2: ps[p].inventory[0] = 1; break;
    case 3: ps[p].inventory[4] = 1; break;
    case 4: ps[p].inventory[1] = 1; break;
    }
    insptr++;
    CON_NEXT();

case CON_ADDAMMO:
    insptr++;
    ps[p].ammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_HITRADIUS:
    hitradius(i, insptr[1], insptr[2]);
    insptr += 3;
    CON_NEXT();

case CON_IFP:
    insptr++;
    flags = *insptr;
    speed = player_vel[p] + player_svel[p];
    PARSE_IF(((flags & 8) && ps[p].on_ground && (player_bits[p] & 2)) ||
        ((flags & 16) && !ps[p].poszv && !ps[p].on_ground &&
            sector[ps[p].cursectnum].floorz - ps[p].posz > 18432) ||
        ((flags & 32) && ps[p].poszv > 348) ||
        ((flags & 1) && speed >= 0 && speed < 8) ||
        ((flags & 2) && speed >= 8 && !(player_bits[p] & 32)) ||
        ((flags & 4) && speed >= 8 && (player_bits[p] & 32)) ||
        ((flags & 64) && sprite[i].z - 12288 > ps[p].posz) ||
        ((flags & 128) && speed <= -8 && !(player_bits[p] & 32)) ||
        ((flags & 256) && speed <= -8 && (player_bits[p] & 32)) ||
        ((flags & 512) && ps[p].horiz < -128) ||
        ((flags & 1024) && ps[p].horiz > 128) ||
        ((flags & 2048) ?
            ((((sprite[i].ang + 3200 - sprite[facing_sprite].ang) & 2047) >> 8) < 2) :
            (((flags & 4096) && ps[p].shooting) ||
             ((flags & 8192) && ps[p].on_ground) ||
             ((flags & 16384) && ps[p].jetpack_on))));
    CON_NEXT();

case CON_ADDRAMMO:
    insptr++;
    ps[p].rammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_SINK:
    sprite[i].z += 4096;
    sprite[i].xvel >>= 1;
    insptr++;
    CON_NEXT();

case CON_IFSTRENGTH:
    insptr++;
    if (sprite[i].picnum == APLAYER)
        j = ps[sprite[i].yvel].health;
    else
        j = sprite[i].extra;
    PARSE_IF(j <= *insptr);
    CON_NEXT();

case CON_BLOOD:
    insptr++;
    for (j = 0; j < *insptr; j++)
        EGS(sprite[i].sectnum, sprite[i].x, sprite[i].y, sprite[i].z - 8192,
            krand() & 2, BLOOD, -16, 8 + (krand() & 7), 8 + (krand() & 7),
            sprite[i].ang + (krand() & 127), 0, -1024 - (krand() & 255), i, 5);
    insptr++;
    CON_NEXT();

case CON_GUTS:
    insptr++;
    guts(i);
    CON_NEXT();

case CON_IFSPAWNEDBY:
    insptr++;
    PARSE_IF(sprite[i].owner >= 0 && sprite[i].owner == *insptr);
    CON_NEXT();

case CON_ENDA:
case CON_BREAK:
case CON_ENDS:
    CON_STOP();

default:
    gameexit(ltoa(sprite[i].picnum, tempbuf, 10));
    CON_STOP();
}
