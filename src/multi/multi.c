/* Ken Silverman's Build multiplayer module, reconstructed for December 1994.
 * Later Build MULTI.C carries the Build Engine and Tools copyright and
 * BUILDLIC.TXT license. See ../NOTES.md for the reference and match evidence.
 */

#include <dos.h>
#include <i86.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multi.h"

#define COMBUFSIZ 4096
#define COMCODEBYTES 384
#define COMCODEOFFS 21
#define ESC1 0x81
#define ESC2 0x7e
#define IPXSOCKET 0x4949
#define MAXIPXSIZ 546
#define SYNCBUFSIZ 576

unsigned char multioption = 0, comrateoption = 0;
extern long connectnum[MAXPLAYERS];
extern long crctable[256];
extern long myconnectnum, comspeed, comnum, comvect, comescape, comreset;
extern unsigned char mycompaddr[12];
extern volatile unsigned char *inbuf, *outbuf, *comtype, *comerror, *incnt;
extern volatile short *inbufplc, *inbufend, *outbufplc, *outbufend, *comport;
volatile long *lastpacket2clock = 0, *totalclockinreal = 0;
extern long totalclock;
extern long syncbufleng, outcnt;
extern long damessleng, neti, comi, comtemp, otherconnectnum;
extern long rmsegment16, rmoffset32;
extern unsigned char syncbuf[SYNCBUFSIZ], prevpacket[SYNCBUFSIZ];
extern short prevpacketlength, prevpacketindex;
extern unsigned char netincnt[MAXPLAYERS], netoutcnt[MAXPLAYERS];
extern unsigned char compaddr[MAXPLAYERS][12], bridgeaddr[MAXPLAYERS][6];
extern unsigned char username[MAXPLAYERS][50];
extern long outbufindex[64];
extern volatile unsigned char *ecbget, *ecbput, *ipxin, *ipxout;
extern volatile unsigned char *messin, *messout, *tempinbuf, *tempoutbuf;
extern volatile unsigned char *rmnethandler;
extern unsigned short orig_pm_sel, orig_rm_seg, orig_rm_off;
extern unsigned long orig_pm_off;
extern void (__interrupt __far *oldnethandler)(void);
/* Serial IRQ template assembled from realcom.asm. */
static unsigned char rmbuffer[COMCODEBYTES] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x53, 0x66,
    0x51, 0x52, 0x2e, 0x8b, 0x16, 0x08, 0x00, 0x83, 0xc2, 0x02, 0xec, 0x24,
    0x07, 0x8a, 0xe0, 0x80, 0xfc, 0x02, 0x0f, 0x84, 0x0a, 0x00, 0x80, 0xfc,
    0x04, 0x0f, 0x84, 0x53, 0x00, 0xe9, 0x8e, 0x00, 0x2e, 0x8b, 0x16, 0x08,
    0x00, 0x2e, 0x8a, 0x0e, 0x0a, 0x00, 0x80, 0xe9, 0x01, 0x0f, 0x88, 0x7d,
    0x00, 0x2e, 0x80, 0x3e, 0x0b, 0x00, 0x01, 0x0f, 0x8c, 0x15, 0x00, 0x0f,
    0x84, 0x05, 0x00, 0xb0, 0x81, 0xeb, 0x05, 0x90, 0x2e, 0xa0, 0x0c, 0x00,
    0xee, 0x2e, 0xfe, 0x0e, 0x0b, 0x00, 0xeb, 0xda, 0x2e, 0x8b, 0x1e, 0x04,
    0x00, 0x2e, 0x3b, 0x1e, 0x06, 0x00, 0x0f, 0x84, 0x50, 0x00, 0x2e, 0x8a,
    0x87, 0x80, 0x11, 0xee, 0x43, 0x81, 0xe3, 0xff, 0x0f, 0x2e, 0x89, 0x1e,
    0x04, 0x00, 0xeb, 0xba, 0x2e, 0x8b, 0x16, 0x08, 0x00, 0xec, 0x2e, 0x8b,
    0x1e, 0x02, 0x00, 0x2e, 0x88, 0x87, 0x80, 0x01, 0x43, 0x81, 0xe3, 0xff,
    0x0f, 0x2e, 0x89, 0x1e, 0x02, 0x00, 0x3c, 0x7e, 0x0f, 0x85, 0x0c, 0x00,
    0x2e, 0x66, 0x8b, 0x0e, 0x11, 0x00, 0x2e, 0x66, 0x89, 0x0e, 0x0d, 0x00,
    0x2e, 0x80, 0x3e, 0x0a, 0x00, 0x10, 0x0f, 0x85, 0x08, 0x00, 0x83, 0xc2,
    0x05, 0xec, 0xa8, 0x01, 0x75, 0xc2, 0xf6, 0xc4, 0x01, 0x0f, 0x84, 0x4d,
    0xff, 0xb0, 0x20, 0xe6, 0x20, 0x5a, 0x66, 0x59, 0x5b, 0x58, 0xcf,
};
short myconnectindex = 0;
extern struct rminfo RMI;

int neton(void);
int comon(void);
void comoff(void);
void netoff(void);
void initcrc(void);
void installbicomhandlers(void);
void uninstallbicomhandlers(void);
long allocbicom(void);
long convalloc32(long size);
long simulateint(unsigned char intnum, long daeax, long daebx, long daecx,
                 long daedx, long daesi, long daedi);
int netinitconnection(long newconnectnum, unsigned char *newcompaddr);
int netuninitconnection(short goneindex);
void __interrupt __far nethandler(void);
void __interrupt __far comhandler(void);
int getcrc(unsigned char *buffer, short bufleng);
void processreservedmessage(short length, unsigned char *buffer);

extern long kinp();
extern void koutp();
#pragma aux kinp = "in al, dx" parm [edx] value [eax]
#pragma aux koutp = "out dx, al" parm [edx] [eax]

extern long ipxcall(long ax, long bx, long cx, long dx);
#pragma aux ipxcall = "int 7ah" parm [eax] [ebx] [ecx] [edx] modify [eax ebx ecx edx]
extern long ipxdetect(long ax, long bx, long cx, long dx);
#pragma aux ipxdetect = "int 2fh" parm [eax] [ebx] [ecx] [edx] modify [eax ebx ecx edx]

extern void fixregistersaftersimulate(void);
#pragma aux fixregistersaftersimulate = "cld" "mov ax,ds" "mov es,ax" modify [eax]

void initmultiplayers(unsigned char option, unsigned char rate)
{
    long i;

    multioption = option;
    comrateoption = rate;
    numplayers = 0;
    initcrc();
    *incnt = 0;
    outcnt = 0;
    connecthead = 0;
    for (i = 0; i < MAXPLAYERS; i++)
        connectpoint2[i] = -1;
    for (i = 0; i < MAXPLAYERS; i++)
        connectnum[i] = 0x7fffffff;

    if (multioption == 0)
        myconnectindex = 0;
    else if (multioption >= 1 && multioption <= 4)
    {
        comnum = multioption;
        switch (rate)
        {
            case 0: comspeed = 2400; break;
            case 1: comspeed = 4800; break;
            case 2: comspeed = 9600; break;
            case 3: comspeed = 14400; break;
            case 4: comspeed = 19200; break;
            case 5: comspeed = 28800; break;
        }
        comon();
        syncbufleng = damessleng = 0;
    }
    else if (multioption >= 5)
    {
        if ((i = neton()) != 0)
        {
            if (i == -1) printf("IPX driver not found\n");
            if (i == -2) printf("Socket could not be opened\n");
            exit(0);
        }
    }
    numplayers = 1;
}

void uninitmultiplayers(void)
{
    if (numplayers > 0)
    {
        if (multioption >= 1 && multioption <= 4)
            comoff();
        if (multioption >= 5)
            netoff();
    }
}

void netoff(void)
{
    ipxcall(0, 1, 0, IPXSOCKET);
    _disable();
    _dos_setvect(0x1c, oldnethandler);
    _enable();
}

int neton(void)
{
    short i;

    if ((ipxdetect(0x7a00, 0, 0, 0) & 255) != 255) return -1;
    allocbicom();
    if ((rmoffset32 = convalloc32(1440L)) == 0)
        printf("Can't allocate memory for IPX\n");
    rmsegment16 = rmoffset32 >> 4;

    ecbget = (unsigned char *)(rmoffset32 + 0);
    ecbput = (unsigned char *)(rmoffset32 + 48);
    ipxin = (unsigned char *)(rmoffset32 + 96);
    ipxout = (unsigned char *)(rmoffset32 + 128);
    messin = (unsigned char *)(rmoffset32 + 160);
    messout = (unsigned char *)(rmoffset32 + 720);
    tempinbuf = (unsigned char *)(rmoffset32 + 1280);
    tempoutbuf = (unsigned char *)(rmoffset32 + 1296);
    rmnethandler = (unsigned char *)(rmoffset32 + 1376);

    /* Install the IPX real-mode callback, with its two SI immediates. */
    rmnethandler[0] = 0xfb;                        /* sti */
    rmnethandler[1] = 0xcd;                        /* int 1ch */
    rmnethandler[2] = 0x1c;
    rmnethandler[3] = 0xbb;                        /* mov bx, 4 */
    rmnethandler[4] = 4;
    rmnethandler[5] = 0;
    rmnethandler[6] = 0xbe;                        /* mov si, segment */
    rmnethandler[7] = rmsegment16;
    rmnethandler[8] = rmsegment16 >> 8;
    rmnethandler[9] = 0x8e;                        /* mov es, si */
    rmnethandler[10] = 0xc6;
    rmnethandler[11] = 0xbe;                       /* mov si, ECB offset */
    rmnethandler[12] = (long)ecbget - rmoffset32;
    rmnethandler[13] = ((long)ecbget - rmoffset32) >> 8;
    rmnethandler[14] = 0xcd;                       /* int 7ah */
    rmnethandler[15] = 0x7a;
    rmnethandler[16] = 0xcb;                       /* retf */
    oldnethandler = _dos_getvect(0x1c);
    _disable();
    _dos_setvect(0x1c, nethandler);
    _enable();

    ipxcall(0, 1, 0, IPXSOCKET);
    if ((ipxcall(0xff, 0, 0, IPXSOCKET) & 255) != 0) return -2;
    simulateint(0x7a, 0L, 9L, 0L, 0L, (long)tempoutbuf, 0L);
    memcpy(mycompaddr, (const void *)tempoutbuf, 10);
    mycompaddr[10] = IPXSOCKET & 255;
    mycompaddr[11] = IPXSOCKET >> 8;
    myconnectnum = (long)tempoutbuf[6] + ((long)tempoutbuf[7] << 8)
                 + ((long)tempoutbuf[8] << 16) + ((long)tempoutbuf[9] << 24);
    netinitconnection(myconnectnum, mycompaddr);

    ecbget[8] = 1;
    ecbput[8] = 0;
    *inbufplc = 0;
    *inbufend = 0;
    for (i = 0; i < MAXPLAYERS; i++)
        netoutcnt[i] = netincnt[i] = 0;
    prevpacketlength = prevpacketindex = 0;

    for (i = 0; i < 30; i++) ipxin[i] = 0;
    for (i = 0; i < 48; i++) ecbget[i] = 0;
    ecbget[4] = (long)rmnethandler - rmoffset32;
    ecbget[5] = ((long)rmnethandler - rmoffset32) >> 8;
    ecbget[6] = rmsegment16;
    ecbget[7] = rmsegment16 >> 8;
    ecbget[10] = IPXSOCKET & 255;
    ecbget[11] = IPXSOCKET >> 8;
    ecbget[34] = 2;
    ecbget[35] = 0;
    ecbget[36] = (long)ipxin - rmoffset32;
    ecbget[37] = ((long)ipxin - rmoffset32) >> 8;
    ecbget[38] = rmsegment16;
    ecbget[39] = rmsegment16 >> 8;
    ecbget[40] = 30;
    ecbget[41] = 0;
    ecbget[42] = (long)messin - rmoffset32;
    ecbget[43] = ((long)messin - rmoffset32) >> 8;
    ecbget[44] = rmsegment16;
    ecbget[45] = rmsegment16 >> 8;
    ecbget[46] = MAXIPXSIZ & 255;
    ecbget[47] = MAXIPXSIZ >> 8;
    simulateint(0x7a, 0L, 4L, 0L, 0L, (long)ecbget, 0L);
    return 0;
}

int comon(void)
{
    long divisor, cnt;
    short *ptr;

    if (comnum < 1 || comnum > 4) return -1;
    comvect = 0xb + (comnum & 1);
    installbicomhandlers();
    *inbufplc = 0;
    *inbufend = 0;
    *outbufplc = 0;
    *outbufend = 0;
    ptr = (short *)(0x400L + ((comnum - 1) << 1));
    *comport = *ptr;
    if (*comport == 0) return -1;
    if (comspeed <= 0 || comspeed > 115200) return -1;

    _disable();
    koutp(*comport + 3, 0x80);
    divisor = 115200 / comspeed;
    koutp(*comport + 0, divisor & 255);
    koutp(*comport + 1, divisor >> 8);
    koutp(*comport + 3, 0x03);
    koutp(*comport + 2, 0x87);
    if ((kinp(*comport + 2) & 0xf8) == 0xc0)
        *comtype = 16;
    else
    {
        *comtype = 1;
        koutp(*comport + 2, 0);
    }
    cnt = *comtype;
    while ((kinp(*comport + 5) & 1) > 0 && cnt > 0)
    {
        kinp(*comport);
        cnt--;
    }
    koutp(*comport + 4, 0x0b);
    koutp(*comport + 1, 0);
    koutp(0x21, kinp(0x21) & (255 - (1 << (comvect & 7))));
    kinp(*comport + 6);
    kinp(*comport + 5);
    kinp(*comport + 0);
    kinp(*comport + 2);
    koutp(*comport + 1, 0x03);
    koutp(0x20, 0x20);
    *comerror = 0;
    comreset = comescape = 0;
    _enable();
    return 0;
}

void comoff(void)
{
    while (*outbufplc != *outbufend)
    {
        if ((kinp(*comport + 5) & 0x40) > 0)
        {
            if (*comerror > 0)
            {
                if (*comerror == 2) koutp(*comport, ESC1);
                if (*comerror == 1) koutp(*comport, *incnt);
                *comerror = *comerror - 1;
            }
            else
            {
                koutp(*comport, (long)outbuf[*outbufplc]);
                *outbufplc = (*outbufplc + 1) & (COMBUFSIZ - 1);
            }
        }
    }
    _disable();
    koutp(0x21, kinp(0x21) | (1 << (comvect & 7)));
    koutp(*comport + 1, 0);
    koutp(*comport + 4, 0);
    _enable();
    uninstallbicomhandlers();
}

void netsend(short otherconnectindex, short messleng)
{
    short i;

    i = 32767;
    while (ecbput[8] != 0 && i > 0) i--;
    for (i = 0; i < 30; i++) ipxout[i] = 0;
    for (i = 0; i < 48; i++) ecbput[i] = 0;
    ipxout[5] = 4;
    if (otherconnectindex == -1)
    {
        memcpy((void *)&ipxout[6], &compaddr[0][0], 4);
        ipxout[10] = 0xff; ipxout[11] = 0xff; ipxout[12] = 0xff;
        ipxout[13] = 0xff; ipxout[14] = 0xff; ipxout[15] = 0xff;
        ipxout[16] = IPXSOCKET & 255; ipxout[17] = IPXSOCKET >> 8;
    }
    else
        memcpy((void *)&ipxout[6], &compaddr[otherconnectindex][0], 12);

    ecbput[10] = IPXSOCKET & 255; ecbput[11] = IPXSOCKET >> 8;
    if (otherconnectindex == -1)
    {
        ecbput[28] = 0xff; ecbput[29] = 0xff; ecbput[30] = 0xff;
        ecbput[31] = 0xff; ecbput[32] = 0xff; ecbput[33] = 0xff;
    }
    else
        memcpy((void *)&ecbput[28], &bridgeaddr[otherconnectindex][0], 6);

    ecbput[34] = 2; ecbput[35] = 0;
    ecbput[36] = (long)ipxout - rmoffset32;
    ecbput[37] = ((long)ipxout - rmoffset32) >> 8;
    ecbput[38] = rmsegment16; ecbput[39] = rmsegment16 >> 8;
    ecbput[40] = 30; ecbput[41] = 0;
    ecbput[42] = (long)messout - rmoffset32;
    ecbput[43] = ((long)messout - rmoffset32) >> 8;
    ecbput[44] = rmsegment16; ecbput[45] = rmsegment16 >> 8;
    ecbput[46] = messleng & 255; ecbput[47] = messleng >> 8;
    simulateint(0x7a, 0L, 3L, 0L, 0L, (long)ecbput, 0L);
}

int comsend(unsigned char ch)
{
    if (ch == ESC1)
    {
        outbuf[*outbufend] = ch;
        *outbufend = (*outbufend + 1) & (COMBUFSIZ - 1);
        outbuf[*outbufend] = 64;
    }
    else if (ch == ESC2)
    {
        outbuf[*outbufend] = ESC1;
        *outbufend = (*outbufend + 1) & (COMBUFSIZ - 1);
        /* The integer address form steers Watcom's register allocation. */
        *(volatile unsigned char *)((long)outbuf + (long)*outbufend) = 65;
    }
    else
        outbuf[*outbufend] = ch;
    *outbufend = (*outbufend + 1) & (COMBUFSIZ - 1);
    return 0;
}

void __interrupt __far nethandler(void)
{
    *lastpacket2clock = totalclock;
    damessleng = (((short)ipxin[2]) << 8) + (short)ipxin[3] - 30;
    inbuf[(long)*inbufend & (COMBUFSIZ - 1L)] = damessleng & 255;
    inbuf[(*inbufend + 1) & (COMBUFSIZ - 1)] = (damessleng >> 8) & 255;
    for (neti = 0; neti < damessleng; neti++)
        inbuf[(*inbufend + neti + 2) & (COMBUFSIZ - 1)] = messin[neti];
    *inbufend = (*inbufend + damessleng + 2) & (COMBUFSIZ - 1);
}

void __interrupt __far comhandler(void)
{
    do
    {
        comtemp = kinp(*comport + 2) & 7;
        if (comtemp == 2)
        {
            comi = *comtype;
            while (comi > 0)
            {
                comi--;
                if (*comerror > 0)
                {
                    if (*comerror == 2) koutp(*comport, ESC1);
                    if (*comerror == 1) koutp(*comport, *incnt);
                    *comerror = *comerror - 1;
                    continue;
                }
                if (*outbufplc != *outbufend)
                {
                    koutp(*comport, (long)outbuf[*outbufplc]);
                    *outbufplc = (*outbufplc + 1) & (COMBUFSIZ - 1);
                    continue;
                }
                break;
            }
        }
        else if (comtemp == 4)
        {
            do
            {
                inbuf[*inbufend] = (unsigned char)kinp(*comport);
                if (inbuf[*inbufend] == ESC2)
                    *lastpacket2clock = totalclock;
                *inbufend = (*inbufend + 1) & (COMBUFSIZ - 1);
            } while (*comtype == 16 && (kinp(*comport + 5) & 1) > 0);
        }
    } while ((comtemp & 1) == 0);
    koutp(0x20, 0x20);
}

int netinitconnection(long newconnectnum, unsigned char *newcompaddr)
{
    long i, j, k, newindex, templong;
    int result;
    unsigned char tempchar;

    for (i = 0; i < MAXPLAYERS; i++)
        if (connectnum[i] == newconnectnum)
        {
            result = -1;
            goto done;
        }

    newindex = 0;
    while (connectnum[newindex] != 0x7fffffff && newindex < MAXPLAYERS)
        newindex++;
    if (newindex >= MAXPLAYERS)
    {
        result = -1;
        goto done;
    }
    connectnum[newindex] = newconnectnum;
    numplayers++;

    memcpy(&compaddr[newindex][0], newcompaddr, 10);
    compaddr[newindex][10] = IPXSOCKET & 255;
    compaddr[newindex][11] = IPXSOCKET >> 8;
    memcpy(&bridgeaddr[newindex][0], &compaddr[newindex][4], 6);

    for (i = 1; i < MAXPLAYERS; i++)
        for (j = 0; j < i; j++)
            if (connectnum[i] < connectnum[j])
            {
                templong = connectnum[i];
                connectnum[i] = connectnum[j];
                connectnum[j] = templong;
                for (k = 0; k < 50; k++)
                {
                    tempchar = username[i][k];
                    username[i][k] = username[j][k];
                    username[j][k] = tempchar;
                }
                for (k = 0; k < 12; k++)
                {
                    tempchar = compaddr[i][k];
                    compaddr[i][k] = compaddr[j][k];
                    compaddr[j][k] = tempchar;
                }
                for (k = 0; k < 6; k++)
                {
                    tempchar = bridgeaddr[i][k];
                    bridgeaddr[i][k] = bridgeaddr[j][k];
                    bridgeaddr[j][k] = tempchar;
                }
            }
    connecthead = 0;
    for (i = 0; i < numplayers - 1; i++) connectpoint2[i] = i + 1;
    connectpoint2[numplayers - 1] = -1;
    for (i = 0; i < numplayers; i++)
        if (connectnum[i] == myconnectnum) myconnectindex = i;
    result = 1;

done:
    return result;
}

int netuninitconnection(short goneindex)
{
    long i, j, k;

    connectnum[goneindex] = 0x7fffffff;
    numplayers--;
    j = 0;
    for (i = 0; i < MAXPLAYERS; i++)
        if (connectnum[i] != 0x7fffffff)
        {
            if (j == 0) connecthead = i;
            else connectpoint2[k] = i;
            k = i;
            j++;
        }
    connectpoint2[k] = -1;
    return 0;
}

long convalloc32(long size)
{
    union REGS r;

    r.x.eax = 0x0100;
    r.x.ebx = (size + 15) >> 4;
    int386(0x31, &r, &r);
    if (r.x.cflag != 0) return 0L;
    return (r.x.eax & 0xffff) << 4;
}

long simulateint(unsigned char intnum, long daeax, long daebx, long daecx,
                 long daedx, long daesi, long daedi)
{
    union REGS regs;
    struct SREGS sregs;

    memset(&RMI, 0, sizeof(RMI));
    memset(&sregs, 0, sizeof(sregs));
    RMI.EAX = daeax;
    RMI.EBX = daebx;
    RMI.ECX = daecx;
    RMI.EDX = daedx;
    RMI.ESI = daesi - rmoffset32;
    RMI.EDI = daedi - rmoffset32;
    RMI.ES = RMI.DS = (short)rmsegment16;

    regs.w.ax = 0x0300;
    regs.h.bl = intnum;
    regs.h.bh = 0;
    regs.w.cx = 0;
    sregs.es = FP_SEG(&RMI);
    regs.x.edi = FP_OFF(&RMI);
    int386x(0x31, &regs, &regs, &sregs);
    fixregistersaftersimulate();
    return RMI.EAX;
}

void sendpacket(short otherconnectindex, unsigned char *buffer, short length)
{
    long i, j;

    if (multioption <= 0) return;
    if (multioption < 5)
    {
        if (buffer[0] == 253 || buffer[0] == 254)
            outcnt = 0;
        outbufindex[outcnt] = *outbufend;
        comsend((length & 3) + (outcnt << 2));
        for (i = 0; i < length; i++)
            comsend(buffer[i]);
        if (comrateoption > 0)
        {
            i = getcrc(buffer, length);
            comsend(i & 255);
            i >>= 8;
            comsend(i & 255);
        }
        outbuf[*outbufend] = ESC2;
        *outbufend = (*outbufend + 1) & (COMBUFSIZ - 1);

        _disable();
        if ((kinp(*comport + 5) & 0x40) > 0 && *outbufplc != *outbufend)
        {
            koutp(*comport, outbuf[*outbufplc]);
            *outbufplc = (*outbufplc + 1) & (COMBUFSIZ - 1);
        }
        _enable();
        outcnt = (outcnt + 1) & 63;
    }
    else if (multioption >= 5)
    {
        i = 262144;
        while (ecbput[8] != 0)
            if (--i <= 0) break;
        messout[0] = myconnectindex;
        j = 1;

        if (buffer[0] == 253 || buffer[0] == 254)
        {
            for (i = 0; i < MAXPLAYERS; i++) netoutcnt[i] = 0;
            messout[j++] = 254;
            messout[j++] = length & 255;
            messout[j++] = (length >> 8) & 255;
            for (i = 0; i < length; i++) messout[j++] = buffer[i];
        }
        else
        {
            if (otherconnectindex == -1)
            {
                messout[j++] = 255;
                for (i = connecthead; i >= 0; i = connectpoint2[i])
                    if (i != myconnectindex)
                        messout[j++] = netoutcnt[i]++;
            }
            else
            {
                messout[j++] = otherconnectindex;
                messout[j++] = netoutcnt[otherconnectindex]++;
            }
            messout[j++] = length & 255;
            messout[j++] = (length >> 8) & 255;
            for (i = 0; i < length; i++) messout[j++] = buffer[i];

            if (prevpacketindex == -1)
            {
                messout[j++] = 255;
                for (i = connecthead; i >= 0; i = connectpoint2[i])
                    if (i != myconnectindex)
                        messout[j++] = netoutcnt[i] - 2;
            }
            else
            {
                messout[j++] = prevpacketindex;
                messout[j++] = netoutcnt[prevpacketindex] - 2;
            }
            messout[j++] = prevpacketlength & 255;
            messout[j++] = (prevpacketlength >> 8) & 255;
            for (i = 0; i < prevpacketlength; i++) messout[j++] = prevpacket[i];
            prevpacketlength = length;
            prevpacketindex = otherconnectindex;
            for (i = 0; i < length; i++) prevpacket[i] = buffer[i];
        }
        netsend(otherconnectindex, j);
    }
}

short getpacket(short *otherconnectindex, unsigned char *buffer)
{
    short i, j, length, sublength, start;
    unsigned char bad, totalbad, toindex;

    if (multioption <= 0) return 0;
    if (multioption < 5)
    {
        *otherconnectindex = 1 - myconnectindex;
        while (*inbufplc != *inbufend)
        {
            i = inbuf[*inbufplc];
            *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
            if (i != ESC2)
            {
                if (i == ESC1)
                    comescape++;
                else if (comescape > 0)
                {
                    if (i == 64)
                        syncbuf[syncbufleng++] = ESC1;
                    else if (i == 65)
                        syncbuf[syncbufleng++] = ESC2;
                    else if (i < 64)
                    {
                        j = (outbufindex[i] + COMBUFSIZ - 2) & (COMBUFSIZ - 1);
                        outbuf[j] = ESC1;
                        outbuf[j + 1] = ESC2;
                        _disable();
                        *outbufplc = j;
                        if ((kinp(*comport + 5) & 0x40) > 0)
                        {
                            koutp(*comport, outbuf[*outbufplc]);
                            *outbufplc = (*outbufplc + 1) & (COMBUFSIZ - 1);
                        }
                        _enable();
                    }
                    comescape--;
                }
                else
                    syncbuf[syncbufleng++] = i;

                if (syncbufleng > SYNCBUFSIZ - 1)
                    syncbufleng = SYNCBUFSIZ - 1;
                continue;
            }
            if (comescape > 0)
            {
                syncbufleng = 0;
                comescape = 0;
                comreset = 0;
                if (*comerror == 2) *comerror = 0;
                continue;
            }

            bad = 0;
            length = syncbufleng - 3 + ((comrateoption == 0) << 1);
            if ((syncbuf[0] >> 2) != *incnt)
            {
                if (*incnt == 1 && syncbuf[1] == 254)
                    myconnectindex = myconnectnum < otherconnectnum;
                bad = 1;
            }
            if ((syncbuf[0] & 3) != (length & 3)) bad = 1;
            for (i = 0; i < length; i++) buffer[i] = syncbuf[i + 1];
            if (comrateoption > 0 &&
                getcrc(buffer, length) != ((long)syncbuf[syncbufleng - 2]
                                          + ((long)syncbuf[syncbufleng - 1] << 8)))
                bad = 1;
            syncbufleng = 0;
            if (bad == 1)
            {
                if (*comerror == 0 && comreset == 0)
                {
                    *comerror = 2;
                    comreset = 1;
                }
                continue;
            }
            *incnt = (*incnt + 1) & 63;
            if (length > 0 && buffer[0] >= 200)
                processreservedmessage(length, buffer);
            return length;
        }
    }
    else if (multioption >= 5)
    {
        start = *inbufplc;
        if (*inbufplc == *inbufend) return 0;
        totalbad = 0;
        length = inbuf[*inbufplc];
        *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
        length += (short)inbuf[*inbufplc] << 8;
        *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
        *otherconnectindex = inbuf[*inbufplc];
        *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);

        while (totalbad < 2)
        {
            if (*inbufplc == *inbufend) return 0;
            toindex = inbuf[*inbufplc];
            *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
            bad = 0;
            if (toindex == 255)
            {
                for (i = connecthead; i >= 0; i = connectpoint2[i])
                    if (i != *otherconnectindex)
                    {
                        j = inbuf[*inbufplc];
                        *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
                        if (i == myconnectindex && j != netincnt[*otherconnectindex])
                        {
                            bad++;
                            break;
                        }
                    }
                netincnt[*otherconnectindex]++;
            }
            else if (toindex != 254)
            {
                j = inbuf[*inbufplc];
                *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
                if (j != netincnt[*otherconnectindex]) bad = 1;
                netincnt[*otherconnectindex]++;
            }
            else
                netincnt[*otherconnectindex] = 0;

            sublength = inbuf[*inbufplc];
            *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
            sublength += (short)inbuf[*inbufplc] << 8;
            *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
            for (i = 0; i < sublength; i++)
            {
                buffer[i] = inbuf[*inbufplc];
                *inbufplc = (*inbufplc + 1) & (COMBUFSIZ - 1);
            }

            if (totalbad == 0 && bad == 0)
            {
                *inbufplc = (start + length + 2) & (COMBUFSIZ - 1);
                if (sublength > 0 && buffer[0] >= 200)
                {
                    processreservedmessage(sublength, buffer);
                    if (buffer[0] != 255) return 0;
                    return sublength;
                }
                if (*otherconnectindex == myconnectindex) return 0;
                return sublength;
            }
            if (totalbad == 1 && bad == 0)
            {
                *inbufplc = start;
                if (*otherconnectindex == myconnectindex) return 0;
                return sublength;
            }
            totalbad += bad;
        }
    }
    return 0;
}

void initcrc(void)
{
    long i, j, k, a, polynomial;

    j = 0;
    polynomial = 0x1021;
    do
    {
        k = j << 8;
        a = 0;
        for (i = 8; i >= 1; i--)
        {
            if (((k ^ a) & 0x8000) > 0)
                a = ((a << 1) & 65535) ^ polynomial;
            else
                a = (a << 1) & 65535;
            k = (k << 1) & 65535;
        }
        crctable[j] = (unsigned short)a;
        j++;
    } while (j < 256);
}

int getcrc(unsigned char *buffer, short bufleng)
{
    long i, crc;

    crc = 0;
    for (i = 0; i < bufleng; i++)
        crc = ((crc << 8) & 65535) ^ crctable[((crc >> 8) & 65535) ^ buffer[i]];
    return crc & 65535;
}

void installbicomhandlers(void)
{
    union REGS r;
    struct SREGS sr;
    long lowp;
    void __far *handler;

    r.x.eax = 0x3500 + comvect;
    sr.ds = sr.es = 0;
    int386x(0x21, &r, &r, &sr);
    orig_pm_sel = (unsigned short)sr.es;
    orig_pm_off = r.x.ebx;

    r.x.eax = 0x0200;
    r.h.bl = comvect;
    int386(0x31, &r, &r);
    orig_rm_seg = (unsigned short)r.x.ecx;
    orig_rm_off = (unsigned short)r.x.edx;

    lowp = allocbicom();
    r.x.eax = 0x2500 + comvect;
    handler = (void __far *)comhandler;
    r.x.edx = FP_OFF(handler);
    sr.ds = FP_SEG(handler);
    sr.es = 0;
    int386x(0x21, &r, &r, &sr);

    r.x.eax = 0x0201;
    r.h.bl = comvect;
    r.x.ecx = (lowp >> 4) & 65535;
    r.x.edx = COMCODEOFFS;
    int386(0x31, &r, &r);
}

void uninstallbicomhandlers(void)
{
    union REGS r;
    struct SREGS sr;

    r.x.eax = 0x2500 + comvect;
    r.x.edx = orig_pm_off;
    sr.ds = orig_pm_sel;
    sr.es = 0;
    int386x(0x21, &r, &r, &sr);

    r.x.eax = 0x0201;
    r.h.bl = comvect;
    r.x.ecx = (unsigned long)orig_rm_seg;
    r.x.edx = (unsigned long)orig_rm_off;
    int386(0x31, &r, &r);
}

long allocbicom(void)
{
    long lowp;

    lowp = convalloc32(COMCODEBYTES + (COMBUFSIZ << 1));
    if (!lowp)
        printf("Can't allocate conventional memory.\n");

    inbuf = (unsigned char *)(lowp + COMCODEBYTES);
    outbuf = (unsigned char *)(lowp + COMCODEBYTES + COMBUFSIZ);
    inbufplc = (short *)(lowp + 0);
    inbufend = (short *)(lowp + 2);
    outbufplc = (short *)(lowp + 4);
    outbufend = (short *)(lowp + 6);
    comport = (short *)(lowp + 8);
    comtype = (unsigned char *)(lowp + 10);
    comerror = (unsigned char *)(lowp + 11);
    incnt = (unsigned char *)(lowp + 12);
    lastpacket2clock = (long *)(lowp + 13);
    totalclockinreal = (long *)(lowp + 17);
    *totalclockinreal = totalclock;
    *lastpacket2clock = *totalclockinreal;
    memcpy((void *)lowp, rmbuffer, COMCODEBYTES);
    return lowp;
}

void processreservedmessage(short length, unsigned char *buffer)
{
    long i, j, k, remoteid, id;

    switch (buffer[0])
    {
        case 253:
            if (multioption < 5)
            {
                otherconnectnum = (long)buffer[1] + ((long)buffer[2] << 8);
                buffer[0] = 254;
                sendpacket(-1, buffer, 1);
                myconnectindex = 0;
                connecthead = 0;
                connectpoint2[0] = 1;
                connectpoint2[1] = -1;
                numplayers = 2;
            }
            else if (multioption >= 5)
            {
                remoteid = (long)buffer[1] + ((long)buffer[2] << 8)
                         + ((long)buffer[3] << 16) + ((long)buffer[4] << 24);
                if (remoteid != myconnectnum)
                {
                    netinitconnection(remoteid, &buffer[5]);
                    if (myconnectindex == connecthead ||
                        (connectnum[connecthead] == remoteid &&
                         myconnectindex == connectpoint2[connecthead]))
                    {
                        buffer[0] = 254;
                        j = 1;
                        for (i = 0; i < MAXPLAYERS; i++)
                            if (connectnum[i] != 0x7fffffff && connectnum[i] != remoteid)
                            {
                                buffer[j++] = connectnum[i] & 255;
                                buffer[j++] = (connectnum[i] >> 8) & 255;
                                buffer[j++] = (connectnum[i] >> 16) & 255;
                                buffer[j++] = (connectnum[i] >> 24) & 255;
                                for (k = 0; k < 10; k++) buffer[j++] = compaddr[i][k];
                            }
                        sendpacket(-1, buffer, j);
                        for (i = 0; i < MAXPLAYERS; i++)
                            if (connectnum[i] == remoteid)
                            {
                                sendpacket((short)i, buffer, j);
                                break;
                            }
                    }
                }
            }
            break;
        case 254:
            if (multioption < 5)
            {
                myconnectindex = 1;
                connecthead = 0;
                connectpoint2[0] = 1;
                connectpoint2[1] = -1;
                numplayers = 2;
            }
            else if (multioption >= 5)
            {
                for (j = 1; j < length; j += 14)
                {
                    id = (long)buffer[j] + ((long)buffer[j + 1] << 8)
                       + ((long)buffer[j + 2] << 16) + ((long)buffer[j + 3] << 24);
                    netinitconnection(id, &buffer[j + 4]);
                }
            }
            break;
        case 255:
            netuninitconnection(buffer[1]);
            break;
    }
}

void sendlogon(void)
{
    long i;
    unsigned char tempbuf[16];

    if (multioption <= 0) return;
    tempbuf[0] = 253;
    if (multioption < 5)
    {
        tempbuf[1] = kinp(0x40);
        tempbuf[2] = kinp(0x40);
        myconnectnum = (long)tempbuf[1] + ((long)tempbuf[2] << 8);
        sendpacket(-1, tempbuf, 3);
    }
    else
    {
        tempbuf[1] = myconnectnum & 255;
        tempbuf[2] = (myconnectnum >> 8) & 255;
        tempbuf[3] = (myconnectnum >> 16) & 255;
        tempbuf[4] = (myconnectnum >> 24) & 255;
        for (i = 0; i < 10; i++)
            tempbuf[i + 5] = mycompaddr[i];
        sendpacket(-1, tempbuf, 15);
    }
}

void sendlogoff(void)
{
    unsigned char tempbuf[16];

    if (numplayers <= 1 || multioption <= 0) return;
    tempbuf[0] = 255;
    if (multioption < 5)
        sendpacket(-1, tempbuf, 1);
    else
    {
        tempbuf[1] = myconnectindex;
        sendpacket(-1, tempbuf, 2);
    }
}

long getoutputcirclesize(void)
{
    if (multioption >= 1 && multioption <= 4)
        return (*outbufend - *outbufplc + COMBUFSIZ) & (COMBUFSIZ - 1);
    return 0;
}
