/****************************************************************

                          M A I N . C


This module is the main routine for the Tank Monitor.


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


/*****   vEmbeddedMain   ****************************************

This is the main routine for the embedded system.

RETURNS: None.

*/

void vEmbeddedMain (

/*
INPUTS:  */
   void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

   OSInit();

   vTankDataInit ();
   vTimerInit ();

   vDisplaySystemInit ();

   vFloatInit ();

   vButtonSystemInit ();

   vLevelsSystemInit ();

   vPrinterSystemInit ();

   vHardwareInit ();

   vOverflowSystemInit();


   OSStart();
   
}  

