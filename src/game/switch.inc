/* Wall switch dispatch is also present at the end of checksectors. */
{
    short i, j, lotag;

    switch (wall[w].picnum) {
    case 395: case 396: case 405: case 406:
        break;
    default:
        return;
    }
    lotag = wall[w].lotag;
    for (i = 0; i < numanimwalls; i++) {
        j = animwall[i].wallnum;
        if (lotag == wall[j].lotag)
            switch (wall[j].picnum) {
            case 395: case 405:
                wall[j].picnum++;
                break;
            case 396: case 406:
                wall[j].picnum--;
                break;
            }
    }

    switch (wall[w].picnum) {
    case 395:
#ifdef SWITCH_EFFECTORS_INLINED
#include "wallactivate.inc"
        break;
#endif
    case 396:
#ifdef SWITCH_EFFECTORS_INLINED
#include "wallactivate.inc"
#else
        FTA(4);
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operatemasterswitches(lotag);
#endif
        break;
    case 405:
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == 12 && sprite[i].hitag == wall[w].lotag)
                hittype[i][0] = 2;
#ifdef SWITCH_EFFECTORS_INLINED
#include "wallforcefield.inc"
        break;
#else
        goto toggle_field;
#endif
    case 406:
        for (i = headspritestat[3]; i >= 0; i = nextspritestat[i])
            if (sprite[i].lotag == 12 && sprite[i].hitag == wall[w].lotag)
                hittype[i][0] = 1;
#ifdef SWITCH_EFFECTORS_INLINED
#include "wallforcefield.inc"
#else
toggle_field:
        xyzsound(25, -1, wall[w].x, wall[w].y, 0);
        operateactivators(lotag);
        operateforcefields(lotag);
#endif
        break;
    }
}
