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
        if (actor_header || parsing_state) {
            transnum();
            j = 0;
            while (keyword() == -1) {
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
        for (i = 0; i < 2 && keyword() < 0; i++)
            transnum();
        for (; i < 2; i++)
            *scriptptr++ = 0;
        return 0;

    case CON_MUSIC:
        scriptptr--;
        transnum();
        k = *--scriptptr - 1;
        if (k >= 0) {
            for (j = 0; j < 10 && keyword() == -1; j++) {
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
            for (j = 0; j < 10 && keyword() == -1; j++) {
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
        for (i = 0; i < 5 && keyword() < 0; i++)
            transnum();
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
                while (keyword() == -1) {
                    transnum();
                    j |= *--scriptptr;
                }
                *scriptptr++ = j;
                break;
            }
            if (keyword() >= 0) {
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
            while (keyword() == -1) {
                transnum();
                j |= *--scriptptr;
            }
            *scriptptr++ = j;
        }
        branch = scriptptr++;
        do {
            k = keyword();
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

enum move_flag {
    SLOW_TO_STOP = 1, FACE_PLAYER = 2, FACE_EXIT = 4, GET_H = 8, GET_V = 16,
    HOVER = 32, STICK_TO_CEILING = 64, FACE_TO_PLAYER = 128,
    RHR = 256, ON_OWNER = 512, STICK_TO_FLOOR = 1024
};

void move_actor(short i, short p, long x)
{
#include "con_movement.inc"
}

/* An if operand is followed by the address of its false branch. */
#define PARSE_IF(condition) do { \
    if (condition) { \
        insptr += 2; \
        parse(i, p, x); \
    } else { \
        insptr = (long *)insptr[1]; \
        if (*insptr == CON_ELSE) { \
            insptr += 2; \
            parse(i, p, x); \
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

#define CON_NEXT() return 0
#define CON_STOP() return 1
#include "con_dispatch.inc"
#undef CON_STOP
#undef CON_NEXT
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

    while (!killit_flag) {
        long j, flags, speed, *resume;
        short angle, difference;

#define CON_NEXT() continue
#define CON_STOP() goto end_script
#include "con_dispatch.inc"
#undef CON_STOP
#undef CON_NEXT
    }

end_script:
    if (killit_flag) {
        deletesprite(i);
        return;
    }
    {
#include "con_movement.inc"
    }
}
