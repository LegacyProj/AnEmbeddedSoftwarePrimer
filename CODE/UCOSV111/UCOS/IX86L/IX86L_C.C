/*
*********************************************************************************************************
*                                                uC/OS
*                                         The Real-Time Kernel
*
*                      (c) Copyright 1992, 1993, Jean J. Labrosse, Plantation, FL
*                                          All Rights Reserved
*
*
*                                       80x86/80x88 Specific code
*                                           LARGE MEMORY MODEL
*
* File : Ix86L_C.C
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

#include "INCLUDES.H"

/*
*********************************************************************************************************
*                                        INITIALIZE A TASK'S STACK
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           CREATE A TASK
*********************************************************************************************************
*/
UBYTE OSTaskCreate(void (far *task)(void *pd), void *pdata, void *pstk, UBYTE p)
{
    UWORD OS_FAR *stk;
    UBYTE  err;


    OS_ENTER_CRITICAL();
    if (OSTCBPrioTbl[p] == (OS_TCB *)0) {   /* Make sure task doesn't already exist at this priority   */
        OSTCBPrioTbl[p] = (OS_TCB *)1;      /* Reserve the priority to prevent others from doing ...   */
                                            /* ... the same thing until task is created.               */
        OS_EXIT_CRITICAL();
        stk    = (UWORD OS_FAR *)pstk;      /* Load stack pointer                                      */
        *--stk = (UWORD)FP_SEG(pdata);      /* Simulate call to function with argument                 */
        *--stk = (UWORD)FP_OFF(pdata);
        *--stk = (UWORD)FP_SEG(task);
        *--stk = (UWORD)FP_OFF(task);
        *--stk = (UWORD)0x0202;             /* PSW = Interrupts enabled                                */
        *--stk = (UWORD)FP_SEG(task);       /* Put pointer to task   on top of stack                   */
        *--stk = (UWORD)FP_OFF(task);
        *--stk = (UWORD)0x0000;             /* AX = 0                                                  */
        *--stk = (UWORD)0x0000;             /* CX = 0                                                  */
        *--stk = (UWORD)0x0000;             /* DX = 0                                                  */
        *--stk = (UWORD)0x0000;             /* BX = 0                                                  */
        *--stk = (UWORD)0x0000;             /* SP = 0                                                  */
        *--stk = (UWORD)0x0000;             /* BP = 0                                                  */
        *--stk = (UWORD)0x0000;             /* SI = 0                                                  */
        *--stk = (UWORD)0x0000;             /* DI = 0                                                  */
        *--stk = (UWORD)0x0000;             /* ES = 0                                                  */
        *--stk = _DS;                       /* Save current value of DS                                */
    
        err      = OSTCBInit(p, (void far *)stk);     /* Get and initialize a TCB                      */
        if (err == OS_NO_ERR) {
            if (OSRunning) {                /* Find highest priority task if multitasking has started  */
                OSSched();
            }
        } else {
            OSTCBPrioTbl[p] = (OS_TCB *)0;  /* Make this priority available to others                  */
        }
        return (err);
    } else {
        OS_EXIT_CRITICAL();
        return (OS_PRIO_EXIST);
    }
}
