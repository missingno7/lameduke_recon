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
   module: AL_MIDI.C

   author: James R. Dose
   date:   April 1, 1994

   Low level routines to support General MIDI music on Adlib compatible
   cards.

   (c) Copyright 1994 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <conio.h>
#include <dos.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include "dpmi.h"
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "interrupt.h"
#else
#include "interrup.h"
#endif
#include "sndcards.h"
#include "blaster.h"
#include "user.h"
#include "al_midi.h"
#include "_al_midi.h"
#include "ll_man.h"
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "memcheck.h"
#endif

#define TRUE  ( 1 == 1 )
#define FALSE ( !TRUE )


static unsigned NoteMod12[ MAX_NOTE + 1 ];
static unsigned NoteDiv12[ MAX_NOTE + 1 ];

// Slot numbers as a function of the voice and the operator.
// ( melodic only)

static int slotVoice[ NUM_VOICES ][ 2 ] =
   {
      { 0, 3 },    // voice 0
      { 1, 4 },    // 1
      { 2, 5 },    // 2
      { 6, 9 },    // 3
      { 7, 10 },   // 4
      { 8, 11 },   // 5
      { 12, 15 },  // 6
      { 13, 16 },  // 7
      { 14, 17 },  // 8
   };

// *** VERSIONS RESTORATION ***
// FIXME HACK
#if (LIBVER_ASSREV < 19950821L)
static int VoiceLevel[ NumChipSlots ][ 1 ];
static int VoiceKsl[ NumChipSlots ][ 1 ];
#else
static int VoiceLevel[ NumChipSlots ][ 2 ];
static int VoiceKsl[ NumChipSlots ][ 2 ];
#endif

// This table gives the offset of each slot within the chip.
// offset = fn( slot)

static char offsetSlot[ NumChipSlots ] =
   {
    0,  1,  2,  3,  4,  5,
    8,  9, 10, 11, 12, 13,
   16, 17, 18, 19, 20, 21
   };

static unsigned NotePitch[ FINETUNE_MAX + 1 ][ 12 ] =
   {
      { C, C_SHARP, D, D_SHARP, E, F, F_SHARP, G, G_SHARP, A, A_SHARP, B },
   };

static unsigned OctavePitch[ MAX_OCTAVE + 1 ] =
   {
   OCTAVE_0, OCTAVE_1, OCTAVE_2, OCTAVE_3,
   OCTAVE_4, OCTAVE_5, OCTAVE_6, OCTAVE_7,
   };

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
static int VoiceReserved[ NUM_VOICES ] =
   {
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
   };

static VOICE     Voice[ NUM_VOICES ];
#else
static int VoiceReserved[ NUM_VOICES * 2 ] =
   {
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE
   };

static VOICE     Voice[ NUM_VOICES * 2 ];
#endif
static VOICELIST Voice_Pool;

static CHANNEL   Channel[ NUM_CHANNELS ];

// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
static int PitchBendRange;
#endif

static int AL_LeftPort   = 0x388;
static int AL_RightPort  = 0x388;
static int AL_SendStereo = FALSE;
static int AL_OPL3       = FALSE;
// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
#define AL_MaxMidiChannel 9
#else
static int AL_MaxMidiChannel = 16;
#endif


/**********************************************************************

   Memory locked functions:

**********************************************************************/


#define AL_LockStart AL_SendOutputToPort


/*---------------------------------------------------------------------
   Function: AL_SendOutputToPort

   Sends data to the Adlib using a specified port.
---------------------------------------------------------------------*/

void AL_SendOutputToPort
   (
   int  port,
   int  reg,
   int  data
   )

   {
   int delay;

   outp( port, reg );

   for( delay = 6; delay > 0 ; delay-- )
//   for( delay = 2; delay > 0 ; delay-- )
      {
      inp( port );
      }

   outp( port + 1, data );

//   for( delay = 35; delay > 0 ; delay-- )
   for( delay = 35; delay > 0 ; delay-- )
//   for( delay = 2; delay > 0 ; delay-- )
      {
      inp( port );
      }
   }


/*---------------------------------------------------------------------
   Function: AL_SendOutput

   Sends data to the Adlib.
---------------------------------------------------------------------*/

void AL_SendOutput
   (
#if (LIBVER_ASSREV >= 19950821L) // *** VERSIONS RESTORATION ***
   int  voice,
#endif
   int  reg,
   int  data
   )

   {
   int port;

   if ( AL_SendStereo )
      {
      AL_SendOutputToPort( AL_LeftPort, reg, data );
      AL_SendOutputToPort( AL_RightPort, reg, data );
      }
   else
      {
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
      AL_SendOutputToPort( ADLIB_PORT, reg, data );
#else
      port = ( voice == 0 ) ? AL_RightPort : AL_LeftPort;
      AL_SendOutputToPort( port, reg, data );
#endif
      }
   }
#if (LIBVER_ASSREV < 19950821L) // *** VERSIONS RESTORATION *** - HACK
#define AL_SendOutput(voice, reg, data) AL_SendOutput(reg, data)
#endif


/*---------------------------------------------------------------------
   Function: AL_SetVoiceTimbre

   Programs the specified voice's timbre.
---------------------------------------------------------------------*/

static void AL_SetVoiceTimbre
   (
   int voice
   )

   {
   int    off;
   int    slot;
// *** VERSIONS RESTORATION ***
// FIXME HACK
#if (LIBVER_ASSREV < 19950821L)
#define port 0
#define voc voice
#else
   int    port;
   int    voc;
#endif
   int    patch;
   int    channel;
   TIMBRE *timbre;

   channel = Voice[ voice ].channel;

   if ( channel == 9 )
      {
      #if (LIBVER_ASSREV < 19950821L)
      patch = PercussionTable[ Voice[ voice ].key ].Timbre;
      if ( Voice[ voice ].timbre == patch ) return;
#else
      patch = Voice[ voice ].key + 128;
#endif
      }
   else
      {
      patch = Channel[ channel ].Timbre;
#if (LIBVER_ASSREV < 19950821L)
      if ( Voice[ voice ].timbre == patch ) return;
#endif
      }

#if (LIBVER_ASSREV >= 19950821L)
   if ( Voice[ voice ].timbre == patch )
      {
      return;
      }

#endif

   Voice[ voice ].timbre = patch;
   timbre = &ADLIB_TimbreBank[ patch ];

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   port = Voice[ voice ].port;
   voc  = ( voice >= NUM_VOICES ) ? voice - NUM_VOICES : voice;
#endif
   slot = slotVoice[ voc ][ 0 ];
   off  = offsetSlot[ slot ];

   VoiceLevel[ slot ][ port ] = 63 - ( timbre->Level[ 0 ] & 0x3f );
   VoiceKsl[ slot ][ port ]   = timbre->Level[ 0 ] & 0xc0;

   AL_SendOutput( port, 0xA0 + voc, 0 );
   AL_SendOutput( port, 0xB0 + voc, 0 );

   // Let voice clear the release
   AL_SendOutput( port, 0x80 + off, 0xff );

   AL_SendOutput( port, 0x60 + off, timbre->Env1[ 0 ] );
   AL_SendOutput( port, 0x80 + off, timbre->Env2[ 0 ] );
   AL_SendOutput( port, 0x20 + off, timbre->SAVEK[ 0 ] );
   AL_SendOutput( port, 0xE0 + off, timbre->Wave[ 0 ] );

   AL_SendOutput( port, 0x40 + off, timbre->Level[ 0 ] );
   slot = slotVoice[ voc ][ 1 ];

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   if ( AL_SendStereo )
#else
   if ( AL_SendStereo )
#endif
      {
      AL_SendOutputToPort( AL_LeftPort, 0xC0 + voice,
         ( timbre->Feedback & 0x0f ) | 0x20 );
      AL_SendOutputToPort( AL_RightPort, 0xC0 + voice,
         ( timbre->Feedback & 0x0f ) | 0x10 );
      }
   else
      {
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
      if ( AL_OPL3 )
         {
         AL_SendOutput( port, 0xC0 + voc, ( timbre->Feedback & 0x0f ) |
            0x30 );
         }
      else
#endif
         {
         AL_SendOutputToPort( ADLIB_PORT, 0xC0 + voice, timbre->Feedback );
         }
      }

   off = offsetSlot[ slot ];

   VoiceLevel[ slot ][ port ] = 63 - ( timbre->Level[ 1 ] & 0x3f );
   VoiceKsl[ slot ][ port ]   = timbre->Level[ 1 ] & 0xc0;
   AL_SendOutput( port, 0x40 + off, 63 );

   // Let voice clear the release
   AL_SendOutput( port, 0x80 + off, 0xff );

   AL_SendOutput( port, 0x60 + off, timbre->Env1[ 1 ] );
   AL_SendOutput( port, 0x80 + off, timbre->Env2[ 1 ] );
   AL_SendOutput( port, 0x20 + off, timbre->SAVEK[ 1 ] );
   AL_SendOutput( port, 0xE0 + off, timbre->Wave[ 1 ] );
// *** VERSIONS RESTORATION ***
// FIXME REVERT HACK
#if (LIBVER_ASSREV < 19950821L)
#undef port
#undef voc
#endif
   }


/*---------------------------------------------------------------------
   Function: AL_SetVoiceVolume

   Sets the volume of the specified voice.
---------------------------------------------------------------------*/

static void AL_SetVoiceVolume
   (
   int voice
   )

   {
#if (LIBVER_ASSREV < 19950821L)
   int channel;
   int velocity;
   int slot;
   int pan;
   unsigned long level;
   unsigned long volume;

   channel = Voice[ voice ].channel;
   velocity = Voice[ voice ].velocity + Channel[ channel ].Velocity;
   if ( velocity <= MAX_VELOCITY )
      {
      velocity = MAX_VELOCITY;
      }

   slot = slotVoice[ voice ][ 1 ];
   level = ( unsigned )VoiceLevel[ slot ][ 0 ];
   level *= velocity + 0x80;
   level = ( Channel[ channel ].Volume * level ) >> 15;

   if ( !AL_SendStereo )
      {
      volume = level ^ 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ 0 ];
      AL_SendOutputToPort( ADLIB_PORT, 0x40 + offsetSlot[ slot ], volume );
      }
   else
      {
      volume = level;
      if ( Channel[ channel ].Pan < 64 )
         {
         volume *= Channel[ channel ].Pan;
         volume >>= 6;
         }
      volume ^= 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ 0 ];
      AL_SendOutputToPort( AL_LeftPort, 0x40 + offsetSlot[ slot ], volume );

      volume = level;
      pan = Channel[ channel ].Pan;
      if ( pan > 64 )
         {
         volume = 127 - pan;
         volume *= level;
         volume >>= 6;
         }
      volume ^= 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ 0 ];
      AL_SendOutputToPort( AL_RightPort, 0x40 + offsetSlot[ slot ], volume );
      }
#else
   int channel;
   int velocity;
   int slot;
// *** VERSIONS RESTORATION ***
// FIXME HACK
#if (LIBVER_ASSREV < 19950821L)
#define port 0
#define voc voice
#else
   int port;
   int voc;
#endif
   unsigned long t1;
   unsigned long t2;
   unsigned long volume;
   TIMBRE *timbre;

   channel = Voice[ voice ].channel;

   timbre = &ADLIB_TimbreBank[ Voice[ voice ].timbre ];

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   velocity = Voice[ voice ].velocity + Channel[ channel ].Velocity;
#else
   velocity = Voice[ voice ].velocity + timbre->Velocity;
#endif
   velocity = min( velocity, MAX_VELOCITY );

#if (LIBVER_ASSREV >= 19950821L)
   voc  = ( voice >= NUM_VOICES ) ? voice - NUM_VOICES : voice;
#endif
   slot = slotVoice[ voc ][ 1 ];
#if (LIBVER_ASSREV >= 19950821L)
   port = Voice[ voice ].port;
#endif

   // amplitude
   t1  = ( unsigned )VoiceLevel[ slot ][ port ];
   t1 *= ( velocity + 0x80 );
   t1  = ( Channel[ channel ].Volume * t1 ) >> 15;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   if ( !AL_SendStereo )
#else
   if ( !AL_SendStereo )
#endif
      {
      volume  = t1 ^ 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ port ];

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
      AL_SendOutputToPort( ADLIB_PORT, 0x40 + offsetSlot[ slot ], volume );
#else
      AL_SendOutput( port, 0x40 + offsetSlot[ slot ], volume );
#endif

      // Check if this timbre is Additive
      if ( timbre->Feedback & 0x01 )
         {
         slot = slotVoice[ voc ][ 0 ];

         // amplitude
         t2  = ( unsigned )VoiceLevel[ slot ][ port ];
         t2 *= ( velocity + 0x80 );
         t2  = ( Channel[ channel ].Volume * t1 ) >> 15;

         volume  = t2 ^ 63;
         volume |= ( unsigned )VoiceKsl[ slot ][ port ];

         // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
         AL_SendOutputToPort( ADLIB_PORT, 0x40 + offsetSlot[ slot ], volume );
#else
         AL_SendOutput( port, 0x40 + offsetSlot[ slot ], volume );
#endif
         }
      }
   else
      {
      // Set left channel volume
      volume = t1;
      if ( Channel[ channel ].Pan < 64 )
         {
         volume *= Channel[ channel ].Pan;
         volume >>= 6;
         }

      volume ^= 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ port ];

      AL_SendOutputToPort( AL_LeftPort, 0x40 + offsetSlot[ slot ], volume );

      // Set right channel volume
      volume = t1;
      if ( Channel[ channel ].Pan > 64 )
         {
         volume *= 127 - Channel[ channel ].Pan;
         volume >>= 6;
         }

      volume ^= 63;
      volume |= ( unsigned )VoiceKsl[ slot ][ port ];

      AL_SendOutputToPort( AL_RightPort, 0x40 + offsetSlot[ slot ], volume );

      // Check if this timbre is Additive
      if ( timbre->Feedback & 0x01 )
         {
         // amplitude
         t2  = ( unsigned )VoiceLevel[ slot ][ port ];
         t2 *= ( velocity + 0x80 );
         t2  = ( Channel[ channel ].Volume * t1 ) >> 15;

         slot = slotVoice[ voc ][ 0 ];

         // Set left channel volume
         volume = t2;
         if ( Channel[ channel ].Pan < 64 )
            {
            volume *= Channel[ channel ].Pan;
            volume >>= 6;
            }

         volume ^= 63;
         volume |= ( unsigned )VoiceKsl[ slot ][ port ];

         AL_SendOutputToPort( AL_LeftPort, 0x40 + offsetSlot[ slot ], volume );

         // Set right channel volume
         volume = t2;
         if ( Channel[ channel ].Pan > 64 )
            {
            volume *= 127 - Channel[ channel ].Pan;
            volume >>= 6;
            }

         volume ^= 63;
         volume |= ( unsigned )VoiceKsl[ slot ][ port ];

         AL_SendOutputToPort( AL_RightPort, 0x40 + offsetSlot[ slot ], volume );
         }
      }
// *** VERSIONS RESTORATION ***
// FIXME REVERT HACK
#if (LIBVER_ASSREV < 19950821L)
#undef port
#undef voc
#endif
#endif
   }


/*---------------------------------------------------------------------
   Function: AL_AllocVoice

   Retrieves a free voice from the voice pool.
---------------------------------------------------------------------*/

static int AL_AllocVoice
   (
   void
   )

   {
   int voice;

   if ( Voice_Pool.start )
      {
      voice = Voice_Pool.start->num;
      LL_Remove( VOICE, &Voice_Pool, &Voice[ voice ] );
      return( voice );
      }

   return( AL_VoiceNotFound );
   }


/*---------------------------------------------------------------------
   Function: AL_GetVoice

   Determines which voice is associated with a specified note and
   MIDI channel.
---------------------------------------------------------------------*/

static int AL_GetVoice
   (
   int channel,
   int key
   )

   {
   VOICE *voice;

   voice = Channel[ channel ].Voices.start;

   while( voice != NULL )
      {
      if ( voice->key == key )
         {
         return( voice->num );
         }
      voice = voice->next;
      }

   return( AL_VoiceNotFound );
   }


/*---------------------------------------------------------------------
   Function: AL_SetVoicePitch

   Programs the pitch of the specified voice.
---------------------------------------------------------------------*/

static void AL_SetVoicePitch
   (
   int voice
   )

   {
   int note;
   int channel;
   int patch;
   int detune;
   int ScaleNote;
   int Octave;
   int pitch;
// *** VERSIONS RESTORATION ***
// FIXME HACK
#if (LIBVER_ASSREV < 19950821L)
#define port 0
#define voc voice
#else
   int port;
   int voc;
#endif

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   port = Voice[ voice ].port;
   voc  = ( voice >= NUM_VOICES ) ? voice - NUM_VOICES : voice;
#endif
   channel = Voice[ voice ].channel;

   if ( channel == 9 )
      {
#if (LIBVER_ASSREV < 19950821L)
      note = PercussionTable[ Voice[ voice ].key ].Key;
#else
      patch = Voice[ voice ].key + 128;
      note  = ADLIB_TimbreBank[ patch ].Transpose;
#endif
      }
   else
      {
#if (LIBVER_ASSREV < 19950821L)
      note = Voice[ voice ].key;
#else
      patch = Channel[ channel ].Timbre;
      note  = Voice[ voice ].key + ADLIB_TimbreBank[ patch ].Transpose;
#endif
      }

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   note += Channel[ channel ].KeyOffset + Channel[ channel ].RPN - 12;
#else
   note += Channel[ channel ].KeyOffset - 12;
#endif
   if ( note > MAX_NOTE )
      {
      note = MAX_NOTE;
      }
   if ( note < 0 )
      {
      note = 0;
      }

   detune = Channel[ channel ].KeyDetune;

   ScaleNote = NoteMod12[ note ];
   Octave    = NoteDiv12[ note ];

   pitch = OctavePitch[ Octave ] | NotePitch[ detune ][ ScaleNote ];

   Voice[ voice ].pitchleft = pitch;

   pitch |= Voice[ voice ].status;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   if ( !AL_SendStereo )
      {
      AL_SendOutputToPort( ADLIB_PORT, 0xA0 + voc, pitch );
      AL_SendOutputToPort( ADLIB_PORT, 0xB0 + voc, pitch >> 8 );
      }
#else
   if ( !AL_SendStereo )
      {
      AL_SendOutput( port, 0xA0 + voc, pitch );
      AL_SendOutput( port, 0xB0 + voc, pitch >> 8 );
      }
#endif
   else
      {
      AL_SendOutputToPort( AL_LeftPort, 0xA0 + voice, pitch );
      AL_SendOutputToPort( AL_LeftPort, 0xB0 + voice, pitch >> 8 );

      if ( channel != 9 )
         {
         detune += STEREO_DETUNE;
         }

      if ( detune > FINETUNE_MAX )
         {
         detune -= FINETUNE_RANGE;
         if ( note < MAX_NOTE )
            {
            note++;
            ScaleNote = NoteMod12[ note ];
            Octave    = NoteDiv12[ note ];
            }
         }

      pitch = OctavePitch[ Octave ] | NotePitch[ detune ][ ScaleNote ];

      Voice[ voice ].pitchright = pitch;

      pitch |= Voice[ voice ].status;

      AL_SendOutputToPort( AL_RightPort, 0xA0 + voice, pitch );
      AL_SendOutputToPort( AL_RightPort, 0xB0 + voice, pitch >> 8 );
      }
// *** VERSIONS RESTORATION ***
// FIXME REVERT HACK
#if (LIBVER_ASSREV < 19950821L)
#undef port
#undef voc
#endif
   }


/*---------------------------------------------------------------------
   Function: AL_SetChannelVolume

   Sets the volume of the specified MIDI channel.
---------------------------------------------------------------------*/

static void AL_SetChannelVolume
   (
   int channel,
   int volume
   )

   {
   VOICE *voice;

   volume = max( 0, volume );
   volume = min( volume, AL_MaxVolume );
   Channel[ channel ].Volume = volume;

   voice = Channel[ channel ].Voices.start;
   while( voice != NULL )
      {
      AL_SetVoiceVolume( voice->num );
      voice = voice->next;
      }
   }


/*---------------------------------------------------------------------
   Function: AL_SetChannelPan

   Sets the pan position of the specified MIDI channel.
---------------------------------------------------------------------*/

static void AL_SetChannelPan
   (
   int channel,
   int pan
   )

   {
   // Don't pan drum sounds
   if ( channel != 9 )
      {
      Channel[ channel ].Pan = pan;
      }
   }


/*---------------------------------------------------------------------
   Function: AL_SetChannelDetune

   Sets the stereo voice detune of the specified MIDI channel.
---------------------------------------------------------------------*/

static void AL_SetChannelDetune
   (
   int channel,
   int detune
   )

   {
   Channel[ channel ].Detune = detune;
   }


/*---------------------------------------------------------------------
   Function: AL_ResetVoices

   Sets all voice info to the default state.
---------------------------------------------------------------------*/

static void AL_ResetVoices
   (
   void
   )

   {
   int index;
   int numvoices;

   Voice_Pool.start = NULL;
   Voice_Pool.end = NULL;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   PitchBendRange = AL_DefaultPitchBendRange;
   for( index = 0; index < NUM_VOICES; index++ )
#else
   numvoices = NUM_VOICES;
   if ( ( AL_OPL3 ) && ( !AL_SendStereo ) )
      {
      numvoices = NUM_VOICES * 2;
      }
   for( index = 0; index < numvoices; index++ )
#endif
      {
      if ( VoiceReserved[ index ] == FALSE )
         {
         Voice[ index ].num = index;
         Voice[ index ].key = 0;
         Voice[ index ].velocity = 0;
         Voice[ index ].channel = -1;
         Voice[ index ].timbre = -1;
         // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
         Voice[ index ].port = ( index < NUM_VOICES ) ? 0 : 1;
#endif
         Voice[ index ].status = NOTE_OFF;
         LL_AddToTail( VOICE, &Voice_Pool, &Voice[ index ] );
         }
      }

   for( index = 0; index < NUM_CHANNELS; index++ )
      {
      Channel[ index ].Voices.start    = NULL;
      Channel[ index ].Voices.end      = NULL;
      Channel[ index ].Timbre          = 0;
      Channel[ index ].Pitchbend       = 0;
      Channel[ index ].KeyOffset       = 0;
      Channel[ index ].KeyDetune       = 0;
      Channel[ index ].Volume          = AL_DefaultChannelVolume;
      Channel[ index ].Pan             = 64;
      Channel[ index ].RPN             = 0;
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
      Channel[ index ].Velocity        = 0;
#else
      Channel[ index ].PitchBendRange     = AL_DefaultPitchBendRange;
      Channel[ index ].PitchBendSemiTones = AL_DefaultPitchBendRange / 100;
      Channel[ index ].PitchBendHundreds  = AL_DefaultPitchBendRange % 100;
#endif
      }
   }


/*---------------------------------------------------------------------
   Function: AL_CalcPitchInfo

   Calculates the pitch table.
---------------------------------------------------------------------*/

static void AL_CalcPitchInfo
   (
   void
   )

   {
   int    note;
   int    finetune;
   double detune;

   for( note = 0; note <= MAX_NOTE; note++ )
      {
      NoteMod12[ note ] = note % 12;
      NoteDiv12[ note ] = note / 12;
      }

   for( finetune = 1; finetune <= FINETUNE_MAX; finetune++ )
      {
      detune = pow( 2, ( double )finetune / ( 12.0 * FINETUNE_RANGE ) );
      for( note = 0; note < 12; note++ )
         {
         NotePitch[ finetune ][ note ] = ( ( double )NotePitch[ 0 ][ note ] * detune );
         }
      }
   }


/*---------------------------------------------------------------------
   Function: AL_FlushCard

   Sets all voices to a known (quiet) state.
---------------------------------------------------------------------*/

void AL_FlushCard
   (
   int port
   )

   {
   int i;
   unsigned slot1;
   unsigned slot2;

   for( i = 0 ; i < NUM_VOICES; i++ )
      {
      if ( VoiceReserved[ i ] == FALSE )
         {
         // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
         AL_SendOutputToPort( port, 0xA0 + i, 0 );
         AL_SendOutputToPort( port, 0xB0 + i, 0 );

         AL_SendOutputToPort( port, 0xE0 + offsetSlot[ slotVoice[ i ][ 0 ] ], 0 );
         AL_SendOutputToPort( port, 0xE0 + offsetSlot[ slotVoice[ i ][ 1 ] ], 0 );
#else
         slot1 = offsetSlot[ slotVoice[ i ][ 0 ] ];
         slot2 = offsetSlot[ slotVoice[ i ][ 1 ] ];

         AL_SendOutputToPort( port, 0xA0 + i, 0 );
         AL_SendOutputToPort( port, 0xB0 + i, 0 );

         AL_SendOutputToPort( port, 0xE0 + slot1, 0 );
         AL_SendOutputToPort( port, 0xE0 + slot2, 0 );

         // Set the envelope to be fast and quiet
         AL_SendOutputToPort( port, 0x60 + slot1, 0xff );
         AL_SendOutputToPort( port, 0x60 + slot2, 0xff );
         AL_SendOutputToPort( port, 0x80 + slot1, 0xff );
         AL_SendOutputToPort( port, 0x80 + slot2, 0xff );

         // Maximum attenuation
         AL_SendOutputToPort( port, 0x40 + slot1, 0xff );
         AL_SendOutputToPort( port, 0x40 + slot2, 0xff );
#endif
         }
      }
   }


/*---------------------------------------------------------------------
   Function: AL_Reset

   Sets the card to a known (quiet) state.
---------------------------------------------------------------------*/

void AL_Reset
   (
   void
   )

   {
   AL_SendOutputToPort( ADLIB_PORT, 1, 0x20 );
   AL_SendOutputToPort( ADLIB_PORT, 0x08, 0 );

   // Set the values: AM Depth, VIB depth & Rhythm
   AL_SendOutputToPort( ADLIB_PORT, 0xBD, 0 );

   if ( AL_OPL3 )
      {
      AL_SendOutputToPort( AL_RightPort, 0x5, 1 );
      }

   if ( AL_SendStereo )
      {
      AL_FlushCard( AL_LeftPort );
      AL_FlushCard( AL_RightPort );
      }
   else
      {
      AL_FlushCard( ADLIB_PORT );
      }
   }


/*---------------------------------------------------------------------
   Function: AL_ReserveVoice

   Marks a voice as being not available for use.  This allows the
   driver to use the rest of the card while another driver uses the
   reserved voice.
---------------------------------------------------------------------*/

int AL_ReserveVoice
   (
   int voice
   )

   {
   unsigned flags;

   if ( ( voice < 0 ) || ( voice >= NUM_VOICES ) )
      {
      return( AL_Error );
      }

   if ( VoiceReserved[ voice ] )
      {
      return( AL_Warning );
      }

   flags = DisableInterrupts();

   if ( Voice[ voice ].status == NOTE_ON )
      {
      AL_NoteOff( Voice[ voice ].channel, Voice[ voice ].key, 0 );
      }

   VoiceReserved[ voice ] = TRUE;
   LL_Remove( VOICE, &Voice_Pool, &Voice[ voice ] );

   RestoreInterrupts( flags );
   return( AL_Ok );
   }


/*---------------------------------------------------------------------
   Function: AL_ReleaseVoice

   Marks a previously reserved voice as being free to use.
---------------------------------------------------------------------*/

int AL_ReleaseVoice
   (
   int voice
   )

   {
   unsigned flags;

   if ( ( voice < 0 ) || ( voice >= NUM_VOICES ) )
      {
      return( AL_Error );
      }

   if ( !VoiceReserved[ voice ] )
      {
      return( AL_Warning );
      }

   flags = DisableInterrupts();

   VoiceReserved[ voice ] = FALSE;
   LL_AddToTail( VOICE, &Voice_Pool, &Voice[ voice ] );

   RestoreInterrupts( flags );
   return( AL_Ok );
   }


/*---------------------------------------------------------------------
   Function: AL_NoteOff

   Turns off a note on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_NoteOff
   (
   int channel,
   int key,
   int velocity
   )

   {
   int voice;
// *** VERSIONS RESTORATION ***
// FIXME HACK
#if (LIBVER_ASSREV < 19950821L)
#define port 0
#define voc voice
#else
   int port;
   int voc;
#endif

   // We only play channels 1 through 10
   if ( channel > AL_MaxMidiChannel )
      {
      return;
      }

   voice = AL_GetVoice( channel, key );

   if ( voice == AL_VoiceNotFound )
      {
      return;
      }

   Voice[ voice ].status = NOTE_OFF;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   if ( AL_SendStereo )
#else
   port = Voice[ voice ].port;
   voc  = ( voice >= NUM_VOICES ) ? voice - NUM_VOICES : voice;

   if ( AL_SendStereo )
#endif
      {
      AL_SendOutputToPort( AL_LeftPort, 0xB0 + voice,
         hibyte( Voice[ voice ].pitchleft ) );
      AL_SendOutputToPort( AL_RightPort, 0xB0 + voice,
         hibyte( Voice[ voice ].pitchright ) );
      }
   else
      {
      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
      AL_SendOutputToPort( ADLIB_PORT, 0xB0 + voc, hibyte( Voice[ voice ].pitchleft ) );
#else
      AL_SendOutput( port, 0xB0 + voc, hibyte( Voice[ voice ].pitchleft ) );
#endif
      }

   LL_Remove( VOICE, &Channel[ channel ].Voices, &Voice[ voice ] );
   LL_AddToTail( VOICE, &Voice_Pool, &Voice[ voice ] );
// *** VERSIONS RESTORATION ***
// FIXME REVERT HACK
#if (LIBVER_ASSREV < 19950821L)
#undef port
#undef voc
#endif
   }


/*---------------------------------------------------------------------
   Function: AL_NoteOn

   Plays a note on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_NoteOn
   (
   int channel,
   int key,
   int velocity
   )

   {
   int voice;

   // We only play channels 1 through 10
   if ( channel > AL_MaxMidiChannel )
      {
      return;
      }

   if ( velocity == 0 )
      {
      AL_NoteOff( channel, key, velocity );
      return;
      }

   voice = AL_AllocVoice();

   if ( voice == AL_VoiceNotFound )
      {
      if ( Channel[ 9 ].Voices.start )
         {
         AL_NoteOff( 9, Channel[ 9 ].Voices.start->key, 0 );
         voice = AL_AllocVoice();
         }
      if ( voice == AL_VoiceNotFound )
         {
         return;
         }
      }

   Voice[ voice ].key      = key;
   Voice[ voice ].channel  = channel;
   Voice[ voice ].velocity = velocity;
   Voice[ voice ].status   = NOTE_ON;

   LL_AddToTail( VOICE, &Channel[ channel ].Voices, &Voice[ voice ] );

   AL_SetVoiceTimbre( voice );
   AL_SetVoiceVolume( voice );
   AL_SetVoicePitch( voice );
   }


/*---------------------------------------------------------------------
   Function: AL_AllNotesOff

   Turns off all currently playing voices.
---------------------------------------------------------------------*/

void AL_AllNotesOff
   (
   int channel
   )

   {
   while( Channel[ channel ].Voices.start != NULL )
      {
      AL_NoteOff( channel, Channel[ channel ].Voices.start->key, 0 );
      }
   }


/*---------------------------------------------------------------------
   Function: AL_ControlChange

   Sets the value of a controller on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_ControlChange
   (
   int channel,
   int type,
   int data
   )

   {
   // We only play channels 1 through 10
   if ( channel > AL_MaxMidiChannel )
      {
      return;
      }

   switch( type )
      {
      case MIDI_VOLUME :
         AL_SetChannelVolume( channel, data );
         break;

      case MIDI_PAN :
         AL_SetChannelPan( channel, data );
         break;

      case MIDI_DETUNE :
         AL_SetChannelDetune( channel, data );
         break;

      case MIDI_ALL_NOTES_OFF :
         AL_AllNotesOff( channel );
         break;

      case MIDI_RESET_ALL_CONTROLLERS :
         AL_ResetVoices();
         AL_SetChannelVolume( channel, AL_DefaultChannelVolume );
         AL_SetChannelPan( channel, 64 );
         AL_SetChannelDetune( channel, 0 );
         break;

      // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
      case MIDI_RPN_MSB :
         Channel[ channel ].RPN &= 0x00FF;
         Channel[ channel ].RPN |= ( data & 0xFF ) << 8;
         break;

      case MIDI_RPN_LSB :
         Channel[ channel ].RPN &= 0xFF00;
         Channel[ channel ].RPN |= data & 0xFF;
         break;

      case MIDI_DATAENTRY_MSB :
         if ( Channel[ channel ].RPN == MIDI_PITCHBEND_RPN )
            {
            Channel[ channel ].PitchBendSemiTones = data;
            Channel[ channel ].PitchBendRange     =
               Channel[ channel ].PitchBendSemiTones * 100 +
               Channel[ channel ].PitchBendHundreds;
            }
         break;

      case MIDI_DATAENTRY_LSB :
         if ( Channel[ channel ].RPN == MIDI_PITCHBEND_RPN )
            {
            Channel[ channel ].PitchBendHundreds = data;
            Channel[ channel ].PitchBendRange    =
               Channel[ channel ].PitchBendSemiTones * 100 +
               Channel[ channel ].PitchBendHundreds;
            }
         break;
#endif
      }
   }


/*---------------------------------------------------------------------
   Function: AL_ProgramChange

   Selects the instrument to use on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_ProgramChange
   (
   int channel,
   int patch
   )

   {
   // We only play channels 1 through 10
   if ( channel > AL_MaxMidiChannel )
      {
      return;
      }

   Channel[ channel ].Timbre  = patch;
   }


/*---------------------------------------------------------------------
   Function: AL_SetPitchBend

   Sets the pitch bend amount on the specified MIDI channel.
---------------------------------------------------------------------*/

void AL_SetPitchBend
   (
   int channel,
   int lsb,
   int msb
   )

   {
   int            pitchbend;
   unsigned long  TotalBend;
   VOICE         *voice;

   // We only play channels 1 through 10
   if ( channel > AL_MaxMidiChannel )
      {
      return;
      }

   pitchbend = lsb + ( msb << 8 );
   Channel[ channel ].Pitchbend = pitchbend;

   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   TotalBend  = pitchbend * PitchBendRange;
   TotalBend /= ( PITCHBEND_CENTER / FINETUNE_RANGE );

   Channel[ channel ].KeyOffset  = ( int )( TotalBend / FINETUNE_RANGE );
   Channel[ channel ].KeyOffset -= PitchBendRange;
#else
   TotalBend  = pitchbend * Channel[ channel ].PitchBendRange;
   TotalBend /= ( PITCHBEND_CENTER / FINETUNE_RANGE );

   Channel[ channel ].KeyOffset  = ( int )( TotalBend / FINETUNE_RANGE );
   Channel[ channel ].KeyOffset -= Channel[ channel ].PitchBendSemiTones;
#endif

   Channel[ channel ].KeyDetune = ( unsigned )( TotalBend % FINETUNE_RANGE );

   voice = Channel[ channel ].Voices.start;
   while( voice != NULL )
      {
      AL_SetVoicePitch( voice->num );
      voice = voice->next;
      }
   }


/*---------------------------------------------------------------------
   Function: AL_DetectFM

   Determines if an Adlib compatible card is installed in the machine.
---------------------------------------------------------------------*/

int AL_DetectFM
   (
   void
   )

   {
   int status1;
   int status2;
   int i;

   if ( USER_CheckParameter( NO_ADLIB_DETECTION ) )
      {
      return( FALSE );
      }

   AL_SendOutputToPort( ADLIB_PORT, 4, 0x60 );   // Reset T1 & T2
   AL_SendOutputToPort( ADLIB_PORT, 4, 0x80 );   // Reset IRQ

   status1 = inp( ADLIB_PORT );

   AL_SendOutputToPort( ADLIB_PORT, 2, 0xff );   // Set timer 1
   AL_SendOutputToPort( ADLIB_PORT, 4, 0x21 );   // Start timer 1

   for( i = 100; i > 0; i-- )
      {
      inp( ADLIB_PORT );
      }

   status2 = inp( ADLIB_PORT );

   AL_SendOutputToPort( ADLIB_PORT, 4, 0x60 );
   AL_SendOutputToPort( ADLIB_PORT, 4, 0x80 );

   return( ( ( status1 & 0xe0 ) == 0x00 ) && ( ( status2 & 0xe0 ) == 0xc0 ) );
   }


/*---------------------------------------------------------------------
   Function: AL_LockEnd

   Used for determining the length of the functions to lock in memory.
---------------------------------------------------------------------*/

static void AL_LockEnd
   (
   void
   )

   {
   }


/*---------------------------------------------------------------------
   Function: AL_Shutdown

   Ends use of the sound card and resets it to a quiet state.
---------------------------------------------------------------------*/

void AL_Shutdown
   (
   void
   )

   {
   if ( AL_OPL3 )
      {
      AL_SendOutputToPort( AL_RightPort, 0x5, 0 );
      }

   AL_OPL3 = FALSE;
   AL_ResetVoices();
   AL_Reset();

   DPMI_UnlockMemoryRegion( AL_LockStart, AL_LockEnd );
   DPMI_Unlock( slotVoice );
   DPMI_Unlock( VoiceLevel );
   DPMI_Unlock( VoiceKsl );
   DPMI_Unlock( offsetSlot );
   DPMI_Unlock( NotePitch );
   DPMI_Unlock( OctavePitch );
   DPMI_Unlock( NoteMod12 );
   DPMI_Unlock( NoteDiv12 );
   DPMI_Unlock( VoiceReserved );
   DPMI_Unlock( Voice );
   DPMI_Unlock( Voice_Pool );
   DPMI_Unlock( Channel );
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   DPMI_Unlock( PitchBendRange );
#endif
   DPMI_Unlock( AL_LeftPort );
   DPMI_Unlock( AL_RightPort );
   DPMI_Unlock( AL_SendStereo );
   DPMI_Unlock( AL_OPL3 );
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   DPMI_Unlock( AL_MaxMidiChannel );
#endif
   }


// *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
/*---------------------------------------------------------------------
   Function: AL_SetMaxMidiChannel

   Sets the maximum MIDI channel that FM cards respond to.
---------------------------------------------------------------------*/

void AL_SetMaxMidiChannel
   (
   int channel
   )

   {
   AL_MaxMidiChannel = channel - 1;
   }
#endif

/*---------------------------------------------------------------------
   Function: AL_Init

   Begins use of the sound card.
---------------------------------------------------------------------*/

int AL_Init
   (
   int soundcard
   )

   {
   BLASTER_CONFIG Blaster;
   int status;

   status  = DPMI_LockMemoryRegion( AL_LockStart, AL_LockEnd );
   status |= DPMI_Lock( slotVoice );
   status |= DPMI_Lock( VoiceLevel );
   status |= DPMI_Lock( VoiceKsl );
   status |= DPMI_Lock( offsetSlot );
   status |= DPMI_Lock( NotePitch );
   status |= DPMI_Lock( OctavePitch );
   status |= DPMI_Lock( NoteMod12 );
   status |= DPMI_Lock( NoteDiv12 );
   status |= DPMI_Lock( VoiceReserved );
   status |= DPMI_Lock( Voice );
   status |= DPMI_Lock( Voice_Pool );
   status |= DPMI_Lock( Channel );
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   status |= DPMI_Lock( PitchBendRange );
#endif
   status |= DPMI_Lock( AL_LeftPort );
   status |= DPMI_Lock( AL_RightPort );
   status |= DPMI_Lock( AL_SendStereo );
   status |= DPMI_Lock( AL_OPL3 );
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV >= 19950821L)
   status |= DPMI_Lock( AL_MaxMidiChannel );
#endif

   if ( status != DPMI_Ok )
      {
      return( AL_Error );
      }

   AL_SendStereo = FALSE;
   AL_OPL3   = FALSE;
   AL_LeftPort = 0x388;
   AL_RightPort = 0x388;

   switch( soundcard )
      {
      case ProAudioSpectrum :
      case SoundMan16 :
         AL_OPL3 = TRUE;
         AL_SendStereo = TRUE;
         AL_RightPort = 0x38a;
         break;

      case SoundBlaster :
         if ( BLASTER_GetEnv( &Blaster ) != BLASTER_Ok )
            {
            break;
            }

         switch( Blaster.Type )
            {
            case SBPro2 :
            case SB16 :
               AL_OPL3 = TRUE;
               AL_LeftPort = Blaster.Address;
               AL_SendStereo = TRUE;
               AL_RightPort = Blaster.Address + 2;
               break;

            case SBPro :
               AL_LeftPort = Blaster.Address;
               AL_SendStereo = TRUE;
               AL_RightPort = Blaster.Address + 2;
               break;
            }
         break;
      }

   AL_CalcPitchInfo();
   // *** VERSIONS RESTORATION ***
#if (LIBVER_ASSREV < 19950821L)
   AL_ResetVoices();
   AL_Reset();
#else
   AL_Reset();
   AL_ResetVoices();
#endif

   return( AL_Ok );
   }


