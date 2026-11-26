/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/**********************************************************************
   file:   _MULTIVC.H

   author: James R. Dose
   date:   December 20, 1993

   Private header for MULTIVOC.C

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#ifndef ___MULTIVC_H
#define ___MULTIVC_H

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

#define VOC_8BIT            0x0
#define VOC_CT4_ADPCM       0x1
#define VOC_CT3_ADPCM       0x2
#define VOC_CT2_ADPCM       0x3
#define VOC_16BIT           0x4
#define VOC_ALAW            0x6
#define VOC_MULAW           0x7
#define VOC_CREATIVE_ADPCM  0x200

#define T_SIXTEENBIT_STEREO 0
#define T_8BITS       1
#define T_MONO        2
#define T_16BITSOURCE 4
#define T_LEFTQUIET   8
#define T_RIGHTQUIET  16
#define T_DEFAULT     T_SIXTEENBIT_STEREO

#define MV_MaxPanPosition  31
#define MV_NumPanPositions ( MV_MaxPanPosition + 1 )
#define MV_MaxTotalVolume  255
// *** VERSIONS RESTORATION ***
// Uncomment for earlier versions
#if (LIBVER_ASSREV < 20011231L)
#define MV_MaxVolume       15
#endif
#define MV_NumVoices       8

// *** VERSIONS RESTORATION ***
// Uncomment line for earlier revisions
#if (LIBVER_ASSREV >= 20011231L)
#define MIX_VOLUME( volume ) \
   ( ( max( 0, min( ( volume ), 255 ) ) * ( MV_MaxVolume + 1 ) ) >> 8 )
#else
#define MIX_VOLUME( volume ) \
   ( ( max( 0, min( ( volume ), 255 ) ) ) >> 4 )
#endif

//#define SILENCE_16BIT     0x80008000
#define SILENCE_16BIT     0x80008000
#define SILENCE_8BIT      0x80808080
//#define SILENCE_16BIT_PAS 0

#define MixBufferSize     256

#define NumberOfBuffers   16
#define TotalBufferSize   ( MixBufferSize * NumberOfBuffers )

#define PI                3.1415926536

typedef enum
   {
   Raw,
   VOC,
   DemandFeed,
   WAV
   } wavedata;

typedef enum
   {
   NoMoreData,
   KeepPlaying
   } playbackstatus;


typedef struct VoiceNode
   {
   struct VoiceNode *next;
   struct VoiceNode *prev;
   char *NextBlock;
   char *LoopStart;
   unsigned LoopCount;
   unsigned long BlockLength;
   unsigned long PitchScale;
   char *sound;
   unsigned long length;
   unsigned long SamplingRate;
   unsigned long RateScale;
   unsigned long position;
   int Playing;
   int Active[ NumberOfBuffers ];
   int handle;
   int priority;
   void ( *DemandFeed )( char **ptr, unsigned long *length );
   int Volume;
   int LeftVolume;
   int RightVolume;
   unsigned long callbackval;
   } VoiceNode;

typedef struct
   {
   VoiceNode *start;
   VoiceNode *end;
   } VList;

typedef struct
   {
   char left;
   char right;
   } Pan;

typedef unsigned short MONO16;
typedef signed char  MONO8;

typedef struct
   {
   MONO16 left;
   MONO16 right;
//   unsigned short left;
//   unsigned short right;
   } STEREO16;

typedef struct
   {
   MONO16 left;
   MONO16 right;
   } SIGNEDSTEREO16;

typedef struct
   {
//   MONO8 left;
//   MONO8 right;
   char left;
   char right;
   } STEREO8;

typedef struct
   {
   char          RIFF[ 4 ];
   unsigned long file_size;
   char          WAVE[ 4 ];
   char          fmt[ 4 ];
   unsigned long format_size;
   } riff_header;

typedef struct
   {
   unsigned short wFormatTag;
   unsigned short nChannels;
   unsigned long  nSamplesPerSec;
   unsigned long  nAvgBytesPerSec;
   unsigned short nBlockAlign;
   unsigned short nBitsPerSample;
   } format_header;

typedef struct
   {
   unsigned char DATA[ 4 ];
   unsigned long size;
   } data_header;

typedef MONO8  VOLUME8[ 256 ];
typedef short VOLUME16[ 256 ];

typedef char HARSH_CLIP_TABLE_8[ MV_NumVoices * 256 ];

typedef VOLUME8 VOLUME_TABLE_8BIT[ MV_MaxVolume + 1 ];
typedef VOLUME16 VOLUME_TABLE_16BIT[ MV_MaxVolume + 1 ];
typedef unsigned short HARSH_CLIP_TABLE_16[ MV_NumVoices * 256 * 16 ];

void MV_GetNextVOCBlock( VoiceNode *voice );
void MV_LockEnd( void );

void ClearBuffer_DW( void *ptr, unsigned data, int length );

#pragma aux ClearBuffer_DW = \
   "cld",                    \
   "push   es",              \
   "push   ds",              \
   "pop    es",              \
   "rep    stosd",           \
   "pop    es",              \
parm [ edi ] [ eax ] [ ecx ] modify exact [ ecx edi ];



#endif
