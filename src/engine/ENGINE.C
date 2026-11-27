// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.

// This file has been modified from Ken Silverman's original release

#define ENGINE
// Stack checking is off by command line (/s), as in Ken's makefiles: wcc386 engine /4r /s /or
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <conio.h>
#include <i86.h>
#include "build.h"
#include "pragmas.h"

#define MAXCLIPNUM 512
#define MAXTILEFILES 256
#define MAXYSAVES (MAXXDIM*MAXSPRITES)>>6
#define MAXWALLSB 4096
#define MAXCLIPDIST 1024

static unsigned char enginetempbuf[MAXWALLS];

long artversion, mapversion;
char *pic = NULL;
char picsiz[MAXTILES], tilefilenum[MAXTILES];
long walsiz[MAXTILES], tilefileoffs[MAXTILES];

long cachesize = 0, artsize = 0;
static long cachelistplc, cachelistend, cacheplc;
static long cachelist[MAXTILES], cachelistoffs[MAXTILES];

static short radarang[MAXXDIM], radarang2[MAXXDIM];
static long tantable[1024], sqrtable[2048];
static char britable[16][64];
char textfont[1024], smalltextfont[1024];

static long xb1[MAXWALLSB], yb1[MAXWALLSB], xb2[MAXWALLSB], yb2[MAXWALLSB];
static long rx1[MAXWALLSB], ry1[MAXWALLSB], rx2[MAXWALLSB], ry2[MAXWALLSB];
static short p2[MAXWALLSB], thesector[MAXWALLSB], thewall[MAXWALLSB];

static short bunchfirst[MAXWALLSB], bunchlast[MAXWALLSB];

static short smost[MAXYSAVES], smostcnt;
static short smoststart[MAXWALLSB], smostwall[MAXWALLSB], smostwallcnt;

static short maskwall[MAXWALLSB], maskwallcnt;
static long spritex[MAXSPRITES], spritey[MAXSPRITES];

static long umost[MAXXDIM], dmost[MAXXDIM];
static long uplc[MAXXDIM], dplc[MAXXDIM];
static long uwall[MAXXDIM], dwall[MAXXDIM];
static long swplc[MAXXDIM], lplc[MAXXDIM];
static long swall[MAXXDIM], lwall[MAXXDIM+1];
static long xdimen, xdimenscale, xdimscale, xdimenoffs;
#define halfxdimen (xdimen>>1)
long windowx1, windowy1, windowx2, windowy2;
static long ydimen, viewoffset;
static long ytop, ybot;

static long rxi[8], ryi[8], rzi[8], rxi2[8], ryi2[8], rzi2[8];
static long xsi[8], ysi[8], horizlookup[1024];

static long globalposx, globalposy, globalposz, globalhoriz, dahorizbak;
static short globalang, globalcursectnum;
static long globalpal, globalpalwritten, cosglobalang, singlobalang;

long asm1, asm2, asm3, asm4;
char globalxshift, globalyshift;
long globalxpanning, globalypanning, globalshade;
short globalpicnum, globalshiftval;
long globalzd, globalbufplc, globalyscale, globalorientation;
long globalx1, globaly1, globalx2, globaly2, globalzx;

long page;
#define chainoffset (0xa0000L+(page<<14))

static char vesapageshift, curpag = 0, tsengdriver = 0;
static long ooption0, stereopage, stereofps = 0;
static long stereowidth, stereopixelwidth;
static long curbrightness = 0;

static short sectorborder[256], sectorbordercnt;
static char *screen = NULL, tablesloaded = 0;
long frameplace, pageoffset, ydim16, qsetmode = 0;
long startposx, startposy, startposz;
short startang, startsectnum;
short pointhighlight, linehighlight, highlightcnt;
static long lastx[MAXYDIM+2];
char palette[768], paletteloaded;
char *transluc = NULL, translucloaded = 0;

static short clipnum, hitwalls[4];
static long clipx1[MAXCLIPNUM], clipy1[MAXCLIPNUM];
static long clipx2[MAXCLIPNUM], clipy2[MAXCLIPNUM];
static short clipsectorlist[MAXCLIPNUM], clipsectnum;
static short clipobjectval[MAXCLIPNUM];

short numscans, numhits, numbunches;
static short posfil, capturecount, hitcnt;

static char pcxheader[128] =
{
	0xa,0x5,0x1,0x8,0x0,0x0,0x0,0x0,0x3f,0x1,0xc7,0x0,
	0x40,0x1,0xc8,0x0,0x0,0x0,0x0,0x8,0x8,0x8,0x10,0x10,
	0x10,0x18,0x18,0x18,0x20,0x20,0x20,0x28,0x28,0x28,0x30,0x30,
	0x30,0x38,0x38,0x38,0x40,0x40,0x40,0x48,0x48,0x48,0x50,0x50,
	0x50,0x58,0x58,0x58,0x60,0x60,0x60,0x68,0x68,0x68,0x70,0x70,
	0x70,0x78,0x78,0x78,0x0,0x1,0x40,0x1,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};
static char vgapal16[48] =
{
	00,00,00,00,00,42,00,42,00,00,42,42,42,00,00,42,00,42,42,21,00,42,42,42,
	21,21,21,21,21,63,21,63,21,21,63,63,63,21,21,63,21,63,63,63,21,63,63,63,
};

static char defaultoption[NUMOPTIONS] = {0,0,0,0,0,0};
static char defaultkey[NUMKEYS] =
{
	0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
	0x1e,0x2c,0x4e,0x4a,
	0x9c,0x1c,0xd,0xc,0xf,
};

short editstatus = 0;
short searchit;
long searchx, searchy;                          //search input
short searchsector, searchwall, searchstat;     //search output

static char artfilename[20];
static long localtilestart[MAXTILEFILES], localtileend[MAXTILEFILES];
static long numtilefiles, artfil, artfilnum;

static long totalclocklock;

volatile unsigned char readch, oldreadch, extended;

void (__interrupt __far *oldkeyhandler)();
void __interrupt __far keyhandler(void);
void (__interrupt __far *oldzerohandler)();
void __interrupt __far zerohandler(void);

extern long setbytesperline(long);
#pragma aux setbytesperline parm [eax];
extern long sethlinesizes(long,long,long);
#pragma aux sethlinesizes parm [eax][ebx][ecx];
extern long sethlinechainsizes(long,long,long);
#pragma aux sethlinechainsizes parm [eax][ebx][ecx];
extern long setpalookupaddress(long);
#pragma aux setpalookupaddress parm [eax];
extern long chainsetpalookupaddress(long);
#pragma aux chainsetpalookupaddress parm [eax];
extern long hlineasm64(long,long,long,long,long,long);
#pragma aux hlineasm64 parm [eax][ebx][ecx][edx][esi][edi];
extern long hchainasm64(long,long,long,long,long,long);
#pragma aux hchainasm64 parm [eax][ebx][ecx][edx][esi][edi];
extern long setvlinebpl(long);
#pragma aux setvlinebpl parm [eax];
extern long fixtransluscence(long);
#pragma aux fixtransluscence parm [eax];
extern long vlineasm1(long,long,long,long,long,long);
#pragma aux vlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern long tvlineasm1(long,long,long,long,long,long);
#pragma aux tvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern long setuptvlineasm2(long,long,long);
#pragma aux setuptvlineasm2 parm [eax][ebx][ecx];
extern long tvlineasm2(long,long,long,long,long,long);
#pragma aux tvlineasm2 parm [eax][ebx][ecx][edx][esi][edi];
extern long mvlineasm1(long,long,long,long,long,long);
#pragma aux mvlineasm1 parm [eax][ebx][ecx][edx][esi][edi];
extern long vlineasm4(long,long,long,long,long,long);
#pragma aux vlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern long mvlineasm4(long,long,long,long,long,long);
#pragma aux mvlineasm4 parm [eax][ebx][ecx][edx][esi][edi];
extern void setupspritevline(long,long,long,long,long,long);
#pragma aux setupspritevline parm [eax][ebx][ecx][edx][esi][edi];
extern void spritevline(long,long,long,long,long,long);
#pragma aux spritevline parm [eax][ebx][ecx][edx][esi][edi];
extern long mhline(long,long,long,long,long,long);
#pragma aux mhline parm [eax][ebx][ecx][edx][esi][edi];
extern long mhlineskipmodify(long,long,long,long,long,long);
#pragma aux mhlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern long msethlineshift(long,long);
#pragma aux msethlineshift parm [eax][ebx];
extern long thline(long,long,long,long,long,long);
#pragma aux thline parm [eax][ebx][ecx][edx][esi][edi];
extern long thlineskipmodify(long,long,long,long,long,long);
#pragma aux thlineskipmodify parm [eax][ebx][ecx][edx][esi][edi];
extern long tsethlineshift(long,long);
#pragma aux tsethlineshift parm [eax][ebx];

#pragma aux readkey =\
	"in al, 0x60",\
	"mov readch, al",\
	"in al, 0x61",\
	"or al, 0x80",\
	"out 0x61, al",\
	"and al, 0x7f",\
	"out 0x61, al",\
	modify [eax]\

// 11,12 is 2,1-10
// 13,14 is 4,2-10
// 15,16 is 6,3-10
#pragma aux ksqrtasm =\
	"or eax, eax",\
	"jz endksqrtasm",\
	"jns skipnegate",\
	"neg eax",\
	"skipnegate: bsr ecx, eax",\
	"cmp ecx, 11",\
	"jge over2048",\
	"mov eax, dword ptr sqrtable[eax*4]",\
	"shr eax, 10",\
	"jmp endksqrtasm",\
	"over2048:",\
	"sub cl, 9",\
	"and cl, 0xfe",\
	"shr eax, cl",\
	"mov eax, dword ptr sqrtable[eax*4]",\
	"shr cl, 1",\
	"shl eax, cl",\
	"shr eax, 10",\
	"endksqrtasm:",\
	parm [eax]\
	modify [eax ebx ecx edx]\

#pragma aux vesasetmode =\
	"mov ax, 0x4f02",\
	"int 0x10",\
	"and eax, 0x0000ffff",\
	parm [ebx]\
	modify [eax ebx]\

#pragma aux vesasetpage =\
	"mov cl, vesapageshift",\
	"shl dx, cl",\
	"cmp dl, curpag",\
	"je skipsetpage",\
	"mov curpag, dl",\
	"mov ax, 0x4f05",\
	"xor bx, bx",\
	"int 0x10",\
	"skipsetpage:",\
	parm [edx]\
	modify [eax ebx ecx]\

#pragma aux vesasetpan =\
	"mov eax, 0x00004f07",\
	"xor ebx, ebx",\
	"int 0x10",\
	parm [ecx][edx]\
	modify [eax ebx]\

#pragma aux setgotpic =\
	"mov ecx, eax",\
	"shr eax, 3",\
	"mov ebx, 0x01010101",\
	"rol ebx, cl",\
	"or byte ptr gotpic[eax], bl",\
	parm [eax]\
	modify [ebx ecx]\

void __interrupt __far keyhandler()
{
	oldreadch = readch;
	readkey();
	if ((readch|1) == 0xe1)
		extended = 128;
	else
	{
		if (oldreadch != readch)
		{
			if ((readch&128) == 0)
			{
				if (keystatus[(readch&127)+extended] == 0)
					keystatus[(readch&127)+extended] = 1;
			}
			else
				keystatus[(readch&127)+extended] = 0;
		}
		extended = 0;
	}
	koutp(0x20,0x20);
}

int bunchfront(short, short);
void calcwalls(short);
void drawalls(short);

drawrooms(long daposx, long daposy, long daposz,
			 short daang, long dahoriz, short dacursectnum)
{
	long i, j, z, closest;

	globalposx = daposx; globalposy = daposy; globalposz = daposz;
	globalhoriz = dahoriz; globalang = (daang&2047);
	globalcursectnum = dacursectnum;
	totalclocklock = totalclock;

	cosglobalang = sintable[(globalang+512)&2047];
	singlobalang = sintable[globalang&2047];

	dahorizbak = dahoriz;
	if (xdimen != XDIM)
		globalhoriz = mulscale(globalhoriz-(YDIM>>1),xdimenscale,16)+(YDIM>>1);

	if (stereofps != 0)
	{
		if ((keystatus[0x2a]|keystatus[0x36]) > 0)
		{
			if (keystatus[0x1a] > 0) stereopixelwidth--;   //Shift [
			if (keystatus[0x1b] > 0) stereopixelwidth++;   //Shift ]
		}
		else
		{
			if (keystatus[0x1a] > 0) stereowidth -= 512;   //[
			if (keystatus[0x1b] > 0) stereowidth += 512;   //]
		}

		if (stereopage == 0)
		{
			globalposx += mulscale(singlobalang,stereowidth,24);
			globalposy -= mulscale(cosglobalang,stereowidth,24);
			if (ooption0 == 6)
				frameplace = FP_OFF(screen);
		}
		else
		{
			globalposx -= mulscale(singlobalang,stereowidth,24);
			globalposy += mulscale(cosglobalang,stereowidth,24);
			if (ooption0 == 6)
				frameplace = FP_OFF(screen)+65536;
		}
	}

	clearbuf((long)(&gotsector[0]),(long)((numsectors+31)>>5),0L);
	copybuf((long)(&startumost[0]),(long)(&umost[0]),xdimen);
	copybuf((long)(&startdmost[0]),(long)(&dmost[0]),xdimen);
	clearbuf((long)(&swall[0]),(long)xdimen,0x7fffffff);

	updatesector(globalposx,globalposy,&globalcursectnum);

	numhits = xdimen; numscans = 0; numbunches = 0;
	maskwallcnt = 0; smostwallcnt = 0; smostcnt = 0; spritesortcnt = 0;

	scansector(globalcursectnum);

	while ((numbunches > 0) && (numhits > 0))
	{
		clearbuf((long)(&enginetempbuf[0]),(long)((numbunches+3)>>2),0L);
		enginetempbuf[0] = 1;

		closest = 0;              //Almost works, but not quite :(
		for(i=1;i<numbunches;i++)
		{
			j = bunchfront(i,closest);
			if (j >= 0) enginetempbuf[i] = 1;
			if (j == i) enginetempbuf[closest] = 1, closest = i;
		}

		for(i=0;i<numbunches;i++)   //Double-check (I wanna' get rid of this!)
			if (enginetempbuf[i] == 0)
			{
				j = bunchfront(i,closest);
				if (j >= 0) enginetempbuf[i] = 1;
				if (j == i) enginetempbuf[closest] = 1, closest = i, i = 0;
			}

		calcwalls(closest);
		drawalls(closest);

		if (automapping == 1)
		{
			z = bunchfirst[closest];
			do
			{
				i = thewall[z];
				show2dwall[i>>3] |= (1<<(i&7));

				z = p2[z];
			}
			while (z != -1);
		}

		numbunches--;
		bunchfirst[closest] = bunchfirst[numbunches];
		bunchlast[closest] = bunchlast[numbunches];
	}
}

scansector (short sectnum)
{
	short z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst;
	long xs, ys, xp, yp;
	long screenright;
	short point2;

	if (sectnum == -1) return;

	sectorborder[0] = sectnum, sectorbordercnt = 1;
	while (sectorbordercnt > 0)
	{
		sectnum = sectorborder[--sectorbordercnt];

		z = headspritesect[sectnum];
		while (z != -1)
		{
			xs = sprite[z].x-globalposx;
			ys = sprite[z].y-globalposy;
			yp = ((xs*cosglobalang+ys*singlobalang)>>6);
			if ((yp > (4<<8)) || ((sprite[z].cstat&48) > 0))
			{
				xp = ((ys*cosglobalang-xs*singlobalang)>>6);
				if (yp > (4<<8))
					spritex[spritesortcnt] = (xdimen<<7)+scale(xp,xdimen<<7,yp);
				spritepicnum[spritesortcnt] = sprite[z].picnum;
				spritey[spritesortcnt] = yp;
				if ((sector[sectnum].ceilingstat&1) > 0)
					spritepal[spritesortcnt] = sector[sectnum].ceilingpal;
				else
					spritepal[spritesortcnt] = sector[sectnum].floorpal;
				thesprite[spritesortcnt] = z;
				spritesortcnt++;
			}
			z = nextspritesect[z];
		}

		gotsector[sectnum>>3] |= (1<<(sectnum&7));

		startwall = sector[sectnum].wallptr;
		endwall = startwall + sector[sectnum].wallnum - 1;
		scanfirst = numscansbefore = numscans;
		for(z=startwall;z<=endwall;z++)
		{
			if (addboxlinum(sectnum,z) == 1) p2[numscans-1] = numscans;
			if (z > wall[z].point2)
			{
				if (scanfirst < numscans) p2[numscans-1] = scanfirst;
				scanfirst = numscans;
			}
		}

		bunchfrst = numbunches;
		for(z=numscansbefore;z<numscans;z++)
		{
			point2 = wall[thewall[z]].point2;
			if ((point2 != thewall[p2[z]]) || (screenright = xb2[z], screenright >= xb1[p2[z]]))
				bunchfirst[numbunches++] = p2[z], p2[z] = -1;
		}

		for(z=bunchfrst;z<numbunches;z++)
		{
			zz = bunchfirst[z]; while (p2[zz] != -1) zz = p2[zz];
			bunchlast[z] = zz;
		}
	}
}

wallfront (short l1, short l2)
{
	long x1l1, y1l1, x2l1, y2l1, x1l2, y1l2, x2l2, y2l2;
	long templong0, templong1, templong2, templong3, templong4;

	struct walltype *wal;

	wal = &wall[thewall[l1]]; x1l1 = wal->x; y1l1 = wal->y;
	wal = &wall[wal->point2]; x2l1 = wal->x; y2l1 = wal->y;
	wal = &wall[thewall[l2]]; x1l2 = wal->x; y1l2 = wal->y;
	wal = &wall[wal->point2]; x2l2 = wal->x; y2l2 = wal->y;

	templong1 = (y2l1-y1l1)*(x1l2-x1l1) - (x2l1-x1l1)*(y1l2-y1l1); //p1(l2) vs. l1
	templong2 = (y2l1-y1l1)*(x2l2-x1l1) - (x2l1-x1l1)*(y2l2-y1l1); //p2(l2) vs. l1
	if ((templong1|templong2) == 0) return(-1);
	if (templong1 == 0) templong1 = templong2;
	if (templong2 == 0) templong2 = templong1;
	if ((templong1 < 0) == (templong2 < 0))
	{
		templong0 = (y2l1-y1l1)*(globalposx-x1l1) - (x2l1-x1l1)*(globalposy-y1l1);  //pos vs. l1
		if ((templong1<0) == (templong0<0))
			return(l2);
		else
			return(l1);
	}

	templong1 = (y2l2-y1l2)*(x1l1-x1l2) - (x2l2-x1l2)*(y1l1-y1l2); //p1(l1) vs. l2
	templong2 = (y2l2-y1l2)*(x2l1-x1l2) - (x2l2-x1l2)*(y2l1-y1l2); //p2(l1) vs. l2
	if ((templong1|templong2) == 0) return(-1);
	if (templong1 == 0) templong1 = templong2;
	if (templong2 == 0) templong2 = templong1;
	if ((templong1 < 0) == (templong2 < 0))
	{
		templong0 = (y2l2-y1l2)*(globalposx-x1l2) - (x2l2-x1l2)*(globalposy-y1l2);  //pos vs. l2
		if ((templong1<0) == (templong0<0))
			return(l1);
		else
			return(l2);
	}
	return(-2);
}

spritewallfront(short s, short w)
{
	short p;
	long x1, y1, x2, y2;
	long sdx, wdy, sdy, wdx;

	p = wall[w].point2;
	x1 = wall[w].x;
	y1 = wall[w].y;
	x2 = wall[p].x;
	y2 = wall[p].y;
	wdy = y2-y1;
	sdx = sprite[s].x-x1;
	sdy = sprite[s].y-y1;
	wdx = x2-x1;
	return (sdy*wdx >= wdy*sdx);
}

bunchfront (short b1, short b2)
{
	long x1b1, x2b1, x1b2, x2b2, y1, y2, they;
	short z;

	x1b1 = xb1[bunchfirst[b1]];
	x2b1 = xb2[bunchlast[b1]]+1;
	x1b2 = xb1[bunchfirst[b2]];
	x2b2 = xb2[bunchlast[b2]]+1;
	if ((x1b1 >= x2b2) || (x1b2 >= x2b1))
		return(-1);

	if ((x1b1 >= x1b2) && (x1b1 <= x2b2))
	{
		z = bunchfirst[b2];
		while (xb2[z] < x1b1)
			z = p2[z];

		if (wallfront(z,bunchfirst[b1]) == z)
			return(b2);
		else
			return(b1);
	}
	else if ((x1b2 >= x1b1) && (x1b2 <= x2b1))
	{
		z = bunchfirst[b1];
		while (xb2[z] < x1b2)
			z = p2[z];

		if (wallfront(z,bunchfirst[b2]) == z)
			return(b1);
		else
			return(b2);
	}

	return(-1);
}

void calcwalls(short bunch)
{
	char cstat1, cstat2, andwstat2, andwstat1;
	short z, wallnum, sectnum, nextsectnum;
	// COMPILER_STEERED: Watcom stack-home order for topinc, n, z1 and z2.
	long i, j, k, templong, m, n, top, topinc, bot, botinc, x, y;
	long xrepeat, l, z2, z1, hplc, hplc2, hinc, daplc, llin, rlin, scalej;

	z = bunchfirst[bunch];
	sectnum = thesector[z];
	z1 = sector[sectnum].ceilingz;
	z2 = sector[sectnum].floorz;
	cstat1 = sector[sectnum].ceilingstat;
	cstat2 = sector[sectnum].floorstat;

	andwstat1 = 0xff; andwstat2 = 0xff;

	do
	{
		wallnum = thewall[z];
		nextsectnum = wall[wallnum].nextsector1;

		if (z == bunchfirst[bunch])
			hplc = (65536*2048)/yb1[z];
		else
			hplc = hplc2;
		hplc2 = (65536*2048)/yb2[z];
		hinc = (hplc2-hplc)/(xb2[z]-xb1[z]+1);

		if (xdimen != XDIM)
		{
			hplc = mulscale(hplc,xdimenscale,16);
			hinc = mulscale(hinc,xdimenscale,16);
		}

		if ((cstat2&2) > 0)  //Groudraw
		{
			daplc = hplc;
			for(x=xb1[z];x<=xb2[z];x++)
			{
				grouvline(x,swall[x],daplc,sectnum);
				daplc += hinc;
			}
		}

		if ((nextsectnum == -1) || (sector[nextsectnum].ceilingz > z1) || (sector[nextsectnum].floorz < z2) || ((wall[wallnum].cstat&32) > 0))
		{
			xrepeat = wall[wallnum].xrepeat;
			topinc = -ry1[z]*xrepeat;
			top = (mulscale(rx1[z],xdimen,2)-mulscale(xb1[z]-(xdimen>>1),ry1[z],1))*xrepeat;
			botinc = ((ry2[z]-ry1[z])>>1);
			bot = mulscale(rx1[z]-rx2[z],xdimen,3)+mulscale(xb1[z]-(xdimen>>1),botinc,1);

			for(x=xb1[z];x<=xb2[z]+1;x+=2)
			{
				if (bot != 0) lwall[x] = divscale(top,bot,8);
				top += topinc, bot += botinc;
			}
			for(x=xb1[z]+1;x<=xb2[z];x+=2)
				lwall[x] = ((lwall[x-1]+lwall[x+1])>>1);

			if (lwall[xb1[z]] < 0) lwall[xb1[z]] = 0;
			if (wall[wallnum].xrepeat > 0)
			{
				if (lwall[xb2[z]] >= ((long)wall[wallnum].xrepeat<<9))
					lwall[xb2[z]] = ((long)wall[wallnum].xrepeat<<9)-1;
			}

			if ((wall[wallnum].cstat&8) > 0)
			{
				j = ((long)wall[wallnum].xrepeat<<9)-1;
				for(x=xb1[z];x<=xb2[z];x++) lwall[x] = j-lwall[x];
			}
		}

		andwstat1 &= wallmost(uplc,(long)z,z1-globalposz);
		andwstat2 &= wallmost(dplc,(long)z,z2-globalposz);

		qinterpolate((long)(&swall[xb1[z]]),xb2[z]-xb1[z],hplc,hinc);

		if (searchit != 2) continue;
		if ((searchx < xb1[z]) || (searchx > xb2[z])) continue;
		if (searchy <= uplc[searchx])
		{
			searchsector = sectnum;
			searchstat = 1;  //ceiling
			searchit = 1;
			searchwall = wallnum;
		}
		else if (searchy >= dplc[searchx])
		{
			searchsector = sectnum;
			searchstat = 2;  //floor
			searchit = 1;
			searchwall = wallnum;
		}
		else if (nextsectnum == -1)
		{
			searchsector = sectnum;
			searchwall = wallnum;
			searchstat = 0;  //wall
			searchit = 1;
		}
		else
		{
			if (sector[nextsectnum].ceilingz > z1)
			{
				y = mulscale(swall[searchx],sector[nextsectnum].ceilingz-globalposz,24)+globalhoriz;
				if (y >= searchy)
				{
					searchsector = sectnum;
					searchwall = wallnum;
					searchstat = 0;  //wall
					searchit = 1;
				}
			}
			if (sector[nextsectnum].floorz < z2)
			{
				y = mulscale(swall[searchx],sector[nextsectnum].floorz-globalposz,24)+globalhoriz;
				if (y <= searchy)
				{
					searchsector = sectnum;
					searchwall = wallnum;
					if ((wall[wallnum].cstat&2) > 0) searchwall = wall[wallnum].nextwall1;
					searchstat = 0;  //wall
					searchit = 1;
					continue;
				}
			}
			if ((searchit == 2) && ((wall[wallnum].cstat&32) > 0))
			{
				searchsector = sectnum;
				searchstat = 4;  //1-way wall
				searchit = 1;
				searchwall = wallnum;
			}
		}
	}
	while ((z = p2[z]) != -1);

	if ((andwstat1&3) != 3)      //draw ceilings
	{
		if ((cstat1&3) == 0)
		{
			ceilscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
		}
		else if ((cstat1&1) > 0)  //Parallaxing SKY
		{
			globalpal = sector[sectnum].ceilingpal;
			globalorientation = 0L;
			globalpicnum = sector[sectnum].ceilingpicnum;
			if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
			globalshade = (long)sector[sectnum].ceilingshade;
			globalxpanning = (long)sector[sectnum].ceilingxpanning;
			globalypanning = (long)sector[sectnum].ceilingypanning;
			if ((picanm[globalpicnum]&192) != 0)
				globalpicnum += animateoffs(globalpicnum);
			globalshiftval = (picsiz[globalpicnum]>>4);
			if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
			globalshiftval = 32-globalshiftval;
			globalzd = (((tilesizy[globalpicnum]>>1)+parallaxyoffs)<<globalshiftval)+(globalypanning<<24);
			globalyscale = (8<<(globalshiftval-19));

			k = 11 - (picsiz[globalpicnum]&15) - pskybits;

			x = -1;

			z = bunchfirst[bunch];
			do
			{
				wallnum = thewall[z]; nextsectnum = wall[wallnum].nextsector1;

				if ((nextsectnum == -1) || ((sector[nextsectnum].ceilingstat&1) == 0) || ((wall[wallnum].cstat&32) > 0))
				{
					if (x == -1) x = xb1[z];

					switch(parallaxtype)
					{
						case 0:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								scalej = j;
								uwall[j] = umost[j];
								dwall[j] = uplc[j];
								swplc[j] = xdimenscale;
								lplc[j] = (((((((scalej-halfxdimen)<<9)/xdimen)+globalang)&2047)>>k)<<6);
							}
							break;
						case 1:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								uwall[j] = umost[j];
								dwall[j] = uplc[j];
								swplc[j] = xdimenscale;
								lplc[j] = ((((radarang2[j]+globalang)&2047)>>k)<<6);
							}
							break;
						case 2:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								uwall[j] = umost[j];
								dwall[j] = uplc[j];
								swplc[j] = divscale(16384L,(long)sintable[(radarang2[j]+2560)&2047],16);
								if (xdimen != XDIM)
									swplc[j] = mulscale(swplc[j],xdimenscale,16);
								lplc[j] = ((((radarang2[j]+globalang)&2047)>>k)<<6);
							}
							break;
					}
				}
				else if (x >= 0)
				{
					l = globalpicnum; m = (picsiz[globalpicnum]&15)+6;
					globalpicnum = l+pskyoff[lplc[x]>>m];

					if (globalpicnum == l+pskyoff[lplc[xb1[z]-1]>>m])
					{
						wallscan(x,xb1[z]-1,uwall,dwall,swplc,lplc);
					}
					else
					{
						j = x;
						while (x < xb1[z])
						{
							n = l+pskyoff[lplc[x]>>m];
							if (globalpicnum != n)
							{
								wallscan(j,x-1,uwall,dwall,swplc,lplc);
								j = x;
								globalpicnum = n;
							}
							x++;
						}
						if (j < x)
							wallscan(j,x-1,uwall,dwall,swplc,lplc);
					}

					globalpicnum = l;
					x = -1;
				}
			}
			while ((z = p2[z]) != -1);

			if (x >= 0)
			{
				l = globalpicnum; m = (picsiz[globalpicnum]&15)+6;
				globalpicnum = l+pskyoff[lplc[x]>>m];

				if (globalpicnum == l+pskyoff[lplc[xb2[bunchlast[bunch]]]>>m])
				{
					wallscan(x,xb2[bunchlast[bunch]],uwall,dwall,swplc,lplc);
				}
				else
				{
					j = x;
					while (x <= xb2[bunchlast[bunch]])
					{
						n = l+pskyoff[lplc[x]>>m];
						if (globalpicnum != n)
						{
							wallscan(j,x-1,uwall,dwall,swplc,lplc);
							j = x;
							globalpicnum = n;
						}
						x++;
					}
					if (j <= x)
						wallscan(j,x,uwall,dwall,swplc,lplc);
				}

				globalpicnum = l;
			}
		}
	}

	if ((andwstat2&0xc) != 0xc)   //draw floors
	{
		if ((cstat2&3) == 0)
		{
			florscan(xb1[bunchfirst[bunch]],xb2[bunchlast[bunch]],sectnum);
		}
		else if ((cstat2&1) > 0)  //Parallaxing floor SKY
		{
			globalpal = sector[sectnum].floorpal;
			globalpicnum = sector[sectnum].floorpicnum;
			globalorientation = 0L;
			if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
			globalshade = (long)sector[sectnum].floorshade;
			globalxpanning = (long)sector[sectnum].floorxpanning;
			globalypanning = (long)sector[sectnum].floorypanning;
			if ((picanm[globalpicnum]&192) != 0)
				globalpicnum += animateoffs(globalpicnum);
			globalshiftval = (picsiz[globalpicnum]>>4);
			if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
			globalshiftval = 32-globalshiftval;
			globalzd = (((tilesizy[globalpicnum]>>1)+parallaxyoffs)<<globalshiftval)+(globalypanning<<24);
			globalyscale = (8<<(globalshiftval-19));

			k = 11 - (picsiz[globalpicnum]&15) - pskybits;

			x = -1;

			z = bunchfirst[bunch];
			do
			{
				wallnum = thewall[z]; nextsectnum = wall[wallnum].nextsector1;

				if ((nextsectnum == -1) || ((sector[nextsectnum].floorstat&1) == 0) || ((wall[wallnum].cstat&32) > 0))
				{
					if (x == -1) x = xb1[z];

					switch(parallaxtype)
					{
						case 0:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								scalej = j;
								uwall[j] = dplc[j];
								dwall[j] = dmost[j];
								swplc[j] = xdimenscale;
								lplc[j] = (((((((scalej-halfxdimen)<<9)/xdimen)+globalang)&2047)>>k)<<6);
							}
							break;
						case 1:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								uwall[j] = dplc[j];
								dwall[j] = dmost[j];
								swplc[j] = xdimenscale;
								lplc[j] = ((((radarang2[j]+globalang)&2047)>>k)<<6);
							}
							break;
						case 2:
							for(j=xb1[z];j<=xb2[z];j++)
							{
								uwall[j] = dplc[j];
								dwall[j] = dmost[j];
								swplc[j] = divscale(16384L,(long)sintable[(radarang2[j]+2560)&2047],16);
								if (xdimen != XDIM)
									swplc[j] = mulscale(swplc[j],xdimenscale,16);
								lplc[j] = ((((radarang2[j]+globalang)&2047)>>k)<<6);
							}
							break;
					}
				}
				else if (x >= 0)
				{
					l = globalpicnum; m = (picsiz[globalpicnum]&15)+6;
					globalpicnum = l+pskyoff[lplc[x]>>m];

					if (globalpicnum == l+pskyoff[lplc[xb1[z]-1]>>m])
					{
						wallscan(x,xb1[z]-1,uwall,dwall,swplc,lplc);
					}
					else
					{
						j = x;
						while (x < xb1[z])
						{
							n = l+pskyoff[lplc[x]>>m];
							if (globalpicnum != n)
							{
								wallscan(j,x-1,uwall,dwall,swplc,lplc);
								j = x;
								globalpicnum = n;
							}
							x++;
						}
						if (j < x)
							wallscan(j,x-1,uwall,dwall,swplc,lplc);
					}

					globalpicnum = l;
					x = -1;
				}
			}
			while ((z = p2[z]) != -1);

			if (x >= 0)
			{
				l = globalpicnum; m = (picsiz[globalpicnum]&15)+6;
				globalpicnum = l+pskyoff[lplc[x]>>m];

				if (globalpicnum == l+pskyoff[lplc[xb2[bunchlast[bunch]]]>>m])
				{
					wallscan(x,xb2[bunchlast[bunch]],uwall,dwall,swplc,lplc);
				}
				else
				{
					j = x;
					while (x <= xb2[bunchlast[bunch]])
					{
						n = l+pskyoff[lplc[x]>>m];
						if (globalpicnum != n)
						{
							wallscan(j,x-1,uwall,dwall,swplc,lplc);
							j = x;
							globalpicnum = n;
						}
						x++;
					}
					if (j <= x)
						wallscan(j,x,uwall,dwall,swplc,lplc);
				}

				globalpicnum = l;
			}
		}
	}

	  //DRAW WALLS SECTION!
}

void drawalls(short bunch)
{
	char cstat1, cstat2;
	short z, wallnum, sectnum, nextsectnum;
	long x, templong;

	z = bunchfirst[bunch];
	sectnum = thesector[z];
	cstat1 = sector[sectnum].ceilingstat;
	cstat2 = sector[sectnum].floorstat;
	globalpal = (long)sector[sectnum].floorpal;

	do
	{
		wallnum = thewall[z];
		nextsectnum = wall[wallnum].nextsector1;

		if (nextsectnum == -1)
		{
			globalorientation = (long)wall[wallnum].cstat;
			globalpicnum = wall[wallnum].picnum;
			if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
			globalxpanning = (long)wall[wallnum].xpanning;
			globalypanning = (long)wall[wallnum].ypanning;
			if ((picanm[globalpicnum]&192) != 0)
				globalpicnum += animateoffs(globalpicnum);
			globalshade = (long)wall[wallnum].shade;
			globalshiftval = (picsiz[globalpicnum]>>4);
			if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
			globalshiftval = 32-globalshiftval;
			globalyscale = (wall[wallnum].yrepeat<<(globalshiftval-19));
			if ((globalorientation&4) == 0)
				globalzd = (((globalposz-sector[sectnum].ceilingz)*globalyscale)<<8);
			else
				globalzd = (((globalposz-sector[sectnum].floorz)*globalyscale)<<8);
			globalzd += (globalypanning<<24);

			wallscan(xb1[z],xb2[z],uplc,dplc,swall,lwall);

			for(x=xb1[z];x<=xb2[z];x++)
				if (umost[x] <= dmost[x])
				{
					umost[x] = ybot;
					dmost[x] = ytop;
					numhits--;
				}
			smostwall[smostwallcnt++] = z;
		}
		else
		{
			if ((wall[wallnum].cstat&16) > 0) maskwall[maskwallcnt++] = z;

			if (((cstat1&1) == 0) || ((sector[nextsectnum].ceilingstat&1) == 0))
			{
				if (sector[nextsectnum].ceilingz <= sector[sectnum].ceilingz)
				{
					for(x=xb1[z];x<=xb2[z];x++)
						if (umost[x] <= dmost[x])
							if (uplc[x] > umost[x])
							{
								umost[x] = uplc[x];
								if (umost[x] > dmost[x]) numhits--;
							}

					if ((sector[nextsectnum].ceilingz < sector[sectnum].ceilingz))
					{
						if (smostcnt+xb2[z]-xb1[z]+1 < MAXYSAVES)
						{
							smoststart[smostwallcnt] = smostcnt;
							smostwall[smostwallcnt++] = z+16384;   //16384 for umost
							for(x=xb1[z];x<=xb2[z];x++) smost[smostcnt++] = (short)umost[x];
						}
					}
				}
				else
				{
					if (sector[sectnum].floorz < sector[nextsectnum].ceilingz)
						templong = (sector[sectnum].floorz-globalposz);
					else
						templong = (sector[nextsectnum].ceilingz-globalposz);
					//ddplc = (globalhoriz<<16)+mulscale(swall[xb1[z]],templong,8);
					//if (xb2[z] > xb1[z])
					//   ddinc = mulscale((swall[xb2[z]]-swall[xb1[z]])/(xb2[z]-xb1[z]),templong,8);
					//qinterpolatedown16((long)(&dwall[xb1[z]]),xb2[z]-xb1[z],ddplc,ddinc);

					wallmost(dwall,(long)z,templong);

					globalorientation = (long)wall[wallnum].cstat;
					globalpicnum = wall[wallnum].picnum;
					if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
					globalxpanning = (long)wall[wallnum].xpanning;
					globalypanning = (long)wall[wallnum].ypanning;
					globalshiftval = (picsiz[globalpicnum]>>4);
					if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
					globalshiftval = 32-globalshiftval;
					if ((picanm[globalpicnum]&192) != 0)
						globalpicnum += animateoffs(globalpicnum);
					globalshade = (long)wall[wallnum].shade;
					globalyscale = (wall[wallnum].yrepeat<<(globalshiftval-19));
					if ((globalorientation&4) == 0)
						globalzd = (((globalposz-sector[nextsectnum].ceilingz)*globalyscale)<<8);
					else
						globalzd = (((globalposz-sector[sectnum].ceilingz)*globalyscale)<<8);
					globalzd += (globalypanning<<24);

					wallscan(xb1[z],xb2[z],uplc,dwall,swall,lwall);

					for(x=xb1[z];x<=xb2[z];x++)
						if (umost[x] <= dmost[x])
							if (dwall[x] > umost[x])
							{
								umost[x] = dwall[x];
								if (umost[x] > dmost[x]) numhits--;
							}

					if ((globalposz < sector[nextsectnum].ceilingz))
					{
						if (smostcnt+xb2[z]-xb1[z]+1 < MAXYSAVES)
						{
							smoststart[smostwallcnt] = smostcnt;
							smostwall[smostwallcnt++] = z+16384;   //16384 for umost
							for(x=xb1[z];x<=xb2[z];x++) smost[smostcnt++] = (short)umost[x];
						}
					}
				}
			}
			if (((cstat2&1) == 0) || ((sector[nextsectnum].floorstat&1) == 0))
			{
				if (sector[nextsectnum].floorz >= sector[sectnum].floorz)
				{
					for(x=xb1[z];x<=xb2[z];x++)
						if (umost[x] <= dmost[x])
							if (dplc[x] < dmost[x])
							{
								dmost[x] = dplc[x];
								if (umost[x] > dmost[x]) numhits--;
							}

					if ((sector[nextsectnum].floorz > sector[sectnum].floorz))
					{
						if (smostcnt+xb2[z]-xb1[z]+1 < MAXYSAVES)
						{
							smoststart[smostwallcnt] = smostcnt;
							smostwall[smostwallcnt++] = z+32768;   //32768 for dmost
							for(x=xb1[z];x<=xb2[z];x++) smost[smostcnt++] = (short)dmost[x];
						}
					}
				}
				else
				{
					if (sector[sectnum].ceilingz > sector[nextsectnum].floorz)
						templong = (sector[sectnum].ceilingz-globalposz);
					else
						templong = (sector[nextsectnum].floorz-globalposz);

					wallmost(uwall,(long)z,templong);

					if ((wall[wallnum].cstat&2) > 0)
					{
						wallnum = wall[wallnum].nextwall1;
						globalorientation = (long)wall[wallnum].cstat;
						globalpicnum = wall[wallnum].picnum;
						if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
						globalxpanning = (long)wall[wallnum].xpanning;
						globalypanning = (long)wall[wallnum].ypanning;
						if ((picanm[globalpicnum]&192) != 0)
							globalpicnum += animateoffs(globalpicnum);
						globalshade = (long)wall[wallnum].shade;
						wallnum = thewall[z];
					}
					else
					{
						globalorientation = (long)wall[wallnum].cstat;
						globalpicnum = wall[wallnum].picnum;
						if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
						globalxpanning = (long)wall[wallnum].xpanning;
						globalypanning = (long)wall[wallnum].ypanning;
						if ((picanm[globalpicnum]&192) != 0)
							globalpicnum += animateoffs(globalpicnum);
						globalshade = (long)wall[wallnum].shade;
					}
					globalshiftval = (picsiz[globalpicnum]>>4);
					if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
					globalshiftval = 32-globalshiftval;
					globalyscale = (wall[wallnum].yrepeat<<(globalshiftval-19));
					if ((globalorientation&4) == 0)
						globalzd = (((globalposz-sector[nextsectnum].floorz)*globalyscale)<<8);
					else
						globalzd = (((globalposz-sector[sectnum].ceilingz)*globalyscale)<<8);
					globalzd += (globalypanning<<24);

					wallscan(xb1[z],xb2[z],uwall,dplc,swall,lwall);

					for(x=xb1[z];x<=xb2[z];x++)
						if (umost[x] <= dmost[x])
							if (uwall[x] < dmost[x])
							{
								dmost[x] = uwall[x];
								if (umost[x] > dmost[x]) numhits--;
							}

					if ((globalposz > sector[nextsectnum].floorz))
					{
						if (smostcnt+xb2[z]-xb1[z]+1 < MAXYSAVES)
						{
							smoststart[smostwallcnt] = smostcnt;
							smostwall[smostwallcnt++] = z+32768;   //32768 for dmost
							for(x=xb1[z];x<=xb2[z];x++) smost[smostcnt++] = (short)dmost[x];
						}
					}
				}
			}

			if ((wall[wallnum].cstat&32) > 0)   //1-way wall
			{
				globalorientation = (long)wall[wallnum].cstat;
				globalpicnum = wall[wallnum].overpicnum;
				if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
				globalxpanning = (long)wall[wallnum].xpanning;
				globalypanning = (long)wall[wallnum].ypanning;
				if ((picanm[globalpicnum]&192) != 0)
					globalpicnum += animateoffs(globalpicnum);
				globalshade = (long)wall[wallnum].shade;
				globalshiftval = (picsiz[globalpicnum]>>4);
				if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
				globalshiftval = 32-globalshiftval;
				globalyscale = (wall[wallnum].yrepeat<<(globalshiftval-19));
				if ((globalorientation&4) == 0)
					globalzd = (((globalposz-sector[nextsectnum].ceilingz)*globalyscale)<<8);
				else
					globalzd = (((globalposz-sector[sectnum].ceilingz)*globalyscale)<<8);
				globalzd += (globalypanning<<24);

				wallscan(xb1[z],xb2[z],uplc,dplc,swall,lwall);

				for(x=xb1[z];x<=xb2[z];x++)
					if (umost[x] <= dmost[x])
					{
						umost[x] = ybot;
						dmost[x] = ytop;
						numhits--;
					}
				smostwall[smostwallcnt++] = z;
			}
		}
		if (numhits < 0) return;

		if ((gotsector[nextsectnum>>3]&(1<<(nextsectnum&7))) == 0)
			if ((nextsectnum != -1) && ((wall[wallnum].cstat&32) == 0))
			{
				for(x=xb1[z];x<=xb2[z];x++)
					if (umost[x] < dmost[x])
					{
						scansector(nextsectnum);
						break;
					}
			}
	}
	while ((z = p2[z]) != -1);
}

addboxlinum(short sectore, short walle)
{
	short nextsectnum;
	long x1, y1, x2, y2, xp1, yp1, xp2, yp2, xc, yc, templong, templong1;

	x1 = wall[walle].x-globalposx; y1 = wall[walle].y-globalposy;
	x2 = wall[wall[walle].point2].x-globalposx; y2 = wall[wall[walle].point2].y-globalposy;

	nextsectnum = wall[walle].nextsector1;
	if ((nextsectnum >= 0) && ((wall[walle].cstat&32) == 0))
		if ((gotsector[nextsectnum>>3]&(1<<(nextsectnum&7))) == 0)
		{
			templong = x1*y2-x2*y1;
			if (klabs(templong) < 262144)
				if (mulscale(templong,templong,5) <= (x2-x1)*(x2-x1)+(y2-y1)*(y2-y1))
					sectorborder[sectorbordercnt++] = nextsectnum;
		}

	xp1 = doublemulscale(y1,cosglobalang,-x1,singlobalang,14);
	yp1 = doublemulscale(x1,cosglobalang,y1,singlobalang,14);
	xp2 = doublemulscale(y2,cosglobalang,-x2,singlobalang,14);
	yp2 = doublemulscale(x2,cosglobalang,y2,singlobalang,14);
	if (xp1*yp2 > xp2*yp1) return(0);
	if ((yp1 <= 0) && (yp2 <= 0)) return(0);

	if (-yp1 <= xp1)
	{
		if (xp1 > yp1) return(0);
		if (yp1 == 0) return(0);
		xb1[numscans] = halfxdimen + scale(xp1,halfxdimen,yp1);
		if (xp1 >= 0) xb1[numscans]++;   //Fix for SIGNED divide
		if (xb1[numscans] >= xdimen) xb1[numscans] = xdimen-1;
		yb1[numscans] = yp1;
	}
	else
	{
		if (xp2 < -yp2) return(0);
		xb1[numscans] = 0;
		templong1 = yp1-yp2+xp1-xp2;
		if (templong1 == 0) return(0);
		yb1[numscans] = yp1 + scale(yp2-yp1,xp1+yp1,templong1);
	}
	if (yp2 >= xp2)
	{
		if (xp2 < -yp2) return(0);
		if (yp2 == 0) return(0);
		xb2[numscans] = halfxdimen + scale(xp2,halfxdimen,yp2) - 1;
		if (xp2 >= 0) xb2[numscans]++;   //Fix for SIGNED divide
		if (xb2[numscans] >= xdimen) xb2[numscans] = xdimen-1;
		yb2[numscans] = yp2;
	}
	else
	{
		if (xp1 > yp1) return(0);
		xb2[numscans] = xdimen-1;
		templong1 = xp2-xp1+yp1-yp2;
		if (templong1 == 0) return(0);
		yb2[numscans] = yp1 + scale(yp2-yp1,yp1-xp1,templong1);
	}
	if ((yb1[numscans] <= 0) || (yb2[numscans] <= 0) || (xb1[numscans] > xb2[numscans])) return(0);

		//Made it all the way!
	ry1[numscans] = yp1;
	thesector[numscans] = sectore;
	ry2[numscans] = yp2;
	thewall[numscans] = walle;
	rx1[numscans] = xp1;
	rx2[numscans] = xp2;
	numscans++;
	return(1);
}

ceilscan (long x1, long x2, short sectnum)
{
	long i, x, y1, y2, twall, bwall;

	if (sector[sectnum].ceilingpal != globalpalwritten)
	{
		globalpalwritten = sector[sectnum].ceilingpal;
		if (option[0] == 0)
			chainsetpalookupaddress(FP_OFF(palookup[globalpalwritten]));
		else
			setpalookupaddress(FP_OFF(palookup[globalpalwritten]));
	}

	globalzd = sector[sectnum].ceilingz-globalposz;
	if (globalzd > 0) return;
	globalpicnum = sector[sectnum].ceilingpicnum;
	if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
	if ((picanm[globalpicnum]&192) != 0)
		globalpicnum += animateoffs((short)globalpicnum);
	if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0))
		return;
	if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
	setgotpic(globalpicnum);
	globalbufplc = waloff[globalpicnum];
	globalshade = (long)sector[sectnum].ceilingshade;
	globalorientation = (long)sector[sectnum].ceilingstat;
	globalxpanning = (((long)sector[sectnum].ceilingxpanning)<<24);
	globalypanning = (((long)sector[sectnum].ceilingypanning)<<24);
	if ((globalorientation&64) > 0)
	{
		globalx1 = wall[sector[sectnum].wallptr].x;
		globaly1 = wall[sector[sectnum].wallptr].y;
		globalx2 = wall[wall[sector[sectnum].wallptr].point2].x - globalx1;
		globaly2 = wall[wall[sector[sectnum].wallptr].point2].y - globaly1;
		globalx1 = ((globalx1-globalposx)<<12);
		globaly1 = ((globaly1-globalposy)<<12);
		globalzx = ksqrtasm(globalx2*globalx2+globaly2*globaly2);
		if (globalzx == 0)
			globalzx = 1024;
		else
			globalzx = 1048576/globalzx;
		globalx2 *= globalzx;
		globaly2 *= globalzx;
	}
	if ((globalorientation&8) == 0)
	{
		globalxshift = (8-(picsiz[globalpicnum]&15));
		globalyshift = (8-(picsiz[globalpicnum]>>4));
	}
	else if (picsiz[globalpicnum] == 0x66)
	{
		globalxshift = 3;
		globalyshift = 3;
	}
	else
	{
		globalxshift = 2;
		globalyshift = 2;
	}

	if (keystatus[0x29] == 2)
	{
		while (keystatus[0xe] == 0);
		keystatus[0xe] = 0;
	}

	if (option[0] == 0)
		sethlinechainsizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);
	else
		sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

	y1 = umost[x1]-1; y2 = y1;

	for(x=x1;x<=x2;x++)
	{
		twall = umost[x]-1;
		bwall = uplc[x];
		if (bwall > dmost[x]) bwall = dmost[x];

		if (twall < bwall-1)
		{
			if (twall >= y2)
			{
				while (y1 < y2-1)
				{
					y1++;
					hline(lastx[y1],x-1,y1,sectnum);
				}
				y1 = twall;
			}
			else
			{
				while (y1 < twall)
				{
					y1++;
					hline(lastx[y1],x-1,y1,sectnum);
				}
				while (y1 > twall)
				{
					lastx[y1] = x;
					y1--;
				}
			}

			while (y2 > bwall)
			{
				y2--;
				hline(lastx[y2],x-1,y2,sectnum);
			}
			while (y2 < bwall)
			{
				lastx[y2] = x;
				y2++;
			}
		}
		else
		{
			while (y1 < y2-1)
			{
				y1++;
				hline(lastx[y1],x-1,y1,sectnum);
			}
			y1 = umost[x+1]-1;
			y2 = y1;
		}
	}
	while (y1 < y2-1)
	{
		y1++;
		hline(lastx[y1],x2,y1,sectnum);
	}

}

florscan (long x1, long x2, short sectnum)
{
	long i, x, y1, y2, twall, bwall;

	if (sector[sectnum].floorpal != globalpalwritten)
	{
		globalpalwritten = sector[sectnum].floorpal;
		if (option[0] == 0)
			chainsetpalookupaddress(FP_OFF(palookup[globalpalwritten]));
		else
			setpalookupaddress(FP_OFF(palookup[globalpalwritten]));
	}

	globalzd = globalposz-sector[sectnum].floorz;
	if (globalzd > 0) return;
	globalpicnum = sector[sectnum].floorpicnum;
	if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
	if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0))
		return;
	if ((picanm[globalpicnum]&192) != 0)
		globalpicnum += animateoffs((short)globalpicnum);
	if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
	setgotpic(globalpicnum);
	globalbufplc = waloff[globalpicnum];
	globalshade = (long)sector[sectnum].floorshade;
	globalorientation = (long)sector[sectnum].floorstat;
	globalxpanning = (((long)sector[sectnum].floorxpanning)<<24);
	globalypanning = (((long)sector[sectnum].floorypanning)<<24);
	if ((globalorientation&64) > 0)
	{
		globalx1 = wall[sector[sectnum].wallptr].x;
		globaly1 = wall[sector[sectnum].wallptr].y;
		globalx2 = wall[wall[sector[sectnum].wallptr].point2].x - globalx1;
		globaly2 = wall[wall[sector[sectnum].wallptr].point2].y - globaly1;
		globalx1 = ((globalx1-globalposx)<<12);
		globaly1 = ((globaly1-globalposy)<<12);
		globalzx = ksqrtasm(globalx2*globalx2+globaly2*globaly2);
		if (globalzx == 0)
			globalzx = 1024;
		else
			globalzx = 1048576/globalzx;
		globalx2 *= globalzx;
		globaly2 *= globalzx;
	}
	if ((globalorientation&8) == 0)
	{
		globalxshift = (8-(picsiz[globalpicnum]&15));
		globalyshift = (8-(picsiz[globalpicnum]>>4));
	}
	else if (picsiz[globalpicnum] == 0x66)
	{
		globalxshift = 3;
		globalyshift = 3;
	}
	else
	{
		globalxshift = 2;
		globalyshift = 2;
	}

	if (keystatus[0x29] == 2)
	{
		while (keystatus[0xe] == 0);
		keystatus[0xe] = 0;
	}

	if (option[0] == 0)
		sethlinechainsizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);
	else
		sethlinesizes(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4,globalbufplc);

	y1 = dplc[x1]-1;
	if (y1 < umost[x1]-1) y1 = umost[x1]-1;
	y2 = y1;

	for(x=x1;x<=x2;x++)
	{
		twall = dplc[x]-1;
		if (twall < umost[x]-1) twall = umost[x]-1;
		bwall = dmost[x];

		if (twall < bwall-1)
		{
			if (twall >= y2)
			{
				while (y1 < y2-1)
				{
					y1++;
					hline(lastx[y1],x-1,y1,sectnum);
				}
				y1 = twall;
			}
			else
			{
				while (y1 < twall)
				{
					y1++;
					hline(lastx[y1],x-1,y1,sectnum);
				}
				while (y1 > twall)
				{
					lastx[y1] = x;
					y1--;
				}
			}

			while (y2 > bwall)
			{
				y2--;
				hline(lastx[y2],x-1,y2,sectnum);
			}
			while (y2 < bwall)
			{
				lastx[y2] = x;
				y2++;
			}
		}
		else
		{
			while (y1 < y2-1)
			{
				y1++;
				hline(lastx[y1],x-1,y1,sectnum);
			}
			y1 = dplc[x+1]-1;
			if (y1 < umost[x+1]-1) twall = umost[x+1]-1;
			y2 = y1;
		}
	}
	while (y1 < y2-1)
	{
		y1++;
		hline(lastx[y1],x2,y1,sectnum);
	}

}

wallscan(long x1, long x2, long *uwal, long *dwal, long *swal, long *lwal)
{
	char plane, startplane;
	long x, xinc, xinc3, startx, xnice, ynice;

	long y1v, y2v;                           //vline1
	long vplc, vinc, bufplc, palookupoffs;

	long y1ve[4], y2ve[4], shade, bad;       //vline4
	long vplce[4], vince[4], palookupoffse[4], bufplce[4];
	long u1, u4, d4, d1, dax, z, oldp, p, linum;

	if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;
	if ((uwal[x1] > ybot) && (uwal[x2] > ybot)) return;
	if ((dwal[x1] < ytop) && (dwal[x2] < ytop)) return;

	if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
	setgotpic(globalpicnum);

	if ((globalorientation&256) > 0)
		globalyscale = -globalyscale, globalzd = -globalzd;

	if (keystatus[0x29] == 2)
	{
		while (keystatus[0xe] == 0);
		keystatus[0xe] = 0;
	}

	if (x1 < 0) x1 = 0;
	if (x2 >= xdimen) x2 = xdimen-1;

	if (option[0] == 0)
	{
		xinc = 4; xinc3 = 12;
		plane = (1<<((x1+viewoffset)&3));
		startplane = plane;
		if ((x2-x1) < 4) startplane = (1<<((x2+1+viewoffset)&3));
		koutpw(0x3c4,2+(((short)plane)<<8));
	}
	else
	{
		xinc = 1; xinc3 = 3;
	}
	startx = x1;

	xnice = ((1<<(picsiz[globalpicnum]&15)) == tilesizx[globalpicnum]);
	ynice = ((1<<(picsiz[globalpicnum]>>4)) == tilesizy[globalpicnum]);

	do
	{
		x = startx;
		while ((umost[x] > dmost[x]) && (x <= x2))
			x += xinc;
		while (x <= x2-xinc3)
		{
			bad = 0;                                      //VLINE4

			dax = x;
			for(z=0;z<4;z++)
			{
				y1ve[z] = uwal[dax]; if (y1ve[z] < umost[dax]) y1ve[z] = umost[dax];
				y2ve[z] = dwal[dax]-1; if (y2ve[z] >= dmost[dax]) y2ve[z] = dmost[dax]-1;
				if ((y2ve[z] < y1ve[z]) || (swal[dax] <= 4L))
					{ bad += (1<<z); dax += xinc; continue; }

				vince[z] = divscale(65536L,swal[dax],16);

				shade = (vince[z]>>(visibility+2))+globalshade;
				if (shade < 0) shade = 0;
				if (shade >= numpalookups) shade = numpalookups-1;
				palookupoffse[z] = FP_OFF(palookup[globalpal])+(shade<<8);

				vince[z] *= globalyscale;
				linum = (lwal[dax]>>6) + globalxpanning;
				if (xnice == 0) linum %= tilesizx[globalpicnum]; else linum &= (tilesizx[globalpicnum]-1);
				if (ynice == 0) linum *= tilesizy[globalpicnum]; else linum <<= (picsiz[globalpicnum]>>4);
				bufplce[z] = waloff[globalpicnum]+linum;

				vplce[z] = globalzd + optimul(vince[z],y1ve[z]-globalhoriz+1);

				dax += xinc;
			}
			if (bad != 15)
			{
				u1 = ybot; u4 = ytop; d4 = u1; d1 = u4;
				for(z=0;z<4;z++)
				{
					if (y1ve[z] < u1) u1 = y1ve[z];
					if (y1ve[z] > u4) u4 = y1ve[z];
					if (y2ve[z] < d4) d4 = y2ve[z];
					if (y2ve[z] > d1) d1 = y2ve[z];
				}

				if ((bad > 0) || (u4 >= d4))
				{
					for(z=0;z<4;z++)
						if ((bad&(1<<z)) == 0)
						{
							if (option[0] == 0)
								p = mul80(y1ve[z])+((x+viewoffset)>>2)+z+0xa0000+(page<<14);
							else if (option[0] != 7)
								p = mul320(y1ve[z])+x+z+frameplace+viewoffset;
							else
								p = mul640(y1ve[z])+x+z+frameplace+viewoffset;
							vlineasm1(vince[z],palookupoffse[z],((y2ve[z]-y1ve[z])<<8)+globalshiftval,vplce[z],bufplce[z],p);
						}
				}
				else
				{
						//Draw 1 pixel at a time for top
					for(z=0;z<4;z++)
					{
						if (option[0] == 0)
							p = mul80(y1ve[z])+((x+viewoffset)>>2)+z+0xa0000+(page<<14);
						else if (option[0] != 7)
							p = mul320(y1ve[z])+x+z+frameplace+viewoffset;
						else
							p = mul640(y1ve[z])+x+z+frameplace+viewoffset;
						vplce[z] = vlineasm1(vince[z],palookupoffse[z],((u4-y1ve[z])<<8)+globalshiftval,vplce[z],bufplce[z],p);
					}

					//Draw 4 pixels at a time
					if (d4 > u4)
					{
						if (option[0] == 0)
							p = mul80(u4+1)+((x+viewoffset)>>2)+0xa0000+(page<<14);
						else if (option[0] != 7)
							p = mul320(u4+1)+x+frameplace+viewoffset;
						else
							p = mul640(u4+1)+x+frameplace+viewoffset;
						p = vlineasm4((long)(&vplce[0]),(long)(&vince[0]),((d4-u4)<<8)-256+globalshiftval,(long)(&bufplce[0]),(long)(&palookupoffse[0]),p);
					}

						//Draw 1 pixel at a time for bottom
					oldp = p;
					for(z=0;z<4;z++)
						if (y2ve[z] > d4)
							vlineasm1(vince[z],palookupoffse[z],((y2ve[z]-d4)<<8)-256+globalshiftval,vplce[z],bufplce[z],oldp+z);
				}
			}

			x += (xinc<<2);
		}
		while (x <= x2)
		{
			if (swal[x] > 4L)               //VLINE1
			{
				y1v = uwal[x];
				y2v = dwal[x]-1;
				if (y1v < umost[x]) y1v = umost[x];
				if (y2v >= dmost[x]) y2v = dmost[x]-1;
				if (y2v >= y1v)
				{
					vinc = divscale(65536L,swal[x],16);

					shade = (vinc>>(visibility+2))+globalshade;
					if (shade < 0) shade = 0;
					if (shade >= numpalookups) shade = numpalookups-1;
					vinc *= globalyscale;
					palookupoffs = FP_OFF(palookup[globalpal])+(shade<<8);

					linum = (lwal[x]>>6) + globalxpanning;
					if (xnice == 0) linum %= tilesizx[globalpicnum]; else linum &= (tilesizx[globalpicnum]-1);
					if (ynice == 0) linum *= tilesizy[globalpicnum]; else linum <<= (picsiz[globalpicnum]>>4);
					bufplc = waloff[globalpicnum]+linum;

					vplc = globalzd + optimul(vinc,y1v-globalhoriz+1);

					if (option[0] == 0)
						p = mul80(y1v)+((x+viewoffset)>>2)+0xa0000+(page<<14);
					else if (option[0] != 7)
						p = mul320(y1v)+x+frameplace+viewoffset;
					else
						p = mul640(y1v)+x+frameplace+viewoffset;
					vlineasm1(vinc,palookupoffs,((y2v-y1v)<<8)+globalshiftval,vplc,bufplc,p);
				}
			}

			x += xinc;
		}

		if (option[0] == 0)
		{
			plane <<= 1; if (plane == 16) plane = 1;
			koutpw(0x3c4,2+(((short)plane)<<8));
			startx++;
		}
	}
	while ((xinc == 4) && (plane != startplane));

}

maskwallscan(long x1, long x2, long *uwal, long *dwal, long *swal, long *lwal)
{
	char startplane, plane;
	long x, xinc, xinc3, startx, xnice, ynice;

	long y1v, y2v;                           //vline1
	long vplc, vinc, bufplc, palookupoffs;

	long y1ve[4], y2ve[4], shade, bad;       //vline4
	long vplce[4], vince[4], palookupoffse[4], bufplce[4];
	long u1, u4, d4, d1, dax, z, oldp, p, linum;

	if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0))
		return;
	if ((uwal[x1] > ybot) && (uwal[x2] > ybot)) return;
	if ((dwal[x1] < ytop) && (dwal[x2] < ytop)) return;

	if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
	setgotpic(globalpicnum);

	if ((globalorientation&256) > 0)
		globalyscale = -globalyscale, globalzd = -globalzd;

	if (x1 < 0) x1 = 0;
	if (x2 >= xdimen) x2 = xdimen-1;

	if (option[0] == 0)
	{
		xinc = 4; xinc3 = 12;
		plane = (1<<((x1+viewoffset)&3));
		startplane = plane;
		if ((x2-x1) < 4) startplane = (1<<((x2+1+viewoffset)&3));
		koutpw(0x3c4,2+(((short)plane)<<8));
	}
	else
	{
		xinc = 1; xinc3 = 3;
	}
	startx = x1;

	xnice = ((1<<(picsiz[globalpicnum]&15)) == tilesizx[globalpicnum]);
	ynice = ((1<<(picsiz[globalpicnum]>>4)) == tilesizy[globalpicnum]);

	do
	{
		x = startx;
		while ((umost[x] > dmost[x]) && (x <= x2))
			x += xinc;
		while (x <= x2-xinc3)
		{
			bad = 0;                               //MVLINE4

			dax = x;
			for(z=0;z<4;z++)
			{
				y1ve[z] = uwal[dax];
				if (y1ve[z] < umost[dax]) y1ve[z] = umost[dax];
				y2ve[z] = dwal[dax]-1;
				if (y2ve[z] >= dmost[dax]) y2ve[z] = dmost[dax]-1;

				if ((y2ve[z] < y1ve[z]) || (swal[dax] <= 4L))
				{
					bad += (1<<z);

					dax += xinc;
					continue;
				}

				vince[z] = divscale(65536L,swal[dax],16);

				shade = (vince[z]>>(visibility+2))+globalshade;
				if (shade < 0) shade = 0;
				if (shade >= numpalookups) shade = numpalookups-1;
				palookupoffse[z] = FP_OFF(palookup[globalpal]) + (shade<<8);

				vince[z] *= globalyscale;

				linum = (lwal[dax]>>6) + globalxpanning;
				if (xnice == 0)
					linum %= tilesizx[globalpicnum];
				else
					linum &= (tilesizx[globalpicnum]-1);
				if (ynice == 0)
					linum *= tilesizy[globalpicnum];
				else
					linum <<= (picsiz[globalpicnum]>>4);
				bufplce[z] = waloff[globalpicnum]+linum;

				vplce[z] = globalzd + vince[z]*(y1ve[z]-globalhoriz+1);

				dax += xinc;
			}
			if (bad != 15)
			{
				u1 = ybot; u4 = ytop; d4 = u1; d1 = u4;
				for(z=0;z<4;z++)
				{
					if (y1ve[z] < u1) u1 = y1ve[z];
					if (y1ve[z] > u4) u4 = y1ve[z];
					if (y2ve[z] < d4) d4 = y2ve[z];
					if (y2ve[z] > d1) d1 = y2ve[z];
				}

				if ((bad > 0) || (u4 >= d4))
				{
					for(z=0;z<4;z++)
					if ((bad&(1<<z)) == 0)
					{
						if (option[0] == 0)
							p = mul80(y1ve[z])+((x+viewoffset)>>2)+z+0xa0000+(page<<14);
						else if (option[0] != 7)
							p = mul320(y1ve[z])+x+z+frameplace+viewoffset;
						else
							p = mul640(y1ve[z])+x+z+frameplace+viewoffset;
						mvlineasm1(vince[z],palookupoffse[z],(((long)(y2ve[z]-y1ve[z]))<<8)+globalshiftval,vplce[z],bufplce[z],p);
					}
				}
				else
				{
						//Draw 1 pixel at a time for top
					for(z=0;z<4;z++)
					{
						if (option[0] == 0)
							p = mul80(y1ve[z])+((x+viewoffset)>>2)+z+0xa0000+(page<<14);
						else if (option[0] != 7)
							p = mul320(y1ve[z])+x+z+frameplace+viewoffset;
						else
							p = mul640(y1ve[z])+x+z+frameplace+viewoffset;
						vplce[z] = mvlineasm1(vince[z],palookupoffse[z],(((long)(u4-y1ve[z]))<<8)+globalshiftval,vplce[z],bufplce[z],p);
					}

					//Draw 4 pixels at a time
					if (d4 > u4)
					{
						if (option[0] == 0)
							p = mul80(u4+1)+((x+viewoffset)>>2)+0xa0000+(page<<14);
						else if (option[0] != 7)
							p = mul320(u4+1)+x+frameplace+viewoffset;
						else
							p = mul640(u4+1)+x+frameplace+viewoffset;
						p = mvlineasm4((long)(&vplce[0]),(long)(&vince[0]),((d4-u4-1)<<8)+globalshiftval,(long)(&bufplce[0]),(long)(&palookupoffse[0]),p);
					}

						//Draw 1 pixel at a time for bottom
					oldp = p;
					for(z=0;z<4;z++)
						if (y2ve[z] > d4)
							mvlineasm1(vince[z],palookupoffse[z],(((long)(y2ve[z]-d4-1))<<8)+globalshiftval,vplce[z],bufplce[z],oldp+z);
				}
			}

			x += (xinc<<2);
		}
		while (x <= x2)
		{
			if (swal[x] > 4L)               //VLINE1
			{
				y1v = uwal[x];
				y2v = dwal[x]-1;
				if (y1v < umost[x]) y1v = umost[x];
				if (y2v >= dmost[x]) y2v = dmost[x]-1;

				if (y2v >= y1v)
				{
					vinc = divscale(65536L,swal[x],16);

					shade = (vinc>>(visibility+2))+globalshade;
					if (shade < 0) shade = 0;
					if (shade >= numpalookups) shade = numpalookups-1;
					palookupoffs = FP_OFF(palookup[globalpal]) + (shade<<8);

					vinc *= globalyscale;

					linum = (lwal[x]>>6) + globalxpanning;
					if (xnice == 0)
						linum %= tilesizx[globalpicnum];
					else
						linum &= (tilesizx[globalpicnum]-1);
					if (ynice == 0)
					  linum *= tilesizy[globalpicnum];
					else
						linum <<= (picsiz[globalpicnum]>>4);
					bufplc = waloff[globalpicnum]+linum;

					vplc = globalzd + vinc*(y1v-globalhoriz+1);

					if (option[0] == 0)
						p = mul80(y1v)+((x+viewoffset)>>2)+0xa0000+(page<<14);
					else if (option[0] != 7)
						p = mul320(y1v)+x+frameplace+viewoffset;
					else
						p = mul640(y1v)+x+frameplace+viewoffset;
					mvlineasm1(vinc,palookupoffs,(((long)(y2v-y1v))<<8)+globalshiftval,vplc,bufplc,p);
				}
			}

			x += xinc;
		}

		if (option[0] == 0)
		{
			plane <<= 1; if (plane == 16) plane = 1;
			koutpw(0x3c4,2+(((short)plane)<<8));
			startx++;
		}
	}
	while ((xinc == 4) && (plane != startplane));

}

transmaskwallscan(long x1, long x2)
{
	char startplane, plane;
	long x, startx;


	if ((tilesizx[globalpicnum] <= 0) || (tilesizy[globalpicnum] <= 0)) return;

	if ((globalorientation&256) > 0)
		globalyscale = -globalyscale, globalzd = -globalzd;

	if (option[0] == 0)
	{
		plane = ((x1+viewoffset)&3);
		startplane = plane;
		if ((x2-x1) < 4) startplane = ((x2+1+viewoffset)&3);
		startx = x1;
		do
		{
			koutpw(0x3c4,2+(((short)1<<plane)<<8));
			koutpw(0x3ce,4+((((short)plane)&3)<<8));

			x = startx;
			while ((umost[x] > dmost[x]) && (x <= x2)) x += 4;

			while (x <= x2) transmaskvline(x), x += 4;

			plane = ((plane+1)&3);
			startx++;
		}
		while (plane != startplane);
	}
	else
	{
		x = x1;
		while ((umost[x] > dmost[x]) && (x <= x2)) x++;

		while (x <= x2) transmaskvline(x), x++;
	}

}

loadboard(char *filename, long *daposx, long *daposy, long *daposz,
			 short *daang, short *dacursectnum)
{
	short int fil, i, numsprites;

	if ((fil = open(filename,O_BINARY|O_RDWR,S_IREAD)) == -1)
	{
		mapversion = 5L;
		return(-1);
	}

	read(fil,&mapversion,4);
	if (mapversion != 5L) return(-1);

	initspritelists();

	clearbuf((long)(&show2dsprite[0]),(long)((MAXSPRITES+3)>>5),0L);
	clearbuf((long)(&show2dwall[0]),(long)((MAXWALLS+3)>>5),0L);

	read(fil,daposx,4);
	read(fil,daposy,4);
	read(fil,daposz,4);
	read(fil,daang,2);
	read(fil,dacursectnum,2);

	read(fil,&numsectors,2);
	for(i=0;i<numsectors;i++) read(fil,&sector[i],sizeof(struct sectortype));

	read(fil,&numwalls,2);
	for(i=0;i<numwalls;i++) read(fil,&wall[i],sizeof(struct walltype));

	read(fil,&numsprites,2);
	for(i=0;i<numsprites;i++)
	{
		read(fil,&sprite[i],sizeof(struct spritetype));
		insertsprite(sprite[i].sectnum,sprite[i].statnum);
	}

	read(fil,&numextras,2);
	for(i=0;i<numextras;i++) read(fil,&extra[i],sizeof(struct extratype));

		//Must be after loading sectors, etc!
	updatesector(*daposx,*daposy,dacursectnum);

	close(fil);
	return(0);
}

saveboard(char *filename, long *daposx, long *daposy, long *daposz,
			 short *daang, short *dacursectnum)
{
	short int fil, i, j, numsprites;

	if ((fil = open(filename,O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		return(-1);
	write(fil,&mapversion,4);

	write(fil,daposx,4);
	write(fil,daposy,4);
	write(fil,daposz,4);
	write(fil,daang,2);
	write(fil,dacursectnum,2);

	write(fil,&numsectors,2);
	for(i=0;i<numsectors;i++) write(fil,&sector[i],sizeof(struct sectortype));

	write(fil,&numwalls,2);
	for(i=0;i<numwalls;i++) write(fil,&wall[i],sizeof(struct walltype));

	numsprites = 0;
	for(j=0;j<MAXSTATUS;j++)
	{
		i = headspritestat[j];
		while (i != -1)
		{
			numsprites++;
			i = nextspritestat[i];
		}
	}
	write(fil,&numsprites,2);

	for(j=0;j<MAXSTATUS;j++)
	{
		i = headspritestat[j];
		while (i != -1)
		{
			write(fil,&sprite[i],sizeof(struct spritetype));
			i = nextspritestat[i];
		}
	}

	write(fil,&numextras,2);
	for(i=0;i<numextras;i++) write(fil,&extra[i],sizeof(struct extratype));

	close(fil);
	return(0);
}

loadtables()
{
	long i, fil;

	if (tablesloaded == 0)
	{
		for(i=0;i<2048;i++)
			sqrtable[i] = calcksqrt(i<<20);
		if ((fil = open("tables.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
		{
			read(fil,&sintable[0],4096);
			read(fil,&tantable[0],4096);
			read(fil,&radarang[0],640);
			read(fil,&textfont[0],1024);
			read(fil,&smalltextfont[0],1024);
			read(fil,&britable[0],1024);
			close(fil);
		}
		if ((fil = open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
		{
			read(fil,&option[0],NUMOPTIONS);
			read(fil,&keys[0],NUMKEYS);
			close(fil);
		}
		else
		{
			for(i=0;i<NUMOPTIONS;i++) option[i] = defaultoption[i];
			for(i=0;i<NUMKEYS;i++) keys[i] = defaultkey[i];
		}
		tablesloaded = 1;
	}
}

loadpalette()
{
	long i, j, fil;

	if (paletteloaded == 0)
	{
		if ((fil = open("palette.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
		{
			numpalookups = ((filelength(fil)-768)>>7);
			if ((numpalookups&1) > 0)
			{   //Transluscent palette found in palette.dat
				numpalookups = ((numpalookups-255)>>1);

				if (translucloaded == 0)
				{
						//Allocate transluscent buffer
					if ((transluc = (char *)malloc(65536L)) != NULL)
					{
						fixtransluscence(FP_OFF(transluc));
						translucloaded = 1;
					}
					else if (cachesize >= 262144)
					{
						initcache();
						cachesize -= 65536L;
						transluc = (char *)(FP_OFF(pic)+cachesize);
						fixtransluscence(FP_OFF(pic)+cachesize);
						translucloaded = 1;
					}
				}
			}
			else
			{    //No transluscent palette in palette.dat
				numpalookups >>= 1;
			}
			read(fil,&palette[0],768);

				//Allocate palookup buffer
			if ((palookup[0] = (char *)malloc(numpalookups<<8)) == NULL)
			{
				initcache();
				cachesize -= (numpalookups<<8);
				palookup[0] = (char *)(FP_OFF(pic)+cachesize);
			}

			globalpal = 0;
			globalpalwritten = globalpal;
			if (option[0] == 0)
				chainsetpalookupaddress(FP_OFF(palookup[globalpalwritten]));
			else
				setpalookupaddress(FP_OFF(palookup[globalpalwritten]));

			read(fil,palookup[globalpal],(numpalookups<<8));
			if (translucloaded == 1)
			{
				for(i=0;i<255;i++)
				{
					read(fil,&transluc[(i<<8)+i+1],255-i);
					for(j=i+1;j<256;j++)
						transluc[(j<<8)+i] = transluc[(i<<8)+j];
				}
				for(i=0;i<256;i++)
					transluc[(i<<8)+i] = i;
			}
			close(fil);
		}
		paletteloaded = 1;
	}
}

setgamemode()
{
	long i;

	if (qsetmode != 200)
	{
		if ((ooption0 == 5) || (ooption0 == 6))
		{
			setvmode(0x13);
			if (ooption0 == 5)
				stereoinit();
			stereopage = 0;
			if (ooption0 == 6)
			{
				setredbluepalette();
				goto modeset;
			}
		}
		else
		{
			if (option[0] == 7)
			{
				setvesamode(0x101);
				clearbuf(frameplace,(640L*480L)>>2,0L);
			}
			else
				setvmode(0x13);

			if (option[0] == 0)
			{
				koutp(0x3c4,0x4); koutp(0x3c5,0x6);
				koutp(0x3d4,0x14); koutp(0x3d5,0x0);
				koutp(0x3d4,0x17); koutp(0x3d5,0xe3);

				koutp(0x3c4,2); koutp(0x3c5,15);
				clearbuf(0xa0000,(4096<<2),0L);
			}
			if (option[0] == 2)
			{
				for(i=0;i<4;i++)
				{
					koutp(0x3cd,i);
					clearbuf(0xa0000,16000L,0L);
				}
			}
			if (option[0] == 3)
			{
				koutp(0x3ce,0xf); koutp(0x3cf,0x5);
				koutp(0x3d4,0x29); koutp(0x3d5,0x85);
				koutp(0x3c4,0x6); koutp(0x3c5,0x48);
				koutp(0x3d4,0x2f); koutp(0x3d5,0);   //set mod = 256k

				for(i=0;i<4;i++)
				{
					koutp(0x3ce,0x9); koutp(0x3cf,i<<4);
					clearbuf(0xa0000,16000L,0L);
				}
			}
			if (option[0] == 4)
			{
				koutp(0x3d4,0x38); koutp(0x3d5,0x48);           //S3 Extensions enable #1
				koutp(0x3d4,0x39); koutp(0x3d5,0xa5);           //S3 Extensions enable #2
				koutp(0x3d4,0x31); koutp(0x3d5,kinp(0x3d5)|9);   //Init for start address

				for(i=0;i<4;i++)
				{
					koutp(0x3d4,0x35); koutp(0x3d5,i);
					clearbuf(0xa0000,16000L,0L);
				}
			}
		}
		setbrightness((char)curbrightness);
	}

modeset:
	qsetmode = 200;
}

hline (long xl, long xr, long yp, short sectnum)
{
	// COMPILER_STEERED: Watcom stack-home order for r and xr.
	long y, p, r, x, temp, xinc, yinc, tempx, tempy, shade;

	if (xl > xr) return;
	if (yp == globalhoriz-1)
		r = divscale(globalzd,160L,14);
	else
		r = divscale(globalzd,320L*klabs(globalhoriz-1-yp),14);
	xinc = mulscale(r,singlobalang,4);
	yinc = mulscale(r,cosglobalang,4);
	x = xinc*xl - (xinc+yinc)*(xdimen>>1) + (globalposx<<20);
	y = yinc*xl + (xinc-yinc)*(xdimen>>1) - (globalposy<<20);

	if ((globalorientation&0x74) > 0)
	{
		if ((globalorientation&0x40) > 0)
		{
			tempx = mulscale(xinc,xl-(xdimen>>1),8) - mulscale(yinc,xdimen>>1,8) - globalx1;
			tempy = -mulscale(yinc,xl-(xdimen>>1),8) - mulscale(xinc,xdimen>>1,8) - globaly1;
			x = mulscale(globalx2,tempx,12) + mulscale(tempy,globaly2,12);
			y = mulscale(globalx2,tempy,12) - mulscale(tempx,globaly2,12);
			tempx = xinc;
			tempy = -yinc;
			xinc = mulscale(globalx2,tempx,20) + mulscale(tempy,globaly2,20);
			yinc = mulscale(globalx2,tempy,20) - mulscale(tempx,globaly2,20);
		}

		if ((globalorientation&0x4) > 0)
		{
			temp = x; x = y; y = temp;
			temp = xinc; xinc = yinc; yinc = temp;
		}
		if ((globalorientation&0x10) > 0) x ^= 0xffffffff, xinc ^= 0xffffffff;
		if ((globalorientation&0x20) > 0) y ^= 0xffffffff, yinc ^= 0xffffffff;
	}

	if (yp > globalhoriz) y ^= 0xffffffff, yinc ^= 0xffffffff;

	x <<= globalxshift;
	xinc <<= globalxshift;
	y <<= globalyshift;
	yinc <<= globalyshift;
	x += globalxpanning;
	y += globalypanning;

	shade = (klabs(r)>>visibility)+globalshade;
	if (shade < 0) shade = 0;
	if (shade >= numpalookups) shade = numpalookups-1;
	asm1 = xinc;
	asm2 = yinc;

	if (option[0] == 0)
	{
		p = ((xl+viewoffset)>>2)+mul80(yp)+0xa0000+(page<<14);
		asm3 = xr-xl;
		hchainasm64((long)(((xr+1)&3)<<8)+((xl+viewoffset)&3),y,shade<<8,0L,x,p);
		return;
	}
	if (option[0] != 7)
		p = mul320(yp)+xl+frameplace+viewoffset;
	else
		p = mul640(yp)+xl+frameplace+viewoffset;
	asm3 = xr-xl+1;
	hlineasm64(0L,0L,shade,y,x,p);
}

transmaskvline (long x)
{
	short y1v, y2v;
	long vplc, vinc, p, bufplc, palookupoffs, linum, shade;

	if ((x < 0) || (x >= xdimen) || (swall[x] <= 4L)) return;

	y1v = uwall[x];
	y2v = dwall[x]-1;
	if (y1v < umost[x]) y1v = umost[x];
	if (y2v >= dmost[x]) y2v = dmost[x]-1;

	if (y2v < y1v) return;

	vinc = divscale(65536L,swall[x],16);

	shade = (vinc>>(visibility+2))+globalshade;
	if (shade < 0) shade = 0;
	if (shade >= numpalookups) shade = numpalookups-1;
	palookupoffs = FP_OFF(palookup[globalpal]) + (shade<<8);

	vinc *= globalyscale;
	vplc = globalzd + vinc*(y1v-globalhoriz+1);

	linum = (lwall[x]>>6) + globalxpanning;
	linum = (linum%tilesizx[globalpicnum])*tilesizy[globalpicnum];
	if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
	setgotpic(globalpicnum);
	bufplc = waloff[globalpicnum]+linum;

	if (option[0] == 0)
		p = mul80(y1v)+((x+viewoffset)>>2)+0xa0000+(page<<14);
	else if (option[0] != 7)
		p = mul320(y1v)+x+frameplace+viewoffset;
	else
		p = mul640(y1v)+x+frameplace+viewoffset;

	if (translucloaded == 0)
		mvlineasm1(vinc,palookupoffs,(((long)(y2v-y1v))<<8)+globalshiftval,vplc,bufplc,p);
	else
		tvlineasm1(vinc,palookupoffs,(((long)(y2v-y1v))<<8)+globalshiftval,vplc,bufplc,p);
}

initengine()
{
	long i;

	loadtables();

	if (option[0] == 7)
		setview(0L,0L,639L,479L);
	else
		setview(0L,0L,319L,199L);

	pskyoff[0] = 0; pskybits = 0;

	parallaxtype = 2; parallaxyoffs = 0L;
	paletteloaded = 0;
	searchx = halfxdimen; searchy = (YDIM>>1); searchit = 0; searchstat = -1;
	scaredfallz = (64<<8);

	for(i=0;i<1024;i++)
		if (i != 511)
			horizlookup[i] = 65536 / (i-511);

	for(i=0;i<MAXPALOOKUPS;i++)
		palookup[i] = NULL;

	clearbuf((long)(&waloff[0]),(long)MAXTILES,0xffffffff);

	clearbuf((long)(&show2dsprite[0]),(long)((MAXSPRITES+3)>>5),0L);
	clearbuf((long)(&show2dwall[0]),(long)((MAXWALLS+3)>>5),0L);
	automapping = 0;

	if (editstatus == 1)
	{
		if (option[4] > 0) option[4] = 0;
		if (option[3] == 0) option[3] = 1;
	}
	pointhighlight = -1;
	linehighlight = -1;
	highlightcnt = 0;

	moustat = 0;  //Must initialize mouse stuff before timer!  But why?
	if (option[3] > 0)
	{
		switch(option[3])
		{
			case 1:
			case 2:
				if (setupmouse() != 0)
					moustat = 1;
				break;
		}
	}

	for(i=0;i<=MAXYDIM+1;i++)
		lastx[i] = -1L;
	//oldzerohandler = _dos_getvect(0);
	//_disable(); _dos_setvect(0, zerohandler); _enable();
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();

	resettiming();
	capturecount = 0;
	visibility = 13;
	clearbuf((long)(&keystatus[0]),64L,0L);
	page = 0;

	loadpalette();

	//videocard = getcard();

	switch(option[0])
	{
		case 0:
			frameplace = 0xa0000;
			setvlinebpl(80);
			page = 0;
			break;
		case 1:
			screen = (char *)malloc(65536);
			frameplace = FP_OFF(screen);
			clearbuf(frameplace,16000L,0L);
			break;
		case 2:
			frameplace = 0xa0000;
			page = 0; koutp(0x3cd,page+(page<<4));
			break;
		case 3:
			frameplace = 0xa0000;
			page = 0; koutp(0x3ce,0x9); koutp(0x3cf,page<<4);
			break;
		case 4:
			frameplace = 0xa0000;
			page = 0; koutp(0x3d4,0x35); koutp(0x3d5,page);
			break;
		case 5:
			screen = (char *)malloc(65536);
			frameplace = FP_OFF(screen);
			ooption0 = option[0]; option[0] = 1;
			stereofps = 1;
			stereowidth = 32768;
			stereopixelwidth = 32;
			break;
		case 6:
			screen = (char *)malloc(131072);
			frameplace = FP_OFF(screen);
			ooption0 = option[0]; option[0] = 1;
			stereofps = 1;
			stereowidth = 32768;
			stereopixelwidth = 32;
			break;
		case 7:
			screen = (char *)malloc(640*480);
			frameplace = FP_OFF(screen);
			clearbuf(frameplace,(640*480)>>2,0L);
			setvlinebpl(640);
			break;
	}
}

uninitengine()
{
	char *ptr;
	long i;

	_dos_setvect(0x9, oldkeyhandler);
	ptr = (char *)0x417;              //Turn off shift key status
	*ptr &= ~0x0f;                    //so the other keyboard handler
	ptr++;                            //doesn't think they're still stuck
	*ptr &= ~0x03;                    //down when you entered the engine

	//_disable(); _dos_setvect(0, oldzerohandler); _enable();
	koutp(0x43,54); koutp(0x40,255); koutp(0x40,255);

	if (artfil != -1) close(artfil);

	if (transluc != NULL)
	{
	   free(transluc);         //Free buffers
	}
	if (pic != NULL)
	{
	   free(pic);
	}
	if (screen != NULL)
	{
	   free(screen);
	}
	for(i=0;i<MAXPALOOKUPS;i++)
		if (palookup[i] != NULL)
		{
		 	free(palookup[i]);
		}
}

nextpage()
{
	long totbytes, i;

	switch(qsetmode)
	{
		case 200:
			if (stereofps != 0)
			{
				if (ooption0 == 5)
				{
					stereoblit(stereopage,&screen[0]);
					stereopage ^= 1;
				}
				else
				{
					if (stereopage == 0)
						stereoredblueblit(&screen[0],&screen[65536]);
					stereopage ^= 1;
				}
			}
			else
			{
				switch(option[0])
				{
					case 0:
						koutp(0x3d4,0xc); koutp(0x3d5,page<<6);
						page = ((page+1)&3);
						break;
					case 1:
						copybuf(frameplace,0xa0000,16000);
						break;
					case 2:
						koutp(0x3d4,0xc); koutp(0x3d5,page<<6);
						page = ((page+1)&3);
						koutp(0x3cd,page+(page<<4));
						break;
					case 3:
						koutp(0x3d4,0xc); koutp(0x3d5,page<<6);
						page = ((page+1)&3);
						koutp(0x3ce,0x9); koutp(0x3cf,page<<4);
						break;
					case 4:
						koutp(0x3d4,0xc); koutp(0x3d5,page<<6);
						page = ((page+1)&3);
						koutp(0x3d4,0x35); koutp(0x3d5,page);

						if (keystatus[0x29] > 0)
						{
							keystatus[0x29] = 2;
							page = 0;
							koutp(0x3d4,0xc); koutp(0x3d5,0);
							koutp(0x3d4,0x35); koutp(0x3d5,0);
							clearbuf(0xa0000,mul80(startdmost[0]-startumost[0]),0L);
						}

						break;
					case 7:
						totbytes = (640*480);
						for(i=0;i<totbytes;i+=65536)
						{

							vesasetpage(i>>16);
							if (i+65536 <= totbytes)
								copybuf(frameplace+i,0xa0000,16384);
							else
								copybuf(frameplace+i,0xa0000,(totbytes-i)>>2);
						}
						break;
				}
			}
			break;

		case 350:
			koutp(0x3d4,0xc), koutp(0x3d5,pageoffset>>11);
			limitrate();
			pageoffset = 225280-pageoffset; //225280 is 352(multiple of 16)*640
			break;

		case 480:
			koutp(0x3d4,0xc), koutp(0x3d5,pageoffset>>11);
			limitrate();
			pageoffset = 399360-pageoffset;
			break;
	}


	numframes++;
}

initcache()
{
	long i;

	for(i=0;i<MAXTILES;i++)
	{
		// don't free permanent tiles
		if (waloff[i] < FP_OFF(pic)+cachesize)
			waloff[i] = -1;
	}

	cachelistplc = 0;
	cachelistend = 0;
	cacheplc = 0;
}

allocache (short tilenume)
{
	if (cacheplc+walsiz[tilenume] > cachesize)
	{
		while ((cachelistoffs[cachelistplc] >= cacheplc) && (cachelistplc != cachelistend))
		{
			waloff[cachelist[cachelistplc]] = -1;
			cachelistplc = ((cachelistplc+1)&(MAXTILES-1));
		}
		while ((cachelistoffs[cachelistplc] < walsiz[tilenume]) && (cachelistplc != cachelistend))
		{
			waloff[cachelist[cachelistplc]] = -1;
			cachelistplc = ((cachelistplc+1)&(MAXTILES-1));
		}

		waloff[tilenume] = 0;
		cacheplc = walsiz[tilenume];
	}
	else
	{
		while ((cachelistoffs[cachelistplc] < cacheplc+walsiz[tilenume]) && (cachelistoffs[cachelistplc] >= cacheplc) && (cachelistplc != cachelistend))
		{
			waloff[cachelist[cachelistplc]] = -1;
			cachelistplc = ((cachelistplc+1)&(MAXTILES-1));
		}

		waloff[tilenume] = cacheplc;
		cacheplc += walsiz[tilenume];
	}

	cachelist[cachelistend] = tilenume;
	cachelistoffs[cachelistend] = waloff[tilenume];
	cachelistend = ((cachelistend+1)&(MAXTILES-1));

	if (waloff[tilenume] != -1)
		waloff[tilenume] += FP_OFF(pic);
}

allocatepermanenttile(short tilenume, long xsiz, long ysiz)
{
	long i, j, x, y, totsiz;

	if ((xsiz <= 0) || (ysiz <= 0) || ((tilenume & ~(MAXTILES-1)) != 0))
		return(-1);

	totsiz = xsiz*ysiz;
	if (cachesize-totsiz < 262144)
		return(-1);

	cachesize -= totsiz;
	waloff[tilenume] = FP_OFF(pic)+cachesize;
	tilesizx[tilenume] = xsiz;
	tilesizy[tilenume] = ysiz;
	walsiz[tilenume] = totsiz;
	picanm[tilenume] = 0;

	j = 15;
	while ((j > 0) && ((1<<j) > xsiz))
		j--;
	picsiz[tilenume] = ((char)j);
	j = 15;
	while ((j > 0) && ((1<<j) > ysiz))
		j--;
	picsiz[tilenume] += ((char)(j<<4));

	initcache();
	return(waloff[tilenume]);
}

loadtile(short tilenume)
{
	long i;
	char *ptr;

	if ((walsiz[tilenume] <= 0) || (waloff[tilenume] >= 0)) return;

	i = tilefilenum[tilenume];
	if (i != artfilnum)
	{
		if (artfil != -1) close(artfil);
		artfilename[7] = (i%10)+48;
		artfilename[6] = ((i/10)%10)+48;
		artfilnum = i;
		artfilename[5] = ((i/100)%10)+48;
		artfil = open(artfilename,O_BINARY|O_RDONLY,S_IREAD);
	}
	allocache(tilenume);
	lseek(artfil,tilefileoffs[tilenume],SEEK_SET);
	ptr = (char *)waloff[tilenume];
	read(artfil,ptr,walsiz[tilenume]);
}

// The ART loader uses the library entry even with the capture copy intrinsic.
extern char *artstrcpy(char *, const char *);
#pragma aux artstrcpy "strcpy_"

loadpics(char *filename)
{
	long offscount, siz;
	short int fil, i, j, k;

	for(i=0;i<MAXTILES;i++)
		walsiz[i] = 0;
	initcache();

	artstrcpy(artfilename,filename);

	for(i=0;i<MAXTILES;i++)
	{
		tilesizx[i] = 0;
		tilesizy[i] = 0;
		picanm[i] = 0L;
	}

	artsize = 0L;

	numtilefiles = 0;
	do
	{
		k = numtilefiles;

		artfilename[7] = (k%10)+48;
		artfilename[6] = ((k/10)%10)+48;
		artfilename[5] = ((k/100)%10)+48;
		if ((fil = open(artfilename,O_BINARY|O_RDONLY,S_IREAD)) != -1)
		{
			read(fil,&artversion,4);
			if (artversion != 1) return(-1);
			read(fil,&numtiles,4);
			read(fil,&localtilestart[k],4);
			read(fil,&localtileend[k],4);
			read(fil,&tilesizx[localtilestart[k]],(localtileend[k]-localtilestart[k]+1)<<1);
			read(fil,&tilesizy[localtilestart[k]],(localtileend[k]-localtilestart[k]+1)<<1);
			read(fil,&picanm[localtilestart[k]],(localtileend[k]-localtilestart[k]+1)<<2);

			offscount = 4+4+4+4+((localtileend[k]-localtilestart[k]+1)<<3);
			for(i=localtilestart[k];i<=localtileend[k];i++)
			{
				tilefilenum[i] = k;
				tilefileoffs[i] = offscount;
				walsiz[i] = (long)(tilesizx[i]*tilesizy[i]);
				offscount += walsiz[i];
				artsize += walsiz[i];
			}
			close(fil);

			numtilefiles++;
		}
	}
	while (k != numtilefiles);

	clearbuf((long)(&gotpic[0]),(long)(MAXTILES>>5),0L);

	cachesize = artsize;
	while ((pic = (char *)malloc(cachesize)) == NULL)
		cachesize -= 65536L;

	for(i=0;i<MAXTILES;i++)
	{
		j = 15;
		while ((j > 0) && ((1<<j) > tilesizx[i])) j--;
		picsiz[i] = ((char)j);
		j = 15;
		while ((j > 0) && ((1<<j) > tilesizy[i])) j--;
		picsiz[i] += ((char)(j<<4));
	}

	artfil = -1;
	artfilnum = -1;

	return(0);
}

showengineinfo()
{
	long templong;

	if (totalclock != 0)
	{
		templong = (numframes*12000L)/totalclock;
		printf("%d.%1d%1d frames per second\n",(short)(templong/100),(short)((templong/10)%10),(short)(templong%10));
	}
}

resettiming()
{
	numframes = 0L;
	totalclock = 0L;
	totalclocklock = 0L;
}

clipinsidebox(long x, long y, short wallnum, long walldist)
{
	short wallpoint2;
	long x1, y1, x2, y2, templong1, templong2, templong3, templong4, boxsiz;

	wallpoint2 = wall[wallnum].point2;
	boxsiz = (walldist<<1);

	x1 = wall[wallnum].x-x+walldist;
	x2 = wall[wallpoint2].x-x+walldist;
	if ((x1 < 0) && (x2 < 0)) return(0);
	if ((x1 >= boxsiz) && (x2 >= boxsiz)) return(0);

	y1 = wall[wallnum].y-y+walldist;
	y2 = wall[wallpoint2].y-y+walldist;
	if ((y1 < 0) && (y2 < 0)) return(0);
	if ((y1 >= boxsiz) && (y2 >= boxsiz)) return(0);

	templong1 = (x2-x1)*(-y1)-(-x1)*(y2-y1);
	templong2 = (x2-x1)*(boxsiz-y1)-(-x1)*(y2-y1);
	if ((templong1<=0) != (templong2<=0)) return(1);
	templong3 = (x2-x1)*(-y1)-(boxsiz-x1)*(y2-y1);
	if ((templong2<=0) != (templong3<=0)) return(1);
	templong4 = (x2-x1)*(boxsiz-y1)-(boxsiz-x1)*(y2-y1);
	if ((templong3<=0) != (templong4<=0)) return(1);

	return(0);
}

clipinsideboxline(long x, long y, long x1, long y1, long x2, long y2, long walldist)
{
	long templong1, templong2, templong3, templong4, boxsiz;

	boxsiz = (walldist<<1);

	x1 += (walldist-x); x2 += (walldist-x);
	if ((x1 < 0) && (x2 < 0)) return(0);
	if ((x1 >= boxsiz) && (x2 >= boxsiz)) return(0);

	y1 += (walldist-y); y2 += (walldist-y);
	if ((y1 < 0) && (y2 < 0)) return(0);
	if ((y1 >= boxsiz) && (y2 >= boxsiz)) return(0);

	templong1 = (x2-x1)*(-y1)-(-x1)*(y2-y1);
	templong2 = (x2-x1)*(boxsiz-y1)-(-x1)*(y2-y1);
	if ((templong1<=0) != (templong2<=0)) return(1);
	templong3 = (x2-x1)*(-y1)-(boxsiz-x1)*(y2-y1);
	if ((templong2<=0) != (templong3<=0)) return(1);
	templong4 = (x2-x1)*(boxsiz-y1)-(boxsiz-x1)*(y2-y1);
	if ((templong3<=0) != (templong4<=0)) return(1);

	return(0);
}

getpixel16(long p)
{
	long mask, dat;

	mask = (1<<(p&7^7));

	if (qsetmode == 480)
	{
		if (p < 640*336)
			p += pageoffset;
		else
			p -= 640*336;
	}
	else
		p += pageoffset;

	p >>= 3;

	koutp(0x3ce,0x4);
	koutp(0x3cf,0); dat = ((readpixel(p+0xa0000)&mask)>0);
	koutp(0x3cf,1); dat += (((readpixel(p+0xa0000)&mask)>0)<<1);
	koutp(0x3cf,2); dat += (((readpixel(p+0xa0000)&mask)>0)<<2);
	koutp(0x3cf,3); dat += (((readpixel(p+0xa0000)&mask)>0)<<3);
	return(dat);
}

#pragma intrinsic(strcpy)
screencapture()
{
	char *ptr, filename[13];
	long ncol;
	long fil, i, bufplc, p, col, count, leng, numbytes, xres, inverseit;

	inverseit = (keystatus[0x2a]|keystatus[0x36]);
	strcpy(filename,"captxxxx.pcx");

	count = capturecount;
	filename[4] = ((count/1000)%10)+48;
	filename[5] = ((count/100)%10)+48;
	filename[6] = ((count/10)%10)+48;
	filename[7] = (count%10)+48;
	capturecount++;
	if ((fil=open(filename,O_BINARY|O_CREAT|O_TRUNC|O_WRONLY,S_IWRITE))==-1)
	{
		capturecount--;
		return(-1);
	}

	if (qsetmode == 200)
	{
		if (option[0] != 7)
		{
			pcxheader[8] = ((320-1)&255); pcxheader[9] = (((320-1)>>8)&255);
			pcxheader[10] = ((200-1)&255); pcxheader[11] = (((200-1)>>8)&255);
			pcxheader[12] = (320&255); pcxheader[13] = ((320>>8)&255);
			pcxheader[14] = (200&255); pcxheader[15] = ((200>>8)&255);
			pcxheader[66] = (320&255); pcxheader[67] = ((320>>8)&255);
		}
		else
		{
			pcxheader[8] = ((640-1)&255); pcxheader[9] = (((640-1)>>8)&255);
			pcxheader[10] = ((480-1)&255); pcxheader[11] = (((480-1)>>8)&255);
			pcxheader[12] = (640&255); pcxheader[13] = ((640>>8)&255);
			pcxheader[14] = (480&255); pcxheader[15] = ((480>>8)&255);
			pcxheader[66] = (640&255); pcxheader[67] = ((640>>8)&255);
		}
	}
	else
	{
		pcxheader[8] = ((640-1)&255); pcxheader[9] = (((640-1)>>8)&255);
		pcxheader[10] = ((qsetmode-1)&255); pcxheader[11] = (((qsetmode-1)>>8)&255);
		pcxheader[12] = (640&255); pcxheader[13] = ((640>>8)&255);
		pcxheader[14] = (qsetmode&255); pcxheader[15] = ((qsetmode>>8)&255);
		pcxheader[66] = (640&255); pcxheader[67] = ((640>>8)&255);
	}

	write(fil,&pcxheader[0],128);

	if (qsetmode == 200)
	{
		if (option[0] == 0)
			ptr = (char *)chainoffset;
		else
			ptr = (char *)frameplace;

		if (option[0] != 7)
			{ numbytes = 64000; xres = 320; }
		else
			{ numbytes = 640*480; xres = 640; }
	}
	else
	{
		numbytes = 640 * qsetmode;
		xres = 640;
	}

	bufplc = 0;
	p = 0;
	while (p < numbytes)
	{
		koutp(97,kinp(97)|3);

		if (qsetmode == 200)
		{
			if (option[0] == 0)
			{
				koutp(0x3ce,4); koutp(0x3cf,p&3); col = *ptr;
				p++; if ((p&3) == 0) ptr++;
			}
			else
				{ col = *ptr; p++; ptr++; }
		}
		else
		{
			col = getpixel16(p);
			p++;
			if ((inverseit == 1) && (((col&7) == 0) || ((col&7) == 7))) col ^= 15;
		}

		leng = 1;

		if (qsetmode == 200)
		{
			if (option[0] == 0)
				{ koutp(0x3ce,4); koutp(0x3cf,p&3); ncol = *ptr; }
			else
				ncol = *ptr;
		}
		else
		{
			ncol = getpixel16(p);
			if ((inverseit == 1) && (((ncol&7) == 0) || ((ncol&7) == 7))) ncol ^= 15;
		}

		while ((ncol == col) && (p < numbytes) && (leng < 63) && ((p%xres) != 0))
		{
			leng++;

			if (qsetmode == 200)
			{
				if (option[0] == 0)
				{
					p++; if ((p&3) == 0) ptr++;
					koutp(0x3ce,4); koutp(0x3cf,p&3); ncol = *ptr;
				}
				else
				{
					p++; ptr++;
					ncol = *ptr;
				}
			}
			else
			{
				p++;
				ncol = getpixel16(p);
				if ((inverseit == 1) && (((ncol&7) == 0) || ((ncol&7) == 7))) ncol ^= 15;
			}
		}

		koutp(97,kinp(97)&252);

		if ((leng > 1) || (col >= 0xc0))
		{
			enginetempbuf[bufplc++] = (leng|0xc0);
			if (bufplc == 256)
			{
				if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
					{ close(fil); capturecount--; return(-1); }
				bufplc = 0;
			}
		}
		enginetempbuf[bufplc++] = col;
		if (bufplc == 256)
		{
			if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
				{ close(fil); capturecount--; return(-1); }
			bufplc = 0;
		}
	}

	enginetempbuf[bufplc++] = 0xc;
	if (bufplc == 256)
	{
		if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
			{ close(fil); capturecount--; return(-1); }
		bufplc = 0;
	}

	if (qsetmode == 200)
	{
		koutp(0x3c7,0);
		for(i=0;i<768;i++)
		{
			enginetempbuf[bufplc++] = (kinp(0x3c9)<<2);
			if (bufplc == 256)
			{
				if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
					{ close(fil); capturecount--; return(-1); }
				bufplc = 0;
			}
		}
	}
	else
	{
		for(i=0;i<768;i++)
		{
			unsigned char entry;
			if (i < 48)
			{
				entry = vgapal16[i]<<2;
				bufplc++;
			}
			else
			{
				entry = 0;
				bufplc++;
			}
			enginetempbuf[bufplc-1] = entry;
			if (bufplc == 256)
			{
				if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
					{ close(fil); capturecount--; return(-1); }
				bufplc = 0;
			}
		}

	}

	if (bufplc > 0)
		if (write(fil,&enginetempbuf[0],bufplc) < bufplc)
			{ close(fil); capturecount--; return(-1); }

	close(fil);
	return(0);
}

inside(long x, long y, short sectnum)
{
	long x1, y1, x2, y2, templong;
	short i, startwall, endwall, cnt;

	if ((sectnum < 0) || (sectnum >= numsectors))
		return(-1);
	cnt = 0;
	startwall = sector[sectnum].wallptr;
	endwall = startwall + sector[sectnum].wallnum - 1;
	for(i=startwall;i<=endwall;i++)
	{
		x1 = wall[i].x; x2 = wall[wall[i].point2].x;
		if ((x1 >= x) || (x2 >= x))
		{
			y1 = wall[i].y; y2 = wall[wall[i].point2].y;
			if (y1 > y2)
			{
				templong = x1, x1 = x2, x2 = templong;
				templong = y1, y1 = y2, y2 = templong;
			}
			if ((y1 <= y) && (y2 > y))
				if (x1*(y-y2)+x2*(y1-y) <= x*(y1-y2))
					cnt ^= 1;
		}
	}
	return(cnt);
}

getangle(long xvect, long yvect)
{
	if ((xvect|yvect) == 0) return(0);
	if (xvect == 0) return(512+((yvect<0)<<10));
	if (yvect == 0) return(((xvect<0)<<10));
	if (xvect == yvect) return(256+((xvect<0)<<10));
	if (xvect == -yvect) return(768+((xvect>0)<<10));
	if (klabs(xvect) > klabs(yvect))
		return(((radarang[((yvect*(XDIM>>1))/xvect)+(XDIM>>1)]+((xvect<0)<<10))+2048)&2047);
	return(((radarang[(XDIM>>1)-((xvect*(XDIM>>1))/yvect)]+512+((yvect<0)<<10))+2048)&2047);
}

ksqrt(long num)
{
	return(ksqrtasm(num));
}

calcksqrt(long num)
{
	long root, temp;

	if (num < 0)
		return(-1);
	root = 128;
	do
	{
		temp = root;
		root = ((root+(num/root))>>1);
	}
	while (klabs(temp-root) > 1);
	return(root);
}

copytilepiece(long tilenume1, long sx1, long sy1, long xsiz, long ysiz,
				  long tilenume2, long sx2, long sy2)
{
	char *ptr1, *ptr2, dat;
	long xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

	xsiz1 = tilesizx[tilenume1]; ysiz1 = tilesizy[tilenume1];
	xsiz2 = tilesizx[tilenume2]; ysiz2 = tilesizy[tilenume2];
	if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
	{
		if (waloff[tilenume1] == -1) loadtile(tilenume1);
		if (waloff[tilenume2] == -1) loadtile(tilenume2);

		x1 = sx1;
		for(i=0;i<xsiz;i++)
		{
			y1 = sy1;
			for(j=0;j<ysiz;j++)
			{
				x2 = sx2+i;
				y2 = sy2+j;
				if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
				{
					ptr1 = (char *)(waloff[tilenume1] + x1*ysiz1 + y1);
					ptr2 = (char *)(waloff[tilenume2] + x2*ysiz2 + y2);
					dat = *ptr1;
					if (dat != 255)
						*ptr2 = *ptr1;
				}

				y1++; if (y1 >= ysiz1) y1 = 0;
			}
			x1++; if (x1 >= xsiz1) x1 = 0;
		}
	}
}

printext(long x, long y, char buffer[42], short tilenum, char invisiblecol)
{
	long p, startp, bufplc;
	char *ptr, ch, plane;
	unsigned char dat;
	short i, dax, day, theyinc;
	long dapage, danumpages;

	if (waloff[tilenum] == -1) loadtile(tilenum);
	setgotpic(tilenum);
	bufplc = waloff[tilenum];

	danumpages = 4;
	if (option[0] == 1)
		danumpages = 1;

	for (dapage=0;dapage<danumpages;dapage++)
	{
		if (option[0] == 0)
		{
			startp = mul80(y)+(x>>2)+0xa0000+(((long)dapage)<<14);
			plane = (x&3);
			theyinc = 80;
		}
		else
		{
			if (option[0] != 7)
			{
				startp = mul320(y) + x+frameplace;
				theyinc = 320;
			}
			else
			{
				startp = mul640(y) + x+frameplace;
				theyinc = 640;
			}
			if (option[0] == 2) koutp(0x3cd,dapage+(dapage<<4));
			if (option[0] == 3) koutp(0x3ce,0x9), koutp(0x3cf,dapage<<4);
			if (option[0] == 4) koutp(0x3d4,0x35), koutp(0x3d5,dapage);
		}

		i = 0;
		while (buffer[i] != 0)
		{
			ch = buffer[i];
			ptr = (char *)(bufplc+(((long)ch&15)<<9)+(((long)ch>>4)<<3));
			for(dax=0;dax<8;dax++)
			{
				p = startp;
				if (option[0] == 0) koutpw(0x3c4,2+(256<<(plane&3)));

				if (invisiblecol == 255)
				{
					for(day=0;day<8;day++)
					{
						dat = *ptr;
						if (dat != 255) drawpixel(p,(long)dat);
						p += theyinc;
						ptr++;
					}
				}
				else
				{
					for(day=0;day<8;day++)
					{
						dat = *ptr;
						if (dat == 255) dat = invisiblecol;
						drawpixel(p,dat);
						p += theyinc;
						ptr++;
					}
				}
				if (option[0] == 0)
				{
					plane++;
					if ((plane&3) == 0) startp++;
				}
				else
					startp++;

				ptr += (64-8);
			}
			i++;
		}
	}

	if (option[0] == 2) koutp(0x3cd,page+(page<<4));
	if (option[0] == 3) koutp(0x3ce,0x9), koutp(0x3cf,page<<4);
	if (option[0] == 4) koutp(0x3d4,0x35), koutp(0x3d5,page);
}

void drawsprite(long, long, short, short, char);

drawmasks()
{
	char tempchar;
	short tempshort;
	long i, j, k, gap, temp, templong;

	gap = (spritesortcnt>>1);  //Sort sprite list
	do
	{
		for(i=0;i<spritesortcnt-gap;i++)
		{
			temp = i;
			while ((spritey[temp] > spritey[temp+gap]) && (temp >= 0))
			{
				templong = spritex[temp]; spritex[temp] = spritex[temp+gap]; spritex[temp+gap] = templong;
				templong = spritey[temp]; spritey[temp] = spritey[temp+gap]; spritey[temp+gap] = templong;
				tempshort = spritepicnum[temp]; spritepicnum[temp] = spritepicnum[temp+gap]; spritepicnum[temp+gap] = tempshort;
				tempchar = spritepal[temp]; spritepal[temp] = spritepal[temp+gap]; spritepal[temp+gap] = tempchar;
				tempshort = thesprite[temp]; thesprite[temp] = thesprite[temp+gap]; thesprite[temp+gap] = tempshort;
				temp -= gap;
			}
		}
		gap >>= 1;
	}
	while (gap > 0);

	while ((spritesortcnt > 0) && (maskwallcnt > 0))  //While BOTH > 0
	{
		j = maskwall[maskwallcnt-1];
		if (spritewallfront(thesprite[spritesortcnt-1],thewall[j]) == 0)
		{
			spritesortcnt--;
			drawsprite(spritex[spritesortcnt],spritey[spritesortcnt],spritepicnum[spritesortcnt],thesprite[spritesortcnt],spritepal[spritesortcnt]);
		}
		else
		{
				//Check to see if any sprites behind the masked wall...
			k = -1;
			gap = 0;
			for(i=spritesortcnt-2;i>=0;i--)
				if (((spritex[i]>>8) >= xb1[j]) && ((spritex[i]>>8) <= xb2[j]))
					if (spritewallfront(thesprite[i],thewall[j]) == 0)
					{
						drawsprite(spritex[i],spritey[i],spritepicnum[i],thesprite[i],spritepal[i]);
						thesprite[i] = -1;
						k = i;
						gap++;
					}
			if (k >= 0)       //remove holes in sprite list
			{
				for(i=k;i<spritesortcnt;i++)
					if (thesprite[i] >= 0)
					{
						if (i > k)
						{
							spritex[k] = spritex[i];
							spritey[k] = spritey[i];
							spritepicnum[k] = spritepicnum[i];
							spritepal[k] = spritepal[i];
							thesprite[k] = thesprite[i];
						}
						k++;
					}
				spritesortcnt -= gap;
			}

				//finally safe to draw the masked wall
			maskwallcnt--;
			drawmaskwall(maskwallcnt);
		}
	}
	while (spritesortcnt > 0)                //Finish them off
	{
		spritesortcnt--;
		drawsprite(spritex[spritesortcnt],spritey[spritesortcnt],spritepicnum[spritesortcnt],thesprite[spritesortcnt],spritepal[spritesortcnt]);
	}
	while (maskwallcnt > 0)                  //Finish them off
	{
		maskwallcnt--;
		drawmaskwall(maskwallcnt);
	}
}

drawmaskwall(short damaskwallcnt)
{
	long z, j, k, x, i, wallnum, sectnum, nextsectnum;
	long z2, z1, llin, rlin, top, topinc, bot, botinc;
	long hplc, hinc, dax, lx, rx;
	long xrepeat;

	z = maskwall[damaskwallcnt];
	wallnum = thewall[z];
	sectnum = thesector[z];
	globalpal = (long)sector[sectnum].floorpal;
	nextsectnum = wall[wallnum].nextsector1;
	z1 = sector[nextsectnum].ceilingz;
	if (sector[sectnum].ceilingz > z1) z1 = sector[sectnum].ceilingz;
	z2 = sector[nextsectnum].floorz;
	if (sector[sectnum].floorz < z2) z2 = sector[sectnum].floorz;
	if (z1 >= z2) return;

	xrepeat = wall[wallnum].xrepeat;
	topinc = xrepeat * -ry1[z];
	top = mulscale(rx1[z],xdimen,2)-mulscale(xb1[z]-(xdimen>>1),ry1[z],1);
	top *= xrepeat;
	botinc = ((ry2[z]-ry1[z])>>1);
	bot = mulscale(rx1[z]-rx2[z],xdimen,3) + mulscale(xb1[z]-halfxdimen,botinc,1);

	for(x=xb1[z];x<=xb2[z]+1;x+=2)
	{
		if (bot != 0) lwall[x] = divscale(top,bot,8);
		top += topinc, bot += botinc;
	}
	for(x=xb1[z]+1;x<=xb2[z];x+=2)
	  lwall[x] = ((lwall[x-1]+lwall[x+1])>>1);
	if (lwall[xb1[z]] < 0) lwall[xb1[z]] = 0;
	if (wall[wallnum].xrepeat > 0)
	{
		if (((long)wall[wallnum].xrepeat<<9) <= lwall[xb2[z]])
			lwall[xb2[z]] = ((long)wall[wallnum].xrepeat<<9)-1;
	}

	if ((wall[wallnum].cstat&8) > 0)
	{
		j = ((long)wall[wallnum].xrepeat<<9)-1;
		for(x=xb1[z];x<=xb2[z];x++)
			lwall[x] = j-lwall[x];
	}

	hplc = (65536*2048)/yb1[z];
	hinc = (65536*2048)/yb2[z];
	hinc = (hinc-hplc)/(xb2[z]-xb1[z]+1);
	dax = hplc;
	if (xdimen != XDIM)
	{
		dax = mulscale(hplc,xdimenscale,16);
		hinc = mulscale(hinc,xdimenscale,16);
	}

	wallmost(uwall,z,z1-globalposz);
	wallmost(dwall,z,z2-globalposz);
	qinterpolate((long)(&swall[xb1[z]]),xb2[z]-xb1[z],dax,hinc);

	globalorientation = (long)wall[wallnum].cstat;
	globalpicnum = wall[wallnum].overpicnum;
	if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
	globalxpanning = (long)wall[wallnum].xpanning;
	globalypanning = (long)wall[wallnum].ypanning;
	if ((picanm[globalpicnum]&192) != 0)
		globalpicnum += animateoffs(globalpicnum);
	globalshade = (long)wall[wallnum].shade;
	globalshiftval = (picsiz[globalpicnum]>>4);
	if (tilesizy[globalpicnum] != (1<<globalshiftval)) globalshiftval++;
	globalshiftval = 32-globalshiftval;
	globalyscale = (wall[wallnum].yrepeat<<(globalshiftval-19));
	if ((globalorientation&4) == 0)
		globalzd = (((globalposz-z1)*globalyscale)<<8);
	else
		globalzd = (((globalposz-z2)*globalyscale)<<8);
	globalzd += (globalypanning<<24);

	for(x=xb1[z];x<=xb2[z];x++)
	{
		umost[x] = startumost[x];
		dmost[x] = startdmost[x];
	}
	for(i=0;i<smostwallcnt;i++)
	{
		j = (smostwall[i]&16383);
		lx = xb1[j];
		rx = xb2[j];

		if ((lx <= xb2[z]) && (rx >= xb1[z]))
			if (wallfront(j,z) == j)
			{
				if (lx < xb1[z]) lx = xb1[z];
				if (rx > xb2[z]) rx = xb2[z];

				switch(smostwall[i]&0xc000)
				{
					case 0:
						for(x=lx;x<=rx;x++)
						{
							umost[x] = 1;
							dmost[x] = 0;
						}
						break;
					case 16384:
						k = smoststart[i] - xb1[j];
						for(x=lx;x<=rx;x++)
							if (smost[k+x] > umost[x]) umost[x] = smost[k+x];
						break;
					case 32768:
						k = smoststart[i] - xb1[j];
						for(x=lx;x<=rx;x++)
							if (smost[k+x] < dmost[x]) dmost[x] = smost[k+x];
						break;
				}
			}
	}

	if (searchit >= 1)
		if ((searchx >= xb1[z]) && (searchx <= xb2[z]))
			if ((searchy >= umost[searchx]) && (searchy <= dmost[searchx]))
				if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
				{
					searchsector = sectnum;
					searchwall = wallnum;
					searchstat = 4;  //maskwall
					searchit = 1;
				}

	if ((globalorientation&128) == 0)
		maskwallscan(xb1[z],xb2[z],uwall,dwall,swall,lwall);
	else
		transmaskwallscan(xb1[z],xb2[z]);
}

void drawsprite (long xb, long yp, short tilenum, short spritenum, char dapalnum)
{
	char cstat, seesprite, swapped;
	signed short shade;
	long um, dm, startum, startdm, das, sectnum;
	long siz, xsiz, ysiz, xoff, yoff, xspan, yspan, templong;
	long x1, y1, x2, y2, lx, rx, dalx2, darx2, i, j, k, x, linum, linuminc;
	long yplc, yinc, p, tileoffs, bufplc, palookupoffs;
	long z, z1, z2, xp1, yp1, xp2, yp2;        //Mask-type wall variables...
	long xs, ys, xpos, ypos, xv, yv, top, topinc, bot, botinc, hplc, hinc;
	long templong1;

	long cosang, sinang, dax, day;
	long npoints, npoints2, zz, t, zsgn, zzsgn;
	long lpoint, lmax, rpoint, rmax, dax1, dax2, y;

	if ((picanm[tilenum]&192) != 0)
		tilenum += animateoffs(tilenum);
	if ((tilesizx[tilenum] <= 0) || (tilesizy[tilenum] <= 0) || (spritenum < 0))
		return;

	sectnum = sprite[spritenum].sectnum;
	cstat = sprite[spritenum].cstat;
	if (((cstat&2) > 0) && (translucloaded == 0)) cstat &= ~2;

	xoff = (long)((signed char)((picanm[tilenum]>>8)&255));
	yoff = (long)((signed char)((picanm[tilenum]>>16)&255));
	if ((cstat&4) > 0) xoff = -xoff;
	if ((cstat&8) > 0) yoff = -yoff;

	if (((cstat&48) == 0) || ((cstat&48) == 48))
	{
		if (yp <= (4<<8)) return;

		siz = divscale(131072L,yp,18);
		if (xdimen != XDIM) siz = mulscale(siz,xdimenscale,16);

		xspan = tilesizx[tilenum];
		yspan = tilesizy[tilenum];
		xsiz = mulscale(siz,sprite[spritenum].xrepeat*xspan,14);
		ysiz = mulscale(siz,sprite[spritenum].yrepeat*yspan,14);

		if (((tilesizx[tilenum]>>11) >= xsiz) || (yspan >= (ysiz>>1)))
			return;  //Watch out for divscale overflow

		x1 = xb-(xsiz>>1);
		templong = mulscale(siz,sprite[spritenum].xrepeat*xoff,14);
		if ((cstat&4) == 0)
			x1 -= templong;
		else
			x1 += templong;

		y1 = mulscale(sprite[spritenum].z-globalposz,siz,16);
		y1 -= mulscale(siz,sprite[spritenum].yrepeat*yoff,14);
		templong = (globalhoriz<<8)-ysiz;
		y1 += templong;

		x2 = x1+xsiz;
		y2 = y1+ysiz;
		if ((y1|255) >= (y2|255)) return;

		lx = (x1>>8)+1; if (lx < 0) lx = 0;
		rx = (x2>>8); if (rx >= xdimen) rx = xdimen-1;

		shade = (yp>>(visibility+4))+sprite[spritenum].shade+6;
		if ((sector[sectnum].ceilingstat&1) > 0)
			shade += ((signed short)sector[sectnum].ceilingshade);
		else
			shade += ((signed short)sector[sectnum].floorshade);
		if (shade < 0) shade = 0;
		if (shade >= numpalookups) shade = numpalookups-1;
		palookupoffs = FP_OFF(palookup[dapalnum]) + (((long)shade)<<8);

		if (waloff[tilenum] == -1) loadtile(tilenum);
		setgotpic(tilenum);
		tileoffs = waloff[tilenum];

		yinc = divscale(yspan<<16,ysiz,16);

		if ((sector[sectnum].ceilingstat&1) == 0)
			startum = globalhoriz+mulscale(siz,sector[sectnum].ceilingz-globalposz,24)-1;
		else
			startum = 0;
		if ((sector[sectnum].floorstat&1) == 0)
			startdm = globalhoriz+mulscale(siz,sector[sectnum].floorz-globalposz,24)+1;
		else
			startdm = 0x7fffffff;
		if ((y1>>8) > startum) startum = (y1>>8);
		if ((y2>>8) < startdm) startdm = (y2>>8);

		if (startum < -32768) startum = -32768;
		if (startdm > 32767) startdm = 32767;
		if (startum >= startdm) return;

		if ((cstat&4) == 0)
		{
			linuminc = divscale(xspan,xsiz,20);
			linum = mulscale((lx<<8)-x1,linuminc,8);
		}
		else
		{
			linuminc = -divscale(xspan,xsiz,20);
			linum = mulscale((lx<<8)-x2,linuminc,8);
		}
		if ((cstat&8) > 0)
		{
			yinc = -yinc;
			templong = y1; y1 = y2; y2 = templong;
		}

		seesprite = 0;

		for(x=lx;x<=rx;x++)
		{
			uwall[x] = startumost[x]; if (startum > uwall[x]) uwall[x] = startum;
			dwall[x] = startdmost[x]; if (startdm < dwall[x]) dwall[x] = startdm;
		}
		for(i=0;i<smostwallcnt;i++)
		{
			j = (smostwall[i]&16383);
			dalx2 = xb1[j];
			darx2 = xb2[j];

			if ((dalx2 <= rx) && (darx2 >= lx))
				if ((yp > (yb1[j]<<8)) || (yp > (yb2[j]<<8)))
					if ((spritewallfront((long)spritenum,(long)thewall[j]) == 0) || ((yp > (yb1[j]<<8)) && (yp > (yb2[j]<<8))))
					{
						if (dalx2 < lx) dalx2 = lx;
						if (darx2 > rx) darx2 = rx;

						switch(smostwall[i]&0xc000)
						{
							case 0:
								k = ytop;
								for(x=dalx2;x<=darx2;x++) dwall[x] = k;
								break;
							case 16384:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x] > uwall[x]) uwall[x] = smost[k+x];
								break;
							case 32768:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x] < dwall[x]) dwall[x] = smost[k+x];
								break;
						}
					}
		}

		if (searchit >= 1)
			if ((searchx >= lx) && (searchx <= rx))
				if ((searchy >= uwall[searchx]) && (searchy < dwall[searchx]))
				{
					searchsector = sectnum;
					searchwall = spritenum;
					searchstat = 3;  //sprite
					searchit = 1;
				}

		for(x=lx;x<=rx;x++)
		{
			um = uwall[x];
			dm = dwall[x]-1;

			if (um < dm)
			{
				seesprite = 1;

				bufplc = tileoffs+(linum>>12)*yspan;

				yplc = mulscale(((um+1)<<8)-y1,yinc,8);

				if (option[0] == 0)
				{
					p = mul80(um)+((x+viewoffset)>>2)+chainoffset;
					koutpw(0x3c4,2+(256<<((x+viewoffset)&3)));
					koutpw(0x3ce,4+(((x+viewoffset)&3)<<8));
				}
				else if (option[0] != 7)
					p = mul320(um) + x+frameplace+viewoffset;
				else
					p = mul640(um) + x+frameplace+viewoffset;

				if ((cstat&2) == 0)
					mvlineasm1(yinc,palookupoffs,((long)(dm-um)<<8)+24,yplc,bufplc,p);
				else
					tvlineasm1(yinc,palookupoffs,((long)(dm-um)<<8)+24,yplc,bufplc,p);
			}
			linum += linuminc;
		}

		if (automapping == 1)
			if (seesprite == 1)
				show2dsprite[spritenum>>3] |= (1<<(spritenum&7));

	}
	else if ((cstat&48) == 16)
	{

		xspan = tilesizx[tilenum];
		yspan = tilesizy[tilenum];
		xv = sprite[spritenum].xrepeat*sintable[(sprite[spritenum].ang+2560+1536)&2047];
		yv = sprite[spritenum].xrepeat*sintable[(sprite[spritenum].ang+2048+1536)&2047];
		xs = sprite[spritenum].x - mulscale(xv,(xspan>>1)+xoff,16) - globalposx;
		ys = sprite[spritenum].y - mulscale(yv,(xspan>>1)+xoff,16) - globalposy;
		xp1 = (ys*cosglobalang-xs*singlobalang)>>14;
		yp1 = (xs*cosglobalang+ys*singlobalang)>>14;
		xs += mulscale(xv,xspan,16);
		ys += mulscale(yv,xspan,16);
		xp2 = (ys*cosglobalang-xs*singlobalang)>>14;
		yp2 = (xs*cosglobalang+ys*singlobalang)>>14;

		swapped = 0;
		if (xp1*yp2 > xp2*yp1)     //If wall's NOT facing you
		{
			templong = xp1, xp1 = xp2, xp2 = templong;
			templong = yp1, yp1 = yp2, yp2 = templong;
			swapped = 1;
		}
		if (((cstat&64) > 0) && (swapped == 1)) return;
		if ((yp1 <= 0) && (yp2 <= 0)) return;

		if (xp1 >= -yp1)
		{
			if (xp1 > yp1) return;

			if (yp1 == 0) return;
			xb1[MAXWALLSB-1] = halfxdimen + scale(xp1,halfxdimen,yp1);
			if (xp1 >= 0) xb1[MAXWALLSB-1]++;   //Fix for SIGNED divide
			if (xb1[MAXWALLSB-1] >= xdimen) xb1[MAXWALLSB-1] = xdimen-1;
			yb1[MAXWALLSB-1] = yp1;
		}
		else
		{
			if (xp2 < -yp2) return;
			xb1[MAXWALLSB-1] = 0;
			templong1 = yp1-yp2+xp1-xp2;
			if (templong1 == 0) return;
			yb1[MAXWALLSB-1] = yp1 + scale(yp2-yp1,xp1+yp1,templong1);
		}
		if (xp2 <= yp2)
		{
			if (xp2 < -yp2) return;

			if (yp2 == 0) return;
			xb2[MAXWALLSB-1] = halfxdimen + scale(xp2,halfxdimen,yp2) - 1;
			if (xp2 >= 0) xb2[MAXWALLSB-1]++;   //Fix for SIGNED divide
			if (xb2[MAXWALLSB-1] >= xdimen) xb2[MAXWALLSB-1] = xdimen-1;
			yb2[MAXWALLSB-1] = yp2;
		}
		else
		{
			if (xp1 > yp1) return;

			xb2[MAXWALLSB-1] = xdimen-1;
			templong1 = xp2-xp1+yp1-yp2;
			if (templong1 == 0) return;
			yb2[MAXWALLSB-1] = yp1 + scale(yp2-yp1,yp1-xp1,templong1);
		}

		if ((yb1[MAXWALLSB-1] <= 0) || (yb2[MAXWALLSB-1] <= 0) || (xb1[MAXWALLSB-1] > xb2[MAXWALLSB-1]))
			return;

		topinc = -((yp1*xspan)>>3);
		top = (((mulscale(xp1,xdimen,2) - mulscale(xb1[MAXWALLSB-1]-halfxdimen,yp1,1))*xspan)>>3);
		botinc = ((yp2-yp1)>>1);
		bot = mulscale(xp1-xp2,xdimen,3) + mulscale(xb1[MAXWALLSB-1]-halfxdimen,botinc,1);
		for(x=xb1[MAXWALLSB-1];x<=xb2[MAXWALLSB-1]+1;x+=2)
		{
			if (bot != 0) lwall[x] = divscale(top,bot,8);
			top += topinc, bot += botinc;
		}
		for(x=xb1[MAXWALLSB-1]+1;x<=xb2[MAXWALLSB-1];x+=2)
			lwall[x] = ((lwall[x-1]+lwall[x+1])>>1);

		if (lwall[xb1[MAXWALLSB-1]] < 0) lwall[xb1[MAXWALLSB-1]] = 0;
		if (lwall[xb2[MAXWALLSB-1]] >= (xspan<<6)) lwall[xb2[MAXWALLSB-1]] = (xspan<<6)-1;

		if ((swapped^((cstat&4)>0)) > 0)
		{
			j = (xspan<<6)-1;
			for(x=xb1[MAXWALLSB-1];x<=xb2[MAXWALLSB-1];x++)
				lwall[x] = j-lwall[x];
		}

		rx1[MAXWALLSB-1] = xp1; ry1[MAXWALLSB-1] = yp1;
		rx2[MAXWALLSB-1] = xp2; ry2[MAXWALLSB-1] = yp2;

		y1 = (2097152*64)/yb1[MAXWALLSB-1];
		y2 = (2097152*64)/yb2[MAXWALLSB-1];
		hplc = y1;
		hinc = (y2-y1)/(xb2[MAXWALLSB-1]-xb1[MAXWALLSB-1]+1);
		if (xdimen != XDIM)
		{
			hplc = mulscale(hplc,xdimenscale,16);
			hinc = mulscale(hinc,xdimenscale,16);
		}

		z2 = sprite[spritenum].z;
		z2 -= (yoff*sprite[spritenum].yrepeat)<<2;
		z1 = z2 - ((yspan*sprite[spritenum].yrepeat)<<2);

		globalpal = dapalnum;
		globalorientation = 0;
		globalpicnum = tilenum;
		if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
		globalxpanning = 0L;
		globalypanning = 0L;
		globalshade = (long)sprite[spritenum].shade+6;
		if ((sector[sectnum].ceilingstat&1) > 0)
			globalshade += ((long)sector[sectnum].ceilingshade);
		else
			globalshade += ((long)sector[sectnum].floorshade);
		globalshiftval = (picsiz[globalpicnum]>>4);
		if ((1<<globalshiftval) != tilesizy[globalpicnum]) globalshiftval++;
		globalshiftval = 32-globalshiftval;
		globalyscale = divscale(512,sprite[spritenum].yrepeat,globalshiftval-19);
		globalzd = (((globalposz-z1)*globalyscale)<<8);
		if ((cstat&8) > 0)
		{
			globalyscale = -globalyscale;
			globalzd = (((globalposz-z2)*globalyscale)<<8);
		}

		if ((sector[sectnum].ceilingstat&1) == 0)
			if (z1 < sector[sectnum].ceilingz) z1 = sector[sectnum].ceilingz;
		if ((sector[sectnum].floorstat&1) == 0)
			if (z2 > sector[sectnum].floorz) z2 = sector[sectnum].floorz;

		wallmost(uwall,(long)(MAXWALLSB-1),z1-globalposz);
		wallmost(dwall,(long)(MAXWALLSB-1),z2-globalposz);
		qinterpolate((long)(&swall[xb1[MAXWALLSB-1]]),xb2[MAXWALLSB-1]-xb1[MAXWALLSB-1],hplc,hinc);

		for(x=xb1[MAXWALLSB-1];x<=xb2[MAXWALLSB-1];x++)
		{
			umost[x] = startumost[x];
			dmost[x] = startdmost[x];
		}
		for(i=0;i<smostwallcnt;i++)
		{
			j = (smostwall[i]&16383);
			dalx2 = xb1[j];
			darx2 = xb2[j];

			if ((dalx2 <= xb2[MAXWALLSB-1]) && (darx2 >= xb1[MAXWALLSB-1]))
				if ((yp > (yb1[j]<<8)) || (yp > (yb2[j]<<8)))
				{
						//if (spritewallfront(spritenum,thewall[j]) == 0)
					x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
					x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;
					x = (xp2-xp1)*(sprite[spritenum].y-yp1)-(sprite[spritenum].x-xp1)*(yp2-yp1);
					if ((x < 0) || ((x == 0) && (wall[thewall[j]].nextsector1 == sprite[spritenum].sectnum)))
					{
						if (dalx2 < xb1[MAXWALLSB-1]) dalx2 = xb1[MAXWALLSB-1];
						if (darx2 > xb2[MAXWALLSB-1]) darx2 = xb2[MAXWALLSB-1];

						switch(smostwall[i]&0xc000)
						{
							case 0:
								for(x=dalx2;x<=darx2;x++)
								  dmost[x] = ytop;
								break;
							case 16384:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x] > umost[x]) umost[x] = smost[k+x];
								break;
							case 32768:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x] < dmost[x]) dmost[x] = smost[k+x];
								break;
						}
					}
				}
		}

		if (searchit >= 1)
			if ((searchx >= xb1[MAXWALLSB-1]) && (searchx <= xb2[MAXWALLSB-1]))
				if ((searchy >= umost[searchx]) && (searchy <= dmost[searchx]))
					if ((searchy >= uwall[searchx]) && (searchy <= dwall[searchx]))
					{
						searchsector = sectnum;
						searchwall = spritenum;
						searchstat = 3;  //sprite
						searchit = 1;
					}

		if ((cstat&2) == 0)
			maskwallscan(xb1[MAXWALLSB-1],xb2[MAXWALLSB-1],uwall,dwall,swall,lwall);
		else
			transmaskwallscan(xb1[MAXWALLSB-1],xb2[MAXWALLSB-1]);
	}
	else if ((cstat&48) == 32)
	{
		if ((cstat&64) > 0)
			if ((sprite[spritenum].z < globalposz) == ((cstat&8)==0))
				return;

		xspan = tilesizx[tilenum];
		yspan = tilesizy[tilenum];

			//Rotate center point
		dax = sprite[spritenum].x-globalposx;
		day = sprite[spritenum].y-globalposy;
		rzi[0] = doublemulscale(cosglobalang,dax,singlobalang,day,14);
		rxi[0] = doublemulscale(cosglobalang,day,-singlobalang,dax,14);

			//Get top-left corner
		i = ((sprite[spritenum].ang+2048-globalang)&2047);
		cosang = sintable[(i+512)&2047]; sinang = sintable[i];
		dax = ((xspan>>1)+xoff)*sprite[spritenum].xrepeat;
		day = ((yspan>>1)+yoff)*sprite[spritenum].yrepeat;
		rzi[0] += doublemulscale(sinang,dax,cosang,day,16);
		rxi[0] += doublemulscale(sinang,day,-cosang,dax,16);

			//Get other 3 corners
		dax = xspan*sprite[spritenum].xrepeat;
		day = yspan*sprite[spritenum].yrepeat;
		rzi[1] = rzi[0]-mulscale(sinang,dax,16);
		rxi[1] = rxi[0]+mulscale(cosang,dax,16);
		dax = -mulscale(cosang,day,16);
		day = -mulscale(sinang,day,16);
		rzi[2] = rzi[1]+dax; rxi[2] = rxi[1]+day;
		rzi[3] = rzi[0]+dax; rxi[3] = rxi[0]+day;

			//Put all points on same z
		ryi[0] = (((sprite[spritenum].z-globalposz)<<4)/XDIM);
		if (ryi[0] == 0) return;
		ryi[1] = ryi[0]; ryi[2] = ryi[0]; ryi[3] = ryi[0];

		if ((cstat&4) == 0)
			{ z = 0; z1 = 1; z2 = 3; }
		else
			{ z = 1; z1 = 0; z2 = 2; }

			//Calculate globals for hline texture mapping function
		globalxpanning = (rxi[z]<<16);
		globalypanning = (rzi[z]<<16);
		globalzd = (ryi[z]<<16);

		dax = rzi[z1]-rzi[z]; day = rxi[z1]-rxi[z];
		bot = dax*dax + day*day;
		if (((klabs(dax)>>23) >= bot) || ((klabs(day)>>23) >= bot)) return;
		globalx1 = divscale(dax,bot,22);
		globalx2 = divscale(day,bot,22);

		dax = rzi[z2]-rzi[z]; day = rxi[z2]-rxi[z];
		bot = dax*dax + day*day;
		if (((klabs(dax)>>23) >= bot) || ((klabs(day)>>23) >= bot)) return;
		globaly1 = divscale(dax,bot,22);
		globaly2 = divscale(day,bot,22);

		if (ryi[0] < 0)   //If ceilsprite is above you, reverse order of points
		{
			templong = rxi[1]; rxi[1] = rxi[3]; rxi[3] = templong;
			templong = rzi[1]; rzi[1] = rzi[3]; rzi[3] = templong;
		}

			//Clip polygon in 3-space
		npoints = 4;

			//Clip edge 1
		npoints2 = 0;
		zzsgn = rxi[0]+rzi[0];
		for(z=0;z<npoints;z++)
		{
			zz = z+1; if (zz == npoints) zz = 0;
			zsgn = zzsgn; zzsgn = rxi[zz]+rzi[zz];
			if (zsgn >= 0)
			{
				rxi2[npoints2] = rxi[z]; ryi2[npoints2] = ryi[z]; rzi2[npoints2] = rzi[z];
				npoints2++;
			}
			if ((zsgn^zzsgn) < 0)
			{
				t = divscale(zsgn,zsgn-zzsgn,16);
				rxi2[npoints2] = rxi[z] + mulscale(t,rxi[zz]-rxi[z],16);
				ryi2[npoints2] = ryi[z] + mulscale(t,ryi[zz]-ryi[z],16);
				rzi2[npoints2] = rzi[z] + mulscale(t,rzi[zz]-rzi[z],16);
				npoints2++;
			}
		}
		if (npoints2 <= 2) return;

			//Clip edge 2
		npoints = 0;
		zzsgn = rxi2[0]-rzi2[0];
		for(z=0;z<npoints2;z++)
		{
			zz = z+1; if (zz == npoints2) zz = 0;
			zsgn = zzsgn; zzsgn = rxi2[zz]-rzi2[zz];
			if (zsgn <= 0)
			{
				rxi[npoints] = rxi2[z]; ryi[npoints] = ryi2[z]; rzi[npoints] = rzi2[z];
				npoints++;
			}
			if ((zsgn^zzsgn) < 0)
			{
				t = divscale(zsgn,zsgn-zzsgn,16);
				rxi[npoints] = rxi2[z] + mulscale(t,rxi2[zz]-rxi2[z],16);
				ryi[npoints] = ryi2[z] + mulscale(t,ryi2[zz]-ryi2[z],16);
				rzi[npoints] = rzi2[z] + mulscale(t,rzi2[zz]-rzi2[z],16);
				npoints++;
			}
		}
		if (npoints <= 2) return;

			//Clip edge 3
		npoints2 = 0;
		zzsgn = ryi[0]*(XDIM>>1) + (rzi[0]*(dahorizbak-ytop));
		for(z=0;z<npoints;z++)
		{
			zz = z+1; if (zz == npoints) zz = 0;
			zsgn = zzsgn; zzsgn = ryi[zz]*(XDIM>>1) + (rzi[zz]*(dahorizbak-ytop));
			if (zsgn >= 0)
			{
				rxi2[npoints2] = rxi[z];
				ryi2[npoints2] = ryi[z];
				rzi2[npoints2] = rzi[z];
				npoints2++;
			}
			if ((zsgn^zzsgn) < 0)
			{
				t = divscale(zsgn,zsgn-zzsgn,16);
				rxi2[npoints2] = rxi[z] + mulscale(t,rxi[zz]-rxi[z],16);
				ryi2[npoints2] = ryi[z] + mulscale(t,ryi[zz]-ryi[z],16);
				rzi2[npoints2] = rzi[z] + mulscale(t,rzi[zz]-rzi[z],16);
				npoints2++;
			}
		}
		if (npoints2 <= 2) return;

			//Clip edge 4
		npoints = 0;
		zzsgn = ryi2[0]*(XDIM>>1) - (rzi2[0]*(ybot-dahorizbak));
		for(z=0;z<npoints2;z++)
		{
			zz = z+1; if (zz == npoints2) zz = 0;
			zsgn = zzsgn; zzsgn = ryi2[zz]*(XDIM>>1) - (rzi2[zz]*(ybot-dahorizbak));
			if (zsgn <= 0)
			{
				rxi[npoints] = rxi2[z];
				ryi[npoints] = ryi2[z];
				rzi[npoints] = rzi2[z];
				npoints++;
			}
			if ((zsgn^zzsgn) < 0)
			{
				t = divscale(zsgn,zsgn-zzsgn,16);
				rxi[npoints] = rxi2[z] + mulscale(t,rxi2[zz]-rxi2[z],16);
				ryi[npoints] = ryi2[z] + mulscale(t,ryi2[zz]-ryi2[z],16);
				rzi[npoints] = rzi2[z] + mulscale(t,rzi2[zz]-rzi2[z],16);
				npoints++;
			}
		}
		if (npoints <= 2) return;

			//Project onto screen
		lpoint = -1; lmax = 0x7fffffff;
		rpoint = -1; rmax = 0x80000000;
		for(z=0;z<npoints;z++)
		{
			xsi[z] = scale(rxi[z],xdimen<<15,rzi[z]) + (xdimen<<15);
			ysi[z] = scale(ryi[z],xdimen<<15,rzi[z]) + (globalhoriz<<16);
			if (xsi[z] < 0) xsi[z] = 0;
			if (xsi[z] > (xdimen<<16)) xsi[z] = (xdimen<<16);
			if (ysi[z] < ((long)ytop<<16)) ysi[z] = ((long)ytop<<16);
			if (ysi[z] > ((long)ybot<<16)) ysi[z] = ((long)ybot<<16);
			if (xsi[z] < lmax) lmax = xsi[z], lpoint = z;
			if (xsi[z] > rmax) rmax = xsi[z], rpoint = z;
		}

			//Get umost arrays
		z = lpoint;
		while (z != rpoint)
		{
			zz = z+1; if (zz == npoints) zz = 0;

			dax1 = ((xsi[z]+65535)>>16);
			dax2 = ((xsi[zz]+65535)>>16);
			if (dax2 > dax1)
			{
				yinc = divscale(ysi[zz]-ysi[z],xsi[zz]-xsi[z],16);
				y = ysi[z] + mulscale((dax1<<16)-xsi[z],yinc,16);
				for(x=dax1;x<dax2;x++)
				{
					umost[x] = (y>>16)-1;
					y += yinc;
				}
			}

			z = zz;
		}

			//Get dmost arrays
		while (z != lpoint)
		{
			zz = z+1; if (zz == npoints) zz = 0;

			dax1 = ((xsi[zz]+65535)>>16);
			dax2 = ((xsi[z]+65535)>>16);
			if (dax2 > dax1)
			{
				yinc = divscale(ysi[zz]-ysi[z],xsi[zz]-xsi[z],16);
				y = ysi[zz] + mulscale((dax1<<16)-xsi[zz],yinc,16);
				for(x=dax1;x<dax2;x++)
				{
					dmost[x] = (y>>16);
					y += yinc;
				}
			}

			z = zz;
		}


		lx = ((lmax+65535)>>16);
		rx = ((rmax+65535)>>16);

		for(x=lx;x<=rx;x++)
		{
			if (umost[x] < startumost[x]-1) umost[x] = startumost[x]-1;
			if (dmost[x] > startdmost[x]) dmost[x] = startdmost[x];
		}

			//Additional umost/dmost clipping goes here
		for(i=0;i<smostwallcnt;i++)
		{
			j = (smostwall[i]&16383);
			dalx2 = xb1[j];
			darx2 = xb2[j];

			if ((dalx2 <= rx) && (darx2 >= lx))
				if ((yp > (yb1[j]<<8)) || (yp > (yb2[j]<<8)))
				{
						//if (spritewallfront(spritenum,thewall[j]) == 0)
					x = thewall[j]; xp1 = wall[x].x; yp1 = wall[x].y;
					x = wall[x].point2; xp2 = wall[x].x; yp2 = wall[x].y;
					x = (xp2-xp1)*(sprite[spritenum].y-yp1)-(sprite[spritenum].x-xp1)*(yp2-yp1);
					if ((x < 0) || ((x == 0) && (wall[thewall[j]].nextsector1 == sprite[spritenum].sectnum)))
					{
						if (dalx2 < lx) dalx2 = lx;
						if (darx2 > rx) darx2 = rx;

						switch(smostwall[i]&0xc000)
						{
							case 0:
								for(x=dalx2;x<=darx2;x++)
								  dmost[x] = ytop;
								break;
							case 16384:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x]-1 > umost[x]) umost[x] = smost[k+x]-1;
								break;
							case 32768:
								k = smoststart[i] - xb1[j];
								for(x=dalx2;x<=darx2;x++)
									if (smost[k+x] < dmost[x]) dmost[x] = smost[k+x];
								break;
						}
					}
				}
		}

		if (searchit >= 1)
			if ((searchx >= lx) && (searchx <= rx))
				if ((searchy >= umost[searchx]) && (searchy <= dmost[searchx]))
				{
					searchsector = sectnum;
					searchwall = spritenum;
					searchstat = 3;  //sprite
					searchit = 1;
				}

		globalorientation = cstat;
		globalpicnum = tilenum;
		if ((globalpicnum & ~(MAXTILES-1)) != 0) globalpicnum = 0;
		//if ((picanm[globalpicnum]&192) != 0)
		//   globalpicnum += animateoffs((short)globalpicnum);

		if (waloff[globalpicnum] == -1) loadtile(globalpicnum);
		setgotpic(globalpicnum);
		globalbufplc = waloff[globalpicnum];

		globalpal = dapalnum;
		globalshade = (yp>>(visibility+4))+sprite[spritenum].shade+6;
		if ((sector[sectnum].ceilingstat&1) > 0)
			globalshade += ((long)sector[sectnum].ceilingshade);
		else
			globalshade += ((long)sector[sectnum].floorshade);

		if (globalshade < 0) globalshade = 0;
		if (globalshade >= numpalookups) globalshade = numpalookups-1;
		asm3 = FP_OFF(palookup[globalpal])+(globalshade<<8);

		if ((cstat&2) == 0)
			msethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);
		else
			tsethlineshift(picsiz[globalpicnum]&15,picsiz[globalpicnum]>>4);

			//Draw it!
		ceilspritescan(lx,rx);
	}
}

ceilspritescan (long lx, long rx)
{
	long x, y1, y2, twall, bwall;

	y1 = umost[lx]; y2 = umost[lx];
	if (y1 < -1) y1 = -1;
	if (y1 > YDIM-1) y1 = YDIM-1;
	if (y2 < 0) y2 = 0;
	if (y2 > YDIM) y2 = YDIM;
	for(x=lx;x<rx;x++)
	{
		twall = umost[x];
		bwall = dmost[x];
		if (twall < bwall-1)
		{
			if (twall < -1) twall = -1;
			if (twall > YDIM-1) twall = YDIM-1;
			if (bwall < 0) bwall = 0;
			if (bwall > YDIM) bwall = YDIM;

			if (twall >= y2)
			{
				while (y1 < y2-1)
				{
					y1++;
					ceilspritehline(lastx[y1],x-1,y1);
				}
				y1 = twall;
			}
			else
			{
				while (y1 < twall)
				{
					y1++;
					ceilspritehline(lastx[y1],x-1,y1);
				}
				while (y1 > twall)
				{
					lastx[y1] = x;
					y1--;
				}
			}

			while (y2 > bwall)
			{
				y2--;
				ceilspritehline(lastx[y2],x-1,y2);
			}
			while (y2 < bwall)
			{
				lastx[y2] = x;
				y2++;
			}
		}
		else
		{
			while (y1 < y2-1)
			{
				y1++;
				ceilspritehline(lastx[y1],x-1,y1);
			}
			y1 = umost[x+1];
			y2 = y1;

			if (y1 < -1) y1 = -1;
			if (y1 > YDIM-1) y1 = YDIM-1;
			if (y2 < 0) y2 = 0;
			if (y2 > YDIM) y2 = YDIM;
		}
	}
	while (y1 < y2-1)
	{
		y1++;
		ceilspritehline(lastx[y1],rx-1,y1);
	}

}

ceilspritehline (long x1, long x2, long y)
{
	long v, a, b, bx, by, x, xend, bxinc, byinc, p;

	//x = x1 + (x2-x1)t + (y1-y2)u  ³  x = 160v
	//y = y1 + (y2-y1)t + (x2-x1)u  ³  y = (scrx-160)v
	//z = z1 = z2                   ³  z = posz + (scry-horiz)v

	if (x2 < x1) return;

	v = mulscale(globalzd,horizlookup[y-globalhoriz+512],16);
	a = v*(xdimen>>1) - globalypanning;
	b = v*(x1-(xdimen>>1)) - globalxpanning;
	bx = doublemulscale(globalx1,a,globalx2,b,6);
	by = doublemulscale(globaly1,a,globaly2,b,6);
	asm1 = mulscale(globalx2,v,6);
	asm2 = mulscale(globaly2,v,6);


	if (option[0] == 0)
	{
		bxinc = asm1; asm1 <<= 2;
		byinc = asm2; asm2 <<= 2;
		x1 += viewoffset; x2 += viewoffset;
		p = mul80(y)+0xa0000+(page<<14);

		outp(0x3c4,2);

		xend = x1+3; if (x2 < xend) xend = x2;

		if ((globalorientation&2) == 0)
		{
			x = x1;
			if (x <= xend)             //First one does self-modifying code
			{
				outp(0x3c5,1<<(x&3));
				mhline(globalbufplc,bx,((x2-x)>>2)<<16,0L,by,p+(x>>2));
				bx += bxinc; by += byinc;
				x++;
			}
			while (x <= xend)          //Rest can skip self-modifying code
			{
				outp(0x3c5,1<<(x&3));
				mhlineskipmodify(globalbufplc,bx,((x2-x)>>2)<<16,0L,by,p+(x>>2));
				bx += bxinc; by += byinc;
				x++;
			}
		}
		else
		{
			koutp(0x3ce,0x4);

			x = x1;
			if (x <= xend)             //First one does self-modifying code
			{
				outp(0x3c5,1<<(x&3)); koutp(0x3cf,x&3);
				thline(globalbufplc,bx,((x2-x)>>2)<<16,0L,by,p+(x>>2));
				bx += bxinc; by += byinc;
				x++;
			}
			while (x <= xend)          //Rest can skip self-modifying code
			{
				outp(0x3c5,1<<(x&3)); koutp(0x3cf,x&3);
				thlineskipmodify(globalbufplc,bx,((x2-x)>>2)<<16,0L,by,p+(x>>2));
				bx += bxinc; by += byinc;
				x++;
			}
		}
	}
	else
	{
		if (option[0] != 7)
			p = mul320(y)+x1+frameplace+viewoffset;
		else
			p = mul640(y)+x1+frameplace+viewoffset;

		if ((globalorientation&2) == 0)
			mhline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
		else
			thline(globalbufplc,bx,(x2-x1)<<16,0L,by,p);
	}
}

overwritesprite (long thex, long they, short tilenum, signed char shade, char orientation, char dapalnum)
{
	char *ptr;
	long xsiz, ysiz, i, x, y, x1, y1, x2, y2, p, tileoffs;
	long lm, rm, um, dm, palookupoffs, yoffs, yince;

	if ((tilesizx[tilenum] <= 0) || (tilesizy[tilenum] <= 0))
		return;

	if (shade < 0) shade = 0;
	if (shade >= numpalookups) shade = numpalookups-1;
	palookupoffs = ((long)shade)<<8;
	palookupoffs += FP_OFF(palookup[dapalnum]);

	if (waloff[tilenum] == -1) loadtile(tilenum);
	setgotpic(tilenum);
	tileoffs = waloff[tilenum];
	xsiz = tilesizx[tilenum];
	ysiz = tilesizy[tilenum];

	x1 = thex;
	y1 = they;
	if ((orientation&1) > 0)
	{
		x1 -= (xsiz>>1) + (signed char)((picanm[tilenum]>>8)&255);
		y1 -= (ysiz>>1) + (signed char)((picanm[tilenum]>>16)&255);
	}
	if ((orientation&2) > 0)
	{
		if (xdimen != XDIM)
		{
			x1 = ((windowx1+windowx2+1)>>1)+mulscale(x1-(XDIM>>1),xdimenscale,16);
			y1 = ((windowy1+windowy2+1)>>1)+mulscale(y1-(YDIM>>1),xdimenscale,16);
		}
		else
		{
			y1 += ((windowy1+windowy2+1)>>1)-(YDIM>>1);
		}
	}

	if (((orientation&2) > 0) && (xdimen != XDIM))
	{
		x2 = x1 + mulscale(xsiz,xdimenscale,16);
		y2 = y1 + mulscale(ysiz,xdimenscale,16);
	}
	else
	{
		x2 = x1 + xsiz;
		y2 = y1 + ysiz;
	}

	lm = x1;
	rm = x2-1;
	if ((orientation&2) > 0)
	{
		if (lm < windowx1) lm = windowx1;
		if (rm > windowx2) rm = windowx2;
	}
	else
	{
		if (lm < 0) lm = 0;
		if (rm >= XDIM) rm = XDIM-1;
	}

	if ((lm > rm) || (rm < 0) || (lm > XDIM))
		return;

	yoffs = (YDIM>>1)-((windowy1+windowy2+1)>>1);
	if ((orientation&2) > 0)
		yince = (xdimscale<<8);
	else
		yince = 16777216;

	for(x=lm;x<=rm;x++)
	{
		ptr = (char *)(tileoffs+((x-x1)*ysiz));
		if ((orientation&2) > 0)
		{
			if (xdimen != XDIM)
			{
				i = mulscale(x-x1,xdimscale,16);
				if (i >= xsiz) return;
				ptr = (char *)(tileoffs+(i*ysiz));
			}
			um = y1;
			dm = y2;
			if (um < startumost[x-windowx1]-yoffs) um = startumost[x-windowx1]-yoffs;
			if (dm > startdmost[x-windowx1]-yoffs) dm = startdmost[x-windowx1]-yoffs;
		}
		else
		{
			um = y1;
			dm = y2;
			if (um < ytop) um = ytop;
			if (dm > ybot) dm = ybot;
		}

		if (option[0] == 0)
		{
			// COMPILER_STEERED: Group the planar page offset separately from the row and column.
			p = ((x>>2)+mul80(um)) + chainoffset;
			koutpw(0x3c4,2+(256<<(x&3)));
		}
		else
		{
			if (option[0] != 7) p = mul320(um);
			else p = mul640(um);
			p += x+frameplace;
		}

		if (dm > um)
		{
			if ((orientation&4) > 0)
				tvlineasm1(yince,palookupoffs,(((long)(dm-um-1))<<8)+24,0L,(long)ptr,p);
			else
				mvlineasm1(yince,palookupoffs,(((long)(dm-um-1))<<8)+24,0L,(long)ptr,p);
		}
	}

}

permanentwritesprite (long thex, long they, short tilenum, signed char shade,
							 long cx1, long cy1, long cx2, long cy2)
{
	long xsiz, ysiz, x, y, x1, y1, x2, y2, p, tileoffs, bufplc;
	long lm, rm, um, dm, z, vplce[4], vince[4], palookupoffse[4], bufplce[4];
	long dapage, danumpages, palookupoffs, plane;

	if ((tilesizx[tilenum] <= 0) || (tilesizy[tilenum] <= 0))
		return;

	if (shade < 0) shade = 0;
	if (shade >= numpalookups) shade = numpalookups-1;
	palookupoffs = FP_OFF(palookup[globalpal]) + (((long)shade)<<8);

	if (waloff[tilenum] == -1) loadtile(tilenum);
	setgotpic(tilenum);
	tileoffs = waloff[tilenum];
	xsiz = tilesizx[tilenum];
	ysiz = tilesizy[tilenum];

	x1 = thex;
	y1 = they;
	x2 = x1 + xsiz;
	y2 = y1 + ysiz;

	lm = x1; if (lm < 0) lm = 0;
	rm = x2; if (rm > XDIM) rm = XDIM;
	um = y1; if (um < ytop) um = ytop;
	dm = y2; if (dm > ybot) dm = ybot;

	if (lm < cx1) lm = cx1;
	if (rm > cx2+1) rm = cx2+1;
	if (um < cy1) um = cy1;
	if (dm > cy2+1) dm = cy2+1;

	if ((rm < 0) || (lm >= XDIM) || (rm <= lm) || (dm < ytop) || (um >= ybot) || (dm <= um))
		return;

	for(z=0;z<4;z++)
	{
		palookupoffse[z] = palookupoffs;
		vince[z] = 16777216;
	}

	danumpages = 4;
	if (option[0] == 1)
		danumpages = 1;

	for (dapage=0;dapage<danumpages;dapage++)
	{
		if (option[0] == 0)
		{
			for(plane=0;plane<4;plane++)
			{
				koutpw(0x3c4,2+(256<<plane));

				x = lm+plane;
				p = mul80(um)+(x>>2)+0xa0000+(((long)dapage)<<14);
				bufplc = tileoffs + (x-x1)*ysiz + (um-y1);

				while (x < rm-12)
				{
					bufplce[0] = bufplc;
					bufplce[1] = bufplce[0]+(ysiz<<2);
					bufplce[2] = bufplce[1]+(ysiz<<2);
					bufplce[3] = bufplce[2]+(ysiz<<2);
					bufplc = bufplce[3]+(ysiz<<2);
					vplce[0] = 0;
					vplce[1] = 0;
					vplce[2] = 0;
					vplce[3] = 0;
					mvlineasm4((long)(&vplce[0]),(long)(&vince[0]),((dm-um-1)<<8)+24,(long)(&bufplce[0]),(long)(&palookupoffse[0]),p);
					p += 4;
					x += 16;
				}
				while (x < rm)
				{
					mvlineasm1(16777216L,palookupoffs,(((long)(dm-um-1))<<8)+24,0L,bufplc,p);
					bufplc += (ysiz<<2);
					p++;
					x += 4;
				}
			}
		}
		else
		{
			if (option[0] == 2) koutp(0x3cd,dapage+(dapage<<4));
			if (option[0] == 3) koutp(0x3ce,0x9), koutp(0x3cf,dapage<<4);
			if (option[0] == 4) koutp(0x3d4,0x35), koutp(0x3d5,dapage);

			x = lm;

			bufplc = tileoffs + (x-x1)*ysiz + (um-y1);

			if (option[0] != 7)
				p = mul320(um) + x+frameplace;
			else
				p = mul640(um) + x+frameplace;

			while (x < rm-3)
			{
				bufplce[0] = bufplc;
				bufplce[1] = bufplce[0]+ysiz;
				bufplce[2] = bufplce[1]+ysiz;
				bufplce[3] = bufplce[2]+ysiz;
				bufplc = bufplce[3]+ysiz;
				vplce[0] = 0;
				vplce[1] = 0;
				vplce[2] = 0;
				vplce[3] = 0;
				mvlineasm4((long)(&vplce[0]),(long)(&vince[0]),((dm-um-1)<<8)+24,(long)(&bufplce[0]),(long)(&palookupoffse[0]),p);
				p += 4;
				x += 4;
			}
			while (x < rm)
			{
				mvlineasm1(16777216L,palookupoffs,(((long)(dm-um-1))<<8)+24,0L,bufplc,p);
				bufplc += ysiz;
				p++;
				x++;
			}
		}
	}
	if (option[0] == 2) koutp(0x3cd,page+(page<<4));
	if (option[0] == 3) koutp(0x3ce,0x9), koutp(0x3cf,page<<4);
	if (option[0] == 4) koutp(0x3d4,0x35), koutp(0x3d5,page);

}

grouvline (long xscr, long d1, long d2, short sectnum)
{
	char *ptr;
	signed char shade;
	short dir[2], index;
	// COMPILER_STEERED: Put the row stride before the traversal arrays for stack allocation.
	long pinc, dist[2], dinc[2], incr[2], p, lastdist;
	long gx, gy, grid[2], tempx, tempy, x1, y1, x2, y2, stx, sty, cnt, dend;
	long tilebufoffs, heibufoffs, zoffs, they, um, dm, zx, dat, col;

	d1 >>= 6;
	d2 >>= 6;
	if ((d1 <= 0) || (d2 <= 0) || (umost[xscr] >= dmost[xscr])) return;
	d1 = (65536 * 2048) / d1;
	d2 = (65536 * 2048) / d2;
	if (xdimen != XDIM)
	{
		d1 = mulscale(d1,xdimenscale,16);
		d2 = mulscale(d2,xdimenscale,16);
	}
	d1++;
	d2--;
	if ((d2 <= d1) || (d1 <= 0) || (d2 <= 0)) return;

	if (option[0] == 0)
	{
		p = ((xscr+viewoffset)>>2)+mul80(dmost[xscr]-1)+0xa0000;
		p += page<<14;
		koutpw(0x3c4,2+(256<<((xscr+viewoffset)&3)));
		pinc = 80;
	}
	else if (option[0] != 7)
	{
		p = mul320(dmost[xscr]-1)+xscr+frameplace+viewoffset;
		pinc = 320;
	}
	else
	{
		p = mul640(dmost[xscr]-1)+xscr+frameplace+viewoffset;
		pinc = 640;
	}

	zx = xscr-(xdimen>>1);
	tempy = (d1*zx)/(xdimen<<1); tempx = (d1>>2);
	x1 = (cosglobalang*tempx-singlobalang*tempy)>>12;
	x1 += (globalposx<<6);
	y1 = (cosglobalang*tempy+singlobalang*tempx)>>12;
	y1 += (globalposy<<6);
	tempy = (d2*zx)/(xdimen<<1); tempx = (d2>>2);
	incr[0] = ((tempx*cosglobalang-tempy*singlobalang)>>12);
	x2 = incr[0] + (globalposx<<6);
	incr[1] = ((tempx*singlobalang+tempy*cosglobalang)>>12);
	y2 = incr[1] + (globalposy<<6);

	dir[0] = ksgn(incr[0]); incr[0] = klabs(incr[0]);
	dir[1] = ksgn(incr[1]); incr[1] = klabs(incr[1]);

	stx = (x1&0xfffffc00); if (dir[0] == 1) stx += 1024;
	sty = (y1&0xfffffc00); if (dir[1] == 1) sty += 1024;
	cnt = ((klabs(stx-x1)*incr[1]-klabs(sty-y1)*incr[0])>>10);
	grid[0] = ((x1>>10)&63);
	grid[1] = ((y1>>10)&63);

	if ((klabs(d2)>>16) < klabs(x2-(globalposx<<6)))
	{
		dinc[0] = divscale(d2,klabs(x2-(globalposx<<6)),16);
		if (xdimen != XDIM) dinc[0] = mulscale(dinc[0],xdimscale,16);
		dist[0] = mulscale(dinc[0],klabs(stx-(globalposx<<6)),10);
	}
	if ((klabs(d2)>>16) < klabs(y2-(globalposy<<6)))
	{
		dinc[1] = divscale(d2,klabs(y2-(globalposy<<6)),16);
		if (xdimen != XDIM) dinc[1] = mulscale(dinc[1],xdimscale,16);
		dist[1] = mulscale(dinc[1],klabs(sty-(globalposy<<6)),10);
	}
	if (dinc[0] < 8) dinc[0] = 8;
	if (dinc[1] < 8) dinc[1] = 8;
	if (dist[0] < 8) dist[0] = 8;
	if (dist[1] < 8) dist[1] = 8;

	shade = (d1>>(visibility+2))+sector[sectnum].floorshade;
	if (shade < 0) shade = 0;
	if (shade >= numpalookups) shade = numpalookups-1;

	dend = (d2<<6);
	if (xdimen != XDIM) dend = mulscale(dend,xdimscale,16);

	um = umost[xscr] - globalhoriz;
	dm = dmost[xscr] - globalhoriz;
	y1 = incr[0]; incr[0] = incr[1]; incr[1] = y1;
	incr[1] = -incr[1];

	if (waloff[sector[sectnum].floorpicnum] == -1) loadtile(sector[sectnum].floorpicnum);
	if (waloff[sector[sectnum].floorheinum] == -1) loadtile(sector[sectnum].floorheinum);
	setgotpic(sector[sectnum].floorpicnum);
	setgotpic(sector[sectnum].floorheinum);

	tilebufoffs = waloff[sector[sectnum].floorpicnum];
	heibufoffs = waloff[sector[sectnum].floorheinum];
	zoffs = sector[sectnum].floorz - globalposz;

	index = (cnt >= 0);
	lastdist = dist[index];
	while (dist[index] < dend)
	{
		dat = groudat(grid[0],grid[1],heibufoffs);
		tempy = divscale(zoffs-(dat<<8),dist[index],15);
		if (tempy < 0) goto restisabove;
		if (tempy <= dm)
		{
			ptr = (char *)(FP_OFF(palookup[globalpal])+(shade<<8)+groudat(grid[0],grid[1],tilebufoffs));
			col = *ptr;
			if (tempy <= um) goto finishgroudraw;
			p = drawslab(p,dm-tempy+1,col,pinc);
			dm = tempy-1;
		}
		grid[index] = ((grid[index]+dir[index])&63);
		lastdist = dist[index];
		dist[index] += dinc[index];
		cnt += incr[index];
		index = (cnt >= 0);
	}

	goto endgroudraw;

	while (dist[index] < dend)
	{
		dat = groudat(grid[0],grid[1],heibufoffs);
restisabove:
		tempy = divscale(zoffs-(dat<<8),lastdist,15);
		if (tempy <= dm)
		{
			ptr = (char *)(FP_OFF(palookup[globalpal])+(shade<<8)+groudat(grid[0],grid[1],tilebufoffs));
			col = *ptr;
			if (tempy <= um) goto finishgroudraw;
			p = drawslab(p,dm-tempy+1,col,pinc);
			dm = tempy-1;
		}
		grid[index] = ((grid[index]+dir[index])&63);
		lastdist = dist[index];
		dist[index] += dinc[index];
		cnt += incr[index];
		index = (cnt >= 0);
	}
	goto endgroudraw;

finishgroudraw:
	if (um < dm)
	{
		p = drawslab(p,dm-um,col,pinc);
		dm = um-1;
	}

endgroudraw:
	dmost[xscr] = dm + globalhoriz;

}

setsprite(short spritenum, long newx, long newy, long newz)
{
	short bad, j, tempsectnum;

	sprite[spritenum].x = newx;
	sprite[spritenum].y = newy;
	sprite[spritenum].z = newz;

	tempsectnum = sprite[spritenum].sectnum;
	updatesector(newx,newy,&tempsectnum);
	if (tempsectnum < 0)
		return(-1);
	if (tempsectnum != sprite[spritenum].sectnum)
		changespritesect(spritenum,tempsectnum);

	return(0);
}

movesprite(short spritenum, long xchange, long ychange, long zchange, long walldist, long ceildist, long flordist, char cliptype, long numtics)
{
	long daz, templong;
	short retval, dasectnum;

	dasectnum = sprite[spritenum].sectnum;

	daz = sprite[spritenum].z;
	daz -= ((tilesizy[sprite[spritenum].picnum]*sprite[spritenum].yrepeat)<<1);
	retval = clipmove(&sprite[spritenum].x,&sprite[spritenum].y,&daz,&dasectnum,((xchange*numtics)<<11),((ychange*numtics)<<11),walldist,ceildist,flordist,cliptype);
	if ((dasectnum != sprite[spritenum].sectnum) && (dasectnum >= 0))
		changespritesect(spritenum,dasectnum);

	daz = sprite[spritenum].z + ((zchange*numtics)>>3);
	if ((daz > sector[dasectnum].ceilingz) && (daz <= sector[dasectnum].floorz))
		sprite[spritenum].z = daz;
	else
	{
		if (retval == 0)
			return(16384+dasectnum);
	}

	return(retval);
}

animateoffs(short tilenum)
{
	long i, k, offs;

	offs = 0;
	i = (totalclocklock>>((picanm[tilenum]>>24)&15));
	if ((picanm[tilenum]&63) > 0)
	{
		switch(picanm[tilenum]&192)
		{
			case 64:
				k = (i%((picanm[tilenum]&63)<<1));
				if (k < (picanm[tilenum]&63))
					offs = k;
				else
					offs = (((picanm[tilenum]&63)<<1)-k);
				break;
			case 128:
				offs = (i%((picanm[tilenum]&63)+1));
					break;
			case 192:
				offs = -(i%((picanm[tilenum]&63)+1));
		}
	}
	return(offs);
}

initspritelists()
{
	long i;

	for (i=0;i<MAXSECTORS;i++)     //Init doubly-linked sprite sector lists
		headspritesect[i] = -1;
	headspritesect[MAXSECTORS] = 0;
	for(i=0;i<MAXSPRITES;i++)
	{
		prevspritesect[i] = i-1;
		nextspritesect[i] = i+1;
		sprite[i].sectnum = MAXSECTORS;
	}
	prevspritesect[0] = -1;
	nextspritesect[MAXSPRITES-1] = -1;


	for(i=0;i<MAXSTATUS;i++)      //Init doubly-linked sprite status lists
		headspritestat[i] = -1;
	headspritestat[MAXSTATUS] = 0;
	for(i=0;i<MAXSPRITES;i++)
	{
		prevspritestat[i] = i-1;
		nextspritestat[i] = i+1;
		sprite[i].statnum = MAXSTATUS;
	}
	prevspritestat[0] = -1;
	nextspritestat[MAXSPRITES-1] = -1;
}

insertsprite(short sectnum, short statnum)
{
	insertspritestat(statnum);
	return(insertspritesect(sectnum));
}

insertspritesect(short sectnum)
{
	short blanktouse;

	if ((sectnum >= MAXSECTORS) || (headspritesect[MAXSECTORS] == -1))
		return(-1);  //list full

	blanktouse = headspritesect[MAXSECTORS];

	headspritesect[MAXSECTORS] = nextspritesect[blanktouse];
	if (headspritesect[MAXSECTORS] >= 0)
		prevspritesect[headspritesect[MAXSECTORS]] = -1;

	prevspritesect[blanktouse] = -1;
	nextspritesect[blanktouse] = headspritesect[sectnum];
	if (headspritesect[sectnum] >= 0)
		prevspritesect[headspritesect[sectnum]] = blanktouse;
	headspritesect[sectnum] = blanktouse;

	sprite[blanktouse].sectnum = sectnum;

	return(blanktouse);
}

insertspritestat(short statnum)
{
	short blanktouse;

	if ((statnum >= MAXSTATUS) || (headspritestat[MAXSTATUS] == -1))
		return(-1);  //list full

	blanktouse = headspritestat[MAXSTATUS];

	headspritestat[MAXSTATUS] = nextspritestat[blanktouse];
	if (headspritestat[MAXSTATUS] >= 0)
		prevspritestat[headspritestat[MAXSTATUS]] = -1;

	prevspritestat[blanktouse] = -1;
	nextspritestat[blanktouse] = headspritestat[statnum];
	if (headspritestat[statnum] >= 0)
		prevspritestat[headspritestat[statnum]] = blanktouse;
	headspritestat[statnum] = blanktouse;

	sprite[blanktouse].statnum = statnum;

	return(blanktouse);
}

deletesprite(short spritenum)
{
	deletespritestat(spritenum);
	return(deletespritesect(spritenum));
}

deletespritesect(short deleteme)
{
	if (sprite[deleteme].sectnum == MAXSECTORS)
		return(-1);

	if (headspritesect[sprite[deleteme].sectnum] == deleteme)
		headspritesect[sprite[deleteme].sectnum] = nextspritesect[deleteme];

	if (prevspritesect[deleteme] >= 0) nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
	if (nextspritesect[deleteme] >= 0) prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];

	if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
	prevspritesect[deleteme] = -1;
	nextspritesect[deleteme] = headspritesect[MAXSECTORS];
	headspritesect[MAXSECTORS] = deleteme;

	sprite[deleteme].sectnum = MAXSECTORS;
	return(0);
}

deletespritestat (short deleteme)
{
	if (sprite[deleteme].statnum == MAXSTATUS)
		return(-1);

	if (headspritestat[sprite[deleteme].statnum] == deleteme)
		headspritestat[sprite[deleteme].statnum] = nextspritestat[deleteme];

	if (prevspritestat[deleteme] >= 0) nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
	if (nextspritestat[deleteme] >= 0) prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];

	if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
	prevspritestat[deleteme] = -1;
	nextspritestat[deleteme] = headspritestat[MAXSTATUS];
	headspritestat[MAXSTATUS] = deleteme;

	sprite[deleteme].statnum = MAXSTATUS;
	return(0);
}

changespritesect(short spritenum, short newsectnum)
{
	if ((newsectnum < 0) || (newsectnum > MAXSECTORS)) return(-1);
	if (sprite[spritenum].sectnum == newsectnum) return(-1);
	if (sprite[spritenum].sectnum == MAXSECTORS) return(-1);
	if (deletespritesect(spritenum) < 0) return(-1);
	insertspritesect(newsectnum);
	return(0);
}

changespritestat(short spritenum, short newstatnum)
{
	if ((newstatnum < 0) || (newstatnum > MAXSTATUS)) return(-1);
	if (sprite[spritenum].statnum == newstatnum) return(-1);
	if (sprite[spritenum].statnum == MAXSTATUS) return(-1);
	if (deletespritestat(spritenum) < 0) return(-1);
	insertspritestat(newstatnum);
	return(0);
}

nextsectorneighborz(short sectnum, long thez, short topbottom, short direction)
{
	long testz, nextz;
	short i, startwall, endwall, sectortouse;

	if (direction == 1)
		nextz = 0x7fffffff;
	else
		nextz = 0x80000000;

	sectortouse = -1;

	startwall = sector[sectnum].wallptr;
	endwall = startwall + sector[sectnum].wallnum - 1;
	for(i=startwall;i<=endwall;i++)
		if (wall[i].nextsector1 >= 0)
		{
			if (topbottom == 1)
			{
				testz = sector[wall[i].nextsector1].floorz;
				if (direction == 1)
				{
					if ((testz > thez) && (testz < nextz))
					{
						nextz = testz;
						sectortouse = wall[i].nextsector1;
					}
				}
				else
				{
					if ((testz < thez) && (testz > nextz))
					{
						nextz = testz;
						sectortouse = wall[i].nextsector1;
					}
				}
			}
			else
			{
				testz = sector[wall[i].nextsector1].ceilingz;
				if (direction == 1)
				{
					if ((testz > thez) && (testz < nextz))
					{
						nextz = testz;
						sectortouse = wall[i].nextsector1;
					}
				}
				else
				{
					if ((testz < thez) && (testz > nextz))
					{
						nextz = testz;
						sectortouse = wall[i].nextsector1;
					}
				}
			}
		}

	return(sectortouse);
}

void __interrupt __far zerohandler(void)
{
	long i;

	setvmode(0x3);
	for(i=0;i<4000;i+=4)
	{
		drawpixel(0xb8000+i,'/');
		drawpixel(0xb8000+i+2,48);
	}
	exit(0);
}

int lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3,
			  long y3, long x4, long y4, long *intx, long *inty, long *intz,
			  short datype);

cansee(long xs, long ys, long zs, short sectnums, long xe, long ye, long ze, short sectnume)
{
	long x1, y1, x2, y2, intx, inty, intz;
	short z, zz, tempshortcnt, tempshortnum, dasector, startwall, endwall;
	short nextsector, bad;

	if ((xs == xe) && (ys == ye)) return(1);

	clipsectorlist[0] = sectnums;
	clipsectorlist[1] = sectnume;
	tempshortcnt = 0;
	tempshortnum = 2;
	while (tempshortcnt < tempshortnum)
	{
		dasector = clipsectorlist[tempshortcnt];

		startwall = sector[dasector].wallptr;
		endwall = startwall + sector[dasector].wallnum - 1;
		for(z=startwall;z<=endwall;z++)
		{
			x1 = wall[z].x;
			y1 = wall[z].y;
			x2 = wall[wall[z].point2].x;
			y2 = wall[wall[z].point2].y;
			if (lintersect(xs,ys,zs,xe,ye,ze,x1,y1,x2,y2,&intx,&inty,&intz,0) == 1)
			{
				nextsector = wall[z].nextsector1;
				if (nextsector == -1)
				{
					return(0);
				}
				else
				{
					if (intz <= sector[dasector].ceilingz) return(0);
					if (intz >= sector[dasector].floorz) return(0);
					if (intz <= sector[nextsector].ceilingz) return(0);
					if (intz >= sector[nextsector].floorz) return(0);

					bad = 0;
					for(zz=0;zz<tempshortnum;zz++)
						if (clipsectorlist[zz] == nextsector)
						{
							bad = 1;
							break;
						}
					if (bad == 0)
						clipsectorlist[tempshortnum++] = nextsector;
				}
			}
		}

		tempshortcnt++;
	}
	return(1);
}

hitscan(long xs, long ys, long zs, short sectnum, long vx, long vy, long vz,
	short *hitsect, short *hitwall, short *hitsprite, long *hitx, long *hity, long *hitz)
{
	// COMPILER_STEERED: Order the intersection results for their stack homes.
	long z, zz, xe, ye, ze, x1, y1, z1, x2, y2, z2, intz, inty, intx;
	long topt, topu, bot, daxsiz, daysiz, dist, offx, offy, dx, dy;
	short tempshortcnt, tempshortnum, dasector, startwall, endwall;
	short nextsector1, bad, point2;


	if (sectnum == -1) return(-1);
	*hitsect = -1;
	xe = xs+vx;
	ye = ys+vy;
	ze = zs+vz;
	clipsectorlist[0] = sectnum;
	tempshortcnt = 0;
	tempshortnum = 1;
	while (tempshortcnt < tempshortnum)
	{
		dasector = clipsectorlist[tempshortcnt];

		startwall = sector[dasector].wallptr;
		endwall = startwall + sector[dasector].wallnum;
		endwall--;
		for(z=startwall;z<=endwall;z++)
		{
			point2 = wall[z].point2;
			x2 = wall[point2].x;
			x1 = wall[z].x;
			y2 = wall[point2].y;
			y1 = wall[z].y;

			if ((x1-xs)*(y2-ys) >= (x2-xs)*(y1-ys))   //If wall's facing you
			{
				if (lintersect(xs,ys,zs,xe,ye,ze,x1,y1,x2,y2,&intx,&inty,&intz,1) == 1)
				{
					nextsector1 = wall[z].nextsector1;

					bad = 0;
					if (vz != 0)
						if ((intz <= sector[dasector].ceilingz) || (intz >= sector[dasector].floorz))
							if ((*hitsect == -1) || (klabs(intx-xs)+klabs(inty-ys) < klabs(*hitx-xs)+klabs(*hity-ys)))
								{
									*hitsect = dasector;
									*hitwall = -1;
									*hitsprite = -1;
									if (vz > 0)
										*hitz = sector[dasector].floorz;
									else
										*hitz = sector[dasector].ceilingz;
									*hitx = xs + (vx*(*hitz-zs)) / vz;
									*hity = ys + (vy*(*hitz-zs)) / vz;
									bad = 1;
								}

					if (bad == 0)
					{
						if ((nextsector1 == -1) || ((wall[z].cstat&64) > 0))
						{
							if ((*hitsect == -1) || (klabs(intx-xs)+klabs(inty-ys) < klabs(*hitx-xs)+klabs(*hity-ys)))
							{
								*hitsect = dasector;
								*hitwall = z;
								*hitsprite = -1;
								*hitx = intx;
								*hity = inty;
								*hitz = intz;
							}
						}
						else
						{
							if ((intz <= sector[nextsector1].ceilingz) || (intz >= sector[nextsector1].floorz))
								if ((*hitsect == -1) || (klabs(intx-xs)+klabs(inty-ys) < klabs(*hitx-xs)+klabs(*hity-ys)))
								{
									*hitsect = dasector;
									*hitwall = z;
									*hitsprite = -1;
									*hitx = intx;
									*hity = inty;
									*hitz = intz;
									bad = 1;
								}

							if (bad == 0)
							{
								for (zz=0;zz<tempshortnum;zz++)
									if (clipsectorlist[zz] == nextsector1)
									{
										bad = 1;
										break;
									}

								if (bad == 0)
									clipsectorlist[tempshortnum++] = nextsector1;
							}
						}
					}
				}
			}
		}

		z = headspritesect[dasector];
		while (z != -1)
		{
			if (((sprite[z].cstat&1) > 0) && ((sprite[z].cstat&48) != 32))
			{
				z1 = sprite[z].z;
				x1 = sprite[z].x;
				y1 = sprite[z].y;

				dx = x1-xs; dy = y1-ys;
				topt = vx*dx + vy*dy;
				if (topt > 0)
				{
					bot = vx*vx + vy*vy;
					if (bot != 0)
					{
						intz = zs+scale(vz,topt,bot);
						// COMPILER_STEERED: Keep the shifted height calculation in the lower-bound test.
						daysiz = tilesizy[sprite[z].picnum]*sprite[z].yrepeat;
						if ((z1 >= intz) && (z1-((daysiz<<8)>>6) <= intz))
						{
							topu = vx*dy - vy*dx;

							offx = scale(vx,topu,bot);
							offy = scale(vy,topu,bot);
							dist = offx*offx + offy*offy;
							daxsiz = (tilesizx[sprite[z].picnum]*sprite[z].xrepeat);
							daxsiz *= daxsiz;
							if (dist <= (daxsiz>>7))
							{
								intx = xs + scale(vx,topt,bot);
								inty = ys + scale(vy,topt,bot);

								if ((*hitsect == -1) || (klabs(intx-xs)+klabs(inty-ys) < klabs(*hitx-xs)+klabs(*hity-ys)))
								{
									*hitsect = dasector;
									*hitwall = -1;
									*hitsprite = z;
									*hitx = intx;
									*hity = inty;
									*hitz = intz;
								}
							}
						}
					}
				}
			}

			z = nextspritesect[z];
		}

		tempshortcnt++;
	}
	return(0);
}

neartag (long xs, long ys, long zs, short sectnum, short ange, short *neartagsector, short *neartagwall, short *neartagsprite, long *neartaghitdist, long neartagrange)
{
	long z, zz, xe, ye, ze, x1, y1, z1, x2, y2, z2, intx, inty, intz;
	long topt, topu, bot, daxsiz, daysiz, dist, offx, offy;
	short tempshortcnt, tempshortnum, dasector, startwall, endwall;
	short nextsector1, bad, good;
	long vx, vy, vz;


	*neartagsector = -1;
	*neartagwall = -1;
	*neartagsprite = -1;
	*neartaghitdist = 0;

	if (sectnum == -1)
		return(0);

	vx = mulscale(sintable[(ange+2560)&2047],neartagrange,14);
	vy = mulscale(sintable[(ange+2048)&2047],neartagrange,14);
	vz = 0;

	xe = xs+vx;
	ye = ys+vy;
	ze = zs+vz;

	/* COMPILER_STEERED. Queue initialization order affects register allocation. */
	tempshortcnt = 0;
	tempshortnum = 1;
	clipsectorlist[0] = sectnum;
	while (tempshortcnt < tempshortnum)
	{
		dasector = clipsectorlist[tempshortcnt];

		startwall = sector[dasector].wallptr;
		endwall = startwall + sector[dasector].wallnum - 1;
		for(z=startwall;z<=endwall;z++)
		{
			good = 0;
			if (wall[z].nextsector1 >= 0)
			{
				if ((sector[wall[z].nextsector1].lotag|sector[wall[z].nextsector1].hitag) != 0)
					good = 1;
			}
			if ((wall[z].lotag|wall[z].hitag) != 0) good |= 2;

			if ((good != 0) || (wall[z].nextsector1 >= 0))
			{
				x1 = wall[z].x;
				y1 = wall[z].y;
				x2 = wall[wall[z].point2].x;
				y2 = wall[wall[z].point2].y;

				if ((x1-xs)*(y2-ys) >= (x2-xs)*(y1-ys))   //If wall's facing you
				{
					if (lintersect(xs,ys,zs,xe,ye,ze,x1,y1,x2,y2,&intx,&inty,&intz,0) == 1)
					{
						if ((good&1) > 0)
						{
							*neartagsector = wall[z].nextsector1;
							*neartaghitdist = mulscale(intx-xs,sintable[(ange+2560)&2047],14) + mulscale(inty-ys,sintable[(ange+2048)&2047],14);
							xe = intx;
							ye = inty;
							ze = intz;
						}
						if ((good&2) > 0)
						{
							*neartagwall = z;
							*neartaghitdist = mulscale(intx-xs,sintable[(ange+2560)&2047],14) + mulscale(inty-ys,sintable[(ange+2048)&2047],14);
							xe = intx;
							ye = inty;
							ze = intz;
						}

						if (wall[z].nextsector1 >= 0)
						{
							nextsector1 = wall[z].nextsector1;
							bad = 0;
							for(zz=0;zz<tempshortnum;zz++)
								if (clipsectorlist[zz] == nextsector1)
								{
									bad = 1;
									break;
								}
							if (bad == 0)
							{
								clipsectorlist[tempshortnum] = nextsector1;
								tempshortnum++;
							}
						}
					}
				}
			}
		}

		z = headspritesect[dasector];
		while (z != -1)
		{
			if ((sprite[z].lotag|sprite[z].hitag) != 0)
			{
				x1 = sprite[z].x;
				y1 = sprite[z].y;
				z1 = sprite[z].z;

				topt = vx*(x1-xs) + vy*(y1-ys);
				if (topt > 0)
				{
					long permute_tmp0;
					permute_tmp0 = vy*vy;
					bot = vx*vx + permute_tmp0;
					if (bot != 0)
					{
						intz = zs+scale(vz,topt,bot);
						daysiz = (tilesizy[sprite[z].picnum]*sprite[z].yrepeat)<<8;
						if ((intz <= z1) && (intz >= z1-(daysiz>>6)))
						{
							topu = vx*(y1-ys) - vy*(x1-xs);

							offx = scale(vx,topu,bot);
							offy = scale(vy,topu,bot);
							dist = offx*offx + offy*offy;
							daxsiz = (tilesizx[sprite[z].picnum]*sprite[z].xrepeat);
							daxsiz *= daxsiz;
							if (dist <= (daxsiz>>7))
							{
								intx = xs + scale(vx,topt,bot);
								inty = ys + scale(vy,topt,bot);

								if (klabs(intx-xs)+klabs(inty-ys) < klabs(xe-xs)+klabs(ye-ys))
								{
									*neartagsprite = z;
									*neartaghitdist = mulscale(intx-xs,sintable[(ange+2560)&2047],14) + mulscale(inty-ys,sintable[(ange+2048)&2047],14);
									xe = intx;
									ye = inty;
									ze = intz;
								}
							}
						}
					}
				}
			}

			z = nextspritesect[z];
		}


		tempshortcnt++;
	}
	return(0);
}

lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3,
			  long y3, long x4, long y4, long *intx, long *inty, long *intz,
			  short datype)
{
	long templong1, templong2, bot, topt;
	short intersect;

	//if datype == 0 then p1 to p2 is a line segment
	//if datype == 1 then p1 towards p2 is a ray

	if (datype == 0)
	{
		if ((x1 < x3) && (x1 < x4) && (x2 < x3) && (x2 < x4)) return(0);
		if ((x1 > x3) && (x1 > x4) && (x2 > x3) && (x2 > x4)) return(0);
		if ((y1 < y3) && (y1 < y4) && (y3 > y2) && (y2 < y4)) return(0);
		if ((y1 > y3) && (y1 > y4) && (y3 < y2) && (y2 > y4)) return(0);
	}

	intersect = 0;
	templong1 = (x3-x1)*(y2-y1) - (y3-y1)*(x2-x1);
	templong2 = (x4-x1)*(y2-y1) - (y4-y1)*(x2-x1);
	if (((templong1<=0) && (templong2>=0)) || ((templong1>=0) && (templong2<=0)))
	{
		templong1 = (x1-x3)*(y4-y3) - (y1-y3)*(x4-x3);
		templong2 = (x2-x3)*(y4-y3) - (y2-y3)*(x4-x3);
		if (((templong1<=0) && (templong2>=0)) || ((templong1>=0) && (templong2<=0)))
			intersect = 1;
		if (datype == 1)
			if (klabs(templong2) < klabs(templong1))
				intersect = 1;
	}

	if (intersect == 1)
	{
		bot = (x2-x1)*(y3-y4) - (x3-x4)*(y2-y1);
		if (bot == 0) return(0);
		topt = (x3-x1)*(y3-y4) - (x3-x4)*(y3-y1);
		*intx = x1 + scale(x2-x1,topt,bot);
		*inty = y1 + scale(y2-y1,topt,bot);
		*intz = z1 + scale(z2-z1,topt,bot);
		return(1);
	}
	return(0);
}

dragpoint(short pointhighlight, long dax, long day)
{
	short cnt, tempshort;

	wall[pointhighlight].x = dax;
	wall[pointhighlight].y = day;

	cnt = MAXWALLS;
	tempshort = pointhighlight;    //search points CCW
	do
	{
		if (wall[tempshort].nextwall1 >= 0)
		{
			tempshort = wall[wall[tempshort].nextwall1].point2;
			wall[tempshort].x = dax;
			wall[tempshort].y = day;
		}
		else
		{
			tempshort = pointhighlight;    //search points CW if not searched all the way around
			do
			{
				if (wall[lastwall(tempshort)].nextwall1 >= 0)
				{
					tempshort = wall[lastwall(tempshort)].nextwall1;
					wall[tempshort].x = dax;
					wall[tempshort].y = day;
				}
				else
				{
					break;
				}
				cnt--;
			}
			while ((tempshort != pointhighlight) && (cnt > 0));
			break;
		}
		cnt--;
	}
	while ((tempshort != pointhighlight) && (cnt > 0));
}

lastwall(short point)
{
	long cnt, tempshort;

	tempshort = point;
	cnt = MAXWALLS;
	while ((wall[tempshort].point2 != point) && (cnt > 0))
	{
		tempshort = wall[tempshort].point2;
		cnt--;
	}
	if (cnt <= 0)
		tempshort = point;
	return(tempshort);
}

void keepaway(long *x, long *y, short clipwall);

clipmove (long *x, long *y, long *z, short *sectnum,
			 long xvect, long yvect,
			 long walldist, long ceildist, long flordist, char cliptype)
{
	long i, j, templong1, templong2;
	long oyvect, oxvect, goalx, goaly, intx, inty, lx, ly, retval;
	short hitwall, cnt, bad;

	retval = 0;
	if ((xvect|yvect) == 0) return(retval);

	oxvect = xvect;
	oyvect = yvect;

	goalx = *x + (xvect>>14);
	goaly = *y + (yvect>>14);

	getcliplines(*x, *y, *z, goalx, goaly, *sectnum, walldist, ceildist, flordist, cliptype);

	hitwall = 0;
	cnt = 3;
	while (((xvect|yvect) != 0) && (hitwall >= 0) && (cnt > 0))
	{
		if ((hitwall = raytrace(*x,*y,xvect,yvect,&intx,&inty)) >= 0)
		{
			lx = clipx2[hitwall]-clipx1[hitwall];
			ly = clipy2[hitwall]-clipy1[hitwall];
			templong2 = lx*lx + ly*ly;
			if (templong2 > 0)
			{
				templong1 = (goalx-intx)*lx + (goaly-inty)*ly;
				i = divscale(templong1,templong2,20);
				xvect = mulscale(lx,i,6);
				yvect = mulscale(ly,i,6);
			}

			templong1 = lx*(oxvect>>6)+ly*(oyvect>>6);
			for(i=cnt+1;i<=3;i++)
			{
				templong2 = (clipx2[hitwalls[i]]-clipx1[hitwalls[i]])*(oxvect>>6) + (clipy2[hitwalls[i]]-clipy1[hitwalls[i]])*(oyvect>>6);
				if ((templong1<0) != (templong2<0)) return(retval);
			}

			goalx = intx + (xvect>>14);
			goaly = inty + (yvect>>14);
			keepaway(&goalx, &goaly, hitwall);
			xvect = ((goalx-intx)<<14);
			yvect = ((goaly-inty)<<14);

			if (cnt == 3) retval = clipobjectval[hitwall];
			hitwalls[cnt] = hitwall;
		}
		*x = intx;
		*y = inty;
		cnt--;
	}

	bad = 1;
	for(j=0;j<clipsectnum;j++)
		if (inside(*x,*y,clipsectorlist[j]) == 1)
		{
			*sectnum = clipsectorlist[j];
			bad = 0;
			break;
		}
	if (bad == 1)
		for(j=0;j<numsectors;j++)
			if (inside(*x,*y,j) == 1)
			{
				*sectnum = j;
				bad = 0;
				break;
			}
	if (bad == 1) *sectnum = -1;
	return(retval);
}

getcliplines(long x, long y, long z, long goalx, long goaly, short sectnum, long walldist, long ceildist, long flordist, char cliptype)
{
	short point2;
	long i, j, k, l, clipsectcnt, startwall, endwall;
	long x1, y1, x2, y2, cx, cy, rad, xmin, ymin, xmax, ymax, daz, daz2;
	long bsz, dax, day, xoff, yoff, xspan, yspan, cosang, sinang, tilenum;
	long xrepeat, yrepeat, gx, gy, dx, dy;
	char cstat, clipyou, bad;

	clipnum = 0;

	cx = ((x+goalx)>>1);
	cy = ((y+goaly)>>1);
		//Extra walldist for sprites on sector lines
	// COMPILER_STEERED: Stage the range calculation to preserve the December extent.
	gy = goaly-y; gx = goalx-x;
	rad = ksqrtasm(gx*gx + gy*gy);
	rad += (walldist<<1) + 8;
	xmin = cx-rad;
	ymin = cy-rad;
	clipsectcnt = 0;
	xmax = cx+rad;
	clipsectnum = 1;
	ymax = cy+rad;
	clipsectorlist[0] = sectnum;

	while (clipsectcnt < clipsectnum)
	{
		startwall = sector[clipsectorlist[clipsectcnt]].wallptr;
		endwall = startwall + sector[clipsectorlist[clipsectcnt]].wallnum-1;
		for(j=startwall;j<=endwall;j++)
		{
			point2 = wall[j].point2;
			x1 = wall[j].x; y1 = wall[j].y; x2 = wall[point2].x; y2 = wall[point2].y;

			dx = x2-x1; dy = y2-y1;
			if ((x2-x)*(y1-y) <= (x1-x)*(y2-y)) //If wall's facing you
			{
				if ((x1 < xmin) && (x2 < xmin)) continue;
				if ((x1 > xmax) && (x2 > xmax)) continue;
				if ((y1 < ymin) && (y2 < ymin)) continue;
				if ((y1 > ymax) && (y2 > ymax)) continue;
				if (dx > 0) dax = dx*(ymin-y1); else dax = dx*(ymax-y1);
				if (dy > 0) day = dy*(xmax-x1); else day = dy*(xmin-x1);
				if (dax-day >= 0) continue;

				clipyou = 0;
				if (wall[j].nextsector1 == -1) clipyou = 1;
				if (((cliptype == 0) || (cliptype == 2)) && ((wall[j].cstat&1) > 0)) clipyou = 1;
				if ((cliptype == 1) && ((wall[j].cstat&64) != 0)) clipyou = 1;
				if (editstatus == 0)
				{
					if ((sector[wall[j].nextsector1].ceilingstat&1) == 0)
						if (z <= sector[wall[j].nextsector1].ceilingz+(ceildist-1)) clipyou = 1;
					if ((sector[wall[j].nextsector1].floorstat&1) == 0)
					{
						if (z >= sector[wall[j].nextsector1].floorz-(flordist-1)) clipyou = 1;
						if ((cliptype == 2) && (sector[wall[j].nextsector1].floorz > z+scaredfallz)) clipyou = 1;
					}
				}

				if (clipyou == 1)
				{
						//Add 2 boxes at endpoints
					bsz = walldist; if (goalx < x) bsz = -bsz;
					addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(short)j+32768);
					addclipline(x2-bsz,y2-bsz,x2-bsz,y2+bsz,point2+32768);
					bsz = walldist; if (goaly < y) bsz = -bsz;
					addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(short)j+32768);
					addclipline(x2+bsz,y2-bsz,x2-bsz,y2-bsz,point2+32768);

					dax = walldist; if (dy > 0) dax = -dax;
					day = walldist; if (dx < 0) day = -day;
					addclipline(x1+dax,y1+day,x2+dax,y2+day,(short)j+32768);
				}
				else
				{
					bad = 0;
					for(i=clipsectnum-1;i>=0;i--)
						if (wall[j].nextsector1 == clipsectorlist[i]) { bad = 1; break; }
					if (bad == 0) clipsectorlist[clipsectnum++] = wall[j].nextsector1;
				}
			}
		}
		clipsectcnt++;
	}

	for(i=0;i<clipsectnum;i++)
	{
		j = headspritesect[clipsectorlist[i]];
		while (j != -1)
		{
			cstat = sprite[j].cstat;
			if ((cstat&1) > 0)
			{
				x1 = sprite[j].x; y1 = sprite[j].y;

				switch(cstat&48)
				{
					case 0:
						if ((x1 >= xmin-rad) && (x1 <= xmax+rad) && (y1 >= ymin-rad) && (y1 <= ymax+rad))
						{
							if ((z <= sprite[j].z+ceildist) && (z >= sprite[j].z-flordist-((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2)))
							{
								bsz = walldist<<1; if (goalx < x) bsz = -bsz;
								addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(short)j+49152);
								bsz = walldist<<1; if (goaly < y) bsz = -bsz;
								addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(short)j+49152);
							}
						}
						break;
					case 16:
						if ((z <= sprite[j].z+ceildist) && (z >= sprite[j].z-flordist-((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2)))
						{
								//These lines get the 2 points of the rotated sprite
								//Given: (x1, y1) starts out as the center point
							tilenum = sprite[j].picnum;
							xoff = (long)((signed char)((picanm[tilenum]>>8)&255));
							if ((cstat&4) > 0) xoff = -xoff;
							k = sprite[j].ang; l = sprite[j].xrepeat;
							dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
							l = tilesizx[tilenum]; k = (l>>1)+xoff;
							x1 -= mulscale(dax,k,16); x2 = x1+mulscale(dax,l,16);
							y1 -= mulscale(day,k,16); y2 = y1+mulscale(day,l,16);
							if (clipinsideboxline(cx,cy,x1,y1,x2,y2,rad) > 0)
							{
								bsz = walldist; if (goalx < x) bsz = -bsz;
								addclipline(x1-bsz,y1-bsz,x1-bsz,y1+bsz,(short)j+49152);
								addclipline(x2-bsz,y2-bsz,x2-bsz,y2+bsz,(short)j+49152);
								bsz = walldist; if (goaly < y) bsz = -bsz;
								addclipline(x1+bsz,y1-bsz,x1-bsz,y1-bsz,(short)j+49152);
								addclipline(x2+bsz,y2-bsz,x2-bsz,y2-bsz,(short)j+49152);

								if ((x1-x)*(y2-y) < (x2-x)*(y1-y))
								{
									dax = x1; x1 = x2; x2 = dax;
									day = y1; y1 = y2; y2 = day;
								}
								dax = walldist; if (y2 > y1) dax = -dax;
								day = walldist; if (x2 < x1) day = -day;
								addclipline(x1+dax,y1+day,x2+dax,y2+day,(short)j+49152);
							}
						}
						break;
					case 32:
						daz = sprite[j].z+ceildist;
						daz2 = sprite[j].z-flordist;
						if ((z <= daz) && (z >= daz2))
						{
														tilenum = sprite[j].picnum;
							xoff = (long)((signed char)((picanm[tilenum]>>8)&255));
							yoff = (long)((signed char)((picanm[tilenum]>>16)&255));
							if ((cstat&4) > 0) xoff = -xoff;
							if ((cstat&8) > 0) yoff = -yoff;

							k = sprite[j].ang;
							cosang = sintable[(k+512)&2047]; sinang = sintable[k];
							xspan = tilesizx[tilenum]; xrepeat = sprite[j].xrepeat;
							yspan = tilesizy[tilenum]; yrepeat = sprite[j].yrepeat;

							dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
							rxi[0] = x1 + doublemulscale(sinang,dax,cosang,day,16);
							ryi[0] = y1 + doublemulscale(sinang,day,-cosang,dax,16);
							l = xspan*xrepeat;
							rxi[1] = rxi[0] - mulscale(sinang,l,16);
							ryi[1] = ryi[0] + mulscale(cosang,l,16);
							l = yspan*yrepeat;
							k = -mulscale(cosang,l,16); rxi[2] = rxi[1]+k; rxi[3] = rxi[0]+k;
							k = -mulscale(sinang,l,16); ryi[2] = ryi[1]+k; ryi[3] = ryi[0]+k;

							for(k=0;k<4;k++)
							{
								l = (k+1)&3;
								if ((x-rxi[k])*(ryi[l]-ryi[k]) <= (y-ryi[k])*(rxi[l]-rxi[k])) continue;
								if (clipinsideboxline(cx,cy,rxi[k],ryi[k],rxi[l],ryi[l],rad) <= 0) continue;

								bsz = walldist; if (goalx < x) bsz = -bsz;
								addclipline(rxi[k]-bsz,ryi[k]-bsz,rxi[k]-bsz,ryi[k]+bsz,(short)j+49152);
								addclipline(rxi[l]-bsz,ryi[l]-bsz,rxi[l]-bsz,ryi[l]+bsz,(short)j+49152);
								bsz = walldist; if (goaly < y) bsz = -bsz;
								addclipline(rxi[k]+bsz,ryi[k]-bsz,rxi[k]-bsz,ryi[k]-bsz,(short)j+49152);
								addclipline(rxi[l]+bsz,ryi[l]-bsz,rxi[l]-bsz,ryi[l]-bsz,(short)j+49152);

								dax = walldist; if (ryi[l] < ryi[k]) dax = -dax;
								day = walldist; if (rxi[k] < rxi[l]) day = -day;
								addclipline(rxi[l]+dax,ryi[l]+day,rxi[k]+dax,ryi[k]+day,(short)j+49152);
							}
						}
						break;
				}
			}
			j = nextspritesect[j];
		}
	}
}

addclipline(long x1, long y1, long x2, long y2, short oval)
{
	clipx1[clipnum] = x1; clipy1[clipnum] = y1;
	clipx2[clipnum] = x2; clipy2[clipnum] = y2;
	clipobjectval[clipnum] = oval;  //Add 32768-walls, 49152-sprites
	clipnum++;
}

void keepaway(long *x, long *y, short clipwall)
{
	short first;
	long bakx, baky, daside;

	bakx = 0; baky = 0;
	if (clipx2[clipwall] > clipx1[clipwall]) baky = 1;
	if (clipx2[clipwall] < clipx1[clipwall]) baky = -1;
	if (clipy2[clipwall] > clipy1[clipwall]) bakx = -1;
	if (clipy2[clipwall] < clipy1[clipwall]) bakx = 1;

	if (klabs(clipx2[clipwall]-clipx1[clipwall]) > klabs(clipy2[clipwall]-clipy1[clipwall]))
		first = 0;
	else
		first = 1;

	daside = (clipx2[clipwall]-clipx1[clipwall])*(*y-clipy1[clipwall])-(*x-clipx1[clipwall])*(clipy2[clipwall]-clipy1[clipwall]);
	while (daside <= 0)
	{
		if (first == 0)
			*x += bakx;
		else
			*y += baky;
		first ^= 1;
		daside = (clipx2[clipwall]-clipx1[clipwall])*(*y-clipy1[clipwall])-(*x-clipx1[clipwall])*(clipy2[clipwall]-clipy1[clipwall]);
	}

}

raytrace (long x, long y, long xvect, long yvect, long *intx, long *inty)
{
	short z, hitwall;
	// COMPILER_STEERED: Order the first endpoint locals for Watcom's stack homes.
	long y1, x2, x3, x4, x1, y2, y3, y4, dx, dy, bot, topu, nintx, ninty;
	long templong1, templong2, templong3, templong4, cnt;

	x3 = x; y3 = y;
	x4 = x3+(xvect>>14); y4 = y3+(yvect>>14);
	*intx = x4; *inty = y4;
	z = 0; hitwall = -1;

	for (z=0;z<clipnum;z++)
	{
		x1 = clipx1[z]; y1 = clipy1[z];
		x2 = clipx2[z]; y2 = clipy2[z];

		dx = x2-x1; dy = y2-y1;
		templong1 = dx*(y3-y1) - (x3-x1)*dy;
		templong2 = dx*(y4-y1) - (x4-x1)*dy;
		if ((templong1 >= 0) && (templong2 < 0))
		{
			templong3 = (x4-x3)*(y1-y3) - (x1-x3)*(y4-y3);
			templong4 = (x4-x3)*(y2-y3) - (x2-x3)*(y4-y3);
			if ((templong3 <= 0) && (templong4 > 0))
			{
				bot = (x2-x1)*(y3-y4) - (x3-x4)*(y2-y1);
				if (bot != 0)
				{
					topu = (x2-x1)*(y3-y1) - (x3-x1)*(y2-y1);
					cnt = 256;
					do
					{
						nintx = x3 + scale(x4-x3,topu,bot);
						ninty = y3 + scale(y4-y3,topu,bot);
						templong2 = (x2-x1)*(ninty-y1) - (nintx-x1)*(y2-y1);
						cnt--;
						topu--;
						if (cnt < 0)
						{
							*intx = x3; *inty = y3;
							return(z);
						}
					} while (templong2 <= 0);
					if (klabs(x3-nintx)+klabs(y3-ninty) < klabs(x3 - *intx)+klabs(y3 - *inty))
					{
						*intx = nintx;
						*inty = ninty;
						hitwall = z;
					}
				}
			}
		}
	}

	return(hitwall);
}

updatesector(long x, long y, short *sectnum)
{
	short i, j, startwall, endwall;

	if (inside(x,y,*sectnum) == 1) return;

	if ((*sectnum >= 0) && (*sectnum < numsectors))
	{
		startwall = sector[*sectnum].wallptr;
		endwall = startwall+sector[*sectnum].wallnum-1;
		for(j=startwall;j<=endwall;j++)
		{
			i = wall[j].nextsector1;
			if (i >= 0)
				if (inside(x,y,i) == 1)
				{
					*sectnum = i;
					return;
				}
		}
	}

	for(i=0;i<numsectors;i++)
		if (inside(x,y,i) == 1)
		{
			*sectnum = i;
			return;
		}

	*sectnum = -1;
}

rotatepoint(long xpivot, long ypivot, long x, long y, short daang, long *x2, long *y2)
{
	long dacos, dasin;

	dacos = sintable[(daang+2560)&2047];
	dasin = sintable[(daang+2048)&2047];
	x -= xpivot;
	y -= ypivot;
	*x2 = doublemulscale(x,dacos,-y,dasin,14) + xpivot;
	*y2 = doublemulscale(y,dacos,x,dasin,14) + ypivot;
}

getmousevalues(short *mousx, short *mousy, short *bstatus)
{
	readmousexy(mousx,mousy);
	readmousebstatus(bstatus);
}

printscreeninterrupt()
{
	int5();
}

drawline16(long x1, long y1, long x2, long y2, char col)
{
	long i, dx, dy, p, pinc, d, leftbyte, rightbyte;
	char lmask, rmask;

	if ((x1 < 0) && (x2 < 0)) return;
	if ((x1 >= 640) && (x2 >= 640)) return;
	if ((y1 < 0) && (y2 < 0)) return;
	if ((y1 >= ydim16) && (y2 >= ydim16)) return;

	setcolor16((long)col);
	if (x1 == x2)
	{
		if (y2 < y1) i = y1, y1 = y2, y2 = i;
		if (y1 < 0) y1 = 0;
		if (y2 >= ydim16) y2 = ydim16-1;
		p = ((y1*640+x1+pageoffset)>>3)+0xa0000;
		koutp(0x3ce,8);
		koutp(0x3cf,1<<((x1&7)^7));
		vlin16(p,y2-y1+1);
		return;
	}
	if (y1 == y2)
	{
		if (x2 < x1) i = x1, x1 = x2, x2 = i;
		if (x1 < 0) x1 = 0;
		if (x2 >= 640) x2 = 639;
		lmask = 0xff>>(x1&7);
		rmask = 0xff80>>(x2&7);
		p = (y1*640+x1+pageoffset)>>3;

		leftbyte = x1|7;
		rightbyte = x2|7;
		koutp(0x3ce,8);
		p += 0xa0000;
		if (leftbyte == rightbyte)
			rmask &= lmask;
		else
		{
			koutp(0x3cf,lmask);
			drawpixel(p,readpixel(p));
			p++;
			dx = ((rightbyte-leftbyte)>>3)-1;
			koutp(0x3cf,0xff);
			if (dx > 0) p = hlin16(p,dx);
		}
		koutp(0x3cf,rmask);
		drawpixel(p,readpixel(p));
		return;
	}

	if ((x1 < 0) && (x1 != x2))
		{ y1 -= ((y2-y1)*x1)/(x2-x1); x1 = 0; }
	if ((x2 < 0) && (x1 != x2))
		{ y2 -= ((y2-y1)*x2)/(x2-x1); x2 = 0; }
	if ((y1 < 0) && (y1 != y2))
		{ x1 -= ((x2-x1)*y1)/(y2-y1); y1 = 0; }
	if ((y2 < 0) && (y1 != y2))
		{ x2 -= ((x2-x1)*y2)/(y2-y1); y2 = 0; }
	if ((x1 >= 640) && (x1 != x2))
		{ y1 += ((y2-y1)*(639-x1))/(x2-x1); x1 = 639; }
	if ((x2 >= 640) && (x1 != x2))
		{ y2 += ((639-x2)*(y2-y1))/(x2-x1); x2 = 639; }
	if ((y1 >= ydim16) && (y1 != y2))
		{ x1 += ((x2-x1)*(ydim16-1-y1))/(y2-y1); y1 = ydim16-1; }
	if ((y2 >= ydim16) && (y1 != y2))
		{ x2 += ((ydim16-1-y2)*(x2-x1))/(y2-y1); y2 = ydim16-1; }

	if ((x1 < 0) && (x2 < 0)) return;
	if ((x1 >= 640) && (x2 >= 640)) return;
	if ((y1 < 0) && (y2 < 0)) return;
	if ((y1 >= ydim16) && (y2 >= ydim16)) return;

	if (x2 > x1) dx = x2-x1; else dx = x1-x2;
	dx++;
	if (y2 > y1) dy = y2-y1; else dy = y1-y2;
	dy++;
	if (dx >= dy)
	{
		if (x2 < x1)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}
		p = y1*640+x1+pageoffset;
		d = 0;
		if (y2 > y1) pinc = 640; else pinc = -640;
		for(i=dx;i>0;i--)
		{
			drawpixel16(p);
			d += dy;
			if (d >= dx) { d -= dx; p += pinc; }
			p++;
		}
	}
	if (dx < dy)
	{
		if (y2 < y1)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}
		p = y1*640+x1+pageoffset;
		d = 0;
		if (x2 > x1) pinc = 1; else pinc = -1;
		for(i=dy;i>0;i--)
		{
			drawpixel16(p);
			d += dx;
			if (d >= dy) { d -= dy; p += pinc; }
			p += 640;
		}
	}
}

qsetmode640350()
{
	if (qsetmode != 350)
	{
		if (stereofps != 0) stereofps = 1;

		setvmode(0x10);

		pageoffset = 0;
		ydim16 = 350;
		koutp(0x3d4,0xc); koutp(0x3d5,pageoffset>>11);

		koutp(0x3ce,0); koutp(0x3cf,15);  //set/reset
		koutp(0x3ce,1); koutp(0x3cf,15);  //enable set/reset
		fillscreen16(0L,0L,640L*350L);
	}
	qsetmode = 350;
}

qsetmode640480()
{
	short i;

	if (qsetmode != 480)
	{
		if (stereofps != 0) stereofps = 1;

		setvmode(0x12);

		i = 479-144;
		koutp(0x3d4,0x18); koutp(0x3d5,i&255);             //line compare
		koutp(0x3d4,0x7); koutp(0x3d5,(kinp(0x3d5)&239)|((i&256)>>4));
		koutp(0x3d4,0x9); koutp(0x3d5,(kinp(0x3d5)&191)|((i&512)>>3));

		pageoffset = 92160;
		koutp(0x3d4,0xc); koutp(0x3d5,pageoffset>>11);

		koutp(0x3ce,0); koutp(0x3cf,15);  //set/reset
		koutp(0x3ce,1); koutp(0x3cf,15);  //enable set/reset
		fillscreen16(0L,8L,640L*144L);
		fillscreen16((640L*144L)>>3,0L,640L*336L);
		pageoffset = 92160; ydim16 = 336;
	}

	qsetmode = 480;
}

clear2dscreen()
{
	long p, count;

	p = pageoffset>>3;
	if (qsetmode == 350)
		count = 640L*350L;
	else if (qsetmode == 480)
		count = 640L*336L;
	else
		return;
	fillscreen16(p,0L,count);
}

draw2dgrid(long posxe, long posye, short ange, long zoome, short gride)
{
	long i, xp1, yp1, xp2, yp2, tempy, templong;
	char mask;

	if (gride > 0)
	{
		yp1 = 200-mulscale(posye+65536,zoome,14);
		if (yp1 < 0) yp1 = 0;
		yp2 = 200-mulscale(posye-65536,zoome,14);
		if (yp2 >= ydim16) yp2 = ydim16-1;

		if ((yp1 < ydim16) && (yp2 >= 0) && (yp2 >= yp1))
		{
			setcolor16(8);
			koutp(0x3ce,0x8);

			templong = ((yp1*640+pageoffset)>>3)+0xa0000;
			tempy = yp2-yp1+1;
			mask = 0;
			xp1 = 320-mulscale(posxe+65536,zoome,14);

			for(i=-65536;i<=65536;i+=(2048>>gride))
			{
				xp2 = xp1;
				xp1 = 320-mulscale(posxe-i,zoome,14);

				if (xp1 >= 640) break;
				if (xp1 >= 0)
				{
					if ((xp1|7) != (xp2|7))
					{
						koutp(0x3cf,mask);
						if (((xp2>>3) >= 0) && ((xp2>>3) < 80))
							vlin16first(templong+(xp2>>3),tempy);
						mask = 0;
					}
					mask |= (1<<(xp1&7^7));
				}
			}
			if ((i >= 65536) && (xp1 < 640))
				xp2 = xp1;
			if ((mask != 0) && ((xp2>>3) >= 0) && ((xp2>>3) < 80))
			{
				koutp(0x3cf,mask);
				vlin16first(templong+(xp2>>3),tempy);
			}
		}

		xp1 = mulscale(posxe+65536,zoome,14);
		xp2 = mulscale(posxe-65536,zoome,14);
		tempy = 0x80000000;
		for(i=-65536;i<=65536;i+=(2048>>gride))
		{
			yp1 = (((posye-i)*zoome)>>14);
			if (yp1 != tempy)
			{
				if ((yp1 > -200) && (yp1 <= 200))
				{
					drawline16(320-xp1,200-yp1,320-xp2,200-yp1,8);
					tempy = yp1;
				}
			}
		}
	}
}

draw2dscreen(long posxe, long posye, short ange, long zoome, short gride)
{
	long i, j, xp1, yp1, xp2, yp2, templong, scrx1, scry1;
	char col;
	short spritenum;

	if (qsetmode == 200) return;

	if (editstatus == 0)
	{

		clear2dscreen();

		draw2dgrid(posxe,posye,ange,zoome,gride);
	}

	for(i=0;i<numwalls;i++)
	{
		if (editstatus == 0)
		{
			if ((show2dwall[i>>3]&(1<<(i&7))) == 0) continue;
			j = wall[i].nextwall1;
			if ((j >= 0) && (i > j))
				if ((show2dwall[j>>3]&(1<<(j&7))) > 0) continue;
		}
		else
		{
			j = wall[i].nextwall1;
			if ((j >= 0) && (i > j)) continue;
		}

		if (wall[i].nextwall1 == -1)
		{
			col = 7;
			if (i == linehighlight) col += ((numframes&2)<<2);
		}
		else
		{
			col = 4;
			if ((wall[i].cstat&1) != 0) col = 5;
			if ((wall[i].cstat&64) != 0) col = 133;
			if ((i == linehighlight) || ((linehighlight >= 0) && (i == wall[linehighlight].nextwall1)))
				col += ((numframes&2)<<2);
		}

		xp1 = mulscale(wall[i].x-posxe,zoome,14);
		yp1 = mulscale(wall[i].y-posye,zoome,14);
		xp2 = mulscale(wall[wall[i].point2].x-posxe,zoome,14);
		yp2 = mulscale(wall[wall[i].point2].y-posye,zoome,14);

		if ((unsigned char)col > 15)
		{
			drawline16(320+xp1+1,200+yp1,320+xp2+1,200+yp2,col-128);
			drawline16(320+xp1-1,200+yp1,320+xp2-1,200+yp2,col-128);
			drawline16(320+xp1,200+yp1+1,320+xp2,200+yp2+1,col-128);
			drawline16(320+xp1,200+yp1-1,320+xp2,200+yp2-1,col-128);
			col = 13;
		}
		drawline16(320+xp1,200+yp1,320+xp2,200+yp2,col);

		if ((zoome >= 256) && (editstatus == 1))
			if (((320+xp1) >= 2) && ((320+xp1) <= 637))
				if (((200+yp1) >= 2) && ((200+yp1) <= ydim16-3))
				{
					col = 2;
					if (i == pointhighlight) col += ((numframes&2)<<2);
					else if (highlightcnt > 0)
					{
						if ((show2dwall[i>>3]&(1<<(i&7))) > 0)
							col += ((numframes&2)<<2);
					}

					templong = (200+yp1)*640+(320+xp1)+pageoffset;
					setcolor16((long)col);
					drawpixel16(templong-2-1280);
					drawpixel16(templong-1-1280);
					drawpixel16(templong+0-1280);
					drawpixel16(templong+1-1280);
					drawpixel16(templong+2-1280);

					drawpixel16(templong-2+1280);
					drawpixel16(templong-1+1280);
					drawpixel16(templong+0+1280);
					drawpixel16(templong+1+1280);
					drawpixel16(templong+2+1280);

					drawpixel16(templong-2-640);
					drawpixel16(templong-2+0);
					drawpixel16(templong-2+640);

					drawpixel16(templong+2-640);
					drawpixel16(templong+2+0);
					drawpixel16(templong+2+640);
				}
	}


	if ((zoome >= 256) || (editstatus == 0))
		for(i=0;i<numsectors;i++)
			for(spritenum=headspritesect[i];spritenum!=-1;spritenum=nextspritesect[spritenum])
				if ((editstatus == 1) || ((show2dsprite[spritenum>>3]&(1<<(spritenum&7))) > 0))
				{
					col = 3;
					if ((sprite[spritenum].cstat&1) > 0) col = 5;
					if (editstatus == 1)
					{
						if (spritenum+MAXSPRITES == pointhighlight)
							col += ((numframes&2)<<2);
						else if (highlightcnt > 0)
						{
							if ((show2dsprite[spritenum>>3]&(1<<(spritenum&7))) > 0)
								col += ((numframes&2)<<2);
						}
					}

					xp1 = mulscale(sprite[spritenum].x-posxe,zoome,14);
					yp1 = mulscale(sprite[spritenum].y-posye,zoome,14);
					scrx1 = 320+xp1; scry1 = 200+yp1;
					if (((scrx1) >= 2) && ((scrx1) <= 637))
						if (((scry1) >= 2) && ((scry1) <= ydim16-3))
						{
							templong = (scry1)*640+(scrx1)+pageoffset;
							setcolor16((long)col);
							drawpixel16(templong-1-1280);
							drawpixel16(templong+0-1280);
							drawpixel16(templong+1-1280);

							drawpixel16(templong-1+1280);
							drawpixel16(templong+0+1280);
							drawpixel16(templong+1+1280);

							drawpixel16(templong-2-640);
							drawpixel16(templong-2+0);
							drawpixel16(templong-2+640);

							drawpixel16(templong+2-640);
							drawpixel16(templong+2+0);
							drawpixel16(templong+2+640);

							drawpixel16(templong+1+640);
							drawpixel16(templong-1+640);
							drawpixel16(templong+1-640);
							drawpixel16(templong-1-640);

							xp2 = mulscale(sintable[(sprite[spritenum].ang+2560)&2047],zoome,11) / 768;
							yp2 = mulscale(sintable[(sprite[spritenum].ang+2048)&2047],zoome,11) / 768;

							drawline16(scrx1,scry1,scrx1+xp2,scry1+yp2,col);
						}
				}

	xp1 = mulscale(sintable[(ange+2560)&2047],zoome,11) / 768; //Draw white arrow
	yp1 = mulscale(sintable[(ange+2048)&2047],zoome,11) / 768;
	drawline16(320+xp1,200+yp1,320-xp1,200-yp1,15);
	drawline16(320+xp1,200+yp1,320+yp1,200-xp1,15);
	drawline16(320+xp1,200+yp1,320-yp1,200+xp1,15);
}



printext16(long xpos, long ypos, short col, short backcol, char name[82], char fontsize)
{
	long p, z, zz, charxsiz, daxpos;
	short mask;
	unsigned char ch, dat;
	char *fontptr;

	daxpos = xpos;

	koutp(0x3ce,0x5); koutp(0x3cf,(kinp(0x3cf)&(255-3))+2);
	koutp(0x3ce,0x8);

	if (fontsize == 1)
	{
		fontptr = smalltextfont;
		charxsiz = 4;
	}
	else
	{
		fontptr = textfont;
		charxsiz = 8;
	}

	z = 0;
	while (name[z] != 0)
	{
		ch = name[z];
		z++;

		mask = (1<<(8-(daxpos&7)))-1;
		p = daxpos>>3;
		p += mul80(ypos);
		p += 0xa0000;

		if ((daxpos&7) == 0)
		{
			for(zz=0;zz<8;zz++)
			{
				if (backcol >= 0)
				{
					koutp(0x3cf,0xff);
					if (charxsiz == 4) koutp(0x3cf,0x7c);
					readpixel(p), drawpixel(p,(long)backcol);
				}
				koutp(0x3cf,fontptr[(((long)ch)<<3)+zz]&255);
				if (charxsiz == 4) koutp(0x3cf,(0x7c&fontptr[(((long)ch)<<3)+zz])&255);
				readpixel(p), drawpixel(p,col);
				p += 80;
			}
		}
		else
		{
			for(zz=0;zz<8;zz++)
			{
				if (backcol >= 0)
				{
					if (charxsiz == 8)
					{
						koutp(0x3cf,mask);
						readpixel(p), drawpixel(p,backcol);
						koutp(0x3cf,0xff-mask);
						readpixel(p+1), drawpixel(p+1,backcol);
					}
					else
					{
						koutp(0x3cf,0x7c>>(daxpos&7));
						readpixel(p), drawpixel(p,backcol);
						koutp(0x3cf,0x7c<<(8-(daxpos&7)));
						readpixel(p+1), drawpixel(p+1,backcol);
					}
				}
				dat = fontptr[(((long)ch)<<3)+zz];
				if (charxsiz == 8)
				{
					koutp(0x3cf,mask&(dat>>(daxpos&7)));
					readpixel(p), drawpixel(p,col);
					koutp(0x3cf,(dat<<(8-(daxpos&7)))&(255-mask));
					readpixel(p+1), drawpixel(p+1,col);
				}
				else
				{
					koutp(0x3cf,(0x7c&dat)>>(daxpos&7));
					readpixel(p), drawpixel(p,col);
					koutp(0x3cf,(0x7c&dat)<<(8-(daxpos&7)));
					readpixel(p+1), drawpixel(p+1,col);
				}
				p += 80;    //Do not make ylookup[1]!
			}
		}

		daxpos += charxsiz;
	}
	koutp(0x3ce,0x5); koutp(0x3cf,(kinp(0x3cf)&(255-3))+0);
}

printmessage16(char name[82])
{
	char snotbuf[60];
	long i;

	i = 0;
	while ((name[i] != 0) && (i < 56))
	{
		snotbuf[i] = name[i];
		i++;
	}
	while (i < 56)
	{
		snotbuf[i] = 32;
		i++;
	}
	snotbuf[56] = 0;

	printext16(184, 8, 0, 6, snotbuf, 0);
}

getspritescreencoord(short spritenum, long *scrx, long *scry)
{
	long scaleval;

	scaleval = divscale(131072L,spritey[spritenum],18);
	*scrx = spritex[spritenum]>>8;
	*scry = sprite[thesprite[spritenum]].z-globalposz;
	*scry -= (tilesizy[spritepicnum[spritenum]]*sprite[thesprite[spritenum]].yrepeat)<<1;
	*scry = globalhoriz + mulscale(scaleval,*scry,24);
}

printext256(long xpos, long ypos, short col, short backcol, char name[82], char fontsize)
{
	long stx, zx, x, y, charxsiz;
	short dat, dacol;
	char *fontptr;

	stx = xpos;
	if (fontsize == 1)
	{
		fontptr = smalltextfont;
		charxsiz = 4;
	}
	else
	{
		fontptr = textfont;
		charxsiz = 8;
	}

	for(zx=0;zx<strlen(name);zx++)
	{
		dat = name[zx];

		for(y=0;y<8;y++)
			for(x=0;x<charxsiz;x++)
			{
				if ((fontptr[(dat<<3)+y]&(1<<(7-x))) > 0)
					dacol = (short)col;
				else if (backcol >= 0)
					dacol = (short)backcol;
				else
					dacol = -1;

				if (dacol >= 0)
				{
					if (option[0] == 0)
					{
						koutp(0x3c4,2); koutp(0x3c5,1<<((x+stx)&3));
						drawpixel(((x+stx)>>2)+(y+ypos)*80+(page<<14)+0xa0000,(long)dacol);
					}
					else if (option[0] != 7)
						drawpixel((y+ypos)*320+(x+stx)+frameplace,(long)dacol);
					else
						drawpixel((y+ypos)*640+(x+stx)+frameplace,(long)dacol);
				}
			}

		stx += charxsiz;
	}
}

krand()
{
	randomseed = ((randomseed*21+1)&65535);
	return(randomseed);
}

getzrange(long x, long y, long z, short sectnum,
			 long *ceilz, long *ceilhit, long *florz, long *florhit,
			 long walldist, char cliptype)
{
	long clipsectcnt, startwall, endwall, tilenum, xoff, yoff, dax, day;
	long xmin, ymin, xmax, ymax, i, j, k, l, daz, daz2, dx, dy;
	long x1, y1, x2, y2, x3, y3, x4, y4, ang, cosang, sinang;
	long xspan, yspan, xrepeat, yrepeat;
	char cstat;

		//Extra walldist for sprites on sector lines
	dx = (walldist<<1)+1;
	xmin = x-dx; ymin = y-dx;
	xmax = x+dx; ymax = y+dx;

	*ceilz = sector[sectnum].ceilingz;
	*florz = sector[sectnum].floorz;
	*ceilhit = sectnum+16384; *florhit = sectnum+16384;

	// Seed the queue before starting the wall scan.
	clipsectcnt = 0; clipsectnum = 0;
	clipsectorlist[0] = sectnum; clipsectnum = 1;

	while (clipsectcnt < clipsectnum) //Collect sectors inside your square first
	{
		startwall = sector[clipsectorlist[clipsectcnt]].wallptr;
		endwall = startwall + sector[clipsectorlist[clipsectcnt]].wallnum;
		endwall--;
		for(j=startwall;j<=endwall;j++)
		{
			if (wall[j].nextsector1 >= 0)
			{
				x1 = wall[j].x; x2 = wall[wall[j].point2].x;
				y1 = wall[j].y; y2 = wall[wall[j].point2].y;
				if ((x1 < xmin) && (x2 < xmin)) continue;
				if ((x1 > xmax) && (x2 > xmax)) continue;
				if ((y1 < ymin) && (y2 < ymin)) continue;
				if ((y1 > ymax) && (y2 > ymax)) continue;

				dx = x2-x1; dy = y2-y1;
				if ((x1-x)*(y2-y) < (x2-x)*(y1-y)) continue; //back
				if (dx > 0) dax = dx*(ymin-y1); else dax = dx*(ymax-y1);
				if (dy > 0) day = dy*(xmax-x1); else day = dy*(xmin-x1);
				if (dax-day >= 0) continue;

				if (wall[j].nextsector1 == -1) continue;
				if (((cliptype == 0) || (cliptype == 2)) && ((wall[j].cstat&1) != 0)) continue;
				if ((cliptype == 1) && ((wall[j].cstat&64) != 0)) continue;
				if (editstatus == 0)
				{
					if ((sector[wall[j].nextsector1].ceilingstat&1) == 0)
						if (z <= sector[wall[j].nextsector1].ceilingz+(3<<8)) continue;
					if ((sector[wall[j].nextsector1].floorstat&1) == 0)
						{
							if (z >= sector[wall[j].nextsector1].floorz-(3<<8)) continue;
							if ((cliptype == 2) && (sector[wall[j].nextsector1].floorz > z+scaredfallz)) continue;
						}
				}

				for(i=clipsectnum-1;i>-1;i--)
					if (clipsectorlist[i] == wall[j].nextsector1) break;
				if (i > -1) continue;
				k = wall[j].nextsector1;
				clipsectorlist[clipsectnum] = wall[j].nextsector1;
				clipsectnum++;

				if ((x1 < xmin+walldist) && (x2 < xmin+walldist)) continue;
				if ((x1 > xmax-walldist) && (x2 > xmax-walldist)) continue;
				if ((y1 < ymin+walldist) && (y2 < ymin+walldist)) continue;
				if ((y1 > ymax-walldist) && (y2 > ymax-walldist)) continue;
				if ((x1-x)*(y2-y) < (x2-x)*(y1-y)) continue;
				if (x2 > x1) dax = (ymin+walldist-y1)*(x2-x1); else dax = (ymax-walldist-y1)*(x2-x1);
				if (y2 > y1) day = (xmax-walldist-x1)*(y2-y1); else day = (xmin+walldist-x1)*(y2-y1);
				if (dax-day >= 0) continue;

					//It actually got here, through all the continue's!!!
				if (sector[k].ceilingz > *ceilz)
					{ *ceilz = sector[k].ceilingz; *ceilhit = k+16384; }
				if (sector[k].floorz < *florz)
					{ *florz = sector[k].floorz; *florhit = k+16384; }
			}
		}
		clipsectcnt++;
	}

	for(i=0;i<clipsectnum;i++)
	{
		j = headspritesect[clipsectorlist[i]];
		while (j != -1)
		{
			cstat = sprite[j].cstat;
			if ((cstat&1) > 0)
			{
				x1 = sprite[j].x; y1 = sprite[j].y;

				if ((cstat&48) == 0)
				{
					if ((x1 >= xmin) && (x1 <= xmax) && (y1 >= ymin) && (y1 <= ymax))
					{
						daz = sprite[j].z+1024;
						daz2 = sprite[j].z-1024 - ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2);
face_wall_range:
						if ((z > daz) && (daz > *ceilz))
							{ *ceilz = daz; *ceilhit = j+49152; }
						if ((z < daz2) && (daz2 < *florz))
							{ *florz = daz2; *florhit = j+49152; }
					}
				}
				else if ((cstat&48) == 16)
				{
					tilenum = sprite[j].picnum;
					xoff = (long)((signed char)((picanm[tilenum]>>8)&255));
					if ((cstat&4) > 0) xoff = -xoff;
					k = sprite[j].ang; l = sprite[j].xrepeat;
					dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
					l = tilesizx[tilenum]; k = (l>>1)+xoff;
					x1 -= mulscale(dax,k,16); x2 = x1+mulscale(dax,l,16);
					y1 -= mulscale(day,k,16); y2 = y1+mulscale(day,l,16);
					if (clipinsideboxline(x,y,x1,y1,x2,y2,walldist+1) > 0)
					{
						daz = sprite[j].z+1024;
						daz2 = sprite[j].z-1024 - ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<2);
						goto face_wall_range;
					}
				}
				else if ((cstat&48) == 32)
				{
					long floor_x1 = x1, floor_y1 = y1, floor_x2, floor_y2;
					long floor_day;
					tilenum = sprite[j].picnum;
					xoff = (long)((signed char)((picanm[tilenum]>>8)&255));
					yoff = (long)((signed char)((picanm[tilenum]>>16)&255));
					if ((cstat&4) > 0) xoff = -xoff;
					if ((cstat&8) > 0) yoff = -yoff;

					ang = sprite[j].ang;
					cosang = sintable[(ang+512)&2047]; sinang = sintable[ang];
					xspan = tilesizx[tilenum]; xrepeat = sprite[j].xrepeat;
					yspan = tilesizy[tilenum]; yrepeat = sprite[j].yrepeat;

					dax = ((xspan>>1)+xoff)*xrepeat; floor_day = ((yspan>>1)+yoff)*yrepeat;
					floor_x1 += doublemulscale(sinang,dax,cosang,floor_day,16);
					floor_y1 += doublemulscale(sinang,floor_day,-cosang,dax,16);
					l = xspan*xrepeat;
					floor_x2 = floor_x1 - mulscale(sinang,l,16);
					floor_y2 = floor_y1 + mulscale(cosang,l,16);
					l = yspan*yrepeat;
					k = -mulscale(cosang,l,16); x3 = floor_x2+k; x4 = floor_x1+k;
					k = -mulscale(sinang,l,16); y3 = floor_y2+k; y4 = floor_y1+k;

					l = walldist+1;
					if (clipinsideboxline(x,y,floor_x1,floor_y1,floor_x2,floor_y2,l) > 0 ||
						clipinsideboxline(x,y,floor_x2,floor_y2,x3,y3,l) > 0 ||
						clipinsideboxline(x,y,x3,y3,x4,y4,l) > 0 ||
						clipinsideboxline(x,y,x4,y4,floor_x1,floor_y1,l) > 0 ||
						(((floor_y2-floor_y1)*(x-floor_x1) <= (floor_x2-floor_x1)*(y-floor_y1)) &&
						 ((y3-floor_y2)*(x-floor_x2) <= (x3-floor_x2)*(y-floor_y2)) &&
						 ((y4-y3)*(x-x3) <= (x4-x3)*(y-y3)) &&
						 ((floor_y1-y4)*(x-x4) <= (floor_x1-x4)*(y-y4))))
					{
						daz = sprite[j].z+1024; daz2 = sprite[j].z-1024;
						if ((z > daz) && (daz > *ceilz))
							{ *ceilz = daz; *ceilhit = j+49152; }
						if ((z < daz2) && (daz2 < *florz))
							{ *florz = daz2; *florhit = j+49152; }
					}
				}
			}
			j = nextspritesect[j];
		}
	}
}

makeradarang(long xdime)
{
	long i, j, ang, xmul, x, maxj;

	if (xdime <= 320) maxj = 256;
	else maxj = 512;
	for(i=0;i<=(xdime>>1);i++)
	{
		ang = divscale(i,xdime>>1,15)-32768;
		x = ang;
		xmul = -mulscale(x,x,15);
		for(j=3;j<maxj-((i>>1)<<2);j+=2)
		{
			x *= xmul;
			ang += x/(j<<15);
			x >>= 15;
		}
		radarang2[i] = divscale(ang,6588397L,16);
		if (xdime-i < xdime) radarang2[xdime-i] = -radarang2[i];
	}
}

setview(long x1, long y1, long x2, long y2)
{
	long i, j, k;

	windowx1 = x1; windowx2 = x2;
	windowy1 = y1; windowy2 = y2;
	xdimen = x2-x1+1;
	ydimen = y2-y1+1;
	xdimenscale = divscale(xdimen,320L,16);
	xdimscale = divscale(320L,xdimen,16);
	xdimenoffs = x1;
	makeradarang(xdimen);

	for(i=0;i<xdimen;i++)
	{
		j = y2-y1+1;
		k = (YDIM>>1)-(j>>1);
		j += k;
		startumost[i] = k;
		startdmost[i] = j;
	}
	// COMPILER_STEERED: Reuse the span and loop locals for the view and bottom bounds.
	j = ((option[0] != 7) ? (y1-startumost[0])*320
	                      : (y1-startumost[0])*640)+x1;
	viewoffset = j;

	if (xdimen < XDIM)
		for(i=xdimen;i<XDIM;i++)
		{
			startumost[i] = 1;
			startdmost[i] = 0;
		}
	if (option[0] == 7)
	{
		ytop = (YDIM-ydimen)>>1;
		i = (YDIM+ydimen)>>1;
	}
	else
	{
		ytop = 0;
		i = YDIM;
	}
	ybot = i;
}

precache()
{
	long i;
	short j;

	for(i=0;i<numsectors;i++)
	{
		j = sector[i].ceilingpicnum;
		if (waloff[j] == -1) loadtile(j);
		j = sector[i].floorpicnum;
		if (waloff[j] == -1) loadtile(j);
	}
	for(i=0;i<numwalls;i++)
	{
		j = wall[i].picnum;
		if (waloff[j] == -1) loadtile(j);
	}
	for(i=0;i<MAXSPRITES;i++)
		if (sprite[i].statnum < MAXSTATUS)
		{
			j = sprite[i].picnum;
			if (waloff[j] == -1) loadtile(j);
		}
}

wallmost(long *mostbuf, long w, long z)
{
	long bad, x, intx, inty, xcross, y, yy, yinc;

	bad = ((z<<3) < (ytop-dahorizbak)*yb1[w]);
	bad += (((z<<3) < (ytop-dahorizbak)*yb2[w])<<1);
	bad += (((z<<3) > (ybot-dahorizbak)*yb1[w])<<2);
	bad += (((z<<3) > (ybot-dahorizbak)*yb2[w])<<3);

	if ((bad&3) == 3)
	{
		for(x=xb1[w];x<=xb2[w];x++) mostbuf[x] = ytop;
		return(bad);
	}

	if ((bad&0xc) == 0xc)
	{
		for(x=xb1[w];x<=xb2[w];x++) mostbuf[x] = ybot;
		return(bad);
	}

	if ((bad&3) > 0)
	{
		inty = (z<<3) / -dahorizbak;
		intx = scale(inty-ry1[w],rx2[w]-rx1[w],ry2[w]-ry1[w])+rx1[w];
		xcross = ((intx*halfxdimen)/inty)+halfxdimen;
		if (intx > 0) xcross--;

		if ((bad&3) == 2)
		{
			if (xb1[w] <= xcross)
			{
				y = divscale(z,yb1[w],19);
				yy = divscale(z,inty,19);
				if (xdimen != XDIM)
				{
					y = mulscale(y,xdimenscale,16);
					yy = mulscale(yy,xdimenscale,16);
				}
				yinc = (yy-y) / (xcross-xb1[w]+1);
				y += (globalhoriz<<16);
				qinterpolatedown16((long)(&mostbuf[xb1[w]]),xcross-xb1[w],y,yinc);
			}
			for(x=xcross+1;x<=xb2[w];x++) mostbuf[x] = ytop;
		}
		else
		{
			if (xcross <= xb2[w])
			{
				y = divscale(z,inty,19);
				yy = divscale(z,yb2[w],19);
				if (xdimen != XDIM)
				{
					y = mulscale(y,xdimenscale,16);
					yy = mulscale(yy,xdimenscale,16);
				}
				yinc = (yy-y) / (xb2[w]-xcross+1);
				y += (globalhoriz<<16);
				qinterpolatedown16((long)(&mostbuf[xcross]),xb2[w]-xcross,y,yinc);
			}
			for(x=xb1[w];x<xcross;x++) mostbuf[x] = ytop;
		}
		if (mostbuf[xcross] < ytop) mostbuf[xcross] = ytop;
		return(bad);
	}

	if ((bad&0xc) > 0)
	{
		inty = (z<<3) / (ybot-dahorizbak);
		intx = scale(inty-ry1[w],rx2[w]-rx1[w],ry2[w]-ry1[w])+rx1[w];
		xcross = ((intx*halfxdimen)/inty)+halfxdimen;
		if (intx > 0) xcross--;

		if ((bad&0xc) == 8)
		{
			if (xb1[w] <= xcross)
			{
				y = divscale(z,yb1[w],19);
				yy = divscale(z,inty,19);
				if (xdimen != XDIM)
				{
					y = mulscale(y,xdimenscale,16);
					yy = mulscale(yy,xdimenscale,16);
				}
				yinc = (yy-y) / (xcross-xb1[w]+1);
				y += (globalhoriz<<16);
				qinterpolatedown16((long)(&mostbuf[xb1[w]]),xcross-xb1[w],y,yinc);
			}
			for(x=xcross+1;x<=xb2[w];x++) mostbuf[x] = ybot;
		}
		else
		{
			if (xcross <= xb2[w])
			{
				y = divscale(z,inty,19);
				yy = divscale(z,yb2[w],19);
				if (xdimen != XDIM)
				{
					y = mulscale(y,xdimenscale,16);
					yy = mulscale(yy,xdimenscale,16);
				}
				yinc = (yy-y) / (xb2[w]-xcross+1);
				y += (globalhoriz<<16);
				qinterpolatedown16((long)(&mostbuf[xcross]),xb2[w]-xcross,y,yinc);
			}
			for(x=xb1[w];x<xcross;x++) mostbuf[x] = ybot;
		}
		if (mostbuf[xcross] > ybot) mostbuf[xcross] = ybot;
		return(bad);
	}

	y = divscale(z,yb1[w],19);
	yy = divscale(z,yb2[w],19);
	if (xdimen != XDIM)
	{
		y = mulscale(y,xdimenscale,16);
		yy = mulscale(yy,xdimenscale,16);
	}
	yinc = (yy-y) / (xb2[w]-xb1[w]+1);
	y += (globalhoriz<<16);
	qinterpolatedown16((long)(&mostbuf[xb1[w]]),xb2[w]-xb1[w],y,yinc);

	return(bad);
}

rotatesprite(long sx, long sy, long z, short a, short picnum)
{
	// COMPILER_STEERED: Save the column origins and keep the raster
	// height and count widths explicit to recover the historical extent.
	long xsiz, ysiz, cosang, sinang, xstep, ystep;
	long gx, gy, t, firstx, lastx, x, y1, y2, yoffs;
	long startu, startv, umin, umax, vmin, vmax, u, v, du, dv, i, k;
	long bufplc, p, pitch, buf, frac, inc, stride;
	long count;
	short tileheight;

	if (z <= 16) return(0);
	if ((tilesizx[picnum]|tilesizy[picnum]) == 0) return(0);
	if ((picanm[picnum]&192) != 0) picnum += animateoffs(picnum);
	xsiz = tilesizx[picnum];
	tileheight = tilesizy[picnum];
	ysiz = tileheight;

	t = mulscale(z,xsiz+ysiz,1);
	if ((sx+t < 0) || (sy+t < 0) || (sx-t > (320L<<16)) ||
		 (sy-t > (200L<<16))) return(0);

	sinang = sintable[(a+2048)&2047];
	cosang = sintable[(a+2560)&2047];
	i = divscale(65536L,z,16);
	ystep = mulscale(cosang,i,14);
	xstep = mulscale(sinang,i,14);

	gx = sx-mulscale(cosang*xsiz-sinang*ysiz,z,15);
	gy = sy-mulscale(sinang*xsiz+cosang*ysiz,z,15);

	umin = 0; umax = ysiz<<16;
	vmin = 0; vmax = xsiz<<16;
	du = mulscale(cosang*xsiz,z,22);
	dv = mulscale((-sinang)*ysiz,z,22);
	firstx = 0; lastx = 0;
	if (du < firstx) firstx = du-1;
	if (du > lastx) lastx = du+1;
	if (dv < firstx) firstx = dv-1;
	if (dv > lastx) lastx = dv+1;
	dv += du;
	if (dv < firstx) firstx = dv-1;
	if (dv > lastx) lastx = dv+1;

	firstx += gx>>16; lastx += gx>>16;
	if (firstx < 0) firstx = 0;
	if (lastx >= 320) lastx = 319;
	if (firstx > lastx) return(0);

	if (waloff[picnum] == -1) loadtile(picnum);
	setgotpic(picnum);
	bufplc = waloff[picnum];
	if (option[0] == 0) koutp(0x3c4,2);

	yoffs = (YDIM>>1)-((windowy1+windowy2+1)>>1);
	i = (firstx<<16)-gx; k = -gy;
	v = mulscale(ystep,k,16)-mulscale(i,xstep,16);
	u = mulscale(ystep,i,16)+mulscale(k,xstep,16);

	for(x=firstx;x<=lastx;x++)
	{
		startv = v; startu = u;
		y1 = startumost[x-windowx1]-yoffs;
		y2 = startdmost[x-windowx1]-yoffs-1;

		if (ystep > 0)
		{
			if (v >= umax) y2 = -1;
			else
			{
				i = (umax-v)/ystep-1; if (i < y2) y2 = i;
				if (v < umin) { i = (umin-v)/ystep+1; if (i > y1) y1 = i; }
			}
		}
		else if (ystep < 0)
		{
			if (v < umin) y2 = -1;
			else
			{
				i = (umin-v)/ystep-1; if (i < y2) y2 = i;
				if (v >= umax) { i = (umax-v)/ystep+1; if (i > y1) y1 = i; }
			}
		}
		else if ((v < umin) || (v >= umax)) y2 = -1;

		if (xstep > 0)
		{
			if (u >= vmax) y2 = -1;
			else
			{
				i = (vmax-u)/xstep-1; if (i < y2) y2 = i;
				if (u < vmin) { i = (vmin-u)/xstep+1; if (i > y1) y1 = i; }
			}
		}
		else if (xstep < 0)
		{
			if (u < vmin) y2 = -1;
			else
			{
				i = (vmin-u)/xstep-1; if (i < y2) y2 = i;
				if (u >= vmax) { i = (vmax-u)/xstep+1; if (i > y1) y1 = i; }
			}
		}
		else if ((u < vmin) || (u >= vmax)) y2 = -1;

		if ((y1 <= y2) && (y2 >= 0) && (y1 < YDIM))
		{
			v += ystep*y1;
			u += xstep*y1;
			count = y2-y1;

			if (option[0] == 0)
			{
				koutp(0x3c5,1<<(x&3));
				p = y1*80+(x>>2)+chainoffset;
				pitch = 80;
			}
			else if (option[0] != 7)
			{
				p = y1*320+x+frameplace;
				pitch = 320;
			}
			else
			{
				p = y1*640+x+frameplace;
				pitch = 640;
			}

			buf = bufplc+(u>>16)*tileheight;
			if (xstep > 0)
			{
				inc = xstep>>16;
				inc *= tileheight;
				frac = xstep&65535;
				u &= 65535;
				stride = tileheight;
			}
			else
			{
				inc = -((-xstep)>>16);
				inc *= tileheight;
				frac = (-xstep)&65535;
				u = (-u)&65535;
				stride = -tileheight;
			}
			asm1 = pitch;
			asm4 = stride;
			asm3 = frac<<16;
			asm2 = inc;
			spritevline(ystep,v,(short)count,u<<16,buf,p);
		}
		v = startv-xstep; u = startu+ystep;
	}
	return(0);
}

makepalookup(long palnum, char *remapbuf, signed char r, signed char g, signed char b)
{
	long i, j, k, paletteoffset, palidx, ravg, gavg, bavg, dist, mindist;
	char col, *ptr, *ptr2;

	if (paletteloaded == 0) return;
	if ((r|g|b|63) != 63) return;
	if (palookup[palnum] == NULL)
	{
		if ((palookup[palnum] = (char *)malloc(numpalookups<<8)) == NULL)
		{
			initcache();
			cachesize -= (numpalookups<<8);
			palookup[palnum] = (char *)(FP_OFF(pic)+cachesize);
		}
	}
	if ((r|g|b) == 0)
	{
		for(i=0;i<256;i++)
		{
			j = 0;
			ptr = palookup[0];
			ptr += (remapbuf[i]&255);
			ptr2 = palookup[palnum]+i;
			for(;j<numpalookups;j++)
			{
				*ptr2 = *ptr;
				ptr += 256;
				ptr2 += 256;
			}
		}
	}
	else
	{
		for(i=0;i<256;i++)
		{
			/* COMPILER_STEERED. Form the RGB offset with adds and advance it by three. */
			palidx = remapbuf[i]&255;
			palidx = palidx+palidx+palidx;
			ptr2 = palookup[palnum];
			ptr2 += i;
			for(j=0;j<numpalookups;j++)
			{
				ravg = palette[palidx]+((r-palette[palidx])*j)/numpalookups;
				gavg = palette[palidx+1]+((g-palette[palidx+1])*j)/numpalookups;
				bavg = palette[palidx+2]+((b-palette[palidx+2])*j)/numpalookups;
				mindist = 0x7fffffff;
				col = 0;
				for(k=0,paletteoffset=0;k<256;k++,paletteoffset+=3)
				{
					dist = klabs(palette[paletteoffset]-ravg)+klabs(palette[paletteoffset+1]-gavg)+klabs(palette[paletteoffset+2]-bavg);
					if (dist < mindist)
					{
						col = k;
						mindist = dist;
					}
				}
				*ptr2 = col;
				ptr2 += 256;
			}
		}
	}
}

setvesamode(short vesamode)
{
	static struct rminfo
	{
		long EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX;
		short flags, ES, DS, FS, GS, IP, CS, SP, SS;
	} RMI;

	union REGS regs;
	struct SREGS sregs;
	short selector, segment;
	short *ptr, gran;

	memset(&sregs,0,sizeof(sregs));
	regs.w.ax = 0x0100;
	regs.w.bx = (256>>4);             //Number of paragraphs
	int386x(0x31,&regs,&regs,&sregs); // DPMI call 100h allocates DOS memory
	segment = regs.w.ax;
	selector = regs.w.dx;

	memset(&RMI,0,sizeof(RMI));    // Set up real-mode call structure
	RMI.EAX = 0x00004f01;
	RMI.ECX = (long)vesamode;
	RMI.ES = segment;
	RMI.EDI = 0;

	regs.w.ax = 0x0300;            // Use DMPI call 300h to issue the DOS interrupt
	regs.h.bl = 0x10;
	regs.h.bh = 0;
	regs.w.cx = 0;
	sregs.es = FP_SEG(&RMI);
	regs.x.edi = FP_OFF(&RMI);
	int386x(0x31,&regs,&regs,&sregs);

	if ((RMI.EAX&65535) != 0x004f)
		return(-1);

	ptr = (short *)((((long)segment)<<4)+4);
	gran = 64 / (*ptr);
	vesapageshift = 0;
	while (gran > 1)
	{
		vesapageshift++;
		gran >>= 1;
	}

	if (vesasetmode(vesamode) != 0x4f)
		return(-1);

	return(0);
}

stereoinit()
{
	long i;

	setverts(510L,487L,0L,479L,12L,479L);

	outp(0x3d4,0x38); outp(0x3d5,0x48);           //S3 Extensions enable #1
	outp(0x3d4,0x39); outp(0x3d5,0xa5);           //S3 Extensions enable #2
	outp(0x3d4,0x31); outp(0x3d5,inp(0x3d5)|9);   //Init for start address

	outp(0x3d4,0x9); outp(0x3d5,inp(0x3d5)&(255-1));

	for(i=0;i<4;i++)
	{
		outp(0x3d4,0x35); outp(0x3d5,i);
		clearbuf(0xa0000,16384L,0L);
	}
}

setverts(long vertotal, long startverblank, long endverblank,
			long veretstart, long veretend, long verdispenab)
{
	char oflow;

	outp(0x3d4,0x11); outp(0x3d5,inp(0x3d5)&127);
	outp(0x3d4,0x6); outp(0x3d5,vertotal&255);

	oflow = 0;
	oflow += ((vertotal&256)>>8);
	oflow += ((verdispenab&256)>>7);
	oflow += ((veretstart&256)>>6);
	oflow += ((startverblank&256)>>5);
	oflow += ((vertotal&512)>>4);
	oflow += ((verdispenab&512)>>3);
	oflow += ((veretstart&512)>>2);
	outp(0x3d4,0x7); outp(0x3d5,(inp(0x3d5)&0x10)|oflow);

	outp(0x3d4,0x9); outp(0x3d5,(inp(0x3d5)&(255-32))|((startverblank&512)>>4));
	outp(0x3d4,0x10); outp(0x3d5,veretstart&255);
	outp(0x3d4,0x11); outp(0x3d5,(inp(0x3d5)&0xf0)+(veretend&15));
	outp(0x3d4,0x12); outp(0x3d5,verdispenab&255);
	outp(0x3d4,0x15); outp(0x3d5,startverblank&255);
	outp(0x3d4,0x16); outp(0x3d5,endverblank&255);

	outp(0x3d4,0x9); outp(0x3d5,inp(0x3d5)&(255-1));
}

stereoblit(long dastereopage, char *stereobuf)
{
	if (stereopixelwidth >= 0)
	{
		if (dastereopage == 0)
		{
			outp(0x3d4,0x35); outp(0x3d5,page+0); copybuf(stereobuf+stereopixelwidth,0xa0000,64000>>2);
		}
		else
		{
			outp(0x3d4,0x35); outp(0x3d5,page+1); copybuf(stereobuf,0xa4000,49152>>2);
			outp(0x3d4,0x35); outp(0x3d5,page+2); copybuf(stereobuf+49152,0xa0000,14848>>2);
		}
	}
	else
	{
	}
}

stereoredblueblit(char *left, char *right)
{
	long i;
	char *dst;

	if (stereopixelwidth >= 0)
	{
		dst = (char *)0xa0000;
		for(i=0;i<64000;i++)
			*dst++ = left[i+stereopixelwidth] + (right[i]<<4);
	}
}

setredbluepalette()
{
	char *ptr;
	long i, j, dist, daval;

	koutp(0x3c8,0);
	for(j=0;j<16;j++)
		for(i=0;i<16;i++)
		{
			koutp(0x3c9,i<<2);
			koutp(0x3c9,0);
			koutp(0x3c9,j<<2);
		}

	for(i=0;i<256;i++)
	{
		dist = palette[i*3]+palette[i*3+1]+palette[i*3+2];
		dist >>= 1;
		ptr = (char *)(FP_OFF(palookup[0])+i);
		for(j=0;j<32;j++)
		{
			daval = ((dist*(31-j))>>7);
			if (daval > 15) daval = 15;
			*ptr = daval;
			ptr += 256;
		}
	}
}

setbrightness(char dabrightness)
{
	long i;

	curbrightness = (long)dabrightness;
	if (curbrightness < 0) curbrightness = 0;
	if (curbrightness > 15) curbrightness = 15;

	koutp(0x3c8,0);
	for(i=0;i<768;i++)
		koutp(0x3c9,britable[curbrightness][palette[i]]);
}

