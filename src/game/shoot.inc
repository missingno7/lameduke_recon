/* Shared projectile dispatch. The caller supplies i, tile and move_to_hit. */
do {
    long x, y, z, zvel, distance;
    short angle, sect, hitsect, hitwall, hitsprite, p, j;

    if (move_to_hit) {
        hitscan(sprite[i].x, sprite[i].y, sprite[i].z, sprite[i].sectnum,
            sintable[(sprite[i].ang + 512) & 2047], sintable[sprite[i].ang & 2047], 0,
            &hitsect, &hitwall, &hitsprite, &x, &y, &z);
        if (ksqrt((sprite[i].x - x) * (sprite[i].x - x)
            + (sprite[i].y - y) * (sprite[i].y - y)) > 512)
            break;
        if (sector[sprite[i].sectnum].ceilingz + 2560 <= sprite[i].z
            || checkhitceiling(sprite[i].sectnum) == 0) {
            setsprite(i, x, y, z);
            if (hitwall >= 0) {
                checkhitswitch(hitwall);
                checkhitwall(hitwall, x, y, z, tile);
            }
        }
        break;
    }

    if (sprite[i].picnum == APLAYER) {
        p = sprite[i].yvel;
        x = ps[p].posx + (player_avel[p] >> 4);
        y = ps[p].posy + (player_avel[p] >> 4);
        z = ps[p].posz + 3584;
        angle = ps[p].ang - 8;
        if (tile == RPG)
            angle += ps[p].lookang;
        sect = ps[p].cursectnum;
        ps[p].muzzle_flash = 1;
        switch (tile) {
        case SHOTSPARK1:
            zvel = (100 - ps[p].horiz) * 31;
            break;
        case RPG:
            zvel = (100 - ps[p].horiz) * 15;
            z -= 1536;
            break;
        default:
            zvel = (100 - ps[p].horiz) * 22;
            break;
        }
        if (ps[p].flash_sprite)
            sprite[ps[p].flash_sprite].shade = 0;
    } else {
        x = sprite[i].x;
        y = sprite[i].y;
        z = sprite[i].z - 8192;
        angle = sprite[i].ang;
        p = findplayer(i, &distance);
        j = ps[p].i;
        zvel = ((ps[p].posz - sprite[i].z) << 7)
            / (klabs(sprite[j].x - sprite[i].x) + klabs(sprite[j].y - sprite[i].y));
        sect = sprite[i].sectnum;
    }

    switch ((unsigned short)tile) {
    case CLUB:
        x += sintable[(angle + 544) & 2047] >> 7;
        y += sintable[(angle + 32) & 2047] >> 7;
        z -= 2048;
        EGS(sect, x, y, z, 0, 46, -16, 0, 0, angle, 32, 0, i, 4);
        if (actorscrptr[CLUB])
            sprite[i].extra = *actorscrptr[CLUB];
        break;
    case SHOTSPARK1:
        angle -= 8;
        hitscan(x, y, z, sect, sintable[(angle + 512) & 2047],
            sintable[angle & 2047], zvel << 6, &hitsect, &hitwall, &hitsprite, &x, &y, &z);
        spritesound(31, i);
        if (sector[hitsect].ceilingz + 1024 > z)
            z = sector[hitsect].ceilingz + 2048;
        if (sprite[i].picnum == APLAYER) {
            j = EGS(hitsect, x, y, z - 2816, 0, OTHERSHOTSPARK, -16, 10, 10, angle, 64, zvel, i, 4);
            sprite[j].yvel = hitwall;
        } else
            EGS(hitsect, x, y, z - 2304, 0, SHOTSPARK1, -16, 16, 16, angle, 64, zvel, i, 4);
        break;
    case BULLET:
        spritesound(1, i);
        spritesound(24, i);
        EGS(sect, x + (sintable[(angle + 512) & 2047] >> 6),
            y + (sintable[angle & 2047] >> 6), z - 2048,
            1, BULLET, -127, 12, 12, angle, 0, zvel, i, 4);
        break;
    case RPG:
        spritesound(38, i);
        EGS(sect, x + (sintable[(angle + 640) & 2047] >> 5),
            y + (sintable[(angle + 128) & 2047] >> 5), z - 2304,
            65, RPG, -16, 16, 16, angle, 0, zvel, i, 4);
        break;
    }
} while (0);
