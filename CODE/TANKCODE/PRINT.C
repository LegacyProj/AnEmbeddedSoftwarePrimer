/****************************************************************

                          P R I N T . C


This module deals with the Tank Monitor printer.


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
#define MSG_PRINT_ALL         0x0020
#define MSG_PRINT_TANK_HIST   0x0010

/* Static Functions */
static far void vPrinterTask (void *p_vData);

/* Static Data */
   /* The stack and input queue for the Printer task */
      #define STK_SIZE 1024
      static UWORD PrinterTaskStk[STK_SIZE];

      #define Q_SIZE 10
      static OS_EVENT *QPrinterTask;
      static void *a_pvQPrinterData[Q_SIZE];

   /* Semaphore to wait for report to finish. */
      static OS_EVENT *semPrinter;
      

   /* Place to construct report. */
      static char a_chPrint[10][21];   

      /* Count of lines in report. */
      static int iLinesTotal;

      /* Count of lines printed so far. */
      static int iLinesPrinted;

/*****   vPrinterSystemInit   ***********************************

This routine initializes the Printer system.

RETURNS: None.

*/

void vPrinterSystemInit(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   QPrinterTask = OSQCreate (&a_pvQPrinterData[0], Q_SIZE);

   OSTaskCreate (vPrinterTask,  NULLP, 
      (void *)&PrinterTaskStk[STK_SIZE], TASK_PRIORITY_PRINTER);

   /* Initialize the semaphore as already taken. */
   semPrinter = OSSemCreate (0);
}

/*****   vPrinterTask   *****************************************

This routine is the task that handles the Printer

RETURNS: None.

*/

static far void vPrinterTask(

/*
INPUTS:  */
   void *p_vData)       /* Unused pointer to data */
{

/*
LOCAL VARIABLES:  */
   #define MAX_HISTORY 5
   BYTE byErr;          /* Error code back from the OS */
   WORD wMsg;           /* Message received from the queue */
   int aa_iTime[MAX_HISTORY][4]; 
                        /* Time of day */
   int iTank;           /* Tank iterator. */
   int a_iLevel[MAX_HISTORY];
                        /* Place to get level of tank. */
   int iLevels;         /* Number of history level entries */
   int i;               /* The usual */

/*-------------------------------------------------------------*/

   /* Keep the compiler warnings away. */
   p_vData = p_vData;

   while (TRUE)
   {
      /* Wait for a message. */
      wMsg = (int) OSQPend (QPrinterTask, WAIT_FOREVER, &byErr);

      if (wMsg == MSG_PRINT_ALL)
      {
         /* Format 'all' report. */
         iLinesTotal = 0;
         vTimeGet (aa_iTime[0]);
         sprintf (a_chPrint[iLinesTotal++], 
            "Time: %02d:%02d:%02d", 
            aa_iTime[0][0], aa_iTime[0][1], aa_iTime[0][2]);
         for (iTank = 0; iTank < COUNTOF_TANKS; ++iTank)
         {
            if (iTankDataGet (iTank, a_iLevel, NULLP, 1) IS 1)
               /* We have data for this tank; display it. */
               sprintf (a_chPrint[iLinesTotal++], 
                  "  Tank %d: %d gls.", iTank + 1, a_iLevel[0]);
         }
         sprintf (a_chPrint[iLinesTotal++], 
            "--------------------");
         sprintf (a_chPrint[iLinesTotal++], " ");

      }

      else
      {
         /* Print the history of a single tank. */
         iLinesTotal = 0;
         iTank = wMsg - MSG_PRINT_TANK_HIST;
         iLevels = iTankDataGet (iTank, a_iLevel, 
            (int *) aa_iTime, MAX_HISTORY);
         sprintf (a_chPrint[iLinesTotal++], "Tank %d", 
            iTank + 1);
         for (i = iLevels - 1; i >= 0; --i)
         {
            sprintf (a_chPrint[iLinesTotal++], 
               "  %02d:%02d:%02d %4d gls.", 
               aa_iTime[i][0], aa_iTime[i][1], aa_iTime[i][2], 
               a_iLevel[i]);
         }
         sprintf (a_chPrint[iLinesTotal++], 
            "--------------------");
         sprintf (a_chPrint[iLinesTotal++], " ");
      }

      iLinesPrinted = 0;   
      vHardwarePrinterOutputLine (a_chPrint[iLinesPrinted++]);

      /* Wait for print job to finish. */
      OSSemPend (semPrinter, WAIT_FOREVER, &byErr);
   }  
}

/*****   vPrinterInterrupt   ************************************

This routine is called when the printer interrupts.

RETURNS: None.

*/

void  vPrinterInterrupt (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   if (iLinesPrinted IS iLinesTotal)
      /* The report is done.  Release the semaphore. */
      OSSemPost (semPrinter);

   else
      /* Print the next line. */
      vHardwarePrinterOutputLine (a_chPrint[iLinesPrinted++]);
}  

/*****   vPrinter.....   ****************************************

This routine is called when a printout is needed. */

void vPrintAll(void)
{
   OSQPost (QPrinterTask, (void *) MSG_PRINT_ALL);
}

/*
This routine is called when the user requests printing 
a tank level.
*/

void vPrintTankHistory(int iTank)
{
   /* Check that the parameter is valid. */
   ASSERT (iTank >= 0 AND iTank < COUNTOF_TANKS);

   OSQPost (QPrinterTask, 
      (void *) (MSG_PRINT_TANK_HIST + iTank));
}

