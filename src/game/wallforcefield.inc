/* Each shot-switch position expands this field toggle and activator walk. */
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        for (i = headspritestat[8]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == lotag) {
                if (sprite[i].picnum == 23)
                    sector[sprite[i].sectnum].lotag ^= 0x4000;
                else
                    operatesectors(sprite[i].sectnum, i);
            }
        for (i = 0; i < numanimwalls; i++) {
            j = animwall[i].wallnum;
            if (lotag == wall[j].lotag || lotag == -1)
                switch (wall[j].overpicnum) {
                case FORCEFIELD: case FORCEFIELD + 1: case FORCEFIELD + 2:
                    animwall[i].tag = 0;
                    if (wall[j].cstat) {
                        wall[j].cstat = 0;
                        sector[wall[j].nextsector].ceilingpal = sector[wall[j].nextsector].floorpal = 0;
                    } else {
                        wall[j].cstat = 85;
                        sector[wall[j].nextsector].ceilingpal = sector[wall[j].nextsector].floorpal = 2;
                    }
                    break;
                }
        }
