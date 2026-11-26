/* Shared air update body. The caller supplies short p. */
    if ((sector[ps[p].cursectnum].lotag & 32767) == 2) {
        if ((krand() >> 8) >= 239)
            spritesound(10, ps[p].i);
        if (ps[p].scuba_on == 0) {
            if (ps[p].airleft > 0)
                ps[p].airleft -= 32;
            else if (disable_armor == 0 && ps[p].armor > 0)
                ps[p].armor -= 4;
            else if (ps[p].health > 0)
                ps[p].health--;
            else
                ps[p].health = 0;
            FX_SetVolume(sound_volume);
        } else {
            FX_SetVolume(sound_volume - 32);
            if (--ps[p].airleft < 1) {
                ps[p].scuba_on = 0;
                ps[p].airleft = 1728;
            }
        }
    } else {
        FX_SetVolume(sound_volume);
        if (ps[p].scuba_on == 0)
            ps[p].airleft = 1728;
    }
