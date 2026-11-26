/* Shared pistol tracer body. The caller supplies short p. */
    short hitsprite, hitwall, hitsect, i;
    short ang;
    long hitz, hity, hitx, size;
    long weapon_pos;

    weapon_pos = ps[p].weapon_pos & 127;
    ang = ps[p].ang - 16 + ((6 - weapon_pos) << 4);
    hitscan(ps[p].posx + (sintable[(ang + 512) & 2047] >> 8),
        ps[p].posy + (sintable[ang & 2047] >> 8),
        ps[p].posz - (weapon_pos << 10) + 5376, ps[p].cursectnum,
        sintable[(ang + (player_avel[p] >> 3) + 512) & 2047],
        sintable[(ang + (player_avel[p] >> 3)) & 2047],
        (100 - ps[p].horiz) << 11, &hitsect, &hitwall, &hitsprite,
        &hitx, &hity, &hitz);

    ang += 1024;
    i = EGS(hitsect, hitx + (sintable[(ang + 512) & 2047] >> 8),
        hity + (sintable[ang & 2047] >> 8),
        hitz + 1024 + (sintable[ps[p].bobcounter & 2047] >> 8),
        0, LASERSITE, -16, 12, 12, ang, 0, 0, ps[p].i, 5);
    size = (ksqrt((ps[screenpeek].posx - sprite[i].x) * (ps[screenpeek].posx - sprite[i].x)
        + (ps[screenpeek].posy - sprite[i].y) * (ps[screenpeek].posy - sprite[i].y)) >> 8) + 1;
    if (size > 32)
        size = 32;
    sprite[i].xrepeat = sprite[i].yrepeat = size;
    if (sector[sprite[i].sectnum].floorz - 768 < sprite[i].z)
        sprite[i].z -= 768;
