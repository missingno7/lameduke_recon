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
   module: PITCH.C

   author: James R. Dose
   date:   June 14, 1993

   Routines for pitch scaling.

   (c) Copyright 1993 James R. Dose.  All Rights Reserved.
**********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "dpmi.h"
#include "standard.h"
#include "pitch.h"
#if (LIBVER_ASSREV < 20011231L) // *** VERSIONS RESTORATION ***
#include "memcheck.h"
#endif

#define MAXDETUNE 25

static unsigned long PitchTable[ 12 ][ MAXDETUNE ];
static int PITCH_Installed = FALSE;


/*---------------------------------------------------------------------
   Function: PITCH_Init

   Initializes pitch table.
---------------------------------------------------------------------*/
void PITCH_Init
   (
   void
   )

   {
   int note;
   int detune;

   if ( !PITCH_Installed )
      {
      for( note = 0; note < 12; note++ )
         {
         for( detune = 0; detune < MAXDETUNE; detune++ )
            {
            PitchTable[ note ][ detune ] = 0x10000 *
               pow( 2, ( note * MAXDETUNE + detune ) / ( 12.0 * MAXDETUNE ) );
            }
         }

      PITCH_Installed = TRUE;
      }
   }

/**********************************************************************

   Memory locked functions:

**********************************************************************/


#define PITCH_LockStart PITCH_GetScale


/*---------------------------------------------------------------------
   Function: PITCH_GetScale

   Returns a fixed-point value to scale number the specified amount.
---------------------------------------------------------------------*/

unsigned long PITCH_GetScale
   (
   int pitchoffset
   )

   {
   unsigned long scale;
   int octaveshift;
   int noteshift;
   int note;
   int detune;

   if ( !PITCH_Installed )
      {
      PITCH_Init();
      }

   if ( pitchoffset == 0 )
      {
      return( PitchTable[ 0 ][ 0 ] );
      }

   noteshift = pitchoffset % 1200;
   if ( noteshift < 0 )
      {
      noteshift += 1200;
      }

   note   = noteshift / 100;
   detune = ( noteshift % 100 ) / ( 100 / MAXDETUNE );
   scale = PitchTable[ note ][ detune ];
   octaveshift = ( pitchoffset - noteshift ) / 1200;

   if ( octaveshift < 0 )
      {
      scale >>= -octaveshift;
      }
   else
      {
      scale <<= octaveshift;
      }

   return( scale );
   }


/*---------------------------------------------------------------------
   Function: PITCH_LockEnd

   Used for determining the length of the functions to lock in memory.
---------------------------------------------------------------------*/

static void PITCH_LockEnd
   (
   void
   )

   {
   }


/*---------------------------------------------------------------------
   Function: PITCH_UnlockMemory

   Unlocks all neccessary data.
---------------------------------------------------------------------*/

void PITCH_UnlockMemory
   (
   void
   )

   {
   DPMI_UnlockMemoryRegion( PITCH_LockStart, PITCH_LockEnd );
   DPMI_Unlock( PitchTable );
   DPMI_Unlock( PITCH_Installed );
   }


/*---------------------------------------------------------------------
   Function: PITCH_LockMemory

   Unlocks all neccessary data.
---------------------------------------------------------------------*/

int PITCH_LockMemory
   (
   void
   )

   {
   int status;

   status  = DPMI_LockMemoryRegion( PITCH_LockStart, PITCH_LockEnd );
   status |= DPMI_Lock( PitchTable );
   status |= DPMI_Lock( PITCH_Installed );

   if ( status != DPMI_Ok )
      {
      PITCH_UnlockMemory();
      return( PITCH_Error );
      }

   return( PITCH_Ok );
   }
