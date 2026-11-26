#ifndef MULTI_H_INCLUDED
#define MULTI_H_INCLUDED

#define MAXPLAYERS 16

extern short numplayers, myconnectindex, connecthead;
extern short connectpoint2[MAXPLAYERS];
extern volatile long *lastpacket2clock;

/* DOS/4GW real-mode interrupt register frame. */
struct rminfo {
    long EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX;
    short flags, ES, DS, FS, GS, IP, CS, SP, SS;
};

void initmultiplayers(unsigned char option, unsigned char rate);
void uninitmultiplayers(void);
void sendpacket(short otherconnectindex, unsigned char *buffer, short length);
short getpacket(short *otherconnectindex, unsigned char *buffer);
void sendlogon(void);
void sendlogoff(void);
long getoutputcirclesize(void);

#endif
