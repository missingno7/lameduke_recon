#ifndef OPERATE_HELPERS_H
#define OPERATE_HELPERS_H

/* The public dispatcher expands the setter. The copies inside caller
   publics use the callable entry instead. */
#define SET_ANIMATION_INLINE(sn_, ptr_, goal_, vel_, result_) \
    do { \
        short sector_ = (sn_); \
        long *position = (ptr_); \
        long target = (goal_); \
        long velocity = (vel_); \
        long index, slot; \
        if (animatecnt >= MAXANIMATES - 1) { \
            (result_) = -1; \
        } else { \
            slot = animatecnt; \
            for (index = 0; index < animatecnt; index++) \
                if (position == animateptr[index]) { \
                    slot = index; \
                    break; \
                } \
            animatesect[slot] = sector_; \
            animateptr[slot] = position; \
            animategoal[slot] = target; \
            animatevel[slot] = velocity; \
            if (slot == animatecnt) \
                animatecnt++; \
            (result_) = animatecnt - 1; \
        } \
    } while (0)

/* Lotag 15 expands this traversal only in the public dispatcher. */
#define WARP_ELEVATORS_INLINE(effector_, direction_) \
    do { \
        short lift_sprite = (effector_); \
        short lift_dir = (direction_); \
        short e; \
        e = headspritestat[3]; \
        while (e >= 0) { \
            if (sprite[e].lotag == 17 && sprite[e].hitag == sprite[lift_sprite].hitag) \
                if (klabs(sector[sprite[lift_sprite].sectnum].floorz - hittype[lift_sprite][2]) > sprite[e].yvel || \
                    sector[sprite[e].sectnum].hitag == sector[sprite[lift_sprite].sectnum].hitag - lift_dir) \
                    break; \
            e = nextspritestat[e]; \
        } \
        if (e == -1) \
            lift_dir = 0; \
        else if (lift_dir == 0) \
            spritesound(SOUND_LIFT_STOP, lift_sprite); \
        else \
            spritesound(SOUND_LIFT_START, lift_sprite); \
        for (e = headspritestat[3]; e >= 0; e = nextspritestat[e]) \
            if (sprite[e].lotag == 17 && sprite[e].hitag == sprite[lift_sprite].hitag) \
                hittype[e][1] = hittype[e][0] = lift_dir; \
    } while (0)

/* The caller copies retain these calls. Separate C aliases keep Watcom from
   inlining the short bodies a second time. */
extern short query_animation(long *position);
extern short player_in_sector(short sn);
#pragma aux query_animation "getanimationgoal_";
#pragma aux player_in_sector "checkcursectnums_";

#endif
