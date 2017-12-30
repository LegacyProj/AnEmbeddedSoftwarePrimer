/****************************************************************

                          O V E R F L O W . C


This module deals with detecting overflows.


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
#include <stdio.h>

/* Program Include Files */
#include "publics.h"

/* Local Defines */
   #define MSG_OFLOW_TIME        0xc010
   #define MSG_OFLOW_ADD_TANK    0xc000

   /* How long to watch tanks */
   #define OFLOW_WATCH_TIME      (3 * 10)
   #define OFLOW_THRESHOLD       7500

/* Local Structures */
typedef struct
{
   int iTime;     /* Time (in 1/3 seconds) to watch this tank */
   int iLevel;    /* Level last time this tank was checked */
}  TANK_WATCH;

/* Static Functions */
static far void vOverflowTask (void *p_vData);
static void vFloatCallback (int iFloatLevel);

/* Static Data */

   /* The stack and input queue for the Overflow task */
      #define STK_SIZE 1024
      static UWORD OverflowTaskStk[STK_SIZE];

      #define Q_SIZE 10
      static OS_EVENT *QOverflowTask;
      static void *a_pvQOverflowData[Q_SIZE];

/*****   vOverflowSystemInit   **********************************

This routine initializes the Overflow system.

RETURNS: None.

*/

void vOverflowSystemInit(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   QOverflowTask = OSQCreate (&a_pvQOverflowData[0], Q_SIZE);

   OSTaskCreate (vOverflowTask,  NULLP, 
      (void *)&OverflowTaskStk[STK_SIZE], TASK_PRIORITY_OVERFLOW);
}

/*****   vOverflowTask   ****************************************

This routine is the task that handles the Overflow

RETURNS: None.

*/

static far void vOverflowTask(

/*
INPUTS:  */
   void *p_vData)       /* Unused pointer to data */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;          /* Error code back from the OS */
   WORD wMsg;           /* Message received from the queue */
   TANK_WATCH tw[3];    /* Structure with which to watch tanks */
   int i;               /* The usual iterator */
   int iTank;           /* Tank number to watch */
   int iFloatTank;      /* The tank whose float we're reading */

/*-------------------------------------------------------------*/

   /* Keep the compiler warnings away. */
   p_vData = p_vData;

   /* We are watching no tanks */
   for (i = 0; i < 3; ++i)
      tw[i].iTime = 0;
   iFloatTank = NO_TANK;

   while (TRUE)
   {
      /* Wait for a message. */
      wMsg = (int) OSQPend (QOverflowTask, WAIT_FOREVER, &byErr);

      if (wMsg IS MSG_OFLOW_TIME)
      {
         if (iFloatTank IS NO_TANK)
         {
            /* Find the first tank on the watch list. */
            i = 0;
            while (i < COUNTOF_TANKS AND iFloatTank IS NO_TANK)
            {
               if (tw[i].iTime IS_NOT 0)
               {
                  /* This tank is on the watch list */
                  /* Reduce the time gor this tank. */
                  --tw[i].iTime;

                  /* Get the floats looking for the level 
                     in this tank. */
                  iFloatTank = i;
						vReadFloats (iFloatTank, vFloatCallback);
               }
               ++i;
            }
         }
      }

      else if (wMsg >= MSG_OFLOW_ADD_TANK)
      {
         /* Add a tank to the watch list */
         iTank = wMsg - MSG_OFLOW_ADD_TANK;
         tw[iTank].iTime = OFLOW_WATCH_TIME;
         iTankDataGet (iTank, &tw[iTank].iLevel, NULLP, 1);
      }
      else /* wMsg must be a float level. */
      {
         /* If the tank is still rising . . .  */
         if (wMsg > tw[iFloatTank].iLevel)
         {
            /* . . . If the level is too high . . . */
            if (wMsg >= OFLOW_THRESHOLD)
            {
               /* Warn the user */
               vHardwareBellOn ();
               vDisplayOverflow (iFloatTank);

               /* Stop watching this tank */
               tw[iFloatTank].iTime = 0;

               
            }
            else
               /* Keep watching it. */
               tw[iFloatTank].iTime = OFLOW_WATCH_TIME;
         }

         /* Store the new level */
         tw[iFloatTank].iLevel = wMsg;

         /* Find the first tank on the watch list. */
         i = iFloatTank + 1;
         iFloatTank = NO_TANK;
         while (i < COUNTOF_TANKS AND iFloatTank IS NO_TANK)
         {
            if (tw[i].iTime IS_NOT 0)
            {
               /* This tank is on the watch list */
               /* Reduce the time gor this tank. */
               --tw[i].iTime;

               /* Get the floats looking for the level 
                  in this tank. */
               iFloatTank = i;
               vReadFloats (iFloatTank, vFloatCallback);
            }
            ++i;
         }
      }


   }  
}

/*****   vFloatCallback   ***************************************

This is the routine that the floats module calls when it has 
a float reading.

RETURNS: None.

*/

static void vFloatCallback (

/*
INPUTS:  */
   int iFloatLevelNew)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Put the level on the queue for the task. */
   OSQPost (QOverflowTask, (void *) iFloatLevelNew);
}

/*****   vOverflow.....   ***************************************

This routine is called three times a second. */

void vOverflowTime(void)
{
   OSQPost (QOverflowTask, (void *) MSG_OFLOW_TIME);
}

/*
This routine is called when a tank level is increasing.
*/

void vOverflowAddTank(int iTank)
{
   /* Check that the parameter is valid. */
   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   OSQPost (QOverflowTask, 
      (void *) (MSG_OFLOW_ADD_TANK + iTank));
}


