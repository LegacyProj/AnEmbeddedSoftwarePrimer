/****************************************************************

                          F L O A T S . C


This module deals with the float hardware.


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
*****************************************************************/

/* System Include Files */
#include "os_cfg.h"
#include "ix86s.h"
#include "ucos.h"
#include "probstyl.h"

/* Program Include Files */
#include "publics.h"

/* Local Defines */
#define WAIT_FOREVER    0

/* Static Data */
static V_FLOAT_CALLBACK vFloatCallback = NULLP;
static OS_EVENT *semFloat;


/*****   vFloatInit   *****************************************

This routine is the task that initializes the float routines.

RETURNS: None.

*/

void vFloatInit(

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   /* Initialize the semaphore that protects the data. */
   semFloat = OSSemCreate (1);
}


/*****   vFloatInterrupt   **************************************

This routine is the one that is called when the floats interrupt 
with a new tank level reading.

RETURNS: None.

*/

void  vFloatInterrupt (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */
   int iFloatLevel;
   V_FLOAT_CALLBACK vFloatCallbackTemp;

/*-------------------------------------------------------------*/

   /* Get the float level. */
   iFloatLevel = iHardwareFloatGetData ();

   /* Remember the callback function to call later. */
   vFloatCallbackTemp = vFloatCallback;
   vFloatCallback = NULLP;

   /* We are no longer using the floats.  
      Release the semaphore. */
   OSSemPost (semFloat);

   /* Call back the callback routine. */
   vFloatCallbackTemp (iFloatLevel);
}  

/*****   vReadFloats   ******************************************

This routine is the task that initializes the float routines.

RETURNS: None.

*/

void vReadFloats (

/*
INPUTS:  */
   int iTankNumber,        /* The number of the tank to read. */
   V_FLOAT_CALLBACK vCb)   /* The function to call 
                              with the result. */
{

/*
LOCAL VARIABLES:  */
   BYTE byErr;       /* Place for OS to return an error. */

/*-------------------------------------------------------------*/

   /* Check that the parameter is valid. */
   ASSERT (iTankNumber >= 0 AND iTankNumber < COUNTOF_TANKS);

   OSSemPend (semFloat, WAIT_FOREVER, &byErr);

   /* Set up the callback function */
   vFloatCallback = vCb;

   /* Get the hardware started reading the value. */
   vHardwareFloatSetup (iTankNumber);
}  
