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
   module: FX_MAN.C

   author: James R. Dose
   date:   March 17, 1994

   Device independant sound effect routines.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "sndcards.h"
#include "multivoc.h"
#include "blaster.h"
#include "pas16.h"
#include "sndscape.h"
#include "guswave.h"
#include "sndsrc.h"
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
#include "adlibfx.h"
#include "pcfx.h"
#endif
#include "ll_man.h"
#include "user.h"
#include "fx_man.h"
#if (LIBVER_ASSREV < 19960116L) // *** VERSIONS RESTORATION ***
#include "memcheck.h"
#endif

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )

static unsigned FX_MixRate;

int FX_SoundDevice = -1;
int FX_ErrorCode = FX_Ok;
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
int FX_Installed = FALSE;
#endif

void TextMode( void );
#pragma aux TextMode =  \
    "mov    ax, 0003h", \
    "int    10h"        \
    modify [ ax ];

#define FX_SetErrorCode( status ) \
   FX_ErrorCode = ( status );

/*---------------------------------------------------------------------
   Function: FX_ErrorString

   Returns a pointer to the error message associated with an error
   number.  A -1 returns a pointer the current error.
---------------------------------------------------------------------*/

char *FX_ErrorString
   (
   int ErrorNumber
   )

   {
   char *ErrorString;

   switch( ErrorNumber )
      {
      case FX_Warning :
      case FX_Error :
         ErrorString = FX_ErrorString( FX_ErrorCode );
         break;

      case FX_Ok :
         ErrorString = "Fx ok.";
         break;

      case FX_SoundCardError :
         switch( FX_SoundDevice )
         {
            case SoundBlaster :
            case Awe32 :
               ErrorString = BLASTER_ErrorString( BLASTER_Error );
               break;

            case ProAudioSpectrum :
            case SoundMan16 :
               ErrorString = PAS_ErrorString( PAS_Error );
               break;

            // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
            case Adlib :
               ErrorString = ADLIBFX_ErrorString( ADLIBFX_Error );
               break;

            case PC :
               ErrorString = PCFX_ErrorString( PCFX_Error );
               break;
#endif


            case UltraSound :
               ErrorString = GUSWAVE_ErrorString( GUSWAVE_Error );
               break;

            case SoundSource :
            case TandySoundSource :
               ErrorString = SS_ErrorString( SS_Error );
               break;
            }
         break;

      case FX_InvalidCard :
         ErrorString = "Invalid Sound Fx device.";
         break;

      case FX_MultiVocError :
         ErrorString = MV_ErrorString( MV_Error );
         break;

      case FX_InvalidVOC :
         ErrorString = "Invalid VOC file.";
         break;

      case FX_DPMI_Error :
         ErrorString = "DPMI Error in FX_MAN.";
         break;

      default :
         ErrorString = "Unknown Fx error code.";
         break;
      }

   return( ErrorString );
   }


/*---------------------------------------------------------------------
   Function: FX_SetupCard

   Sets the configuration of a sound device.
---------------------------------------------------------------------*/

int FX_SetupCard
   (
   int SoundCard,
   fx_device *device
   )

   {
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   BLASTER_CONFIG Blaster;
#endif
   int status;
   int DeviceStatus;

   FX_SoundDevice = SoundCard;

   status = FX_Ok;

   switch( SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
         DeviceStatus = BLASTER_GetEnv( &Blaster );
         if ( DeviceStatus != BLASTER_Ok )
#else
         DeviceStatus = BLASTER_Init();
         if ( DeviceStatus != BLASTER_Ok )
#endif
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            break;
            }

         // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
         BLASTER_SetCardSettings( Blaster );

         DeviceStatus = BLASTER_Init();
         if ( DeviceStatus != BLASTER_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            return( FX_Error );
            }
#endif
         device->MaxVoices = 8;
         BLASTER_GetCardInfo( &device->MaxSampleBits, &device->MaxChannels );
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         DeviceStatus = PAS_Init();
         if ( DeviceStatus != PAS_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            break;
            }

         device->MaxVoices = 8;
         PAS_GetCardInfo( &device->MaxSampleBits, &device->MaxChannels );
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
      case Adlib :
      case PC :
         device->MaxVoices     = 1;
         device->MaxChannels   = 1;
         device->MaxSampleBits = status;
         break;
#endif

      case GenMidi :
      case SoundCanvas :
      case WaveBlaster :
         device->MaxVoices     = 0;
         device->MaxSampleBits = 0;
         device->MaxChannels   = 0;
         break;


      case UltraSound :
         if ( GUSWAVE_Init( 8 ) != GUSWAVE_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            break;
            }

         device->MaxVoices     = 8;
         device->MaxSampleBits = status;
         device->MaxChannels   = status;
         break;

      case SoundSource :
      case TandySoundSource :
         DeviceStatus = SS_SetPort( 0x378 );
         if ( DeviceStatus != SS_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            break;
            }
         device->MaxVoices     = 8;
         device->MaxSampleBits = 8;
         device->MaxChannels   = 1;
         break;

      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }

   return( status );
   }






/*---------------------------------------------------------------------
   Function: FX_Init

   Selects which sound device to use.
---------------------------------------------------------------------*/

int FX_Init
   (
   int SoundCard,
   int numvoices,
   int numchannels,
   int samplebits,
   unsigned mixrate
   )

   {
   int status;
   int devicestatus;
   int mode;

   status = LL_LockMemory();
   if ( status != LL_Ok )
      {
      FX_SetErrorCode( FX_DPMI_Error );
      status = FX_Error;
      goto done;
      }

   FX_MixRate = mixrate;

   status = FX_Ok;
   FX_SoundDevice = SoundCard;
   switch( SoundCard )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
      case SoundSource :
      case TandySoundSource :
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
      case UltraSound :
#endif
         mode = 0;
         if ( numchannels == 2 )
            {
            mode = 1;
            }
         if ( samplebits == 16 )
            {
            mode |= 2;
            }
         devicestatus = MV_Init( SoundCard, FX_MixRate, numvoices, mode );
         if ( devicestatus != MV_Ok )
            {
            FX_SetErrorCode( FX_MultiVocError );
            status = FX_Error;
            }
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
#if (LIBVER_ASSREV < 19950821L)
      case UltraSound :
         devicestatus = GUSWAVE_Init( numvoices );
         if ( devicestatus != GUSWAVE_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            }
         break;

#endif
      case Adlib :
         devicestatus = ADLIBFX_Init();
         if ( devicestatus != ADLIBFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            }
         break;

      case PC :
         devicestatus = PCFX_Init();
         if ( devicestatus != PCFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Error;
            }
         break;

#endif // LIBVER_ASSREV
      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }

   if ( status != FX_Ok )
      {
      LL_UnlockMemory();
      }
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   else
      {
      FX_Installed = TRUE;
      }
#endif

done:
   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_Shutdown

   Terminates use of sound device.
---------------------------------------------------------------------*/

#pragma aux FX_Shutdown frame;
int FX_Shutdown
   (
   void
   )

   {
   int status;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   if ( !FX_Installed )
      {
      return( FX_Ok );
      }
#endif

   status = FX_Ok;
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
      case SoundSource :
      case TandySoundSource :
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
      case UltraSound :
#endif
         status = MV_Shutdown();
         if ( status != MV_Ok )
            {
            FX_SetErrorCode( FX_MultiVocError );
            status = FX_Error;
            }
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
#if (LIBVER_ASSREV < 19950821L)
      case UltraSound :
         GUSWAVE_Shutdown();
         break;

#endif
      case Adlib :
         ADLIBFX_Shutdown();
         break;

      case PC :
         PCFX_Shutdown();
         break;

#endif // LIBVER_ASSREV
      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   FX_Installed = FALSE;
#endif
   LL_UnlockMemory();

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetCallback

   Sets the function to call when a voice is done.
---------------------------------------------------------------------*/

int FX_SetCallBack
   (
   void ( *function )( unsigned long )
   )

   {
   int status;

   status = FX_Ok;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
      case SoundSource :
      case TandySoundSource :
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
      case UltraSound :
#endif
         MV_SetCallBack( function );
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
#if (LIBVER_ASSREV < 19950821L)
      case UltraSound :
         GUSWAVE_SetCallBack( function );
         break;

#endif
      case Adlib :
         ADLIBFX_SetCallBack( function );
         break;

      case PC :
         PCFX_SetCallBack( function );
         break;

#endif // LIBVER_ASSREV
      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetVolume

   Sets the volume of the current sound device.
---------------------------------------------------------------------*/

void FX_SetVolume
   (
   int volume
   )

   {
   int status;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
         if ( BLASTER_CardHasMixer() )
            {
            BLASTER_SetVoiceVolume( volume );
            }
         else
            {
            MV_SetVolume( volume );
            }
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         status = PAS_SetPCMVolume( volume );
         if ( status != PAS_Ok )
            {
            MV_SetVolume( volume );
            }
         break;

      case GenMidi :
      case SoundCanvas :
      case WaveBlaster :
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
      case SoundSource :
      case TandySoundSource :
#endif
         MV_SetVolume( volume );
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
      case Adlib :
         ADLIBFX_SetTotalVolume( volume );
         break;

#endif
      case UltraSound :
         GUSWAVE_SetVolume( volume );
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
      case PC :
         PCFX_SetTotalVolume( volume );
         break;
#else
      case SoundSource :
      case TandySoundSource :
         MV_SetVolume( volume );
         break;
#endif
      }
   }


/*---------------------------------------------------------------------
   Function: FX_GetVolume

   Returns the volume of the current sound device.
---------------------------------------------------------------------*/

int FX_GetVolume
   (
   void
   )

   {
   int volume;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
         if ( BLASTER_CardHasMixer() )
            {
            volume = BLASTER_GetVoiceVolume();
            }
         else
            {
            volume = MV_GetVolume();
            }
         break;

      case ProAudioSpectrum :
      case SoundMan16 :
         volume = PAS_GetPCMVolume();
         if ( volume == PAS_Error )
            {
            volume = MV_GetVolume();
            }
         break;

      case GenMidi :
      case SoundCanvas :
      case WaveBlaster :
         volume = 255;
         break;


      case UltraSound :
         volume = GUSWAVE_GetVolume();
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
      case Adlib :
         volume = ADLIBFX_GetTotalVolume();
         break;

      case PC :
         volume = PCFX_GetTotalVolume();
         break;

#endif
      case SoundSource :
      case TandySoundSource :
         volume = MV_GetVolume();
         break;

      default :
         volume = 0;
      }
   return( volume );
   }










// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
/*---------------------------------------------------------------------
   Function: FX_GetMaxReverbDelay

   Returns the maximum delay time for reverb.
---------------------------------------------------------------------*/

int FX_GetMaxReverbDelay
   (
   void
   )

   {
   return MV_GetMaxReverbDelay();
   }


/*---------------------------------------------------------------------
   Function: FX_GetReverbDelay

   Returns the current delay time for reverb.
---------------------------------------------------------------------*/

int FX_GetReverbDelay
   (
   void
   )

   {
   return MV_GetReverbDelay();
   }


/*---------------------------------------------------------------------
   Function: FX_SetReverbDelay

   Sets the delay level of reverb to add to mix.
---------------------------------------------------------------------*/

void FX_SetReverbDelay
   (
   int delay
   )

   {
   MV_SetReverbDelay( delay );
   }
#endif // LIBVER_ASSREV



// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19960514L)
/*---------------------------------------------------------------------
   Function: FX_EndLooping

   Stops the voice associated with the specified handle from looping
   without stoping the sound.
---------------------------------------------------------------------*/

int FX_EndLooping
   (
   int handle
   )

   {
   int status;

   status = MV_EndLooping( handle );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }

   return( status );
   }
#endif

/*---------------------------------------------------------------------
   Function: FX_SetPan

   Sets the stereo and mono volume level of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPan
   (
   int handle,
   int vol,
   int left,
   int right
   )

   {
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   int status = FX_Ok;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         status = MV_SetPan( handle, vol, left, right );
         if ( status == MV_Error )
            {
            FX_SetErrorCode( FX_MultiVocError );
            status = FX_Warning;
            }
         break;

      case Adlib :
         status = ADLIBFX_SetVolume( handle, vol );
         if ( status == ADLIBFX_Error )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Warning;
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case PC :
         break;

      default:
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }
#else // LIBVER_ASSREV
   int status;

   status = MV_SetPan( handle, vol, left, right );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }
#endif // LIBVER_ASSREV

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SetPitch

   Sets the pitch of the voice associated with the specified handle.
---------------------------------------------------------------------*/

int FX_SetPitch
   (
   int handle,
   int pitchoffset
   )

   {
// *** VERSIONS RESTORATION ***
// FIXME - Verify error codes come from the right enums, in ALL files
#if (LIBVER_ASSREV < 19960116L)
   int status = FX_Ok;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         status = MV_SetPitch( handle, pitchoffset );
         if ( status == MV_Error )
            {
            FX_SetErrorCode( FX_MultiVocError );
            status = FX_Warning;
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         status = GUSWAVE_SetPitch( handle, pitchoffset );
         // VERSIONS RESTORATION - A vanilla bug
         if ( handle == GUSWAVE_Error )
            {
            FX_SetErrorCode( FX_SoundCardError );
            }
         break;

#endif
      case PC :
         break;

      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }
#else // LIBVER_ASSREV
   int status;

   status = MV_SetPitch( handle, pitchoffset );
   if ( status == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }
#endif // LIBVER_ASSREV

   return( status );
   }




/*---------------------------------------------------------------------
   Function: FX_PlayVOC

   Begin playback of sound data with the given volume and priority.
---------------------------------------------------------------------*/

int FX_PlayVOC
   (
   char *ptr,
   int pitchoffset,
   int vol,
   int left,
   int right,
   int priority,
   unsigned long callbackval
   )

   {
   int handle;

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         handle = MV_PlayVOC( ptr, pitchoffset, vol, left, right,
            priority, callbackval );
         if ( handle == MV_Error )
            {
            FX_SetErrorCode( FX_MultiVocError );
            handle = FX_Warning;
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         handle = GUSWAVE_PlayVOC( ptr, pitchoffset, 0, vol,
            priority, callbackval );
         if ( handle == GUSWAVE_Error )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

#endif
      case Adlib :
         // FIXME (RESTORATION) - Might be a vanilla bug here (swapped args)
         handle = ADLIBFX_Play( ptr, priority, vol, callbackval );
         if ( handle < ADLIBFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

      case PC :
         handle = PCFX_Play( ptr, priority, callbackval );
         if ( handle < PCFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

      default :
         FX_SetErrorCode( FX_InvalidCard );
         handle = FX_Error;
      }
#else // LIBVER_ASSREV
   handle = MV_PlayVOC( ptr, pitchoffset, vol, left, right,
      priority, callbackval );
   if ( handle == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }
#endif // LIBVER_ASSREV

   return( handle );
   }








/*---------------------------------------------------------------------
   Function: FX_PlayVOC3D

   Begin playback of sound data at specified angle and distance
   from listener.
---------------------------------------------------------------------*/

int FX_PlayVOC3D
   (
   char *ptr,
   int pitchoffset,
   int angle,
   int distance,
   int priority,
   unsigned long callbackval
   )

   {
   int handle;

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         handle = MV_PlayVOC3D( ptr, pitchoffset, angle, distance,
            priority, callbackval );
         if ( handle == MV_Error )
            {
            FX_SetErrorCode( FX_MultiVocError );
            handle = FX_Warning;
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         handle = GUSWAVE_PlayVOC( ptr, pitchoffset, angle, 255-distance,
            priority, callbackval );
         if ( handle == GUSWAVE_Error )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

#endif
      case Adlib :
         handle = ADLIBFX_Play( ptr, 255-distance, priority, callbackval );
         if ( handle < ADLIBFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

      case PC :
         handle = PCFX_Play( ptr, priority, callbackval );
         if ( handle < PCFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            handle = FX_Warning;
            }
         break;

      default :
         FX_SetErrorCode( FX_InvalidCard );
         handle = FX_Error;
      }
#else // LIBVER_ASSREV
   handle = MV_PlayVOC3D( ptr, pitchoffset, angle, distance,
      priority, callbackval );
   if ( handle == MV_Error )
      {
      FX_SetErrorCode( FX_MultiVocError );
      handle = FX_Warning;
      }
#endif // LIBVER_ASSREV

   return( handle );
   }








/*---------------------------------------------------------------------
   Function: FX_Pan3D

   Set the angle and distance from the listener of the voice associated
   with the specified handle.
---------------------------------------------------------------------*/

int FX_Pan3D
   (
   int handle,
   int angle,
   int distance
   )

   {
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   int status = FX_Ok;

   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         status = MV_Pan3D( handle, angle, distance );
         if ( status != MV_Ok )
            {
            FX_SetErrorCode( FX_MultiVocError );
            status = FX_Warning;
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         status = GUSWAVE_SetPan3D( handle, angle, distance );
         break;

#endif
      case Adlib :
         status = ADLIBFX_SetVolume( handle, 255-distance );
         if ( status != ADLIBFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            status = FX_Warning;
            }
         break;

      case PC :
         break;

      default :
         FX_SetErrorCode( FX_InvalidCard );
         status = FX_Error;
      }
#else // LIBVER_ASSREV
   int status;

   status = MV_Pan3D( handle, angle, distance );
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      status = FX_Warning;
      }
#endif // LIBVER_ASSREV

   return( status );
   }


/*---------------------------------------------------------------------
   Function: FX_SoundActive

   Tests if the specified sound is currently playing.
---------------------------------------------------------------------*/

int FX_SoundActive
   (
   int handle
   )

   {
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         return( MV_VoicePlaying( handle ) );

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         return( GUSWAVE_VoicePlaying( handle ) );

#endif
      case Adlib :
         return( ADLIBFX_SoundPlaying( handle ) );

      case PC :
         return( PCFX_SoundPlaying( handle ) );
      }
   return( FALSE );
#else // LIBVER_ASSREV
   return( MV_VoicePlaying( handle ) );
#endif // LIBVER_ASSREV
   }




/*---------------------------------------------------------------------
   Function: FX_StopSound

   Halts playback of a specific voice
---------------------------------------------------------------------*/

int FX_StopSound
   (
   int handle
   )

   {
   int status;

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         status = MV_Kill( handle );
         if ( status != MV_Ok )
            {
            FX_SetErrorCode( FX_MultiVocError );
            return( FX_Warning );
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         status = GUSWAVE_Kill( handle );
         if ( status != GUSWAVE_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            return( FX_Warning );
            }
         break;

#endif
      case Adlib :
         status = ADLIBFX_Stop( handle );
         if ( status != ADLIBFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            return( FX_Warning );
            }
         break;

      case PC :
         status = PCFX_Stop( handle );
         if ( status != PCFX_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            return( FX_Warning );
            }
         break;

      default:
         FX_SetErrorCode( FX_InvalidCard );
       }
#else // LIBVER_ASSREV
   status = MV_Kill( handle );
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      return( FX_Warning );
      }
#endif // LIBVER_ASSREV

   return( FX_Ok );
   }


/*---------------------------------------------------------------------
   Function: FX_StopAllSounds

   Halts playback of all sounds.
---------------------------------------------------------------------*/

int FX_StopAllSounds
   (
   void
   )

   {
   int status;

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19960116L)
   switch( FX_SoundDevice )
      {
      case SoundBlaster :
      case Awe32 :
      case ProAudioSpectrum :
      case SoundMan16 :
#if (LIBVER_ASSREV >= 19950821L) // VERSIONS RESTORATION
      case UltraSound :
#endif
      case SoundSource :
      case TandySoundSource :
         status = MV_KillAllVoices();
         if ( status != MV_Ok )
            {
            FX_SetErrorCode( FX_MultiVocError );
            return( FX_Warning );
            }
         break;

#if (LIBVER_ASSREV < 19950821L) // VERSIONS RESTORATION
      case UltraSound :
         status = GUSWAVE_KillAllVoices();
         if ( status != GUSWAVE_Ok )
            {
            FX_SetErrorCode( FX_SoundCardError );
            return( FX_Warning );
            }
         break;

#endif
      case Adlib :
      case PC :
         break;

      default:
         FX_SetErrorCode( FX_InvalidCard );
      }
#else // LIBVER_ASSREV
   status = MV_KillAllVoices();
   if ( status != MV_Ok )
      {
      FX_SetErrorCode( FX_MultiVocError );
      return( FX_Warning );
      }
#endif // LIBVER_ASSREV

   return( FX_Ok );
   }






