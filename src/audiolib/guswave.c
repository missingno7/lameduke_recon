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
   file:   GUSWAVE.C

   author: James R. Dose
   date:   March 23, 1994

   Digitized sound playback routines for the Gravis Ultrasound.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "interrupt.h"
#else
#include "interrup.h"
#endif
#include "ll_man.h"
#include "pitch.h"
#include "user.h"
#include "multivoc.h"
#include "_guswave.h"
#include "newgf1.h"
#include "gusmidi.h"
#include "guswave.h"
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "memcheck.h"
#endif

#define ATR_INDEX               0x3c0
#define STATUS_REGISTER_1       0x3da

#define SetBorderColor(color) \
   { \
   inp  (STATUS_REGISTER_1); \
   outp (ATR_INDEX,0x31);    \
   outp (ATR_INDEX,color);   \
   }

static const int GUSWAVE_PanTable[ 32 ] =
   {
      8,  9, 10, 11, 11, 12, 13, 14,
     15, 14, 13, 12, 11, 10,  9,  8,
      7,  6,  5,  4,  4,  3,  2,  1,
      0,  1,  2,  3,  4,  5,  6,  7
   };

static voicelist VoiceList;
static voicelist VoicePool;

static voicestatus VoiceStatus[ MAX_VOICES ];
//static
VoiceNode GUSWAVE_Voices[ VOICES ];

static int GUSWAVE_VoiceHandle  = GUSWAVE_MinVoiceHandle;
static int GUSWAVE_MaxVoices = VOICES;
//static
int GUSWAVE_Installed = FALSE;

static void ( *GUSWAVE_CallBackFunc )( unsigned long ) = NULL;

// current volume for dig audio - from 0 to 4095
static int GUSWAVE_Volume = MAX_VOLUME;




int GUSWAVE_ErrorCode = GUSWAVE_Ok;

#define GUSWAVE_SetErrorCode( status ) \
   GUSWAVE_ErrorCode   = ( status );


/*---------------------------------------------------------------------
   Function: GUSWAVE_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *GUSWAVE_ErrorString
   (
   int ErrorNumber
   )

   {
   char *ErrorString;

   switch( ErrorNumber )
      {
      case GUSWAVE_Warning :
      case GUSWAVE_Error :
         ErrorString = GUSWAVE_ErrorString( GUSWAVE_ErrorCode );
         break;

      case GUSWAVE_Ok :
         ErrorString = "GUSWAVE ok.";
         break;

      case GUSWAVE_GUSError :
         ErrorString = GUS_ErrorString( GUS_Error );
         break;

      case GUSWAVE_NotInstalled :
         ErrorString = "GUSWAVE not installed.";
         break;

      case GUSWAVE_NoVoices :
         ErrorString = "No free voices available to GUSWAVE.";
         break;

      case GUSWAVE_UltraNoMem :
         ErrorString = "Not enough Ultrasound memory available for GUSWAVE.";
         break;

      case GUSWAVE_VoiceNotFound :
         ErrorString = "No voice with matching handle found.";
         break;

      default :
         ErrorString = "Unknown GUSWAVE error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_CallBack

   GF1 callback service routine.
---------------------------------------------------------------------*/

static int LOADDS GUSWAVE_CallBack
   (
   int reason,
   int voice,
   unsigned char **buf,
   unsigned long *size
   )

   {
   VoiceNode *Voice;

   if ( reason == DIG_MORE_DATA )
      {
      Voice = VoiceStatus[ voice ].Voice;
      if ( ( Voice != NULL ) && Voice->Playing )
         {
         GUSWAVE_GetNextVOCBlock( Voice );
         if ( Voice->Playing )
            {
            *buf = Voice->sound;
            *size = Voice->length;
            return( DIG_MORE_DATA );
            }
         }
      return( DIG_DONE );
      }

   if ( reason == DIG_DONE && VoiceStatus[ voice ].playing )
      {
      Voice = VoiceStatus[ voice ].Voice;
      VoiceStatus[ voice ].playing = FALSE;
      if ( Voice != NULL )
         {
         Voice->Active = FALSE;
         Voice->Playing = FALSE;
         LL_Remove( VoiceNode, &VoiceList, Voice );
         LL_AddToTail( VoiceNode, &VoicePool, Voice );
         }

      if ( GUSWAVE_CallBackFunc )
         {
         GUSWAVE_CallBackFunc( Voice->callbackval );
         }
      }

   return( DIG_DONE );
   }




/*---------------------------------------------------------------------
   Function: GUSWAVE_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

static VoiceNode *GUSWAVE_GetVoice
   (
   int handle
   )

   {
   VoiceNode *voice;
   unsigned  flags;

   flags = DisableInterrupts();

   voice = VoiceList.start;

   while( voice != NULL )
      {
      if ( handle == voice->handle )
         {
         break;
         }

      voice = voice->next;
      }

   RestoreInterrupts( flags );

   if ( voice == NULL )
      {
      GUSWAVE_SetErrorCode( GUSWAVE_VoiceNotFound );
      }

   return( voice );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int GUSWAVE_VoicePlaying
   (
   int handle
   )

   {
   VoiceNode   *voice;

   voice = GUSWAVE_GetVoice( handle );
   if ( voice != NULL )
      {
      return( voice->Active );
      }

   return( FALSE );
   }




/*---------------------------------------------------------------------
   Function: GUSWAVE_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/

int GUSWAVE_VoicesPlaying
   (
   void
   )

   {
   int index;
   int NumVoices;
   unsigned flags;

   flags = DisableInterrupts();
   NumVoices = 0;
   for( index = 0; index < VOICES; index++ )
      {
      if ( GUSWAVE_Voices[ index ].Active )
         {
         NumVoices++;
         }
      }
   RestoreInterrupts( flags );

   return( NumVoices );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int GUSWAVE_Kill
   (
   int handle
   )

   {
   VoiceNode *voice;
   unsigned  flags;

   flags = DisableInterrupts();

   voice = GUSWAVE_GetVoice( handle );

   if ( voice == NULL )
      {
      RestoreInterrupts( flags );
      GUSWAVE_SetErrorCode( GUSWAVE_VoiceNotFound );


      return( GUSWAVE_Warning );
      }

   RestoreInterrupts( flags );

   if ( voice->Active )
      {
      gf1_stop_digital( voice->GF1voice );
      }

//   RestoreInterrupts( flags );

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   GUSWAVE_SetErrorCode( GUSWAVE_Ok );
#endif
   return( GUSWAVE_Ok );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int GUSWAVE_KillAllVoices
   (
   void
   )

   {
   int i;
   unsigned  flags;

   if ( !GUSWAVE_Installed )
      {
      return( GUSWAVE_Ok );
      }


   flags = DisableInterrupts();

   // Remove all the voices from the list
   for( i = 0; i < VOICES; i++ )
      {
      if ( GUSWAVE_Voices[ i ].Active )
         {
//         GUSWAVE_Kill( GUSWAVE_Voices[ i ].handle );

         gf1_stop_digital( GUSWAVE_Voices[ i ].GF1voice );
         }
      }

   for( i = 0; i < MAX_VOICES; i++ )
      {
      VoiceStatus[ i ].playing = FALSE;
      VoiceStatus[ i ].Voice   = NULL;
      }

   VoicePool.start = NULL;
   VoicePool.end   = NULL;
   VoiceList.start = NULL;
   VoiceList.end   = NULL;

   for( i = 0; i < VOICES; i++ )
      {
      GUSWAVE_Voices[ i ].Active = FALSE;
      LL_AddToTail( VoiceNode, &VoicePool, &GUSWAVE_Voices[ i ] );
      }

   RestoreInterrupts( flags );

   return( GUSWAVE_Ok );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_SetPitch

   Sets the pitch for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int GUSWAVE_SetPitch
   (
   int handle,
   int pitchoffset
   )

   {
   VoiceNode *voice;
   unsigned  flags;

   flags = DisableInterrupts();

   voice = GUSWAVE_GetVoice( handle );

   if ( voice == NULL )
      {
      RestoreInterrupts( flags );

      GUSWAVE_SetErrorCode( GUSWAVE_VoiceNotFound );
      return( GUSWAVE_Warning );
      }

   if ( voice->Active )
      {
      voice->PitchScale  = PITCH_GetScale( pitchoffset );
      voice->RateScale   = ( voice->SamplingRate * voice->PitchScale ) >> 16;
      gf1_dig_set_freq( voice->GF1voice, voice->RateScale );
      }

   RestoreInterrupts( flags );

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   GUSWAVE_SetErrorCode( GUSWAVE_Ok );
#endif
   return( GUSWAVE_Ok );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_SetPan3D

   Sets the pan position of the voice with the specified handle.
---------------------------------------------------------------------*/

int GUSWAVE_SetPan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
   VoiceNode *voice;
   int        pan;
   unsigned  flags;

   flags = DisableInterrupts();

   voice = GUSWAVE_GetVoice( handle );

   if ( voice == NULL )
      {
      RestoreInterrupts( flags );

      GUSWAVE_SetErrorCode( GUSWAVE_VoiceNotFound );
      return( GUSWAVE_Warning );
      }

   if ( voice->Active )
      {
      angle &= 31;

      pan = GUSWAVE_PanTable[ angle ];

      if ( distance < 0 )
         {
         distance = 0;
         }
      if ( distance > 255 )
         {
         distance = 255;
         }

      gf1_dig_set_pan( voice->GF1voice, pan );
      gf1_dig_set_vol( voice->GF1voice, GUSWAVE_Volume - distance * 4 );
      }

   RestoreInterrupts( flags );

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   GUSWAVE_SetErrorCode( GUSWAVE_Ok );
#endif
   return( GUSWAVE_Ok );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_SetVolume

   Sets the total volume of the digitized sounds.
---------------------------------------------------------------------*/

void GUSWAVE_SetVolume
   (
   int volume
   )

   {
   volume = max( 0, volume );
   volume = min( 255, volume );
   GUSWAVE_Volume = MAX_VOLUME - ( 255 - volume ) * 4;
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_GetVolume

   Returns the total volume of the digitized sounds.
---------------------------------------------------------------------*/

int GUSWAVE_GetVolume
   (
   void
   )

   {
   return( 255 - ( ( MAX_VOLUME - GUSWAVE_Volume ) / 4 ) );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

static VoiceNode *GUSWAVE_AllocVoice
   (
   int priority
   )

   {
   VoiceNode   *voice;
   unsigned flags;

   if ( GUSWAVE_VoicesPlaying() >= GUSWAVE_MaxVoices )
      {
      flags = DisableInterrupts();
      voice = VoiceList.start;
      while( voice != NULL && priority < voice->priority )
         {
         voice = voice->next;
         }
      RestoreInterrupts( flags );
      if ( voice == NULL )
         {
         return( NULL );
         }

      GUSWAVE_Kill( voice->handle );
      }

   // Check if any voices are in the voice pool
   flags = DisableInterrupts();

   voice = VoicePool.start;
   if ( voice != NULL )
      {
      LL_Remove( VoiceNode, &VoicePool, voice );
      }

   RestoreInterrupts( flags );

   if ( voice != NULL )
      {
      do
         {
         GUSWAVE_VoiceHandle++;
         if ( GUSWAVE_VoiceHandle < GUSWAVE_MinVoiceHandle )
            {
            GUSWAVE_VoiceHandle = GUSWAVE_MinVoiceHandle;
            }
         }
      while( GUSWAVE_VoicePlaying( GUSWAVE_VoiceHandle ) );

      voice->handle = GUSWAVE_VoiceHandle;
      }

   return( voice );
   }




/*---------------------------------------------------------------------
   Function: GUSWAVE_GetNextVOCBlock

   Interperate the information of a VOC format sound file.
---------------------------------------------------------------------*/

void GUSWAVE_GetNextVOCBlock
   (
   VoiceNode *voice
   )

   {
   unsigned char *ptr;
   int blocktype;
   unsigned long blocklength;
   unsigned long samplespeed;
   unsigned int tc;
   int packtype;
   int done;
   unsigned BitsPerSample;
   unsigned Channels;
   unsigned Format;

   if ( voice->BlockLength > 0 )
      {
      voice->sound += MAX_BLOCK_LENGTH;
      voice->length = min( voice->BlockLength, MAX_BLOCK_LENGTH );
      voice->BlockLength -= voice->length;
      return;
      }

   ptr = ( unsigned char * )voice->NextBlock;
   voice->Playing = TRUE;

   done = FALSE;
   do
      {
      blocktype = *ptr;
      blocklength = ( *( unsigned long * )( ptr + 1 ) ) & 0x00ffffff;
      ptr += 4;
      switch( blocktype )
         {
         case 0 :
            voice->Playing = FALSE;
            done = TRUE;
            break;

         case 1 :
            tc = ( unsigned int )*ptr << 8;
            packtype = *( ptr + 1 );
            ptr += 2;
            blocklength -= 2;
            samplespeed = 256000000L / ( 65536 - tc );
            if ( packtype )
               {
               ptr += blocklength;
               }
            else
               {
               done = TRUE;
               }
            break;

         case 2 :
            samplespeed = voice->SamplingRate;
            done = TRUE;
            break;

         case 3 :
            ptr += blocklength;
            break;

         case 4 :
            ptr += blocklength;
            break;

         case 5 :
            ptr += blocklength;
            break;

         case 6 :
            voice->LoopCount = *( unsigned short * )ptr;
            ptr += blocklength;
            voice->LoopStart = ptr;
            break;

         case 7 :
            ptr += blocklength;
            if ( voice->LoopCount > 0 && voice->LoopStart != NULL )
               {
               ptr = voice->LoopStart;
               if ( voice->LoopCount < 0xffff )
                  {
                  voice->LoopCount--;
                  if ( voice->LoopCount == 0 )
                     {
                     voice->LoopStart = NULL;
                     }
                  }
               }
            break;

         case 8 :
            ptr += blocklength;
            break;

         case 9 :
            samplespeed = *( unsigned long * )ptr;
            BitsPerSample = *( ptr + 4 );
            Channels = *( ptr + 5 );
            Format = *( unsigned short * )( ptr + 6 );
            if ( BitsPerSample == 8 && Channels == 1 && Format == VOC_8BIT )
               {
               done = TRUE;
               ptr += 12;
               }
            else
               {
               ptr += blocklength;
               }
            break;

         default :
            voice->Playing = FALSE;
            done = TRUE;
            break;
         }
      }
   while( !done );

   if ( voice->Playing )
      {
      voice->NextBlock = ptr + blocklength;
      voice->sound = ptr;
      voice->SamplingRate = samplespeed;
      voice->RateScale = ( voice->SamplingRate * voice->PitchScale ) >> 16;
      voice->length = min( blocklength, MAX_BLOCK_LENGTH );
      voice->BlockLength = blocklength - voice->length;
      }
   }






/*---------------------------------------------------------------------
   Function: GUSWAVE_PlayVOC

   Begins playback of digitized sound.
---------------------------------------------------------------------*/

int GUSWAVE_PlayVOC
   (
   char *sample,
   int   pitchoffset,
   int   angle,
   int   volume,
   int   priority,
   unsigned long callbackval
   )

   {
   VoiceNode *voice;
   int VoiceNumber;
   int type;
   int pan;
   unsigned short gf1pan;
   unsigned dataOffset;
   unsigned flags;

   voice = GUSWAVE_AllocVoice( priority );
   if ( voice == NULL )
      {
      GUSWAVE_SetErrorCode( GUSWAVE_NoVoices );
      return( GUSWAVE_Warning );
      }

   type = 0;
   type = TYPE_8BIT | TYPE_INVERT_MSB;
   voice->GF1voice = -1;
   dataOffset = *( unsigned short * )( sample + 0x14 );
   voice->NextBlock = sample + dataOffset;
   voice->LoopStart = NULL;
   voice->LoopCount = 0;
   voice->BlockLength = 0;
   voice->PitchScale = PITCH_GetScale( pitchoffset );
   GUSWAVE_GetNextVOCBlock( voice );
   voice->callbackval = callbackval;

   angle &= 31;
   pan = GUSWAVE_PanTable[ angle ];
   gf1pan = pan;
   if ( volume < 0 )
      {
      volume = 0;
      }
   if ( volume > 255 )
      {
      volume = 255;
      }

   volume = 255 - volume;
   VoiceNumber = gf1_play_digital( priority, voice->sound, voice->length,
      voice->mem, GUSWAVE_Volume - volume * 4, gf1pan,
      voice->RateScale, type, &GUS_HoldBuffer, GUSWAVE_CallBack );

   if ( VoiceNumber == NO_MORE_VOICES )
      {
      flags = DisableInterrupts();
      LL_AddToTail( VoiceNode, &VoicePool, voice );
      RestoreInterrupts( flags );
      GUSWAVE_SetErrorCode( GUSWAVE_NoVoices );
      return( GUSWAVE_Warning );
      }

   flags = DisableInterrupts();
   voice->GF1voice = VoiceNumber;
   voice->Active = TRUE;
   LL_AddToTail( VoiceNode, &VoiceList, voice );
   VoiceStatus[ VoiceNumber ].playing = TRUE;
   VoiceStatus[ VoiceNumber ].Voice = voice;
   RestoreInterrupts( flags );

   return( voice->handle );
   }










/*---------------------------------------------------------------------
   Function: GUSWAVE_InitVoices

   Begins playback of digitized sound.
---------------------------------------------------------------------*/

static int GUSWAVE_InitVoices
   (
   void
   )

   {
   int i;

   for( i = 0; i < MAX_VOICES; i++ )
      {
      VoiceStatus[ i ].playing = FALSE;
      VoiceStatus[ i ].Voice   = NULL;
      }

   VoicePool.start = NULL;
   VoicePool.end   = NULL;
   VoiceList.start = NULL;
   VoiceList.end   = NULL;

   for( i = 0; i < VOICES; i++ )
      {
      GUSWAVE_Voices[ i ].num      = i;
      GUSWAVE_Voices[ i ].Active   = FALSE;
      GUSWAVE_Voices[ i ].GF1voice = 0;
      GUSWAVE_Voices[ i ].mem = gf1_malloc( GF1BSIZE );
      if ( GUSWAVE_Voices[ i ].mem == NULL )
         {
         GUSWAVE_SetErrorCode( GUSWAVE_UltraNoMem );
         return( GUSWAVE_Error );
         }

      LL_AddToTail( VoiceNode, &VoicePool, &GUSWAVE_Voices[ i ] );
      }

   return( GUSWAVE_Ok );
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/

void GUSWAVE_SetCallBack
   (
   void ( *function )( unsigned long )
   )

   {
   GUSWAVE_CallBackFunc = function;
   }


/*---------------------------------------------------------------------
   Function: GUSWAVE_Init

   Initializes the Gravis Ultrasound for digitized sound playback.
---------------------------------------------------------------------*/

int GUSWAVE_Init
   (
   int numvoices
   )

   {
   int status;

   if ( GUSWAVE_Installed )
      {
      GUSWAVE_Shutdown();
      }

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   GUSWAVE_SetErrorCode( GUSWAVE_Ok );
#endif

   status = GUS_Init();
   if ( status != GUS_Ok )
      {
      GUSWAVE_SetErrorCode( GUSWAVE_GUSError );
      return( GUSWAVE_Error );
      }

   GUSWAVE_MaxVoices = min( numvoices, VOICES );
   GUSWAVE_MaxVoices = max( GUSWAVE_MaxVoices, 0 );

   status = GUSWAVE_InitVoices();
   if ( status != GUSWAVE_Ok )
      {
      GUS_Shutdown();
      return( status );
      }

   PITCH_Init();

   GUSWAVE_CallBackFunc = NULL;
   GUSWAVE_Installed = TRUE;

   return( GUSWAVE_Ok );
   }
