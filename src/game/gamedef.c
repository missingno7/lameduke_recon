/* GAMEDEF: the CON script compiler (loadefs/parsecommand -> script[] of 60-keyword bytecode) and interpreter (execute/parse/move) for actors.
   Earliest known form of 3D Realms' CON language (Todd Replogle; Duke Nukem 3D 1.5 GAMEDEF.C, 1996, is its descendant: 49 of LameDuke's 60 keyword slots keep number and role there, 45 with the same keyword).
   Lexer helpers ispecial/isaltok/getlabel/keyword are byte-identical to the 1996 text; the rest is BINARY_DERIVED with Duke 1.5 semantics. */

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
extern struct user_defs ud;         /* the 21-byte USERDEFS.TMP block defined by GLOBAL */

/* dataown: one object, one name. Code uses the fields of GLOBAL's settings block ud directly; the unused
   externs disable_armor and facing_sprite in gamedef.h only keep this unit's symbol slots (docs/BSS_LAYOUT.md). */

/* Angle step from a toward na (Build 2048-unit circle), shifted right by shift. Later Duke: getincangle(a,na). */
short getincangle(short a, short na, short shift)
{
    a &= 2047;
    na &= 2047;

    if (klabs(a - na) < 1024)
        return (na - a) >> shift;
    return (((a + 1024) & 2047) - na) >> shift;
}

/* True if c ends a label (LF, space, CR); LF advances line_number. */
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

/* True if c may appear in a CON token (isalnum or { } / * - _ .). */
char isaltok(char c)
{
    return TOKEN_CHAR(c);
}

/* Read the next label into label+(labelcnt<<6); the slot becomes permanent only if the caller increments labelcnt. */
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

/* Peek the next token and return its keyword number (-1 = not a keyword, 0 = end of text). */
long keyword(void)
{
    long i;
    char *p;

    p = textptr;
    while (!isaltok(*p)) {
        p++;
        if (*p == 0)
            return 0;
    }

    i = 0;
    while (isaltok(*p)) {
        tempbuf[i] = *p++;
        i++;
    }
    tempbuf[i] = 0;

    for (i = 0; i < NUMKEYWORDS; i++)
        if (strcmp(tempbuf, keyw[i]) == 0)
            return i;
    return -1;
}

/* Translate the next word: emit keyword number (or label value) at scriptptr and return it; -1 on EOF or undefined symbol. */
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

/* Translate a numeric operand (label value or decimal literal) into script[]. */
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




/* Compile one CON command into script[]. Returns 1 to end the enclosing loop (end, '}', EOF or too many errors). */
char parsecommand(void)
{
    long i, j, k, *tempscrptr;
    short tw, temp_line_number;
    int fp;
    char *origtptr, temp_ifelse_check, c;

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
        if (!parsing_actor && !parsing_state) {
            getlabel();
            parsing_state = 1;
            labelcode[labelcnt++] = (long)--scriptptr;
            return 0;
        }
        getlabel();
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
        getlabel();
        for (i = 0; i < 38; i++) {
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
        getlabel();
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
        if (parsing_actor || parsing_state) {
            transnum();
            j = 0;
            for (;;) {
                k = keyword();
                if (k != -1)
                    break;
                transnum();
                j |= *--scriptptr;
            }
            *scriptptr++ = j;
            return 0;
        }
        scriptptr--;
        getlabel();
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
            k = keyword();
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
                i = keyword();
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
                    music_fn[k][j][i] = textptr[i];
                    i++;
                }
                textptr += i;
                music_fn[k][j][i] = 0;
            }
            for (i = 9; i > j; i--)
                strcpy(music_fn[k][i], music_fn[k][j]);
        } else {
            for (j = 0; j < 10; j++) {
                i = keyword();
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
                    env_music_fn[j][i] = textptr[i];
                    i++;
                }
                textptr += i;
            }
            for (i = 9; i > j; i--)
                strcpy(env_music_fn[i], env_music_fn[j]);
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
        fp = open(tempbuf, O_BINARY | O_RDWR, S_IREAD);
        j = filelength(fp);
        temp_line_number = line_number;
        temp_ifelse_check = checking_ifelse;
        checking_ifelse = 0;
        origtptr = textptr;
        textptr = bootleg_cache_alloc(j);
        textptr[j] = 0;
        if (!textptr) {
            puts("  * ERROR! Not enough memory.");
            exit(0);
        }
        read(fp, textptr, j);
        close(fp);
        while (!parsecommand())
            ;
        total_lines += line_number;
        textptr = origtptr;
        line_number = temp_line_number;
        checking_ifelse = temp_ifelse_check;
        bootleg_cache_pop();
        return 0;

    case CON_ACTION:
        if (parsing_actor || parsing_state) {
            transnum();
            return 0;
        }
        scriptptr--;
        getlabel();
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
            k = keyword();
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
        if (parsing_actor) {
            printf("  * ERROR!(L%ld) Found 'actor' within 'actor'.\n", (long)line_number);
            error++;
        }
        brace_depth = 0;
        parsing_actor = --scriptptr;
        transnum();
        actorscrptr[*--scriptptr] = parsing_actor;
        for (i = 0; i < 4; i++)
            parsing_actor[i] = 0;
        for (i = 0; i < 4; i++) {
            if (i == 3) {
                j = 0;
                for (;;) {
                    k = keyword();
                    if (k != -1)
                        break;
                    transnum();
                    j |= *--scriptptr;
                }
                *scriptptr++ = j;
                break;
            }
            k = keyword();
            if (k >= 0) {
                scriptptr += 4 - i;
                break;
            }
            transnum();
            parsing_actor[i] = scriptptr[-1];
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
            tempscrptr = scriptptr++;
            parsecommand();
            *tempscrptr = (long)scriptptr;
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
                k = keyword();
                if (k != -1)
                    break;
                transnum();
                j |= *--scriptptr;
            }
            *scriptptr++ = j;
        }
        tempscrptr = scriptptr++;
        do {
            k = keyword();
            if (k == CON_LINECOMMENT || k == CON_BLOCKCOMMENT)
                parsecommand();
        } while (k == CON_LINECOMMENT || k == CON_BLOCKCOMMENT);
        parsecommand();
        *tempscrptr = (long)scriptptr;
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
        if (!parsing_actor) {
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
        parsing_actor = 0;
        return 0;

    case -1:
    default:
        return 0;
    }
}

/* Single compile pass over the loaded text. */
void passone(void)
{
    while (!parsecommand())
        ;
}

/* Load and compile GAME.CON (and its includes) into script[]; script[0] receives the end pointer. */
void loadefs(void)
{
    int fp;
    long fs;

    fp = open("game.con", O_BINARY | O_RDWR, S_IREAD);
    if (fp) {
        puts("\nLoading script...");
        fs = filelength(fp);
        textptr = bootleg_cache_alloc(fs);
        label = bootleg_cache_alloc(MAXLABELS * LABELSIZE);
        labelcode = bootleg_cache_alloc(MAXLABELS * sizeof(long));
        if (!labelcode || !label) {
            puts("  * ERROR! Not enough memory.");
            exit(0);
        }
        read(fp, textptr, fs);
        close(fp);
        last_used_text = textptr + fs - 2;
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

/* Pick the most open of angs evenly spaced headings around sprite i (longest hitscan). */
short furthestangle(short i, short angs)
{
    short j, hitwall, hitsect, furthest_angle, angincs;
    long hx, hy, hz, d, greatestd;

    angincs = 2048 / angs;
    greatestd = (-2147483647L - 1);
    for (j = 0; j < 2048; j += angincs) {
        hitscan(sprite[i].x, sprite[i].y, sprite[i].z - (8 << 8),
            sprite[i].sectnum, sintable[(j + 512) & 2047], sintable[j & 2047], 0,
            &hitsect, &hitwall, &hitsect, &hx, &hy, &hz);
        d = klabs(hx - sprite[i].x) + klabs(hy - sprite[i].y);
        if (d > greatestd) {
            greatestd = d;
            furthest_angle = j;
        }
    }
    return furthest_angle;
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

/* Apply sprite i's CON move and hitag move flags (Duke 1.5 move()). */
void move_actor(short i, short p, long x)
{
/* Post-script sprite movement. The includer supplies i and p. */
short flags, owner, goalang;
long *moveptr;

flags = sprite[i].hitag;
if (!hittype[i].move || !flags)
    return;

hittype[i].movecount++;
moveptr = hittype[i].move;
if (flags & ON_OWNER) {
    owner = sprite[i].owner;
    if (owner >= 0 && sprite[owner].statnum == STAT_ACTOR) {
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
    sprite[i].xvel += (moveptr[0] - sprite[i].xvel) >> 1;
if (flags & GET_V)
    sprite[i].zvel += ((moveptr[1] << 4) - sprite[i].zvel) >> 1;
if (flags & HOVER) {
    if (sprite[i].z + 8192 < sector[sprite[i].sectnum].floorz)
        sprite[i].zvel += moveptr[1] << 4;
    else if (sprite[i].z + 8192 > sector[sprite[i].sectnum].floorz)
        sprite[i].zvel -= moveptr[1] << 4;
}
if (flags & STICK_TO_CEILING)
    sprite[i].z = sector[sprite[i].sectnum].ceilingz + 2048;
if (flags & STICK_TO_FLOOR)
    sprite[i].z = sector[sprite[i].sectnum].floorz - 2048;
if (flags & FACE_TO_PLAYER) {
    goalang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
    sprite[i].ang += getincangle(sprite[i].ang, goalang, 1);
}
if (sprite[i].xvel | sprite[i].zvel) {
    if (!ssp(i, 1) && (flags & RHR) && (sector[sprite[i].sectnum].lotag & 32767) != 1) {
        sprite[i].ang += 256;
        ssp(i, 1);
    }
}
}

/* An if operand is followed by the address of its false branch. */
#define PARSE_IF(condition) if (condition) { \
        insptr += 2; \
        parse(PARSE_ARGS); \
    } else { \
        insptr = (long *)insptr[1]; \
        if (*insptr == CON_ELSE) { \
            insptr += 2; \
            parse(PARSE_ARGS); \
        } \
    }

#define APLAYER 283
#define SCRAP1 235
#define BLOOD 488

/* Run one CON instruction for actor i (player p at distance x); nonzero ends the current block. */
char parse(short i, short p, long x)
{
    long j, flags, speed, *tempscrptr;
    short goalang, angdif;

    if (killit_flag)
        return 1;

#define PARSE_ARGS i, p, x
#define CON_NEXT() break
#define CON_STOP() return 1
/* Shared opcode bodies. The caller supplies its continuation and exit. */
switch (*insptr) {
case CON_IFRND:
    insptr++;
    PARSE_IF((krand() >> 8) >= 255 - *insptr);
    CON_NEXT();

case CON_IFCANSEE:
    if (cached_cansee == -1) {
        goalang = getangle(ps[p].posx - sprite[i].x, ps[p].posy - sprite[i].y);
        angdif = getincangle(sprite[i].ang, goalang, 0);
        if (angdif > -128 && angdif < 128)
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
    ps[p].ammo_bat_amount += *insptr;
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
    if (!ud.debug && ps[p].shield_amount)
        ps[p].shield_amount -= 4;
    else if (ps[p].health > 0)
        ps[p].health += *insptr;
    else
        ps[p].health = 0;
    insptr++;
    CON_NEXT();

case CON_STATE:
    tempscrptr = insptr + 2;
    insptr = (long *)insptr[1];
    while (!parse(i, p, x))
        ;
    insptr = tempscrptr;
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
    PARSE_IF(ps[p].current_weapon == *insptr);
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
    ps[p].ammo_amount += *insptr;
    insptr++;
    CON_NEXT();

case CON_HITRADIUS:
    hitradius(i, insptr[1], insptr[2]);
    insptr += 3;
    CON_NEXT();

case CON_IFP:
    insptr++;
    flags = *insptr;
    speed = syncsvel[p] + syncvel[p];
    PARSE_IF(((flags & 8) && ps[p].on_ground && (syncbits[p] & SB_CROUCH)) ||
        ((flags & 16) && !ps[p].jumping_counter && !ps[p].on_ground &&
            sector[ps[p].cursectnum].floorz - ps[p].posz > 18432) ||
        ((flags & 32) && ps[p].jumping_counter > 348) ||
        ((flags & 1) && speed >= 0 && speed < 8) ||
        ((flags & 2) && speed >= 8 && !(syncbits[p] & SB_RUN)) ||
        ((flags & 4) && speed >= 8 && (syncbits[p] & SB_RUN)) ||
        ((flags & 64) && sprite[i].z - 12288 > ps[p].posz) ||
        ((flags & 128) && speed <= -8 && !(syncbits[p] & SB_RUN)) ||
        ((flags & 256) && speed <= -8 && (syncbits[p] & SB_RUN)) ||
        ((flags & 512) && ps[p].look_ang < -128) ||
        ((flags & 1024) && ps[p].look_ang > 128) ||
        ((flags & 2048) ?
            ((((sprite[i].ang + 3200 - sprite[ud.camerasprite].ang) & 2047) >> 8) < 2) :
            (((flags & 4096) && ps[p].fire_flag) ||
             ((flags & 8192) && ps[p].on_ground) ||
             ((flags & 16384) && ps[p].jetpack_on))));
    CON_NEXT();

case CON_ADDRAMMO:
    insptr++;
    ps[p].ammo_r_amount += *insptr;
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
return 0;
#undef CON_STOP
#undef CON_NEXT
#undef PARSE_ARGS
}

/* Execute sprite i's actor script (from actorscrptr[picnum]+4) for one game tick. */
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
        sprite[i].lotag += synctics;
        if (sprite[i].lotag > hittype[i].action[4]) {
            hittype[i].actioncount++;
            sprite[i].lotag = 0;
            hittype[i].actionframe += hittype[i].action[3];
        }
        if (klabs(hittype[i].actionframe) >= klabs(hittype[i].action[1] * hittype[i].action[3]))
            hittype[i].actionframe = 0;
    }

    while (!parse(i, p, x))
        ;
    if (killit_flag) {
        deletesprite(i);
        return;
    }
    move_actor(i, p, x);
}
