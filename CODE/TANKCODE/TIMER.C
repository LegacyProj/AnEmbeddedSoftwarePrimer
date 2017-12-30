/****************************************************************

                           T I M E R . C


This module provides timing services.


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

/* Static Data */
   /* Data about the time. */
      static int iHours;
      static int iMinutes;
      static int iSeconds;
      static int iSecondTenths;

   /* The semaphore that protects the data. */
      static OS_EVENT *SemTime;


/*****   vTimerInit   *******************************************

This routine initializes the timer data.

RETURNS: None.

*/

void vTimerInit (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Initialize the time. */
   iHours = 0;
   iMinutes = 0;
   iSeconds = 0;
   iSecondTenths = 0;

   /* Initialize the semaphore that protects the data. */
   SemTime = OSSemCreate (1);
}


/*****   vTimerOneThirdSecond   *********************************

This routine increments the timer stuff.

RETURNS: None.

*/

void vTimerOneThirdSecond (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   /* Get the time semaphore. */
   OSSemPend (SemTime, WAIT_FOREVER, &byErr);

   /* Wake up the overflow task */
   vOverflowTime();
   
   /* Update the time of day. */
   switch (iSecondTenths)
   {
      case 0:
         iSecondTenths = 3;
         break;

      case 3:
         iSecondTenths = 7;
         break;

      case 7:
         iSecondTenths = 0;
         ++iSeconds;
         if (iSeconds IS 60)
         {
            iSeconds = 0;
            ++iMinutes;
         }
         if (iMinutes IS 60)
         {
            iMinutes = 0;
            ++iHours;
         }
         if (iHours IS 24)
            iHours = 0;

         /* Let the display know. */
         vDisplayUpdate ();
         break;
   }

   /* Give back the semaphore. */
   OSSemPost (SemTime);

}


/*****   vTimeGet   *********************************************

This routine gets the time.

RETURNS: None.

*/

void vTimeGet  (

/*
INPUTS:  */
   int *a_iTime)     /* An four-space array in which 
                     to return the time. */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   /* Get the semaphore. */
   OSSemPend (SemTime, WAIT_FOREVER, &byErr);

   a_iTime[0] = iHours;
   a_iTime[1] = iMinutes;
   a_iTime[2] = iSeconds;
   a_iTime[3] = iSecondTenths;
   
   /* Give back the semaphore. */
   OSSemPost (SemTime);

   return;
   
}
