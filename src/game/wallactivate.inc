/* Each shot-switch position expands the activator and master-switch walks. */
        FTA(4);
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        for (i = headspritestat[8]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == lotag) {
                if (sprite[i].picnum == 23)
                    sector[sprite[i].sectnum].lotag ^= 0x4000;
                else
                    operatesectors(sprite[i].sectnum, i);
            }
        for (i = headspritestat[1]; i >= 0; i = nextspritestat[i])
            if (sprite[i].picnum == MASTERSWITCH && sprite[i].lotag == lotag && sprite[i].yvel == 0)
                sprite[i].yvel = 1;
