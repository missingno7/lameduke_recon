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
   module: MULTIVOC.C

   author: James R. Dose
   date:   December 20, 1993

   Routines to provide multichannel digitized sound playback for
   Sound Blaster compatible sound cards.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "dpmi.h"
#include "usrhooks.h"
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "interrupt.h"
#else
#include "interrup.h"
#endif
#include "dpmi.h"
#include "ll_man.h"
#include "sndcards.h"
#include "blaster.h"
#include "sndsrc.h"
#include "pas16.h"
#include "pitch.h"
#include "multivoc.h"
#include "_multivc.h"

#define MAX_BLOCK_LENGTH 0x8000

static int MV_StartPlayback( void );

static int MV_BufferEmpty[ NumberOfBuffers ];
static char *MV_MixBuffer[ NumberOfBuffers ];
static Pan MV_PanTable[ MV_NumPanPositions ][ MV_MaxVolume + 1 ];
static signed char MV_VolumeTable[ sizeof( VOLUME_TABLE_16BIT ) ];
static int MV_BufferDescriptor;
static int MV_TotalMemory;
static int MV_RequestedMixRate;
static char *HarshClipTable;
static int MV_MixRate;

static VOLUME_TABLE_8BIT  *MV_8BitVolumeTable  = ( void * )&MV_VolumeTable[ 0 ];
static VOLUME_TABLE_16BIT *MV_16BitVolumeTable = ( void * )&MV_VolumeTable[ 0 ];

static int MV_Installed   = FALSE;
static int MV_SoundCard   = SoundBlaster;
static int MV_TotalVolume = MV_MaxTotalVolume;
static int MV_MaxVoices   = 1;

static int MV_BufferSize = MixBufferSize;
static int MV_SampleSize = 1;
static int MV_NumberOfBuffers = NumberOfBuffers;

static int MV_MixMode    = MONO_8BIT;
static int MV_Silence    = SILENCE_8BIT;
static int MV_SwapLeftRight = FALSE;

static VoiceNode *MV_Voices = NULL;
//static VoiceNode MV_Voices[ 16 ];

static volatile VList MV_VoiceList = { NULL, NULL };
static volatile VList MV_VoicePool = { NULL, NULL };

static int MV_MixPage      = 0;
static int MV_PlayPage     = 0;
static int MV_VoiceHandle  = MV_MinVoiceHandle;

static void ( *MV_CallBackFunc )( unsigned long ) = NULL;

static void ( *MV_RecordFunc )( char *ptr, int length ) = NULL;

int MV_ErrorCode = MV_Ok;

#define MV_SetErrorCode( status ) \
   MV_ErrorCode   = ( status );


/*---------------------------------------------------------------------
   Function: MV_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *MV_ErrorString
   (
   int ErrorNumber
   )

   {
   char *ErrorString;

   switch( ErrorNumber )
      {
      case MV_Warning :
      case MV_Error :
         ErrorString = MV_ErrorString( MV_ErrorCode );
         break;

      case MV_Ok :
         ErrorString = "Multivoc ok.";
         break;

      case MV_UnsupportedCard :
         ErrorString = "Selected sound card is not supported by Multivoc.";
         break;

      case MV_NotInstalled :
         ErrorString = "Multivoc not installed.";
         break;

      case MV_NoVoices :
         ErrorString = "No free voices available to Multivoc.";
         break;

      case MV_NoMem :
         ErrorString = "Out of memory in Multivoc.";
         break;

      case MV_VoiceNotFound :
         ErrorString = "No voice with matching handle found.";
         break;

      case MV_BlasterError :
         ErrorString = BLASTER_ErrorString( BLASTER_Error );
         break;

      case MV_PasError :
         ErrorString = PAS_ErrorString( PAS_Error );
         break;

      #ifndef SOUNDSOURCE_OFF
      case MV_SoundSourceError :
         ErrorString = SS_ErrorString( SS_Error );
         break;
      #endif

      case MV_DPMI_Error :
         ErrorString = "DPMI Error in Multivoc.";
         break;

      default :
         ErrorString = "Unknown Multivoc error code.";
         break;
      }

   return( ErrorString );
   }


/**********************************************************************

   Memory locked functions:

**********************************************************************/


#define MV_LockStart MV_Mix8bitMono
#pragma aux MV_Mix16bitUnsignedStereo frame;


/*---------------------------------------------------------------------
   Function: MV_Mix8bitMono

   Mixes the sound into the buffer as an 8 bit mono sample.
---------------------------------------------------------------------*/

void MV_Mix8bitMono
   (
   VoiceNode *voice,
   int        buffer
   )

   {
//   MONO8        *to;
   char         *to;
   VOLUME8      *VolumeTable;
   char         *start;
   unsigned char *from;
   unsigned long position;
   int           length;
   unsigned long rate;

   int samp;

   to = ( MONO8 * )MV_MixBuffer[ buffer ];

   if ( voice->DemandFeed != NULL && voice->sound == NULL )
      {
      MV_GetNextVOCBlock( voice );
      if ( voice->sound == NULL || voice->length == 0 )
         {
         return;
         }
      }

   VolumeTable = ( VOLUME8 * )&( *MV_8BitVolumeTable )[ voice->Volume ];
   start    = voice->sound;
   rate     = voice->RateScale;
   position = voice->position;

   length   = MixBufferSize;

   // Add this voice to the mix
   while( length > 0 )
      {
      if ( position >= voice->length )
         {
         voice->position = position;
         MV_GetNextVOCBlock( voice );
         if ( voice->Playing )
            {
            if ( voice->DemandFeed != NULL && voice->sound == NULL )
               {
               return;
               }
            start    = voice->sound;
            rate     = voice->RateScale;
            position = voice->position;
            }
         else
            {
            break;
            }
         }

//      from = ( unsigned char * )start + ( position >> 16 );
//      *to += ( *VolumeTable )[ *from ];
      from = ( unsigned char * )start + ( position >> 16 );
      samp = ( ( int )*to ) + ( *VolumeTable )[ *from ];
      *to  = HarshClipTable[ samp + 0x80 ];

      to++;
      position += rate;

      length--;
      }

   voice->position = position;
   }


/*---------------------------------------------------------------------
   Function: MV_Mix8bitStereo

   Mixes the sound into the buffer as an 8 bit stereo sample.
---------------------------------------------------------------------*/

void MV_Mix8bitStereo
   (
   VoiceNode *voice,
   int        buffer
   )

   {
   STEREO8      *to;
   VOLUME8      *LeftVolumeTable;
   VOLUME8      *RightVolumeTable;
   char         *start;
   unsigned long position;
   int           length;
//   int           sample;
   unsigned long rate;

   unsigned char *from;
   int samp;

   to = ( STEREO8 * )MV_MixBuffer[ buffer ];

   if ( voice->DemandFeed != NULL && voice->sound == NULL )
      {
      MV_GetNextVOCBlock( voice );
      if ( voice->sound == NULL || voice->length == 0 )
         {
         return;
         }
      }

   LeftVolumeTable  = ( VOLUME8 * )&( *MV_8BitVolumeTable )[ voice->LeftVolume ];
   RightVolumeTable = ( VOLUME8 * )&( *MV_8BitVolumeTable )[ voice->RightVolume ];
   start    = voice->sound;
   rate     = voice->RateScale;
   position = voice->position;

   length   = MixBufferSize;

   // Add this voice to the mix
   while( length > 0 )
      {
      if ( position >= voice->length )
         {
         voice->position = position;
         MV_GetNextVOCBlock( voice );
         if ( voice->Playing )
            {
            if ( voice->DemandFeed != NULL && voice->sound == NULL )
               {
               return;
               }
            start    = voice->sound;
            rate     = voice->RateScale;
            position = voice->position;
            }
         else
            {
            break;
            }
         }
//      sample = ( int )*( start + ( position >> 16 ) );
//      to->left  += ( *LeftVolumeTable )[ sample ];
//      to->right += ( *RightVolumeTable )[ sample ];

      from = ( unsigned char * )start + ( position >> 16 );
      samp = ( ( int )to->left ) + ( *LeftVolumeTable )[ *from ];
      to->left = HarshClipTable[ samp + 0x80 ];
      samp = ( ( int )to->right ) + ( *RightVolumeTable )[ *from ];
      to->right = HarshClipTable[ samp + 0x80 ];

      to++;
      position += rate;
      length--;
      }

   voice->position = position;
   }


/*---------------------------------------------------------------------
   Function: MV_Mix16bitUnsignedMono

   Mixes the sound into the buffer as an 16 bit mono sample.
---------------------------------------------------------------------*/

void MV_Mix16bitUnsignedMono
   (
   VoiceNode *voice,
   int        buffer
   )

   {
//   MONO16       *to;
   unsigned short *to;
   VOLUME16     *VolumeTable;
   char         *start;
   unsigned char *from;
   unsigned long position;
   int           length;
   unsigned long rate;

   int samp;

//   to = ( MONO16 * )MV_MixBuffer[ buffer ];
   to = ( unsigned short * )MV_MixBuffer[ buffer ];

   if ( voice->DemandFeed != NULL && voice->sound == NULL )
      {
      MV_GetNextVOCBlock( voice );
      if ( voice->sound == NULL || voice->length == 0 )
         {
         return;
         }
      }

   VolumeTable = ( VOLUME16 * )&( *MV_16BitVolumeTable )[ voice->Volume ];
   start    = voice->sound;
   rate     = voice->RateScale;
   position = voice->position;
   length   = MixBufferSize;

   // Add this voice to the mix
   while( length > 0 )
      {
      if ( position >= voice->length )
         {
         voice->position = position;
         MV_GetNextVOCBlock( voice );
         if ( voice->Playing )
            {
            if ( voice->DemandFeed != NULL && voice->sound == NULL )
               {
               return;
               }
            start    = voice->sound;
            rate     = voice->RateScale;
            position = voice->position;
            }
         else
            {
            break;
            }
         }
//      from = ( unsigned char * )start + ( position >> 16 );
//      *to += ( *VolumeTable )[ *from ];

      from = ( unsigned char * )start + ( position >> 16 );
      samp = *to + ( *VolumeTable )[ *from ];
      if ( samp < 0 )
         {
         samp = 0;
         }
      else if ( samp > 0xffff )
         {
         samp = 0xffff;
         }
      *to = samp;
      to++;
      position += rate;
      length--;
      }

   voice->position = position;
   }


/*---------------------------------------------------------------------
   Function: MV_Mix16bitUnsignedStereo

   Mixes the sound into the buffer as an 16 bit stereo sample.
---------------------------------------------------------------------*/

void MV_Mix16bitUnsignedStereo
   (
   VoiceNode *voice,
   int        buffer
   )

   {
   STEREO16     *to;
   VOLUME16     *LeftVolumeTable;
   VOLUME16     *RightVolumeTable;
   char         *start;
   unsigned long position;
   int           length;
//   int           sample;
   unsigned long rate;

   unsigned char *from;
   int samp;

   to = ( STEREO16 * )MV_MixBuffer[ buffer ];

   if ( voice->DemandFeed != NULL && voice->sound == NULL )
      {
      MV_GetNextVOCBlock( voice );
      if ( voice->sound == NULL || voice->length == 0 )
         {
         return;
         }
      }

   LeftVolumeTable  = ( VOLUME16 * )&( *MV_16BitVolumeTable )[ voice->LeftVolume ];
   RightVolumeTable = ( VOLUME16 * )&( *MV_16BitVolumeTable )[ voice->RightVolume ];
   start    = voice->sound;
   rate     = voice->RateScale;
   position = voice->position;

   length   = MixBufferSize;

   // Add this voice to the mix
   while( length > 0 )
      {
      if ( position >= voice->length )
         {
         voice->position = position;
         MV_GetNextVOCBlock( voice );
         if ( voice->Playing )
            {
            if ( voice->DemandFeed != NULL && voice->sound == NULL )
               {
               return;
               }
            start    = voice->sound;
            rate     = voice->RateScale;
            position = voice->position;
            }
         else
            {
            break;
            }
         }
//      sample = ( int )*( start + ( position >> 16 ) );
//      to->left  += ( *LeftVolumeTable )[ sample ];
//      to->right += ( *RightVolumeTable )[ sample ];

      from = ( unsigned char * )start + ( position >> 16 );
      samp = to->left + ( *LeftVolumeTable )[ *from ];
      if ( samp < 0 )
         {
         samp = 0;
         }
      else if ( samp > 0xffff )
         {
         samp = 0xffff;
         }
      to->left = samp;
//      to->left = ( length & 127 ) << 8;

      samp = to->right + ( *RightVolumeTable )[ *from ];
      if ( samp < 0 )
         {
         samp = 0;
         }
      else if ( samp > 0xffff )
         {
         samp = 0xffff;
         }
      to->right = samp;
//      to->right = ( length & 127 ) << 8;

      to++;
      position += rate;

      length--;
      }

   voice->position = position;
   }


/*---------------------------------------------------------------------
   Function: MV_DeleteDeadVoices

   Removes any voices that have finished playing.
---------------------------------------------------------------------*/

void MV_DeleteDeadVoices
   (
   int page
   )

   {
   VoiceNode   *voice;
   VoiceNode   *next;
   voice = MV_VoiceList.start;
   while( voice != NULL )
      {
      next = voice->next;

      // Is this voice done?
      if ( !voice->Active[ page ] )
         {
         // Yes, move it from the play list into the free list
         LL_Remove( VoiceNode, &MV_VoiceList, voice );
         LL_AddToTail( VoiceNode, &MV_VoicePool, voice );

         if ( MV_CallBackFunc && !voice->DemandFeed )
            {
            MV_CallBackFunc( voice->callbackval );
            }
         voice->DemandFeed = NULL;
         }

      voice = next;
      }

   }


/*---------------------------------------------------------------------
   Function: MV_ServiceVoc

   Starts playback of the waiting buffer and mixes the next one.
---------------------------------------------------------------------*/

void MV_ServiceVoc
   (
   void
   )

   {
   VoiceNode *voice;

   MV_DeleteDeadVoices( MV_PlayPage );
   MV_PlayPage = MV_MixPage;
   MV_MixPage++;
   if ( MV_MixPage >= MV_NumberOfBuffers )
      {
      MV_MixPage = 0;
      }

   if ( !MV_BufferEmpty[ MV_MixPage ] )
      {
      ClearBuffer_DW( MV_MixBuffer[ MV_MixPage ], MV_Silence, MV_BufferSize >> 2 );
      MV_BufferEmpty[ MV_MixPage ] = TRUE;
      }

   voice = MV_VoiceList.start;
   switch( MV_MixMode )
      {
      case MONO_8BIT :
         while ( voice )
            {
            MV_BufferEmpty[ MV_MixPage ] = FALSE;
            MV_Mix8bitMono( voice, MV_MixPage );
            voice->Active[ MV_MixPage ] = voice->Playing;
            voice = voice->next;
            }
         break;
      case STEREO_8BIT :
         while ( voice )
            {
            MV_BufferEmpty[ MV_MixPage ] = FALSE;
            MV_Mix8bitStereo( voice, MV_MixPage );
            voice->Active[ MV_MixPage ] = voice->Playing;
            voice = voice->next;
            }
         break;
      case MONO_16BIT :
         while ( voice )
            {
            MV_BufferEmpty[ MV_MixPage ] = FALSE;
            MV_Mix16bitUnsignedMono( voice, MV_MixPage );
            voice->Active[ MV_MixPage ] = voice->Playing;
            voice = voice->next;
            }
         break;
      case STEREO_16BIT :
         while ( voice )
            {
            MV_BufferEmpty[ MV_MixPage ] = FALSE;
            MV_Mix16bitUnsignedStereo( voice, MV_MixPage );
            voice->Active[ MV_MixPage ] = voice->Playing;
            voice = voice->next;
            }
         break;
      }
   }


/*---------------------------------------------------------------------
   Function: MV_ServiceRecord

   Starts recording of the waiting buffer.
---------------------------------------------------------------------*/

static void MV_ServiceRecord
   (
   void
   )

   {
   if ( MV_RecordFunc )
      {
      MV_RecordFunc( MV_MixBuffer[ 0 ] + MV_MixPage * MixBufferSize,
         MixBufferSize );
      }

   // Toggle which buffer we'll mix next
   MV_MixPage++;
   if ( MV_MixPage >= NumberOfBuffers )
      {
      MV_MixPage = 0;
      }
   }


/*---------------------------------------------------------------------
   Function: MV_GetVoice

   Locates the voice with the specified handle.
---------------------------------------------------------------------*/

VoiceNode *MV_GetVoice
   (
   int handle
   )

   {
   VoiceNode *voice;
   unsigned  flags;

   flags = DisableInterrupts();

   voice = MV_VoiceList.start;

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
      MV_SetErrorCode( MV_VoiceNotFound );
      }

   return( voice );
   }


/*---------------------------------------------------------------------
   Function: MV_VoicePlaying

   Checks if the voice associated with the specified handle is
   playing.
---------------------------------------------------------------------*/

int MV_VoicePlaying
   (
   int handle
   )

   {
   VoiceNode   *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( FALSE );
      }

   voice = MV_GetVoice( handle );

   if ( voice == NULL )
      {
      return( FALSE );
      }

   return( TRUE );
   }


/*---------------------------------------------------------------------
   Function: MV_KillAllVoices

   Stops output of all currently active voices.
---------------------------------------------------------------------*/

int MV_KillAllVoices
   (
   void
   )

   {
   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Remove all the voices from the list
   while( MV_VoiceList.start != NULL )
      {
      MV_Kill( MV_VoiceList.start->handle );
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Kill

   Stops output of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_Kill
   (
   int handle
   )

   {
   VoiceNode *voice;
   unsigned  flags;
   unsigned  long callbackval;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   flags = DisableInterrupts();

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      RestoreInterrupts( flags );
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   callbackval = voice->callbackval;

   // move the voice from the play list to the free list
   LL_Remove( VoiceNode, &MV_VoiceList, voice );
   LL_AddToTail( VoiceNode, &MV_VoicePool, voice );

   RestoreInterrupts( flags );

   if ( MV_CallBackFunc && !voice->DemandFeed )
      {
      MV_CallBackFunc( callbackval );
      }

   MV_SetErrorCode( MV_Ok );
   voice->DemandFeed = NULL;
   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_VoicesPlaying

   Determines the number of currently active voices.
---------------------------------------------------------------------*/

int MV_VoicesPlaying
   (
   void
   )

   {
   VoiceNode   *voice;
   int         NumVoices = 0;
   unsigned    flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( 0 );
      }

   flags = DisableInterrupts();

   voice = MV_VoiceList.start;
   while( voice != NULL )
      {
      NumVoices++;
      voice = voice->next;
      }

   RestoreInterrupts( flags );

   return( NumVoices );
   }


/*---------------------------------------------------------------------
   Function: MV_AllocVoice

   Retrieve an inactive or lower priority voice for output.
---------------------------------------------------------------------*/

VoiceNode *MV_AllocVoice
   (
   int priority
   )

   {
   VoiceNode   *voice;
   VoiceNode   *node;
   unsigned    flags;

   flags = DisableInterrupts();

   // Check if we have any free voices
   if ( MV_VoicePool.start == NULL )
      {
      // check if we have a higher priority than a voice that is playing.
      node = MV_VoiceList.start;
      voice = node;
      while( node != NULL )
         {
         if ( node->priority < voice->priority )
            {
            voice = node;
            }

         node = node->next;
         }

      if ( priority >= voice->priority )
         {
         MV_Kill( voice->handle );
         }
      }

   // Check if any voices are in the voice pool
   if ( MV_VoicePool.start == NULL )
      {
      // No free voices
      RestoreInterrupts( flags );
      return( NULL );
      }

   voice = MV_VoicePool.start;
   LL_Remove( VoiceNode, &MV_VoicePool, voice );
   RestoreInterrupts( flags );

   // Find a free voice handle
   do
      {
      MV_VoiceHandle++;
      if ( MV_VoiceHandle < MV_MinVoiceHandle )
         {
         MV_VoiceHandle = MV_MinVoiceHandle;
         }
      }
   while( MV_VoicePlaying( MV_VoiceHandle ) );

   voice->handle = MV_VoiceHandle;

   return( voice );
   }


/*---------------------------------------------------------------------
   Function: MV_SetPitch

   Sets the pitch for the voice associated with the specified handle.
---------------------------------------------------------------------*/

int MV_SetPitch
   (
   int handle,
   int pitchoffset
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Error );
      }

   voice->PitchScale  = PITCH_GetScale( pitchoffset );
   voice->RateScale   = ( voice->SamplingRate *
      voice->PitchScale ) / MV_MixRate;

   MV_SetErrorCode( MV_Ok );
   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_SetPan
   (
   int handle,
   int vol,
   int left,
   int right
   )

   {
   VoiceNode *voice;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   voice = MV_GetVoice( handle );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_VoiceNotFound );
      return( MV_Warning );
      }

   voice->Volume      = MIX_VOLUME( vol );

   if ( MV_SwapLeftRight )
      {
      // SBPro uses reversed panning
      voice->LeftVolume  = MIX_VOLUME( right );
      voice->RightVolume = MIX_VOLUME( left );
      }
   else
      {
      voice->LeftVolume  = MIX_VOLUME( left );
      voice->RightVolume = MIX_VOLUME( right );
      }

   MV_SetErrorCode( MV_Ok );
   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int MV_Pan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_SetPan( handle, mid, left, right );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_SetMixMode

   Prepares Multivoc to play stereo of mono digitized sounds.
---------------------------------------------------------------------*/

int MV_SetMixMode
   (
   int mode
   )

   {
   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         MV_MixMode = BLASTER_SetMixMode( mode );
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         MV_MixMode = PAS_SetMixMode( mode );
         break;

      #ifndef SOUNDSOURCE_OFF
      case SoundSource :
      case TandySoundSource :
         MV_MixMode = SS_SetMixMode( mode );
         break;
      #endif
      }

   MV_SampleSize = sizeof( MONO8 );
   MV_Silence = SILENCE_8BIT;

   switch( MV_MixMode )
      {
      case MONO_8BIT :
         break;
      case STEREO_8BIT :
         MV_SampleSize = sizeof( STEREO8 );
         break;
      case MONO_16BIT :
         MV_SampleSize = sizeof( MONO16 );
         MV_Silence = SILENCE_16BIT;
         break;
      case STEREO_16BIT :
         MV_SampleSize = sizeof( STEREO16 );
         MV_Silence = SILENCE_16BIT;
         break;
      }

   MV_BufferSize = MixBufferSize * MV_SampleSize;

   MV_NumberOfBuffers = TotalBufferSize / MV_BufferSize;

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_StartPlayback

   Starts the sound playback engine.
---------------------------------------------------------------------*/

static int MV_StartPlayback
   (
   void
   )

   {
   int status;
   int buffer;

   // Initialize the buffers
   ClearBuffer_DW( MV_MixBuffer[ 0 ], MV_Silence, TotalBufferSize >> 2 );

   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_BufferEmpty[ buffer ] = TRUE;
      }

   // Set the mix buffer variables
   MV_PlayPage = 0;
   MV_MixPage  = 1;

   // Start playback
   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         status = BLASTER_BeginBufferedPlayback( MV_MixBuffer[ 0 ],
            TotalBufferSize, MV_NumberOfBuffers,
            MV_RequestedMixRate, MV_MixMode, MV_ServiceVoc );

         if ( status != BLASTER_Ok )
            {
            MV_SetErrorCode( MV_BlasterError );
            return( MV_Error );
            }

         MV_MixRate = BLASTER_GetPlaybackRate();
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         status = PAS_BeginBufferedPlayback( MV_MixBuffer[ 0 ],
            TotalBufferSize, MV_NumberOfBuffers,
            MV_RequestedMixRate, MV_MixMode, MV_ServiceVoc );

         if ( status != PAS_Ok )
            {
            MV_SetErrorCode( MV_PasError );
            return( MV_Error );
            }

         MV_MixRate = PAS_GetPlaybackRate();
         break;

      #ifndef SOUNDSOURCE_OFF
      case SoundSource :
      case TandySoundSource :
         SS_BeginBufferedPlayback( MV_MixBuffer[ 0 ],
            TotalBufferSize, MV_NumberOfBuffers,
            MV_ServiceVoc );
         MV_MixRate = SS_SampleRate;
         break;
      #endif
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_StopPlayback

   Stops the sound playback engine.
---------------------------------------------------------------------*/

void MV_StopPlayback
   (
   void
   )

   {
   VoiceNode *voice;
   VoiceNode *next;
   unsigned flags;

   // Stop sound playback
   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         BLASTER_StopPlayback();
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         PAS_StopPlayback();
         break;

      #ifndef SOUNDSOURCE_OFF
      case SoundSource :
      case TandySoundSource :
         SS_StopPlayback();
         break;
      #endif
      }

   flags = DisableInterrupts();
   voice = MV_VoiceList.start;
   while ( voice )
      {
      next = voice->next;
      LL_Remove( VoiceNode, &MV_VoiceList, voice );
      LL_AddToTail( VoiceNode, &MV_VoicePool, voice );
      if ( MV_CallBackFunc && !voice->DemandFeed )
         {
         MV_CallBackFunc( voice->callbackval );
         }
      voice->DemandFeed = NULL;
      voice = next;
      }
   RestoreInterrupts( flags );
   }


/*---------------------------------------------------------------------
   Function: MV_StartRecording

   Starts the sound recording engine.
---------------------------------------------------------------------*/

int MV_StartRecording
   (
   int MixRate,
   void ( *function )( char *ptr, int length )
   )

   {
   int status;

   MV_StopPlayback();
   ClearBuffer_DW( MV_MixBuffer[ 0 ], SILENCE_8BIT, TotalBufferSize >> 2 );
   MV_MixPage = 0;
   MV_PlayPage = 0;
   MV_RecordFunc = function;

   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         status = BLASTER_BeginBufferedRecord( MV_MixBuffer[ 0 ],
            TotalBufferSize, NumberOfBuffers, MixRate, MONO_8BIT,
            MV_ServiceRecord );
         if ( status != BLASTER_Ok )
            {
            MV_SetErrorCode( MV_BlasterError );
            return( MV_Error );
            }
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         status = PAS_BeginBufferedRecord( MV_MixBuffer[ 0 ],
            TotalBufferSize, NumberOfBuffers, MixRate, MONO_8BIT,
            MV_ServiceRecord );
         if ( status != PAS_Ok )
            {
            MV_SetErrorCode( MV_PasError );
            return( MV_Error );
            }
         break;

      default :
         MV_SetErrorCode( MV_UnsupportedCard );
         return( MV_Error );
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_LockEnd

   Used for determining the length of the functions to lock in memory.
---------------------------------------------------------------------*/

static void MV_LockEnd
   (
   void
   )

   {
   }


/*---------------------------------------------------------------------
   Function: MV_StopRecord

   Stops the sound record engine.
---------------------------------------------------------------------*/

void MV_StopRecord
   (
   void
   )

   {
   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         BLASTER_StopPlayback();
         break;
      case ProAudioSpectrum :
      case SoundMan16 :
         PAS_StopPlayback();
         break;
      }

   MV_StartPlayback();
   }


/*---------------------------------------------------------------------
   Function: MV_StartDemandFeedPlayback

   Plays a digitized sound from a user controlled buffering system.
---------------------------------------------------------------------*/

int MV_StartDemandFeedPlayback
   (
   void ( *function )( char **ptr, unsigned long *length ),
   int rate,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned long callbackval
   )

   {
   VoiceNode *voice;
   unsigned flags;

   voice = MV_AllocVoice( 0x7ffffff );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->NextBlock = NULL;
   voice->LoopStart = NULL;
   voice->LoopCount = 0;
   voice->DemandFeed = function;
   voice->BlockLength = 0;
   voice->PitchScale = PITCH_GetScale( pitchoffset );
   voice->SamplingRate = rate;
   voice->RateScale = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;
   voice->position = 0;
   voice->sound = NULL;
   voice->length = 0;
   voice->BlockLength = 0;
   voice->Playing = TRUE;
   MV_GetNextVOCBlock( voice );
   voice->next = NULL;
   voice->prev = NULL;
   memset( voice->Active, 1, sizeof( voice->Active ) );
   voice->Volume = MIX_VOLUME( vol );
   if ( MV_SwapLeftRight )
      {
      voice->LeftVolume = MIX_VOLUME( right );
      voice->RightVolume = MIX_VOLUME( left );
      }
   else
      {
      voice->LeftVolume = MIX_VOLUME( left );
      voice->RightVolume = MIX_VOLUME( right );
      }
   voice->priority = priority;
   voice->callbackval = callbackval;
   flags = DisableInterrupts();
   LL_AddToTail( VoiceNode, &MV_VoiceList, voice );
   RestoreInterrupts( flags );
   MV_SetErrorCode( MV_Ok );
   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_GetNextVOCBlock

   Interperate the information of a VOC format sound file.
---------------------------------------------------------------------*/

void MV_GetNextVOCBlock
   (
   VoiceNode *voice
   )

   {
   unsigned char *ptr;
   int            blocktype;
   int            lastblocktype;
   unsigned long  blocklength;
   unsigned long  samplespeed;
   unsigned int   tc;
   int            packtype;
   int            voicemode;
   int            done;
   unsigned       BitsPerSample;
   unsigned       Channels;
   unsigned       Format;

   if ( voice->BlockLength > 0 )
      {
      voice->sound       += 0x8000;
      voice->position    -= 0x80000000;
      voice->length       = min( voice->BlockLength, 0x8000 );
      voice->BlockLength -= voice->length;
      voice->length     <<= 16;
      return;
      }

   if ( voice->DemandFeed )
      {
      voice->position = 0;
      voice->DemandFeed( &voice->sound, &voice->BlockLength );
      voice->length = min( voice->BlockLength, MAX_BLOCK_LENGTH );
      voice->Playing = TRUE;
      voice->BlockLength -= voice->length;
      voice->length <<= 16;
      return;
      }

   ptr = ( unsigned char * )voice->NextBlock;

   voice->Playing = TRUE;

   done = FALSE;
   while( !done )
      {
      blocktype = ( int )*ptr;
      blocklength = ( *( unsigned long * )( ptr + 1 ) ) & 0x00ffffff;
      ptr += 4;

      voicemode = 0;
      lastblocktype = 0;
      switch( blocktype )
         {
         case 0 :
            // End of data
            voice->Playing = FALSE;
            done = TRUE;
            break;

         case 1 :
            // Sound data block
            if ( lastblocktype != 8 )
               {
               tc = ( unsigned int )*ptr;
               packtype = *( ptr + 1 );
               }

            ptr += 2;
            blocklength -= 2;

            samplespeed = 256000000L / ( 65536 - ( tc << 8 ) );

            // Skip packed or stereo data
            if ( ( packtype != 0 ) || ( voicemode != 0 ) )
               {
               ptr += blocklength;
               }
            else
               {
               done = TRUE;
               }
            break;

         case 2 :
            // Sound continuation block
            samplespeed = voice->SamplingRate;
            done = TRUE;
            break;

         case 3 :
            // Silence
            // Not implimented.
            ptr += blocklength;
            break;

         case 4 :
            // Marker
            // Not implimented.
            ptr += blocklength;
            break;

         case 5 :
            // ASCII string
            // Not implimented.
            ptr += blocklength;
            break;

         case 6 :
            // Repeat begin
            voice->LoopCount = *( unsigned short * )ptr;
            ptr += blocklength;
            voice->LoopStart = ptr;
            break;

         case 7 :
            // Repeat end
            ptr += blocklength;
            if ( lastblocktype == 6 )
               {
               voice->LoopCount = 0;
               }
            else
               {
               if ( ( voice->LoopCount > 0 ) && ( voice->LoopStart != NULL ) )
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
               }
            break;

         case 8 :
            // Extended block
            tc = *( unsigned short * )ptr;
            packtype = *( ptr + 2 );
            voicemode = *( ptr + 3 );
            ptr += blocklength;
            break;

         case 9 :
            // New sound data block
            samplespeed = *( unsigned long * )ptr;
            BitsPerSample = ( unsigned )*( ptr + 4 );
            Channels = ( unsigned )*( ptr + 5 );
            Format = ( unsigned )*( unsigned short * )( ptr + 6 );

            if ( ( BitsPerSample == 8 ) && ( Channels == 1 ) &&
               ( Format == 0 ) )
               {
               ptr += 12;
               done = TRUE;
               }
            else
               {
               ptr += blocklength;
               }
            break;

         default :
            // Unknown data.  Probably not a VOC file.
            voice->Playing = FALSE;
            done = TRUE;
            break;
         }

      lastblocktype = blocktype;
      }

   if ( voice->Playing )
      {
      voice->NextBlock    = ptr + blocklength;
      voice->sound        = ptr;

      voice->SamplingRate = samplespeed;
      voice->RateScale   = ( voice->SamplingRate * voice->PitchScale ) / MV_MixRate;

      voice->position     = 0;
      voice->length       = min( blocklength, 0x8000 );
      voice->BlockLength  = blocklength - voice->length;
      voice->length     <<= 16;
      }
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVOC

   Begin playback of sound data with the given sound levels and
   priority.
---------------------------------------------------------------------*/

int MV_PlayVOC
   (
   char *ptr,
   int   pitchoffset,
   int   vol,
   int   left,
   int   right,
   int   priority,
   unsigned long callbackval
   )

   {
   VoiceNode   *voice;
   int         buffer;
   unsigned    flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   // Request a voice from the voice pool
   voice = MV_AllocVoice( priority );
   if ( voice == NULL )
      {
      MV_SetErrorCode( MV_NoVoices );
      return( MV_Error );
      }

   voice->NextBlock   = ptr + *( unsigned short int * )( ptr + 0x14 );
   voice->LoopStart   = NULL;
   voice->LoopCount   = 0;
   voice->BlockLength = 0;
   voice->PitchScale  = PITCH_GetScale( pitchoffset );

   MV_GetNextVOCBlock( voice );

   voice->next = NULL;
   voice->prev = NULL;

   for( buffer = 0; buffer < NumberOfBuffers; buffer++ )
      {
      voice->Active[ buffer ] = TRUE;
      }

   voice->Volume      = MIX_VOLUME( vol );

   if ( MV_SwapLeftRight )
      {
      // SBPro uses reversed panning
      voice->LeftVolume  = MIX_VOLUME( right );
      voice->RightVolume = MIX_VOLUME( left );
      }
   else
      {
      voice->LeftVolume  = MIX_VOLUME( left );
      voice->RightVolume = MIX_VOLUME( right );
      }

   voice->priority = priority;
   voice->callbackval = callbackval;

   flags = DisableInterrupts();
   LL_AddToTail( VoiceNode, &MV_VoiceList, voice );
   RestoreInterrupts( flags );

   MV_SetErrorCode( MV_Ok );
   return( voice->handle );
   }


/*---------------------------------------------------------------------
   Function: MV_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int MV_PlayVOC3D
   (
   char *ptr,
   int  pitchoffset,
   int  angle,
   int  distance,
   int  priority,
   unsigned long callbackval
   )

   {
   int left;
   int right;
   int mid;
   int volume;
   int status;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   if ( distance < 0 )
      {
      distance  = -distance;
      angle    += MV_NumPanPositions / 2;
      }

   volume = MIX_VOLUME( distance );

   // Ensure angle is within 0 - 31
   angle &= MV_MaxPanPosition;

   left  = MV_PanTable[ angle ][ volume ].left;
   right = MV_PanTable[ angle ][ volume ].right;
   mid   = max( 0, 255 - distance );

   status = MV_PlayVOC( ptr, pitchoffset, mid, left, right, priority,
      callbackval );

   return( status );
   }


/*---------------------------------------------------------------------
   Function: MV_CalcVolume

   Create the table used to convert sound data to a specific volume
   level.
---------------------------------------------------------------------*/

void MV_CalcVolume
   (
   int MaxLevel
   )

   {
   int volume;
   int val;
   int level;
   int rate;
   int i;
   unsigned    flags;

   flags = DisableInterrupts();


   // For each volume level, create a translation table with the
   // appropriate volume calculated.
   rate  = ( MaxLevel << 16 ) / MV_MaxVolume;
   level = 0;

   if ( MV_MixMode & SIXTEEN_BIT )
      {
      for( volume = 0; volume <= MV_MaxVolume; volume++ )
         {
         for( i = 0; i < 256; i++ )
            {
            val = i - 128;
            val *= level;
//            val /= MV_MaxVoices;
//            val >>= 16;
            val >>= 16;
            ( *MV_16BitVolumeTable )[ volume ][ i ] = val;
            }
         level += rate;
         }
      }
   else
      {
      for( volume = 0; volume <= MV_MaxVolume; volume++ )
         {
         for( i = 0; i < 256; i++ )
            {
            val = i - 128;
            val *= level;
// DEBUG
//            val /= MV_MaxVoices;
            val >>= 24;
            ( *MV_8BitVolumeTable )[ volume ][ i ] = val;
            }
         level += rate;
         }
      for( volume = 0; volume < 128; volume++ )
         {
         HarshClipTable[ volume ] = 0;
         HarshClipTable[ volume + 384 ] = 255;
         }
      for( volume = 0; volume < 256; volume++ )
         {
         HarshClipTable[ volume + 128 ] = volume;
         }
      }

   RestoreInterrupts( flags );
   }


/*---------------------------------------------------------------------
   Function: MV_CalcPanTable

   Create the table used to determine the stereo volume level of
   a sound located at a specific angle and distance from the listener.
---------------------------------------------------------------------*/

void MV_CalcPanTable
   (
   void
   )

   {
   int   level;
   int   angle;
   int   distance;
   int   HalfAngle;
   int   ramp;

   HalfAngle = ( MV_NumPanPositions / 2 );

   for( distance = 0; distance <= MV_MaxVolume; distance++ )
      {
      level = ( 255 * ( MV_MaxVolume - distance ) ) / MV_MaxVolume;
      for( angle = 0; angle <= HalfAngle / 2; angle++ )
         {
         ramp = level - ( ( level * angle ) /
            ( MV_NumPanPositions / 4 ) );

         MV_PanTable[ angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle - angle ][ distance ].left = ramp;
         MV_PanTable[ HalfAngle + angle ][ distance ].left = level;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].left = level;

         MV_PanTable[ angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle - angle ][ distance ].right = level;
         MV_PanTable[ HalfAngle + angle ][ distance ].right = ramp;
         MV_PanTable[ MV_MaxPanPosition - angle ][ distance ].right = ramp;
         }
      }
   }


/*---------------------------------------------------------------------
   Function: MV_SetVolume

   Sets the volume of digitized sound playback.
---------------------------------------------------------------------*/

void MV_SetVolume
   (
   int volume
   )

   {
   int maxlevel;

   volume = max( 0, volume );
   volume = min( volume, MV_MaxTotalVolume );

   MV_TotalVolume = volume;

   maxlevel = ( volume << 8 ) / MV_MaxTotalVolume;
   //printf( "maxlevel = %d\n", maxlevel );

   // Calculate volume table
   MV_CalcVolume( maxlevel );
   }


/*---------------------------------------------------------------------
   Function: MV_GetVolume

   Returns the volume of digitized sound playback.
---------------------------------------------------------------------*/

int MV_GetVolume
   (
   void
   )

   {
   return( MV_TotalVolume );
   }


/*---------------------------------------------------------------------
   Function: MV_SetCallBack

   Set the function to call when a voice stops.
---------------------------------------------------------------------*/

void MV_SetCallBack
   (
   void ( *function )( unsigned long )
   )

   {
   MV_CallBackFunc = function;
   }


/*---------------------------------------------------------------------
   Function: MV_Init

   Perform the initialization of variables and memory used by
   Multivoc.
---------------------------------------------------------------------*/

int MV_Init
   (
   int soundcard,
   int MixRate,
   int Voices,
   int MixMode
   )

   {
   char *ptr;
   int  status;
   int  buffer;
   int  index;

   if ( MV_Installed )
      {
      MV_Shutdown();
      }

   MV_SetErrorCode( MV_Ok );

   status = MV_LockMemory();
   if ( status != MV_Ok )
      {
      return( status );
      }

   MV_TotalMemory = Voices * sizeof( VoiceNode ) + sizeof( HARSH_CLIP_TABLE_8 );
   status = USRHOOKS_GetMem( ( void ** )&ptr, MV_TotalMemory );
   if ( status != USRHOOKS_Ok )
      {
      MV_UnlockMemory();
      MV_SetErrorCode( MV_NoMem );
      return( MV_Error );
      }

   status = DPMI_LockMemory( ptr, MV_TotalMemory );
   if ( status != DPMI_Ok )
      {
      USRHOOKS_FreeMem( ptr );
      MV_UnlockMemory();
      MV_SetErrorCode( MV_DPMI_Error );
      return( MV_Error );
      }
   MV_Voices = ( VoiceNode * )ptr;
   HarshClipTable = ptr + ( MV_TotalMemory - sizeof( HARSH_CLIP_TABLE_8 ) );

   // Set number of voices before calculating volume table
   MV_MaxVoices = Voices;

   MV_VoiceList.start = NULL;
   MV_VoiceList.end   = NULL;
   MV_VoicePool.start = NULL;
   MV_VoicePool.end   = NULL;

   for( index = 0; index < Voices; index++ )
      {
      LL_AddToTail( VoiceNode, &MV_VoicePool, &MV_Voices[ index ] );
      }

   // Allocate mix buffer within 1st megabyte
   status = DPMI_GetDOSMemory( &ptr, &MV_BufferDescriptor,
      2 * TotalBufferSize );

   if ( status )
      {
      DPMI_UnlockMemory( MV_Voices, MV_TotalMemory );
      USRHOOKS_FreeMem( MV_Voices );
      MV_Voices = NULL;
      MV_TotalMemory = 0;
      MV_SetErrorCode( MV_NoMem );
      MV_UnlockMemory();
      return( MV_Error );
      }

   MV_SwapLeftRight = FALSE;

   // Initialize the sound card
   switch( soundcard )
      {
      case SoundBlaster :
      case Awe32 :
         status = BLASTER_Init();
         if ( status != BLASTER_Ok )
            {
            MV_SetErrorCode( MV_BlasterError );
            }

         if ( ( BLASTER_Config.Type == SBPro ) ||
            ( BLASTER_Config.Type == SBPro2 ) )
            {
            MV_SwapLeftRight = TRUE;
            }
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         status = PAS_Init();
         if ( status != PAS_Ok )
            {
            MV_SetErrorCode( MV_PasError );
            }
         break;

      #ifndef SOUNDSOURCE_OFF
      case SoundSource :
      case TandySoundSource :
         status = SS_Init( soundcard );
         if ( status != SS_Ok )
            {
            MV_SetErrorCode( MV_SoundSourceError );
            }
         break;
      #endif

      default :
         MV_SetErrorCode( MV_UnsupportedCard );
         break;
      }

   if ( MV_ErrorCode != MV_Ok )
      {
      DPMI_UnlockMemory( MV_Voices, MV_TotalMemory );
      USRHOOKS_FreeMem( MV_Voices );
      MV_Voices = NULL;
      MV_TotalMemory = 0;
      DPMI_FreeDOSMemory( MV_BufferDescriptor );
      MV_UnlockMemory();
      return( MV_Error );
      }

   MV_SoundCard = soundcard;

   MV_Installed = TRUE;

   MV_CallBackFunc = NULL;
   MV_RecordFunc = NULL;

   // Set the sampling rate
   MV_RequestedMixRate = MixRate;

   // Set Mixer to play stereo digitized sound
   MV_SetMixMode( MixMode );

   // Make sure we don't cross a physical page
   if ( ( ( unsigned long )ptr & 0xffff ) + TotalBufferSize > 0x10000 )
      {
      ptr = ( char * )( ( ( unsigned long )ptr & 0xff0000 ) + 0x10000 );
      }

   for( buffer = 0; buffer < MV_NumberOfBuffers; buffer++ )
      {
      MV_MixBuffer[ buffer ] = ptr;
      ptr += MV_BufferSize;
      }

   // Calculate pan table
   MV_CalcPanTable();

   MV_SetVolume( MV_MaxTotalVolume );

   // Init pitch scaler
   PITCH_Init();

   // Start the playback engine
   status = MV_StartPlayback();
   if ( status != MV_Ok )
      {
      // Preserve error code while we shutdown.
      status = MV_ErrorCode;
      MV_Shutdown();
      MV_SetErrorCode( status );
      return( MV_Error );
      }

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_Shutdown

   Restore any resources allocated by Multivoc back to the system.
---------------------------------------------------------------------*/

int MV_Shutdown
   (
   void
   )

   {
   unsigned flags;

   if ( !MV_Installed )
      {
      MV_SetErrorCode( MV_NotInstalled );
      return( MV_Error );
      }

   flags = DisableInterrupts();

   MV_KillAllVoices();

   MV_Installed = FALSE;

   // Stop the sound playback engine
   MV_StopPlayback();

   // Shutdown the sound card
   switch( MV_SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
         BLASTER_Shutdown();
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         PAS_Shutdown();
         break;

      #ifndef SOUNDSOURCE_OFF
      case SoundSource :
      case TandySoundSource :
         SS_Shutdown();
         break;
      #endif
      }

   RestoreInterrupts( flags );

   // Free any voices we allocated
   DPMI_UnlockMemory( MV_Voices, MV_TotalMemory );
   USRHOOKS_FreeMem( MV_Voices );
   MV_Voices = NULL;
   MV_TotalMemory = 0;

   MV_VoiceList.start = NULL;
   MV_VoiceList.end   = NULL;
   MV_VoicePool.start = NULL;
   MV_VoicePool.end   = NULL;

   MV_MaxVoices = 1;

   // Release the descriptor from our mix buffer
   DPMI_FreeDOSMemory( MV_BufferDescriptor );
   memset( MV_MixBuffer, 0, sizeof( MV_MixBuffer ) );

   return( MV_Ok );
   }


/*---------------------------------------------------------------------
   Function: MV_UnlockMemory

   Unlocks all neccessary data.
---------------------------------------------------------------------*/

void MV_UnlockMemory
   (
   void
   )

   {
   PITCH_UnlockMemory();

   DPMI_UnlockMemoryRegion( MV_LockStart, MV_LockEnd );
   DPMI_Unlock( MV_VolumeTable );
   DPMI_Unlock( MV_8BitVolumeTable );
   DPMI_Unlock( MV_16BitVolumeTable );
   DPMI_Unlock( MV_PanTable );
   DPMI_Unlock( MV_Installed );
   DPMI_Unlock( MV_SoundCard );
   DPMI_Unlock( MV_TotalVolume );
   DPMI_Unlock( MV_MaxVoices );
   DPMI_Unlock( MV_BufferSize );
   DPMI_Unlock( MV_SampleSize );
   DPMI_Unlock( MV_NumberOfBuffers );
   DPMI_Unlock( MV_MixMode );
   DPMI_Unlock( MV_Silence );
   DPMI_Unlock( MV_SwapLeftRight );
   DPMI_Unlock( MV_RequestedMixRate );
   DPMI_Unlock( MV_MixRate );
   DPMI_Unlock( MV_BufferDescriptor );
   DPMI_Unlock( MV_MixBuffer );
   DPMI_Unlock( MV_BufferEmpty );
   DPMI_Unlock( MV_Voices );
   DPMI_Unlock( MV_VoiceList );
   DPMI_Unlock( MV_VoicePool );
   DPMI_Unlock( MV_MixPage );
   DPMI_Unlock( MV_PlayPage );
   DPMI_Unlock( MV_VoiceHandle );
   DPMI_Unlock( MV_CallBackFunc );
   DPMI_Unlock( MV_RecordFunc );
   DPMI_Unlock( HarshClipTable );
   DPMI_Unlock( MV_ErrorCode );
   }


/*---------------------------------------------------------------------
   Function: MV_LockMemory

   Locks all neccessary data.
---------------------------------------------------------------------*/

int MV_LockMemory
   (
   void
   )

   {
   int status;
   int pitchstatus;

   status  = DPMI_LockMemoryRegion( MV_LockStart, MV_LockEnd );
   status |= DPMI_Lock( MV_VolumeTable );
   status |= DPMI_Lock( MV_8BitVolumeTable );
   status |= DPMI_Lock( MV_16BitVolumeTable );
   status |= DPMI_Lock( MV_PanTable );
   status |= DPMI_Lock( MV_Installed );
   status |= DPMI_Lock( MV_SoundCard );
   status |= DPMI_Lock( MV_TotalVolume );
   status |= DPMI_Lock( MV_MaxVoices );
   status |= DPMI_Lock( MV_BufferSize );
   status |= DPMI_Lock( MV_SampleSize );
   status |= DPMI_Lock( MV_NumberOfBuffers );
   status |= DPMI_Lock( MV_MixMode );
   status |= DPMI_Lock( MV_Silence );
   status |= DPMI_Lock( MV_SwapLeftRight );
   status |= DPMI_Lock( MV_RequestedMixRate );
   status |= DPMI_Lock( MV_MixRate );
   status |= DPMI_Lock( MV_BufferDescriptor );
   status |= DPMI_Lock( MV_MixBuffer );
   status |= DPMI_Lock( MV_BufferEmpty );
   status |= DPMI_Lock( MV_Voices );
   status |= DPMI_Lock( MV_VoiceList );
   status |= DPMI_Lock( MV_VoicePool );
   status |= DPMI_Lock( MV_MixPage );
   status |= DPMI_Lock( MV_PlayPage );
   status |= DPMI_Lock( MV_VoiceHandle );
   status |= DPMI_Lock( MV_CallBackFunc );
   status |= DPMI_Lock( MV_RecordFunc );
   status |= DPMI_Lock( HarshClipTable );
   status |= DPMI_Lock( MV_ErrorCode );

   pitchstatus = PITCH_LockMemory();
   if ( ( pitchstatus != PITCH_Ok ) || ( status != DPMI_Ok ) )
      {
      MV_UnlockMemory();
      MV_SetErrorCode( MV_DPMI_Error );
      return( MV_Error );
      }

   return( MV_Ok );
   }
