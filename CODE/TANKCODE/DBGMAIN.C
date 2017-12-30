/****************************************************************

                          D B G M A I N . C


This module has the startup and debugging code.


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
#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Program Include Files */
#include "publics.h"

/* Local Defines */

   /* DOS screen display parameters */

      /* Dividing line between dbg control and system display */
      #define DBG_SCRN_DIV_X        32

      /* Rows on debug control screen */
      #define DBG_SCRN_TIME_ROW     12 
      #define DBG_SCRN_FLOAT_ROW    22

      /* Button locations */
      #define DBG_SCRN_BTN_X        35
      #define DBG_SCRN_BTN_Y        17
      #define DBG_SCRN_BTN_WIDTH    7
      #define DBG_SCRN_BTN_HEIGHT   2
      #define DBG_SCRN_BTN_COLOR          GREEN
      #define DBG_SCRN_BTN_BLINK_COLOR    RED

      /* System display */
      #define DBG_SCRN_DISP_X       33
      #define DBG_SCRN_DISP_Y       13
      #define DBG_SCRN_DISP_WIDTH   20

      /* System printer */
      #define DBG_SCRN_PRNTR_X      57
      #define DBG_SCRN_PRNTR_Y      5
      #define DBG_SCRN_PRNTR_WIDTH  20
      #define DBG_SCRN_PRNTR_HEIGHT 15

      /* Bell display */
      #define DBG_SCRN_BELL_X       43
      #define DBG_SCRN_BELL_Y       5
      #define DBG_SCRN_BELL_WIDTH   10
      #define DBG_SCRN_BELL_HEIGHT  3

   /* Line drawing characters for text mode */
      #define LINE_HORIZ      196
      #define LINE_VERT       179
      #define LINE_CORNER_NW  218
      #define LINE_CORNER_NE  191
      #define LINE_CORNER_SE  217
      #define LINE_CORNER_SW  192
      #define LINE_T_W        195
      #define LINE_T_N        194
      #define LINE_T_E        180
      #define LINE_T_S        193
      #define LINE_CROSS      197


/* Static Functions */
   static void vUtilityDrawBox (int ixNW, int iyNW, 
      int ixSize, int iySize);
   static void vUtilityDisplayFloatLevels (void);
   static void vUtilityPrinterDisplay (void);

/* Static Data */

   /* Data for displaying and getting buttons */
      #define BUTTON_ROWS        3
      #define BUTTON_COLUMNS     3

      static char *p_chButtonText[BUTTON_ROWS][BUTTON_COLUMNS] =
      {
         {" PRT ", "  1  ", "TIME "},
         {" HST ", "  2  ", NULLP},
         {" ALL ", "  3  ", " RST "}
      };

      static char a_chButtonKey[BUTTON_ROWS][BUTTON_COLUMNS] =
      {
         {'P', '1', 'T'},
         {'H', '2', '\x00'},
         {'A', '3', 'R'}
      };

      /* Button the user pressed. */
      static WORD wButton; 

   /* Printer state. */
      /* Printed lines. */
      static char aa_charPrinted
            [DBG_SCRN_PRNTR_HEIGHT][DBG_SCRN_PRNTR_WIDTH + 1];

      /* Printing a line now. */
      static int iPrinting = 0;

   /* Debug variables for reading the tank levels. */
      /* Float levels. */
      static int a_iTankLevels[COUNTOF_TANKS] = 
         {4000, 7200, 6400};

      /* Which tank the system asked about.  NO_TANK means that 
         the simulated float hardware is not reading. */
      static int iTankToRead = NO_TANK;

      /* Which tank the user is changing. */
      static int iTankChanging = 0;

   /* Is time passing automatically? */
      static BOOL fAutoTime = FALSE;

   /* Tasks and stacks for debugging */
      #define STK_SIZE 1024
      UWORD DebugKeyStk[STK_SIZE];
      UWORD DebugTimerStk[STK_SIZE];
      static void far vDebugKeyTask(void *data);
      static void far vDebugTimerTask(void *data);
      static OS_EVENT *semDOS;

   /* Place to store DOS timer interrupt vector. */
      static void interrupt far (*OldTickISR)(void);


/*****   main   *************************************************

This routine starts the system.

RETURNS: None.

*/

void main(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Set up timer and uC/OS interrupts */
   OldTickISR = getvect(0x08);
   setvect(uCOS, (void interrupt (*)(void))OSCtxSw);
   setvect(0x81, OldTickISR);

   /* Start the real system */
   vEmbeddedMain ();

}  


/*****   vHardwareInit   ****************************************

This routine initializes the fake hardware.

RETURNS: None.

*/

void vHardwareInit (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int iColumn, iRow;   /* Iterators */
   BYTE byErr;          /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   /* Start the debugging tasks. */
   OSTaskCreate(vDebugTimerTask, NULLP, 
      (void *)&DebugTimerStk[STK_SIZE], 
      TASK_PRIORITY_DEBUG_TIMER);
   OSTaskCreate(vDebugKeyTask,  NULLP, 
      (void *)&DebugKeyStk[STK_SIZE], 
      TASK_PRIORITY_DEBUG_KEY);

   /* Initialize the DOS protection semaphore */
   semDOS = OSSemCreate (1);
   
   /* Set up the debugging display on the DOS screen */
   OSSemPend (semDOS, WAIT_FOREVER, &byErr);
   clrscr();

   /* Divide the screen. */
   for (iRow = 1; iRow < 25; ++iRow)
   {
      gotoxy (DBG_SCRN_DIV_X, iRow);
      cprintf ("%c", LINE_VERT);
   }

   /*  Set up the debug side of the screen */
   gotoxy (7,2);
   cprintf ("    D E B U G");

   gotoxy (1,4);
   cprintf ("These keys press buttons:");
   gotoxy (1,5);
   cprintf  ("    P   1   T");
   gotoxy (1,6);
   cprintf  ("    H   2");
   gotoxy (1,7);
   cprintf  ("    A   3   R");
   gotoxy (1,9);
   cprintf  ("Press 'X' to exit the program");
   gotoxy (1,10);
   cprintf ("---------------------------");

   gotoxy (1, DBG_SCRN_TIME_ROW - 1);
   cprintf ("TIME:");
   gotoxy (1, DBG_SCRN_TIME_ROW);
   cprintf ("  '!' to make 1/3 second pass");
   gotoxy (1, DBG_SCRN_TIME_ROW + 1);
   cprintf ("  '@' to toggle auto timer");
   gotoxy (1, DBG_SCRN_TIME_ROW + 3);
   cprintf ("Auto-time is:");
   gotoxy (15, DBG_SCRN_TIME_ROW + 3);
   textbackground (RED);
   cprintf (" OFF ");
   textbackground (BLACK);
   gotoxy (1, DBG_SCRN_TIME_ROW + 4);
   cprintf ("---------------------------");

   /* Display the current tank levels. */
   gotoxy (1, DBG_SCRN_FLOAT_ROW - 4);
   cprintf ("FLOATS:");
   gotoxy (1, DBG_SCRN_FLOAT_ROW - 3);
   cprintf ("  '<' and '>' to select float");
   gotoxy (1, DBG_SCRN_FLOAT_ROW - 2);
   cprintf ("  '+' and '-' to change level");
   gotoxy (1, DBG_SCRN_FLOAT_ROW);
   cprintf ("Tank");
   gotoxy (1, DBG_SCRN_FLOAT_ROW + 2);
   cprintf ("Level");

   vUtilityDisplayFloatLevels ();

   /* Start with the buttons. */
   textbackground (DBG_SCRN_BTN_COLOR);
   for (iRow = 0; iRow < BUTTON_ROWS; ++iRow)
      for (iColumn = 0; iColumn < BUTTON_COLUMNS; ++iColumn)
      {
         if (p_chButtonText[iRow][iColumn] IS_NOT NULLP)
         {
            gotoxy (DBG_SCRN_BTN_X + iColumn*DBG_SCRN_BTN_WIDTH,
                  DBG_SCRN_BTN_Y + iRow * DBG_SCRN_BTN_HEIGHT);
            cprintf ("%s", p_chButtonText[iRow][iColumn]);
         }
      }
   textbackground (BLACK);

   /*  Set up the system side of the screen */
   gotoxy (DBG_SCRN_DIV_X + 14, 2);
   cprintf ("    S Y S T E M");

   /* Draw the display */
   vUtilityDrawBox (DBG_SCRN_DISP_X, DBG_SCRN_DISP_Y, 
      DBG_SCRN_DISP_WIDTH, 1);

   /* Draw the printer */
   vUtilityDrawBox (DBG_SCRN_PRNTR_X, DBG_SCRN_PRNTR_Y, 
         DBG_SCRN_PRNTR_WIDTH, DBG_SCRN_PRNTR_HEIGHT);
   vUtilityDrawBox (DBG_SCRN_PRNTR_X, 
         DBG_SCRN_PRNTR_Y + DBG_SCRN_PRNTR_HEIGHT + 1,
         DBG_SCRN_PRNTR_WIDTH, 1);
   gotoxy (DBG_SCRN_PRNTR_X + 1, 
         DBG_SCRN_PRNTR_Y + DBG_SCRN_PRNTR_HEIGHT + 2);
   cprintf ("  ^^  PRINTER  ^^  ");

   /* Initialize printer lines. */
   for (iRow = 0; iRow < DBG_SCRN_PRNTR_HEIGHT; ++iRow)
      strcpy (aa_charPrinted[iRow], "");

   /* Draw the bell. */
   vUtilityDrawBox (DBG_SCRN_BELL_X, DBG_SCRN_BELL_Y, 
      DBG_SCRN_BELL_WIDTH, 
      DBG_SCRN_BELL_HEIGHT);
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 2);
   cprintf ("   BELL   ");

   OSSemPost (semDOS);

}  


/*****   vDebugKeyTask   ****************************************

This routine gets keystrokes from DOS and feeds them to the rest 
of the system.

RETURNS: None.

*/

static void far vDebugKeyTask(

/*
INPUTS:  */
   void *p_vData)
{

/*
LOCAL VARIABLES:  */
   int iKey;                  /* DOS key the user struck */
   int iColumn = 0, iRow = 0; /* System button activated. */
   BOOL fBtnFound = FALSE;    /* TRUE if sys button pressed. */
   BYTE byErr;                /* Place for OS to return error. */

/*-------------------------------------------------------------*/

   /* Keep the compiler happy. */
   p_vData = p_vData;

   /* Redirect the DOS timer interrupt to uC/OS */
   setvect(0x08, (void interrupt (*)(void))OSTickISR);

   while (TRUE) 
   {
      /* Wait for keys to come in */
      OSTimeDly(2);

      /* Are we printing a line? */
      if (iPrinting)
      {    
         /* Yes. */
         --iPrinting;

         if (iPrinting IS 0)
            /* We have finished.  Call the interrupt routine. */
            vPrinterInterrupt ();
      }

      /* Unblink a button, if necessary. */
      if (fBtnFound)
      {
         OSSemPend (semDOS, WAIT_FOREVER, &byErr);
         textbackground (DBG_SCRN_BTN_COLOR);
         gotoxy (DBG_SCRN_BTN_X + iColumn * DBG_SCRN_BTN_WIDTH,
            DBG_SCRN_BTN_Y + iRow * DBG_SCRN_BTN_HEIGHT);
         cprintf ("%s", p_chButtonText[iRow][iColumn]);
         textbackground (BLACK);
         OSSemPost (semDOS);
         fBtnFound = FALSE;
      }

      /* If the system set up the floats, 
         cause the float interrupt. */
      if (iTankToRead IS_NOT NO_TANK)
         vFloatInterrupt ();

      /* See if the tester-user has pressed a DOS key. */
      OSSemPend (semDOS, WAIT_FOREVER, &byErr);
      if (kbhit()) 
      {
         /* He has.  Get the key */
         iKey = getch ();
         switch (iKey) 
         {
            case '!':
               /* If time is not passing automatically, 
                  make 1/3 second pass */
               if (!fAutoTime)
                  vTimerOneThirdSecond ();
               break;

            case '@':
               /* Toggle the state of the automatic timer */
               fAutoTime = !fAutoTime;

               /* . . . and display the result. */
               if (fAutoTime)
               {
                  gotoxy (15, DBG_SCRN_TIME_ROW + 3);
                  textbackground (GREEN);
                  cprintf ("  ON ");
                  textbackground (BLACK);
               }
               else
               {
                  gotoxy (15, DBG_SCRN_TIME_ROW + 3);
                  textbackground (RED);
                  cprintf (" OFF ");
                  textbackground (BLACK);
               }
               break;

            case 't':
            case 'T':
            case '1':
            case '2':
            case '3':
            case 'r':
            case 'R':
            case 'p':
            case 'P':
            case 'a':
            case 'A':
            case 'h':
            case 'H':

               /* Note which button has been pressed. */
               wButton = toupper (iKey);

               iRow = 0;
               fBtnFound = FALSE;
               while (iRow < BUTTON_ROWS AND !fBtnFound)
               {
                  iColumn = 0;
                  while (iColumn < BUTTON_COLUMNS AND !fBtnFound)
                  {
                     if (wButton IS 
                           (WORD) a_chButtonKey[iRow][iColumn])
                        fBtnFound = TRUE;
                     else
                        ++iColumn;
                  }                          
                  if (!fBtnFound)                
                     ++iRow;
               }

               /* Blink the button red. */
               textbackground (DBG_SCRN_BTN_BLINK_COLOR);
               gotoxy (
                  DBG_SCRN_BTN_X + iColumn * DBG_SCRN_BTN_WIDTH,
                  DBG_SCRN_BTN_Y + iRow * DBG_SCRN_BTN_HEIGHT);
               cprintf ("%s", p_chButtonText[iRow][iColumn]);
               textbackground (BLACK);

               /* Fake a button interrupt. */
               vButtonInterrupt ();
               break;

            case '-':
               /* Reduce the level in the current tank. */
               a_iTankLevels[iTankChanging] -= 80;
               if (a_iTankLevels[iTankChanging] < 0)
                  a_iTankLevels[iTankChanging] = 0;
               vUtilityDisplayFloatLevels ();
               break;

            case '+':
               /* Increase the level in the current tank. */
               a_iTankLevels[iTankChanging] += 80;
               if (a_iTankLevels[iTankChanging] > 8000)
                  a_iTankLevels[iTankChanging] = 8000;
               vUtilityDisplayFloatLevels ();
               break;

            case 'x':
            case 'X':
               /* Restore the DOS timer interrupt vector */
               setvect(0x08, OldTickISR);

               /* End the program. */
               exit(0);
               break;

            case '>':
               /* Choose a different tank to modify */
               ++iTankChanging;
               if (iTankChanging IS COUNTOF_TANKS)
                  iTankChanging = COUNTOF_TANKS - 1;
               vUtilityDisplayFloatLevels ();
               break;

            case '<':
               /* Choose a different tank to modify */
               --iTankChanging;
               if (iTankChanging < 0)
                  iTankChanging = 0;
               vUtilityDisplayFloatLevels ();
               break;
         }
      }
      OSSemPost (semDOS);
   }
}



/*****   vDebugTimerTask   **************************************

This routine makes timer interrupts happen, if the tester wants.

RETURNS: None.

*/

static void far vDebugTimerTask(

/*
INPUTS:  */
   void *p_vData)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Keep the compiler happy. */
   p_vData = p_vData;

   while (TRUE) 
   {
      OSTimeDly (6);
      if (fAutoTime)
         vTimerOneThirdSecond ();
   }
}

/*****   vUtilityDrawBox   **************************************

This routine draws a box.

RETURNS: None.

*/

static void vUtilityDrawBox (

/*
INPUTS:  */
   int ixNW,      /* x-coord of northwest corner of the box. */
   int iyNW,      /* y-coord of northwest corner of the box. */
   int ixSize,    /* Inside width of the box. */
   int iySize)    /* Inside height of the box. */
{

/*
LOCAL VARIABLES:  */
   int iColumn, iRow;

/*-------------------------------------------------------------*/

   /* Draw the top of the box. */
   gotoxy (ixNW, iyNW);
   cprintf ("%c", LINE_CORNER_NW);
   for (iColumn = 0; iColumn < ixSize; ++iColumn)
      cprintf ("%c", LINE_HORIZ);
   cprintf ("%c", LINE_CORNER_NE);

   /* Draw the sides. */
   for (iRow = 1; iRow <= iySize; ++iRow)
   {
      gotoxy (ixNW, iyNW + iRow);
      cprintf ("%c", LINE_VERT);
      gotoxy (ixNW + ixSize + 1, iyNW + iRow);
      cprintf ("%c", LINE_VERT);
   }

   /* Draw the bottom. */
   gotoxy (ixNW, iyNW + iySize + 1);
   cprintf ("%c", LINE_CORNER_SW);
   for (iColumn = 0; iColumn < ixSize; ++iColumn)
      cprintf ("%c", LINE_HORIZ);
   cprintf ("%c", LINE_CORNER_SE);
   
}  


/*****   vUtilityDisplayFloatLevels   ***************************

This routine displays the debug floats.

RETURNS: None.

*/

static void vUtilityDisplayFloatLevels (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int iTank;        /* Iterator. */

/*-------------------------------------------------------------*/

   for (iTank = 0; iTank < COUNTOF_TANKS; ++iTank)
   {
      if (iTank IS iTankChanging)
         textbackground (BLUE);
      gotoxy (iTank * 8 + 10, DBG_SCRN_FLOAT_ROW);
      cprintf (" %4d ", iTank + 1);
      gotoxy (iTank * 8 + 10, DBG_SCRN_FLOAT_ROW + 1);
      cprintf ("      ", iTank + 1);
      gotoxy (iTank * 8 + 10, DBG_SCRN_FLOAT_ROW + 2);
      cprintf (" %4d ", a_iTankLevels[iTank]);
      textbackground (BLACK);
   }

   
}  

/*****   vUtilityPrinterDisplay   *******************************

This routine displays all printer output.

RETURNS: None.

*/

static void vUtilityPrinterDisplay (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int i, j;         /* Iterators. */

/*-------------------------------------------------------------*/

   for (i = 0; i < DBG_SCRN_PRNTR_HEIGHT; ++i)
   {
      gotoxy (DBG_SCRN_PRNTR_X + 1, DBG_SCRN_PRNTR_Y + i + 1);
      for (j = 0; j < DBG_SCRN_PRNTR_WIDTH; ++j)
         cprintf (" ");
      gotoxy (DBG_SCRN_PRNTR_X + 1, DBG_SCRN_PRNTR_Y + i + 1);
      cprintf (aa_charPrinted[i]);
   }
}  

/*****   vHardwareDisplayLine   *********************************

This routine Displays on the debug screen whatever the system 
decides should be on the Display

RETURNS: None.

*/

void vHardwareDisplayLine (

/*
INPUTS:  */
   char *a_chDisp)      /* Character string to Display */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   /* Check that the length of the string is OK */
   ASSERT (strlen (a_chDisp) <= DBG_SCRN_DISP_WIDTH);

   /* Display the string. */
   OSSemPend (semDOS, WAIT_FOREVER, &byErr);
   gotoxy (DBG_SCRN_DISP_X + 1, DBG_SCRN_DISP_Y + 1);
   cprintf ("                    ");
   gotoxy (DBG_SCRN_DISP_X + 1, DBG_SCRN_DISP_Y + 1);
   cprintf ("%s", a_chDisp);
   OSSemPost (semDOS);

}


/*****   wHardwareButtonFetch   *********************************

This routine gets the button that has been pressed.

RETURNS: None.

*/

WORD wHardwareButtonFetch (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   return (toupper (wButton));
}


/*****   vHardwareFloatSetup   **********************************

This routine gets the float hardware looking for a reading.

RETURNS: None.

*/

void vHardwareFloatSetup  (

/*
INPUTS:  */
   int iTankNumber)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Check that the parameter is valid. */
   ASSERT (iTankNumber >= 0 AND iTankNumber < COUNTOF_TANKS);

   /* The floats should not be busy. */
   ASSERT (iTankToRead IS NO_TANK);

   /* Remember which tank the system asked about. */
   iTankToRead = iTankNumber;
}


/*****   iHardwareFloatGetData   ********************************

This routine returns a float reading.

RETURNS: None.

*/

int iHardwareFloatGetData   (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int iTankTemp;       /* Temporary tank number. */

/*-------------------------------------------------------------*/

   /* We must have been asked to read something. */
   ASSERT (iTankToRead >= 0 AND iTankToRead < COUNTOF_TANKS);

   /* Remember which tank the system asked about. */
   iTankTemp = iTankToRead;

   /* We're not reading anymore. */
   iTankToRead = NO_TANK;

   /* Return the tank reading. */
   return(a_iTankLevels[iTankTemp]);
   
}

/*****   vHardwareBellOn   **************************************

This routine turns on the alarm bell.

RETURNS: None.

*/

void vHardwareBellOn  (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   OSSemPend (semDOS, WAIT_FOREVER, &byErr);

   /* Set the bell color. */
   textbackground (RED);
   textcolor (BLINK + WHITE);

   /* Draw the bell. */
   gotoxy (DBG_SCRN_BELL_X+ 1, DBG_SCRN_BELL_Y + 1);
   cprintf ("          ");
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 2);
   cprintf ("   BELL   ");
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 3);
   cprintf ("          ");

   /* Reset the text color to normal. */
   textcolor (LIGHTGRAY);
   textbackground (BLACK);

   OSSemPost (semDOS);
}

/*****   vHardwareBellOff   *************************************

This routine turns on the alarm bell.

RETURNS: None.

*/

void vHardwareBellOff  (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   OSSemPend (semDOS, WAIT_FOREVER, &byErr);
   
   /* Draw the bell in plain text. */
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 1);
   cprintf ("          ");
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 2);
   cprintf ("   BELL   ");
   gotoxy (DBG_SCRN_BELL_X + 1, DBG_SCRN_BELL_Y + 3);
   cprintf ("          ");

   OSSemPost (semDOS);
}



/*****   vHardwarePrinterOutputLine   ***************************

This routine displays on the debug screen whatever the system 
decides should be printed.

RETURNS: None.

*/

void vHardwarePrinterOutputLine (

/*
INPUTS:  */
   char *a_chPrint)     /* Character string to print */
{

/*
LOCAL VARIABLES:  */
   int i;            /* The usual. */

/*-------------------------------------------------------------*/

   /* Check that the length of the string is OK */
   ASSERT (strlen (a_chPrint) <= DBG_SCRN_PRNTR_WIDTH);

   /* Move all the old lines up. */
   for (i = 1; i < DBG_SCRN_PRNTR_HEIGHT; ++i)
      strcpy (aa_charPrinted[i - 1], aa_charPrinted[i]);

   /* Add the new line. */
   strcpy (aa_charPrinted[DBG_SCRN_PRNTR_HEIGHT - 1], a_chPrint);

   /* Note that we need to interrupt. */
   iPrinting = 4;

   /* Redraw the printer. */
   vUtilityPrinterDisplay ();
   
}  


