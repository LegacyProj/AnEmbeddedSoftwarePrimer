/****************************************************************

                          L E V E L S . C


This module deals with calculating the tank levels. 


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
#include <time.h>
#include <bios.h>

/* Program Include Files */
#include "publics.h"

/* Local Defines */
#define MSG_LEVEL_VALUE       1

/* Static Functions */
   /* The function to call when the floats have finished. */
      static void vFloatCallback (int iFloatLevel);
   /* The task. */
      static void far vLevelsTask(void *data);

/* Static Data */
   /* Data for the message queue for the button task. */
      #define Q_SIZE 10
      static OS_EVENT *QLevelsTask;
      static void *a_pvQData[Q_SIZE];
      #define STK_SIZE 1024
      static UWORD LevelsTaskStk[STK_SIZE];


/*****   vLevelsSystemInit   ************************************

This routine is the task that initializes the levels task.

RETURNS: None.

*/

void vLevelsSystemInit(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Initialize the queue for this task. */
   QLevelsTask = OSQCreate (&a_pvQData[0], Q_SIZE);

   /* Start the task. */
   OSTaskCreate (vLevelsTask,  NULLP, 
      (void *)&LevelsTaskStk[STK_SIZE], TASK_PRIORITY_LEVELS);

}


/*****   vLevelsTask   ******************************************

This routine is the task that calculates the tank levels.

RETURNS: None.

*/

static void far vLevelsTask(

/*
INPUTS:  */
   void *p_vData)       /* Unused pointer to data */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Error code back from the OS */
   WORD wFloatLevel; /* Message received from the queue */
   int iTank;        /* Tank we're working on. */
   int i,j,k;        /* Variables for pseudo-calculation */
   long l;           /* Ditto */
   int a_iLevels[3]; /* Levels for detecting leaks. */

/*-------------------------------------------------------------*/

   /* Make the compiler warning go away. */
   p_vData = p_vData;

   /* Start with the first tank. */
   iTank = 0;

   while (TRUE)
   {
      /* Get the floats looking for the level in this tank. */
      vReadFloats (iTank, vFloatCallback);

      /* Wait for the result. */
      wFloatLevel = 
         (WORD) OSQPend (QLevelsTask, WAIT_FOREVER, &byErr) - 
            MSG_LEVEL_VALUE;

      /* The "calculation" wastes about 2 seconds. */
      l = 0;
      l = biostime (0, l);    /* Get the time of day */
      while (biostime (0, l) > 1000 AND 
            biostime (0,l) < l + 35L)
      {
         k = 0;
         for (i = 0; i < 1000; i += 2)
            for (j = 0; j < 1000; j += 2)
               if ( (i + j) % 2 IS_NOT 0);
                  ++k;
      }

      /* Now that the "calculation" is done, assume that 
         the number of gallons equals the float level. */


      /* Add the data to the data bank. */
      vTankDataAdd (iTank, wFloatLevel);

      /* Now test for leaks (very simplistically). */
      if (iTankDataGet (iTank, a_iLevels, NULLP, 3) IS 3)
      {
         /* We got three levels.  Test if the levels 
            go down consistently. */
         if (a_iLevels[0] < a_iLevels[1] AND 
               a_iLevels[1] < a_iLevels[2])
         {
            vHardwareBellOn ();
            vDisplayLeak (iTank);
         }

         /* If the tank is rising, watch for overflows */
         if (a_iLevels[0] > a_iLevels[1])
            vOverflowAddTank(iTank);
         
      }

      /* Go to the next tank. */
      ++iTank;
      if (iTank IS COUNTOF_TANKS)
         iTank = 0;
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
   int iFloatLevel)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Put that button on the queue for the task. */
   OSQPost (QLevelsTask, 
      (void *) (iFloatLevel + MSG_LEVEL_VALUE) );
}
