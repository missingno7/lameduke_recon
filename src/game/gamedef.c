#include <conio.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gamedef.h"

char *keyw[NUMKEYWORDS] = {
    "end", "actor", "addbatteryammo", "ifrnd", "enda",
    "ifcansee", "ifhitweapon", "action", "ifpdistl", "ifpdistg",
    "else", "strength", "break", "shoot", "addscore",
    "sound", "fall", "state", "ends", "define",
    "//", "nuke", "killit", "addweapon", "weapon",
    "addhealth", "ifdead", "ifsquished", "cactor", "{",
    "}", "spawn", "move", "ifwasweapon", "ifaction",
    "ifactioncount", "resetactioncount", "debris", "ifskill", "/*",
    "cstat", "ifmove", "resetplayer", "ifonwater", "ifinwater",
    "ifplayersweapon", "ifmovecount", "resetmovecount", "addinventory", "addammo",
    "hitradius", "ifp", "addrammo", "sink", "music",
    "include", "ifstrength", "blood", "guts", "ifspawnedby"
};

/* Compiler and interpreter state, this unit's _BSS (2:b1df0-b1dff); slots come from gamedef.h. */
char *last_used_text;
short line_number, total_lines;
char parsing_state;
signed char brace_depth;
unsigned char checking_ifelse;
short cached_cansee, hit_sprite;
#pragma pack(1)
struct user_settings {          /* the 21-byte USERDEFS.TMP block defined by GLOBAL */
    unsigned char visibility, quotes, brightness, sound, music;
    short debug, sound_volume, music_volume, paused, camera, mouse_sensitivity;
    unsigned char reserved[4];
};
#pragma pack()
extern struct user_settings userdefs;

/* dataown: one object, one name. The spellings below are the GLOBAL definitions or fields of
   its settings block; the extern declarations above keep this unit's symbol slots. */
#define disable_armor userdefs.debug
#define facing_sprite userdefs.camera

short getincangle_shift(short a, short na, short shift)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a - na) < 1024)
        return (na - a) >> shift;
    return (((a + 1024) & 2047) - na) >> shift;
}

char ispecial(char c)
{
    if (c == '\n') {
        line_number++;
        return 1;
    }
    if (c == ' ' || c == '\r')
        return 1;
    return 0;
}

#define TOKEN_CHAR(c) (isalnum(c) || (c) == '{' || (c) == '}' || (c) == '/' \
    || (c) == '*' || (c) == '-' || (c) == '_' || (c) == '.')

char isaltok(char c)
{
    return TOKEN_CHAR(c);
}

void getlabel(void)
{
    long i;

    while (!isalnum(*textptr)) {
        if (*textptr == '\n')
            line_number++;
        textptr++;
        if (*textptr == 0)
            return;
    }

    i = 0;
    while (!ispecial(*textptr))
        label[(labelcnt << 6) + i++] = *textptr++;
    label[(labelcnt << 6) + i] = 0;
}

long keyword(void)
{
    long i;
    char *p, c;

    p = textptr;
    while (!(c = *p, TOKEN_CHAR(c))) {
        p++;
        if (*p == 0)
            return 0;
    }

    i = 0;
    while ((c = *p, TOKEN_CHAR(c))) {
        tempbuf[i] = *p++;
        i++;
    }
    tempbuf[i] = 0;

    for (i = 0; i < NUMKEYWORDS; i++)
        if (strcmp(tempbuf, keyw[i]) == 0)
            return i;
    return -1;
}

long transword(void)
{
    long l, i;
    char c;

    while (!(c = *textptr, TOKEN_CHAR(c))) {
        if (*textptr == '\n')
            line_number++;
        if (*textptr == 0)
            return -1;
        textptr++;
    }

    l = 0;
    while ((c = textptr[l], TOKEN_CHAR(c))) {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for (i = 0; i < NUMKEYWORDS; i++)
        if (strcmp(tempbuf, keyw[i]) == 0) {
            *scriptptr = i;
            textptr += l;
            scriptptr++;
            return i;
        }

    for (i = 0; i < labelcnt; i++)
        if (strcmp(tempbuf, label + (i << 6)) == 0) {
            *scriptptr = labelcode[i];
            textptr += l;
            scriptptr++;
            return labelcode[i];
        }

    textptr += l;
    printf("  * ERROR!(L%ld) Symbol '%s' is undefined.\n", (long)line_number, tempbuf);
    error++;
    return -1;
}

void transnum(void)
{
    long i, l;
    char c;

    while (!(c = *textptr, TOKEN_CHAR(c))) {
        if (*textptr == '\n')
            line_number++;
        textptr++;
        if (*textptr == 0)
            return;
    }

    l = 0;
    while ((c = textptr[l], TOKEN_CHAR(c))) {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for (i = 0; i < labelcnt; i++)
        if (strcmp(tempbuf, label + (i << 6)) == 0) {
            *scriptptr++ = labelcode[i];
            textptr += l;
            return;
        }

    if (!isdigit(*textptr) && *textptr != '-') {
        printf("  * ERROR!(L%ld) Paramater '%s' is undefined.\n", (long)line_number, tempbuf);
        error++;
        textptr += l;
        return;
    }

    *scriptptr = atoi(textptr);
    scriptptr++;
    textptr += l;
}

#define CURRENT_LABEL (label + (labelcnt << 6))



/* The parsecommand copies call the character classifiers out of line. */
extern char special_char(char c);
extern char token_char(char c);
#pragma aux special_char "ispecial_";
#pragma aux token_char "isaltok_";
/* getlabel written out at each parsecommand site */
#define GET_LABEL(done) do { \
    long n; \
    while (!isalnum(*textptr)) { \
        if (*textptr == '\n') \
            line_number++; \
        textptr++; \
        if (*textptr == 0) \
            goto done; \
    } \
    n = 0; \
    while (!special_char(*textptr)) \
        label[(labelcnt << 6) + n++] = *textptr++; \
    label[(labelcnt << 6) + n] = 0; \
done: ; \
} while (0)


/* keyword lookahead written out at each parsecommand site: result in k_, done_ ends the copy */
#define KEYWORD(k_, done_) do { \
    char *p = textptr; \
    long n; \
    while (!token_char(*p)) { \
        p++; \
        if (*p == 0) { \
            (k_) = 0; \
            goto done_; \
        } \
    } \
    n = 0; \
    while (token_char(*p)) \
        tempbuf[n++] = *p++; \
    tempbuf[n] = 0; \
    for ((k_) = 0; (k_) < NUMKEYWORDS; (k_)++) \
        if (strcmp(tempbuf, keyw[(k_)]) == 0) \
            goto done_; \
    (k_) = -1; \
done_: ; \
} while (0)

char parsecommand(void)
{
    long i, j, k, *branch;
    short tw, saved_line;
    int fd;
    char *saved_text, saved_ifelse, c;

    if (warning + error > 12)
        return 1;
    if (*textptr == 0 || textptr[1] == 0)
        return 1;

    tw = (short)transword();
    switch (tw) {
    case CON_BLOCKCOMMENT:
        scriptptr--;
        j = line_number;
        do {
            textptr++;
            if (*textptr == '\n')
                line_number++;
            if (*textptr == 0) {
                printf("  * ERROR!(L%ld) Found '/*' with no '*/'.\n", j, CURRENT_LABEL);
                error++;
                return 0;
            }
        } while (*textptr != '*' || textptr[1] != '/');
        textptr += 2;
        return 0;

    case CON_STATE:
        if (!actor_header && !parsing_state) {
            GET_LABEL(label_done1);
            parsing_state = 1;
            labelcode[labelcnt++] = (long)--scriptptr;
            return 0;
        }
        GET_LABEL(label_done2);
        for (i = 0; i < labelcnt; i++) {
            if (!strcmp(CURRENT_LABEL, label + (i << 6))) {
                *scriptptr = labelcode[i];
                break;
            }
        }
        if (i == labelcnt) {
            printf("  * ERROR!(L%ld) State '%s' not found.\n", (long)line_number, CURRENT_LABEL);
            error++;
        }
        scriptptr++;
        return 0;

    case CON_SOUND:
        GET_LABEL(label_done3);
        for (i = 0; i < 39; i++) {
            if (!strcmp(sound_names[i], CURRENT_LABEL)) {
                *scriptptr++ = i;
                return 0;
            }
        }
        if (i == 39) {
            printf("  * ERROR!(L%ld) Sound '%s' not found.\n", (long)line_number, CURRENT_LABEL);
            error++;
        }
        return 0;

    case CON_ENDS:
        if (!parsing_state) {
            printf("  * ERROR!(L%ld) Found 'ends' with no 'state'.\n", (long)line_number);
            error++;
            return 0;
        }
        parsing_state = 0;
        return 0;

    case CON_DEFINE:
        GET_LABEL(label_done4);
        for (i = 0; i < labelcnt; i++) {
            if (!strcmp(CURRENT_LABEL, label + (i << 6))) {
                warning++;
                printf("  * WARNING.(L%ld) Dupiclate definition '%s' ignored.\n",
                    (long)line_number, CURRENT_LABEL);
                break;
            }
        }
        transnum();
        if (i == labelcnt)
            labelcode[labelcnt++] = scriptptr[-1];
        scriptptr -= 2;
        return 0;

    case CON_END:
        scriptptr--;
        *scriptptr = (-2147483647L - 1);
        return 1;

    case CON_MOVE:
        if (actor_header || parsing_state) {
            transnum();
            j = 0;
            for (;;) {
                KEYWORD(k, move_flags);
                if (k != -1)
                    break;
                transnum();
                j |= *--scriptptr;
            }
            *scriptptr++ = j;
            return 0;
        }
        scriptptr--;
        GET_LABEL(label_done5);
        for (i = 0; i < labelcnt; i++) {
            if (!strcmp(CURRENT_LABEL, label + (i << 6))) {
                warning++;
                printf("  * WARNING.(L%ld) Dupiclate move '%s' ignored.\n",
                    (long)line_number, CURRENT_LABEL);
                break;
            }
        }
        if (i == labelcnt)
            labelcode[labelcnt++] = (long)scriptptr;
        for (i = 0; i < 2; i++) {
            KEYWORD(k, move_args);
            if (k >= 0)
                break;
            transnum();
        }
        for (; i < 2; i++)
            *scriptptr++ = 0;
        return 0;

    case CON_MUSIC:
        scriptptr--;
        transnum();
        k = *--scriptptr - 1;
        if (k >= 0) {
            for (j = 0; j < 10; j++) {
                KEYWORD(i, music_level);
                if (i != -1)
                    break;
                while (!(c = *textptr, TOKEN_CHAR(c))) {
                    if (*textptr == '\n')
                        line_number++;
                    if (*++textptr == 0)
                        break;
                }
                i = 0;
                while ((c = textptr[i], TOKEN_CHAR(c))) {
                    music_files[k][j][i] = textptr[i];
                    i++;
                }
                textptr += i;
                music_files[k][j][i] = 0;
            }
            for (i = 9; i > j; i--)
                strcpy(music_files[k][i], music_files[k][j]);
        } else {
            for (j = 0; j < 10; j++) {
                KEYWORD(i, music_env);
                if (i != -1)
                    break;
                while (!(c = *textptr, TOKEN_CHAR(c))) {
                    if (*textptr == '\n')
                        line_number++;
                    if (*++textptr == 0)
                        break;
                }
                i = 0;
                while ((c = textptr[i], TOKEN_CHAR(c))) {
                    env_music[j][i] = textptr[i];
                    i++;
                }
                textptr += i;
            }
            for (i = 9; i > j; i--)
                strcpy(env_music[i], env_music[j]);
        }
        return 0;

    case CON_INCLUDE:
        scriptptr--;
        while (!(c = *textptr, TOKEN_CHAR(c))) {
            if (*textptr == '\n')
                line_number++;
            if (*++textptr == 0)
                break;
        }
        i = 0;
        while ((c = *textptr, TOKEN_CHAR(c)))
            tempbuf[i++] = *textptr++;
        tempbuf[i] = 0;
        printf("    Loading include file %s\n", tempbuf);
        fd = open(tempbuf, O_BINARY | O_RDWR, S_IREAD);
        j = filelength(fd);
        saved_line = line_number;
        saved_ifelse = checking_ifelse;
        checking_ifelse = 0;
        saved_text = textptr;
        textptr = bootleg_cache_alloc(j);
        textptr[j] = 0;
        if (!textptr) {
            puts("  * ERROR! Not enough memory.");
            exit(0);
        }
        read(fd, textptr, j);
        close(fd);
        while (!parsecommand())
            ;
        total_lines += line_number;
        textptr = saved_text;
        line_number = saved_line;
        checking_ifelse = saved_ifelse;
        bootleg_cache_pop();
        return 0;

    case CON_ACTION:
        if (actor_header || parsing_state) {
            transnum();
            return 0;
        }
        scriptptr--;
        GET_LABEL(label_done6);
        for (i = 0; i < labelcnt; i++) {
            if (!strcmp(CURRENT_LABEL, label + (i << 6))) {
                warning++;
                printf("  * WARNING.(L%ld) Dupiclate action '%s' ignored.\n",
                    (long)line_number, CURRENT_LABEL);
                break;
            }
        }
        if (i == labelcnt)
            labelcode[labelcnt++] = (long)scriptptr;
        for (i = 0; i < 5; i++) {
            KEYWORD(k, action_args);
            if (k >= 0)
                break;
            transnum();
        }
        for (; i < 5; i++)
            *scriptptr++ = 0;
        return 0;

    case CON_ACTOR:
        if (parsing_state) {
            printf("  * ERROR!(L%ld) Found 'actor' within 'state'.\n", (long)line_number);
            error++;
        }
        if (actor_header) {
            printf("  * ERROR!(L%ld) Found 'actor' within 'actor'.\n", (long)line_number);
            error++;
        }
        brace_depth = 0;
        actor_header = --scriptptr;
        transnum();
        actorscrptr[*--scriptptr] = actor_header;
        for (i = 0; i < 4; i++)
            actor_header[i] = 0;
        for (i = 0; i < 4; i++) {
            if (i == 3) {
                j = 0;
                for (;;) {
                    KEYWORD(k, actor_flags);
                    if (k != -1)
                        break;
                    transnum();
                    j |= *--scriptptr;
                }
                *scriptptr++ = j;
                break;
            }
            KEYWORD(k, actor_args);
            if (k >= 0) {
                scriptptr += 4 - i;
                break;
            }
            transnum();
            actor_header[i] = scriptptr[-1];
        }
        checking_ifelse = 0;
        return 0;

    case CON_HITRADIUS:
        transnum();
        /* Fall through. */
    case CON_ADDBATTERYAMMO: case CON_STRENGTH: case CON_SHOOT: case CON_ADDSCORE:
    case CON_ADDWEAPON: case CON_WEAPON: case CON_ADDHEALTH: case CON_CACTOR:
    case CON_SPAWN: case CON_DEBRIS: case CON_CSTAT: case CON_ADDINVENTORY:
    case CON_ADDAMMO: case CON_ADDRAMMO: case CON_BLOOD:
        transnum();
        return 0;

    case CON_ELSE:
        if (checking_ifelse) {
            checking_ifelse--;
            branch = scriptptr++;
            parsecommand();
            *branch = (long)scriptptr;
        } else {
            scriptptr--;
            warning++;
            printf("  * WARNING.(L%ld) Found 'else' with no 'if', ignored.\n", (long)line_number);
        }
        return 0;

    case CON_IFRND: case CON_IFPDISTL: case CON_IFPDISTG: case CON_IFWASWEAPON:
    case CON_IFACTION: case CON_IFACTIONCOUNT: case CON_IFSKILL: case CON_IFMOVE:
    case CON_IFPLAYERSWEAPON: case CON_IFMOVECOUNT: case CON_IFSTRENGTH:
    case CON_IFSPAWNEDBY:
        transnum();
        /* Fall through. */
    case CON_IFCANSEE: case CON_IFHITWEAPON: case CON_IFDEAD: case CON_IFSQUISHED:
    case CON_IFONWATER: case CON_IFINWATER: case CON_IFP:
        if (tw == CON_IFP) {
            j = 0;
            for (;;) {
                KEYWORD(k, ifp_flags);
                if (k != -1)
                    break;
                transnum();
                j |= *--scriptptr;
            }
            *scriptptr++ = j;
        }
        branch = scriptptr++;
        do {
            KEYWORD(k, if_comment);
            if (k == CON_LINECOMMENT || k == CON_BLOCKCOMMENT)
                parsecommand();
        } while (k == CON_LINECOMMENT || k == CON_BLOCKCOMMENT);
        parsecommand();
        *branch = (long)scriptptr;
        checking_ifelse++;
        return 0;

    case CON_LBRACE:
        brace_depth++;
        while (!parsecommand())
            ;
        return 0;

    case CON_RBRACE:
        brace_depth--;
        return 1;

    case CON_LINECOMMENT:
        scriptptr--;
        while (*textptr != '\n')
            textptr++;
        line_number++;
        return 0;

    case CON_ENDA:
        if (!actor_header) {
            printf("  * ERROR!(L%ld) Found 'aend' without defining 'actor'.\n", (long)line_number);
            error++;
            return 0;
        }
        if (brace_depth > 0) {
            printf("  * ERROR!(L%ld) Found more '{' than '}' before 'aend'.\n", (long)line_number);
            error++;
        }
        if (brace_depth < 0) {
            printf("  * ERROR!(L%ld) Found more '}' than '{' before 'aend'.\n", (long)line_number);
            error++;
            return 0;
        }
        actor_header = 0;
        return 0;

    case -1:
    default:
        return 0;
    }
}

void passone(void)
{
    while (!parsecommand())
        ;
}

void loadefs(void)
{
    int fd;
    long size;

    fd = open("game.con", O_BINARY | O_RDWR, S_IREAD);
    if (fd) {
        puts("\nLoading script...");
        size = filelength(fd);
        textptr = bootleg_cache_alloc(size);
        label = bootleg_cache_alloc(MAXLABELS * LABELSIZE);
        labelcode = bootleg_cache_alloc(MAXLABELS * sizeof(long));
        if (!labelcode || !label) {
            puts("  * ERROR! Not enough memory.");
            exit(0);
        }
        read(fd, textptr, size);
        close(fd);
        last_used_text = textptr + size - 2;
        *last_used_text = 0;
        puts("    Compiling...");
        fill_dwords(actorscrptr, 0, MAXTILES);
        labelcnt = 0;
        scriptptr = script + 1;
        warning = 0;
        error = 0;
        line_number = 1;
        total_lines = 0;
        passone();
        script[0] = (long)scriptptr;

        printf("  * Found %ld warning(s) and %ld error(s).\n", (long)warning, (long)error);
        bootleg_cache_pop();
        bootleg_cache_pop();
        bootleg_cache_pop();
        if (!error && warning) {
            puts("  * Press any key to ignore, or ESC to exit.");
            if (getch() < ' ')
                exit(0);
        }
        if (error)
            exit(0);
        printf("    Total Lines:%ld.\n", (long)(total_lines += line_number));
        printf("    Code Size:%ld bytes.\n", (scriptptr - script) * (long)sizeof(long) - (long)sizeof(long));
    }
}

short furthestangle(short i, short angs)
{
    short a, hitwall, hitsect, best_angle, step;
    long hitx, hity, hitz, distance, best_distance;

    step = 2048 / angs;
    best_distance = (-2147483647L - 1);
    for (a = 0; a < 2048; a += step) {
        hitscan(sprite[i].x, sprite[i].y, sprite[i].z - (8 << 8),
            sprite[i].sectnum, sintable[(a + 512) & 2047], sintable[a & 2047], 0,
            &hitsect, &hitwall, &hitsect, &hitx, &hity, &hitz);
        distance = klabs(hitx - sprite[i].x) + klabs(hity - sprite[i].y);
        if (distance > best_distance) {
            best_distance = distance;
            best_angle = a;
        }
    }
    return best_angle;
}

#define SLOW_TO_STOP 1
#define FACE_PLAYER 2
#define FACE_EXIT 4
#define GET_H 8
#define GET_V 16
#define HOVER 32
#define STICK_TO_CEILING 64
#define FACE_TO_PLAYER 128
#define RHR 256
#define ON_OWNER 512
#define STICK_TO_FLOOR 1024

void move_actor(short i, short p, long x)
{
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
}

/* An if operand is followed by the address of its false branch. */
#define PARSE_IF(condition) do { \
    if (condition) { \
        insptr += 2; \
        parse(PARSE_ARGS); \
    } else { \
        insptr = (long *)insptr[1]; \
        if (*insptr == CON_ELSE) { \
            insptr += 2; \
            parse(PARSE_ARGS); \
        } \
    } \
} while (0)

#define APLAYER 283
#define SCRAP1 235
#define BLOOD 488

char parse(short i, short p, long x)
{
    long j, flags, speed, *resume;
    short angle, difference;

    if (killit_flag)
        return 1;

#define PARSE_ARGS i, p, x
#define CON_NEXT() return 0
#define CON_STOP() return 1
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
#undef CON_STOP
#undef CON_NEXT
#undef PARSE_ARGS
}

void execute(short i, short p, long x)
{
    if (!actorscrptr[sprite[i].picnum])
        return;

    insptr = actorscrptr[sprite[i].picnum];
    killit_flag = 0;
    hit_sprite = -1;
    cached_cansee = -1;
    insptr += 4;
    if (hittype[i].action) {
        sprite[i].lotag += script_tics;
        if (sprite[i].lotag > hittype[i].action[4]) {
            hittype[i].actioncount++;
            sprite[i].lotag = 0;
            hittype[i].actionframe += hittype[i].action[3];
        }
        if (klabs(hittype[i].actionframe) >= klabs(hittype[i].action[1] * hittype[i].action[3]))
            hittype[i].actionframe = 0;
    }

    {
    for (;;) {
        short i2 = i, p2 = p;
        long x2 = x;
        long j, flags, speed, *resume;
        short angle, difference;

        if (killit_flag)
            break;

#define PARSE_ARGS i2, p2, x2
#define CON_NEXT() continue
#define CON_STOP() goto end_script
/* Shared opcode bodies. The caller supplies its continuation and exit. */
switch (*insptr) {
case CON_IFRND:
    insptr++;
    PARSE_IF((krand() >> 8) >= 255 - *insptr);
    CON_NEXT();

case CON_IFCANSEE:
    if (cached_cansee == -1) {
        angle = getangle(ps[p2].posx - sprite[i2].x, ps[p2].posy - sprite[i2].y);
        difference = getincangle_shift(sprite[i2].ang, angle, 0);
        if (difference > -128 && difference < 128)
            cached_cansee = cansee(sprite[i2].x, sprite[i2].y,
                sprite[i2].z - (sprite[i2].yrepeat << 7), sprite[i2].sectnum,
                ps[p2].posx, ps[p2].posy, ps[p2].posz - (16 << 8), ps[p2].cursectnum);
        else
            cached_cansee = 0;
    }
    PARSE_IF(cached_cansee);
    CON_NEXT();

case CON_IFHITWEAPON:
    hit_sprite = ifhitbyweapon(i2);
    PARSE_IF(hit_sprite >= 0);
    CON_NEXT();

case CON_IFSQUISHED:
    PARSE_IF(ifsquished(i2, p2) >= 0);
    CON_NEXT();

case CON_IFDEAD:
    if (sprite[i2].picnum == APLAYER)
        j = ps[sprite[i2].yvel].health - 1;
    else
        j = sprite[i2].extra;
    PARSE_IF(j < 0);
    CON_NEXT();

case CON_ACTION:
    hittype[i2].actioncount = 0;
    hittype[i2].actionframe = 0;
    insptr++;
    hittype[i2].action = (long *)*insptr;
    insptr++;
    CON_NEXT();

case CON_IFPDISTL:
    insptr++;
    PARSE_IF(x2 < *insptr);
    CON_NEXT();

case CON_IFPDISTG:
    insptr++;
    PARSE_IF(x2 > *insptr);
    CON_NEXT();

case CON_ELSE:
    insptr = (long *)insptr[1];
    CON_NEXT();

case CON_STRENGTH:
    insptr++;
    if (sprite[i2].picnum == APLAYER)
        ps[sprite[i2].yvel].health = *insptr;
    else
        sprite[i2].extra = *insptr;
    insptr++;
    CON_NEXT();

case CON_CACTOR:
    insptr++;
    sprite[i2].picnum = *insptr++;
    CON_NEXT();

case CON_SHOOT:
    insptr++;
    shoot(i2, *insptr);
    insptr++;
    CON_NEXT();

case CON_ADDSCORE:
    insptr++;
    ps[p2].score += *insptr;
    insptr++;
    CON_NEXT();

case CON_SOUND:
    insptr++;
    spritesound(*insptr, i2);
    insptr++;
    CON_NEXT();

case CON_FALL:
    insptr++;
    if (sprite[i2].z < sector[sprite[i2].sectnum].floorz) {
        if (sprite[i2].zvel < 2048)
            sprite[i2].zvel += 256;
        sprite[i2].z += sprite[i2].zvel;
    } else {
        sprite[i2].zvel = 0;
        sprite[i2].z = sector[sprite[i2].sectnum].floorz;
    }
    CON_NEXT();

case CON_RBRACE:
    insptr++;
    CON_STOP();

case CON_ADDBATTERYAMMO:
    insptr++;
    ps[p2].batteryammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_NUKE:
    insptr++;
    nuke(sprite[i2].x, sprite[i2].y, sprite[i2].z, i2);
    CON_NEXT();

case CON_KILLIT:
    killit_flag = 1;
    insptr++;
    CON_NEXT();

case CON_ADDWEAPON:
    insptr++;
    addweapon(p2, *insptr, 0);
    insptr++;
    CON_NEXT();

case CON_WEAPON:
    insptr += 2;
    CON_NEXT();

case CON_ADDHEALTH:
    insptr++;
    if (!disable_armor && ps[p2].armor)
        ps[p2].armor -= 4;
    else if (ps[p2].health > 0)
        ps[p2].health += *insptr;
    else
        ps[p2].health = 0;
    insptr++;
    CON_NEXT();

case CON_STATE:
    resume = insptr + 2;
    insptr = (long *)insptr[1];
    while (!parse(i2, p2, x2))
        ;
    insptr = resume;
    CON_NEXT();

case CON_LBRACE:
    insptr++;
    while (!parse(i2, p2, x2))
        ;
    CON_NEXT();

case CON_MOVE:
    hittype[i2].movecount = 0;
    insptr++;
    hittype[i2].move = (long *)*insptr++;
    sprite[i2].hitag = *insptr++;
    if (sprite[i2].hitag & FACE_EXIT)
        sprite[i2].ang = furthestangle(i2, 8);
    CON_NEXT();

case CON_SPAWN:
    insptr++;
    spawn(i2, *insptr);
    insptr++;
    CON_NEXT();

case CON_IFWASWEAPON:
    insptr++;
    PARSE_IF(hit_sprite >= 0 && sprite[hit_sprite].picnum == *insptr);
    CON_NEXT();

case CON_IFACTION:
    insptr++;
    PARSE_IF((long)hittype[i2].action == *insptr);
    CON_NEXT();

case CON_IFACTIONCOUNT:
    insptr++;
    PARSE_IF(hittype[i2].actioncount >= *insptr);
    CON_NEXT();

case CON_RESETACTIONCOUNT:
    insptr++;
    hittype[i2].actioncount = 0;
    CON_NEXT();

case CON_DEBRIS:
    insptr++;
    for (j = 0; j < *insptr; j++)
        EGS(sprite[i2].sectnum, sprite[i2].x, sprite[i2].y, sprite[i2].z, 0,
            SCRAP1 + krand() % 11, -8, 32, 32, krand() & 2047,
            64 + (krand() & 128), -512 - (krand() & 511), i2, 5);
    insptr++;
    CON_NEXT();

case CON_IFSKILL:
    insptr++;
    PARSE_IF(ps[p2].skill < *insptr);
    CON_NEXT();

case CON_CSTAT:
    insptr++;
    sprite[i2].cstat = *insptr++;
    CON_NEXT();

case CON_IFMOVE:
    insptr++;
    PARSE_IF((long)hittype[i2].move == *insptr);
    CON_NEXT();

case CON_RESETPLAYER:
    insptr++;
    resetplayer(p2);
    CON_NEXT();

case CON_IFONWATER:
    PARSE_IF((sector[sprite[i2].sectnum].lotag & 32767) == 1 &&
        sector[sprite[i2].sectnum].floorz - sprite[i2].z < (sprite[i2].yrepeat << 7));
    CON_NEXT();

case CON_IFINWATER:
    PARSE_IF((sector[sprite[i2].sectnum].lotag & 32767) == 2);
    CON_NEXT();

case CON_IFPLAYERSWEAPON:
    insptr++;
    PARSE_IF(ps[p2].curr_weapon == *insptr);
    CON_NEXT();

case CON_IFMOVECOUNT:
    insptr++;
    PARSE_IF(hittype[i2].movecount >= *insptr);
    CON_NEXT();

case CON_RESETMOVECOUNT:
    insptr++;
    hittype[i2].movecount = 0;
    CON_NEXT();

case CON_ADDINVENTORY:
    insptr++;
    switch (*insptr) {
    case 0: ps[p2].inventory[2] = 1; break;
    case 1: ps[p2].inventory[3] = 1; break;
    case 2: ps[p2].inventory[0] = 1; break;
    case 3: ps[p2].inventory[4] = 1; break;
    case 4: ps[p2].inventory[1] = 1; break;
    }
    insptr++;
    CON_NEXT();

case CON_ADDAMMO:
    insptr++;
    ps[p2].ammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_HITRADIUS:
    hitradius(i2, insptr[1], insptr[2]);
    insptr += 3;
    CON_NEXT();

case CON_IFP:
    insptr++;
    flags = *insptr;
    speed = player_vel[p2] + player_svel[p2];
    PARSE_IF(((flags & 8) && ps[p2].on_ground && (player_bits[p2] & 2)) ||
        ((flags & 16) && !ps[p2].poszv && !ps[p2].on_ground &&
            sector[ps[p2].cursectnum].floorz - ps[p2].posz > 18432) ||
        ((flags & 32) && ps[p2].poszv > 348) ||
        ((flags & 1) && speed >= 0 && speed < 8) ||
        ((flags & 2) && speed >= 8 && !(player_bits[p2] & 32)) ||
        ((flags & 4) && speed >= 8 && (player_bits[p2] & 32)) ||
        ((flags & 64) && sprite[i2].z - 12288 > ps[p2].posz) ||
        ((flags & 128) && speed <= -8 && !(player_bits[p2] & 32)) ||
        ((flags & 256) && speed <= -8 && (player_bits[p2] & 32)) ||
        ((flags & 512) && ps[p2].horiz < -128) ||
        ((flags & 1024) && ps[p2].horiz > 128) ||
        ((flags & 2048) ?
            ((((sprite[i2].ang + 3200 - sprite[facing_sprite].ang) & 2047) >> 8) < 2) :
            (((flags & 4096) && ps[p2].shooting) ||
             ((flags & 8192) && ps[p2].on_ground) ||
             ((flags & 16384) && ps[p2].jetpack_on))));
    CON_NEXT();

case CON_ADDRAMMO:
    insptr++;
    ps[p2].rammo += *insptr;
    insptr++;
    CON_NEXT();

case CON_SINK:
    sprite[i2].z += 4096;
    sprite[i2].xvel >>= 1;
    insptr++;
    CON_NEXT();

case CON_IFSTRENGTH:
    insptr++;
    if (sprite[i2].picnum == APLAYER)
        j = ps[sprite[i2].yvel].health;
    else
        j = sprite[i2].extra;
    PARSE_IF(j <= *insptr);
    CON_NEXT();

case CON_BLOOD:
    insptr++;
    for (j = 0; j < *insptr; j++)
        EGS(sprite[i2].sectnum, sprite[i2].x, sprite[i2].y, sprite[i2].z - 8192,
            krand() & 2, BLOOD, -16, 8 + (krand() & 7), 8 + (krand() & 7),
            sprite[i2].ang + (krand() & 127), 0, -1024 - (krand() & 255), i2, 5);
    insptr++;
    CON_NEXT();

case CON_GUTS:
    insptr++;
    guts(i2);
    CON_NEXT();

case CON_IFSPAWNEDBY:
    insptr++;
    PARSE_IF(sprite[i2].owner >= 0 && sprite[i2].owner == *insptr);
    CON_NEXT();

case CON_ENDA:
case CON_BREAK:
case CON_ENDS:
    CON_STOP();

default:
    gameexit(ltoa(sprite[i2].picnum, tempbuf, 10));
    CON_STOP();
}
#undef CON_STOP
#undef CON_NEXT
#undef PARSE_ARGS
    }

    }

end_script:
    if (killit_flag) {
        deletesprite(i);
        return;
    }
    {
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
    }
}
