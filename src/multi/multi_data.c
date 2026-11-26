/* Working definitions of the MULTI data that multi.c references by symbol.
 * The historical object kept these variables in the same translation unit.
 */
#include "multi.h"

unsigned char prevpacket[576];
unsigned char netoutcnt[MAXPLAYERS];
unsigned char netincnt[MAXPLAYERS];
short connectpoint2[MAXPLAYERS];
unsigned char username[MAXPLAYERS][50];
long connectnum[MAXPLAYERS];
unsigned char bridgeaddr[MAXPLAYERS][6];
unsigned char compaddr[MAXPLAYERS][12];
long outbufindex[64];
long crctable[256];
unsigned char syncbuf[576];

volatile unsigned char *messin, *messout, *ipxout, *rmnethandler;
volatile unsigned char *tempoutbuf, *tempinbuf, *ipxin;
long neti;
volatile unsigned char *ecbput, *ecbget;
long rmsegment16, rmoffset32, myconnectnum;
unsigned char mycompaddr[12];
unsigned long orig_pm_off;
long comvect, comescape, comspeed;
volatile unsigned char *comerror, *comtype, *incnt;
long comi;
volatile short *outbufplc, *inbufplc;
long comreset, comtemp;
volatile short *outbufend, *inbufend;
long comnum;
volatile short *comport;
long outcnt;
volatile unsigned char *outbuf, *inbuf;
long damessleng, syncbufleng, otherconnectnum;
void (__interrupt __far *oldnethandler)(void);
short prevpacketindex, numplayers, prevpacketlength, connecthead;
unsigned short orig_rm_off, orig_rm_seg, orig_pm_sel;
struct rminfo RMI;
