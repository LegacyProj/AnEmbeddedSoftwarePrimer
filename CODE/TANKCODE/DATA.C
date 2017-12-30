/****************************************************************

                          D A T A . C


This module stores the tank data.


Copyright 1999 by Addison Wesley Longman, Inc.

All rights reserved.  No part of this document or the 
computer-coded information from which it is derived may be 
reproduced, stored in a retrieval system, or transmitted, in 
any form or by any means -- electronic, mechanical, 
photocopying, recording or otherwise -- without the prior 
written permission of Addison Wesley Longman, Inc.


                     C H A N G E   R E C O R D

  Date        Who            Description
--------  ------------  -----------------------------------------
05/08/99  DES           Module released
****************************************************************/

/* System Include Files */
#include "os_cfg.h"
#include "ix86s.h"
#include "ucos.h"
#include "probstyl.h"

/* Program Include Files */
#include "publics.h"

/* Local Defines */
   #define HISTORY_DEPTH 8
   #define WAIT_FOREVER 0

/* Local Structures */
   typedef struct
   {
      int a_iLevel[HISTORY_DEPTH];     
                     /* Tank level */
      int aa_iTime[HISTORY_DEPTH][4];  
                     /* Time level was measured. */
      int iCurrent;                    
                     /* Index to most recent entry */
      BOOL fFull;                      
                     /* TRUE if all history entries have data */
   } TANK_DATA;

/* Static Data */

   /* Data about each of the tanks. */
   static TANK_DATA a_td[COUNTOF_TANKS];

   /* The semaphore that protects the data. */
   static OS_EVENT *SemData;


/*****   vTankDataInit   ****************************************

This routine initializes the tank data.

RETURNS: None.

*/

void vTankDataInit (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int iTank;        /* An iterator */

/*-------------------------------------------------------------*/

   /* Note that all the history tables are empty. */
   for (iTank = 0; iTank < COUNTOF_TANKS; ++iTank)
   {                                   
      a_td[iTank].iCurrent = -1;
      a_td[iTank].fFull = FALSE;
   }

   /* Initialize the semaphore that protects the data. */
   SemData = OSSemCreate (1);
}

/*****   vTankDataAdd   *****************************************

This routine adds new tank data.

RETURNS: None.

*/

void vTankDataAdd (

/*
INPUTS:  */
   int iTank,        /* The tank number. */
   int iLevel)       /* The level. */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   /* Get the semaphore. */
   OSSemPend (SemData, WAIT_FOREVER, &byErr);

   /* Go to the next entry in the tank. */
   ++a_td[iTank].iCurrent;
   if (a_td[iTank].iCurrent IS HISTORY_DEPTH)
   {
      a_td[iTank].iCurrent = 0;  
      a_td[iTank].fFull = TRUE;
   }

   /* Put the data in place. */
   a_td[iTank].a_iLevel[a_td[iTank].iCurrent] = iLevel;
   vTimeGet (a_td[iTank].aa_iTime[a_td[iTank].iCurrent]);

   /* Give back the semaphore. */
   OSSemPost (SemData);

   /* Tell the display that an update may be necessary. */
   vDisplayUpdate ();
}


/*****   iTankDataGet   *****************************************

This routine gets the tank data.

RETURNS: The number of valid entries in a_iLevels 
   when the routine returns.

*/

int iTankDataGet  (

/*
INPUTS:  */
   int iTank,        /* The tank number. */
   int *a_iLevels,   /* An array of levels to return.  
                        a_iLevels[0] will have the most recent 
                        data; a_iLevels[1] the next older data; 
                        and so on. */
   int *aa_iTimes,   /* An array of times corresponding to the 
                        levels.  If this is a null pointer, 
                        then no times will be returned. */
   int iLimit)       /* Number of entries in a_iLevels. */
{

/*
LOCAL VARIABLES:  */
   int iReturn;      /* Value to return. */
   int iIndex;       /* Index into the history data. */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);
   ASSERT (a_iLevels IS_NOT NULLP);
   ASSERT (iLimit > 0);

   /* We haven't found any values yet. */
   iReturn = 0; 

   /* There's only so much history to get. */
   if (iLimit > HISTORY_DEPTH)
      iLimit = HISTORY_DEPTH;

   /* Get the semaphore. */
   OSSemPend (SemData, WAIT_FOREVER, &byErr);

   /* Go through the history entries. */
   iIndex = a_td[iTank].iCurrent;

   while (iIndex >= 0 AND iReturn < iLimit)
   {
      /* Get the next entry into the array. */
      a_iLevels[iReturn] = a_td[iTank].a_iLevel[iIndex];

      /* Get the time, if the caller asked for it. */
      if (aa_iTimes IS_NOT NULLP)
      {
         aa_iTimes[iReturn * 4 + 0] = 
                  a_td[iTank].aa_iTime[iIndex][0];
         aa_iTimes[iReturn * 4 + 1] = 
                  a_td[iTank].aa_iTime[iIndex][1];
         aa_iTimes[iReturn * 4 + 2] = 
                  a_td[iTank].aa_iTime[iIndex][2];
         aa_iTimes[iReturn * 4 + 3] = 
                  a_td[iTank].aa_iTime[iIndex][3];
      }
      ++iReturn;

      /* Find the next oldest element in the array. */
      --iIndex;
      /* If the current pointer has wrapped . . .*/
      if (iIndex IS -1 AND a_td[iTank].fFull)
          /* . . . go back to the end of the array. */
         iIndex = HISTORY_DEPTH - 1;
   }

   /* Give back the semaphore. */
   OSSemPost (SemData);

   return (iReturn);
   
}
