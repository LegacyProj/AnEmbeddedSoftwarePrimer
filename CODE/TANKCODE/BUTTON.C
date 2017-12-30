/****************************************************************

                            B U T T O N . C


This module deals with the buttons.


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

/* Local Structures */
    /* The state of the command state machine. */
        enum CMD_STATE 
        {
            CMD_NONE,
            CMD_PRINT,
            CMD_PRINT_HIST
        };

/* Static Data */
    /* Message queue and stack for the button task. */
        #define Q_SIZE 10
        static OS_EVENT *QButtonTask;
        static void *a_pvQData[Q_SIZE];

        #define STK_SIZE 1024
        static UWORD ButtonTaskStk[STK_SIZE];

/* Static Functions */
   static void far vButtonTask(void *p_vData);


/*****   vButtonTaskInit   **************************************

This routine is the task that initializes the button task.

RETURNS: None.

*/

void vButtonSystemInit(

/*
INPUTS:  */
    void)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

    /* Initialize the queue for this task. */
    QButtonTask = OSQCreate (&a_pvQData[0], Q_SIZE);

    /* Start the task. */
    OSTaskCreate (vButtonTask,  NULLP, 
        (void *)&ButtonTaskStk[STK_SIZE], TASK_PRIORITY_BUTTON);


}


/*****   vButtonTask   ******************************************

This routine is the task that handles the button state machine.

RETURNS: None.

*/

static void far vButtonTask(

/*
INPUTS:  */
    void *p_vData)      /* Unused pointer to data */
{

/*
LOCAL VARIABLES:  */
    BYTE byErr;         /* Error code back from the OS */
    WORD wMsg;          /* Message received from the queue */
    enum CMD_STATE iCmdState;
                        /* State of command state machine. */

/*-------------------------------------------------------------*/

    /* No more compiler warnings. */
    p_vData = p_vData;

    /* Initialize the command state. */
    iCmdState = CMD_NONE;

    while (TRUE)
    {
        /* Wait for a button press. */
        wMsg = (int) OSQPend (QButtonTask, WAIT_FOREVER, &byErr);

        switch (iCmdState)
        {
            case CMD_NONE:
                switch (wMsg)
                {
                    case '1':
                    case '2':
                    case '3':
                        vDisplayTankLevel (wMsg - '1');
                        break;

                    case 'T':
                        vDisplayTime ();
                        break;

                    case 'R':
                        vHardwareBellOff ();
                        vDisplayResetAlarm ();
                        break;

                    case 'P':
                        iCmdState = CMD_PRINT;
                        vDisplayPrompt (0);
                        break;

                }
                break;

            case CMD_PRINT:
                switch (wMsg)
                {
                    case 'R':
                        iCmdState = CMD_NONE;
                        vHardwareBellOff ();
                        vDisplayResetAlarm ();
                        break;

                    case 'A':
                        vPrintAll ();
                        iCmdState = CMD_NONE;
                        vDisplayNoPrompt ();
                        break;

                    case 'H':
                        iCmdState = CMD_PRINT_HIST;
                        vDisplayPrompt (1);
                        break;
                }
                break;

            case CMD_PRINT_HIST:
                switch (wMsg)
                {
                    case 'R':
                        iCmdState = CMD_NONE;
                        vHardwareBellOff ();
                        vDisplayResetAlarm ();
                        break;

                    case '1':
                    case '2':
                    case '3':
                        vPrintTankHistory (wMsg - '1');
                        iCmdState = CMD_NONE;
                        vDisplayNoPrompt ();
                        break;

                }

                break;
        }

    }
}


/*****   vButtonInterrupt   *************************************

This is the button interrupt routine.

RETURNS: None.

*/

void vButtonInterrupt (

/*
INPUTS:  */
    void)
{

/*
LOCAL VARIABLES:  */
    WORD wButton;           /* The button the user pressed. */

/*-------------------------------------------------------------*/

    /* Go to the hardware and see what button was pushed. */
    wButton = wHardwareButtonFetch ();

    /* Put that button on the queue for the task. */
    OSQPost (QButtonTask, (void *) wButton);
}


static char *p_chPromptStrings [] =
{
    "Press: HST or ALL",
    "Press Tank Number"
};  


/*****   p_chGetCommandPrompt   *********************************

This returns a prompt for the display routines to use.

RETURNS: Pointer to the prompt.

*/

char * p_chGetCommandPrompt (

/*
INPUTS:  */
    int iPrompt)
{

/*
LOCAL VARIABLES:  */

/*-------------------------------------------------------------*/

    /* Check that parameter is in range. */
    ASSERT (iPrompt >= 0 AND iPrompt < 
        sizeof (p_chPromptStrings) / sizeof (char *));

    return (p_chPromptStrings[iPrompt]);
    
}