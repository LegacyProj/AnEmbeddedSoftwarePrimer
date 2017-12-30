/****************************************************************

                          P U B L I C S . H


This include file contains the interface information 
for the modules.


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


#ifndef _PUBLICS
#define _PUBLICS

/* Defines */
   #define WAIT_FOREVER    0

   /*  The priorities of the various tasks */
   #define TASK_PRIORITY_DEBUG_TIMER      6
   #define TASK_PRIORITY_DEBUG_KEY        7
   #define TASK_PRIORITY_BUTTON           10
   #define TASK_PRIORITY_DISPLAY          11
   #define TASK_PRIORITY_OVERFLOW         13
   #define TASK_PRIORITY_PRINTER          15
   #define TASK_PRIORITY_LEVELS           20

   #define COUNTOF_TANKS   3
   #define NO_TANK         -1

/* Structures */

   typedef void (*V_FLOAT_CALLBACK) (int iFloatLevel);

/* Public functions in main.c */
   void vEmbeddedMain (void);
      /* The main routine of the hardware-independent software */

/* Public functions in display.c */
   void vDisplaySystemInit(void);
      /* Initializes the software that handles the display. */
   void vDisplayUpdate(void);
      /* Tells the display software to update whatever data is 
         on the display. */
   void vDisplayTankLevel(int iTank);
      /* Tells the display software that the user has requested 
         to view the level in tank iTank. */
   void vDisplayTime(void);
      /* Tells the display software that the user has requested 
         to view the time. */
   void vDisplayPrompt(int iPrompt); 
      /* Tells the display software that the command software 
         wants to display a prompt. */
   void vDisplayNoPrompt(void);
      /* Tells the display software that the command software 
         no longer wants to display a prompt. */
   void vDisplayLeak(int iTank);
      /* Tells the display software that a leak has been 
         detected. */
   void vDisplayOverflow(int iTank);
      /* Tells the display software that an overflow has been 
         detected. */
   void vDisplayResetAlarm(void);
      /* Tells the display software that the user has pressed 
         the reset button. */

/* Public functions in button.c */
   void vButtonSystemInit (void);
      /* Initializes the software that handles the buttons. */
   void vButtonInterrupt (void);
      /* Called by the shell software to indicate that the 
         user/tester has pressed a button.  On the target 
         hardware, this will become part of the button 
         interrupt routine. */
   char * p_chGetCommandPrompt (int iPrompt);
      /* Called by the display software to find the text of the 
         prompt that the command state machine wishes 
         to display. */

/* Public functions in levels.c */
   void vLevelsSystemInit(void);
      /* Initializes the software that handles the levels 
         in the tanks. */

/* Public functions in print.c */
   void vPrinterSystemInit(void);
      /* Initializes the software that formats reports. */
   void  vPrinterInterrupt (void);
      /* Called by the shell software to indicate that the 
         printer has printed a line.  On the target hardware, 
         this will become part of the printer interrupt 
         routine. */
   void vPrintAll(void);
      /* Called when the user requests to print the report that 
         shows the levels in all that tanks. */
   void vPrintTankHistory(int iTank);
      /* Called when the user requests to print the history 
         of levels in one tank. */

/* Hardware-dependent functions (currently in dbgmain.c) */
   void vHardwareInit (void);
      /* Initializes various things in the shell software. */
   void vHardwareDisplayLine (char *a_chDisp);
      /* Displays a string of characters 
         on the (simulated) display. */
   WORD wHardwareButtonFetch (void);
      /* Returns the identity of the (simulated) button 
         that the user/tester has pressed. */
   void vHardwareFloatSetup (int iTankNumber);
      /* Tells the (simulated) floats to look for the level 
         in one of the tanks. */
   int iHardwareFloatGetData (void);
      /* Returns the value that is read 
         by the (simulated) floats. */
   void vHardwareBellOn  (void);
      /* Turns on the (simulated) bell. */
   void vHardwareBellOff  (void);
      /* Turns off the (a simulated) bell. */
   void vHardwarePrinterOutputLine (char *a_chPrint);
      /* Prints a string of characters 
         on the (simulated) printer. */

/* Public functions in timer.c */
   void vTimerInit (void);
      /* Initializes the timer software. */
   void vTimerOneThirdSecond (void);
      /* Called by the shell software to indicate that 1/3 
         of a second has elapsed.  This will become part of the 
         timer interrupt routine on the target system. */
   void vTimeGet  (int *a_iTime);
      /* Returns the current time (since the system 
         started operating). */

/* Public functions in data.c */
   void vTankDataInit (void);
      /* Initializes the software that keeps track of the history 
         of the levels in the tanks. */
   void vTankDataAdd (int iTank, int iLevel);
      /* Adds a new item to the database. */
   int iTankDataGet  (int iTank, int *a_iLevels, 
         int *a_iTimes, int iLimit);
      /* Retrieves one or more items from the database. */

/* Public functions in floats.c */
   void vFloatInit(void);
      /* Initializes the float-reading software. */
   void vReadFloats (int iTankNumber, V_FLOAT_CALLBACK vCb);
      /* Sets up the hardware (with a call to the 
         hardware-dependent software or to the shell software) 
         to read a level from the floats. */
   void  vFloatInterrupt (void);
      /* Called by the shell software to indicate that the 
         floats have been read.  This will become part of the 
         float interrupt routine on the target system. */

/* Public functions in overflow.c */
   void vOverflowSystemInit(void);
      /* Initializes the overflow-detection software. */
   void vOverflowTime(void);
      /* Called by the timer every 1/3 of the second. */
   void vOverflowAddTank(int iTank);
      /* Called by the level-tracking software to indicate that 
         the overflow-detection software should track 
         this tank. */


#endif


