/****************************************************************

                          D I S P L A Y . C


This module deals with the Tank Monitor Display.


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
#define MSG_UPDATE            0x0001
#define MSG_DISP_RESET_ALARM  0x0002
#define MSG_DISP_NO_PROMPT    0x0003
#define MSG_DISP_OFLOW        0x0040
#define MSG_DISP_LEAK         0x0080
#define MSG_USER_REQUEST      0x8000
#define MSG_DISP_TIME         (MSG_USER_REQUEST | 0x0001)
#define MSG_DISP_TANK         (MSG_USER_REQUEST | 0x0400)
#define MSG_DISP_PROMPT       (MSG_USER_REQUEST | 0x0800)

/* Static Functions */
static far void vDisplayTask (void *p_vData);

/* Static Data */
   /* The stack and input queue for the display task */
      #define STK_SIZE 1024
      static UWORD DisplayTaskStk[STK_SIZE];

      #define Q_SIZE 10
      static OS_EVENT *QDisplayTask;
      static void *a_pvQDisplayData[Q_SIZE];


/*****   vDisplaySystemInit   ***********************************

This routine initializes the display system.

RETURNS: None.

*/

void vDisplaySystemInit(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   QDisplayTask = OSQCreate (&a_pvQDisplayData[0], Q_SIZE);

   OSTaskCreate (vDisplayTask,  NULLP, 
      (void *)&DisplayTaskStk[STK_SIZE], TASK_PRIORITY_DISPLAY);
}

/*****   vDisplayTask   *****************************************

This routine is the task that handles the display

RETURNS: None.

*/

static far void vDisplayTask(

/*
INPUTS:  */
   void *p_vData)       /* Unused pointer to data */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;          /* Error code back from the OS */
   WORD wMsg;           /* Message received from the queue */
   int a_iTime[4];      /* Time of day */
   char a_chDisp[21];   /* Place to construct display string. */
   WORD wUserRequest;   /* Code indicating what user requested 
                           to display. */
   int iLevel;          /* Tank level to display. */
   int iTankLeaking;    /* Tank that is leaking. */
   int iTankOverflow;      /* Tank that is overflowing. */
   int iPrompt;         /* Command prompt we are displaying. */

/*-------------------------------------------------------------*/

   /* Keep the compiler warnings away. */
   p_vData = p_vData;

   /* Initialize the display */
   vTimeGet (a_iTime);
   sprintf (a_chDisp, "     %02d:%02d:%02d", 
      a_iTime[0], a_iTime[1], a_iTime[2]);
   vHardwareDisplayLine (a_chDisp);
   wUserRequest = MSG_DISP_TIME;

   /* Note that we don't know of anything that is leaking, 
      overflowing, etc. yet. */
   iTankLeaking = NO_TANK;
   iTankOverflow = NO_TANK;
   iPrompt = -1;

   while (TRUE)
   {
      /* Wait for a queue. */
      wMsg = (int) OSQPend (QDisplayTask, WAIT_FOREVER, &byErr);

      if (wMsg & MSG_USER_REQUEST)
      {
         if (wMsg & ~MSG_USER_REQUEST & MSG_DISP_PROMPT)
            /* Store the prompt we've been asked to display. */
            iPrompt = wMsg - MSG_DISP_PROMPT;

         else
            /* Store what the user asked us to display. */
            wUserRequest = wMsg; 
      }

      else if (wMsg & MSG_DISP_LEAK) 
         /* Store the number of the leaking tank. */
         iTankLeaking = wMsg - MSG_DISP_LEAK;

      else if (wMsg & MSG_DISP_OFLOW)
         /* Store the number of the overflowing tank. */
         iTankOverflow = wMsg - MSG_DISP_OFLOW;
 
      else if (wMsg IS MSG_DISP_RESET_ALARM) 
      {
         iTankLeaking = NO_TANK;
         iTankOverflow = NO_TANK;
         iPrompt = -1;
      }
      else if (wMsg IS MSG_DISP_NO_PROMPT) 
         iPrompt = -1;

      /* ELSE it's an update message. */

      /* Now do the display. */
      if (iTankOverflow IS_NOT NO_TANK)
         /* A tank is leaking.  This takes priority. */
         sprintf (a_chDisp, "Tank %d: OVERFLOW!!",
            iTankOverflow + 1);

      else if (iTankLeaking IS_NOT NO_TANK)
         /* A tank is leaking.  This takes priority. */
         sprintf (a_chDisp, "Tank %d: LEAKING!!", 
            iTankLeaking + 1);

      else if (iPrompt >= 0)
         sprintf (a_chDisp, p_chGetCommandPrompt (iPrompt));

      else if (wUserRequest IS MSG_DISP_TIME)
      {
         /* Display the time. */
         vTimeGet (a_iTime);
         sprintf (a_chDisp, "     %02d:%02d:%02d", 
               a_iTime[0], a_iTime[1], a_iTime[2]);
      }

      else
      {
         /* User must want tank level displayed.  Get a level. */
         if (iTankDataGet (wUserRequest - MSG_DISP_TANK, 
                  &iLevel, NULLP, 1) 
               IS 1)
            /* We have data for this tank; display it. */
            sprintf (a_chDisp, "Tank %d: %d gls.", 
               wUserRequest - MSG_DISP_TANK + 1, iLevel);
         else
            /* A level for this tank is not yet available. */
            sprintf (a_chDisp, "Tank %d: N/A.", 
               wUserRequest - MSG_DISP_TANK + 1);
      }

      vHardwareDisplayLine (a_chDisp);
   }  
}


/*****   vDisplay.....   ****************************************

This routine is called when something happens that may require 
the display to be updated.
*/

void vDisplayUpdate(void)
{
   OSQPost (QDisplayTask, (void *) MSG_UPDATE);
}

/*
This routine is called when the user requests displaying 
   a tank level.
*/

void vDisplayTankLevel(int iTank)
{
   /* Check that the parameter is valid. */
   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   OSQPost (QDisplayTask, (void *) (MSG_DISP_TANK + iTank));
}

/*
This routine is called when the user requests displaying 
   the time.
*/

void vDisplayTime(void)
{
   OSQPost (QDisplayTask, (void *) MSG_DISP_TIME);
}

/*
This routine is called when the command processor needs 
   a prompt display.
*/

void vDisplayPrompt(int iPrompt)    /* Index number of prompt. */
{
   /* We can only encode a certain number of prompts. */
   ASSERT (iPrompt < 0x400);

   OSQPost (QDisplayTask, (void *) (MSG_DISP_PROMPT + iPrompt));
}

/*
This routine is called when the command processor doesn't need 
   a prompt display any more.
*/

void vDisplayNoPrompt(void)   
{
   OSQPost (QDisplayTask, (void *) MSG_DISP_NO_PROMPT);
}

/*
This routine is called when a leak is detected.
*/

void vDisplayLeak(int iTank)
{
   /* Check that the parameter is valid. */
   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   OSQPost (QDisplayTask, (void *) (MSG_DISP_LEAK + iTank));
}

/*
This routine is called when an overflow is detected.
*/

void vDisplayOverflow(int iTank)
{
   /* Check that the parameter is valid. */
   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   OSQPost (QDisplayTask, (void *) (MSG_DISP_OFLOW + iTank));
}

/*
This routine is called when the user resets the alarm.
*/

void vDisplayResetAlarm(void)
{
   OSQPost (QDisplayTask, (void *) MSG_DISP_RESET_ALARM);
}

