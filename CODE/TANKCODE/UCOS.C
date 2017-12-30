/*
*********************************************************************************************************
*                                                 uC/OS
*                                          The Real-Time Kernel
*                                                 KERNEL
*
*                        (c) Copyright 1992-1996, Jean J. Labrosse, Plantation, FL
*                                           All Rights Reserved
*
*                                                  V1.11
*
* File : UCOS.C
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

#define  OS_GLOBALS
#include "INCLUDES.H"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define  OS_LO_PRIO          63        /* IDLE task priority                                           */

                                       /* TASK STATUS                                                  */
#define  OS_STAT_RDY       0x00        /* Ready to run                                                 */
#define  OS_STAT_SEM       0x01        /* Pending on semaphore                                         */
#define  OS_STAT_MBOX      0x02        /* Pending on mailbox                                           */
#define  OS_STAT_Q         0x04        /* Pending on queue                                             */
#define  OS_STAT_SUSPEND   0x08        /* Task is suspended                                            */
/*$PAGE*/
/*
*********************************************************************************************************
*                              MAPPING TABLE TO MAP BIT POSITION TO BIT MASK
*
* Note: Index into table is desired bit position, 0..7
*       Indexed value corresponds to bit mask
*********************************************************************************************************
*/

UBYTE const OSMapTbl[]   = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*
*********************************************************************************************************
*                                       PRIORITY RESOLUTION TABLE
*
* Note: Index into table is bit pattern to resolve highest priority
*       Indexed value corresponds to highest priority bit position (i.e. 0..7)
*********************************************************************************************************
*/

UBYTE const OSUnMapTbl[] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};
/*$PAGE*/
/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static  OS_TCB     *OSTCBList;                 /* Pointer to doubly linked list of TCBs                */
static  UBYTE       OSRdyGrp;                  /* Ready list group                                     */
static  UBYTE       OSRdyTbl[8];               /* Table of tasks which are ready to run                */
static  UBYTE       OSLockNesting;             /* Multitasking lock nesting level                      */
static  OS_TCB     *OSTCBFreeList;             /* Pointer to list of free TCBs                         */
static  OS_EVENT   *OSEventFreeList;           /* Pointer to list of free EVENT control blocks         */

static  ULONG       OSTime;                    /* Current value of system time (in ticks)              */
static  UBYTE       OSIntExitY;                /* Variable used by 'OSIntExit' to prevent using locals */
static  OS_STK_TYPE OSTaskIdleStk[OS_TASK_IDLE_STK_SIZE];  /* Idle task stack                          */
static  OS_TCB      OSTCBTbl[OS_MAX_TASKS+1];              /* Table of TCBs                            */

#if     OS_MAX_EVENTS > 0
static  OS_EVENT    OSEventTbl[OS_MAX_EVENTS];             /* Table of EVENT control blocks            */
#endif

#if     OS_Q_EN && (OS_MAX_QS > 0)
static  OS_Q       *OSQFreeList;               /* Pointer to list of free QUEUE control blocks         */
static  OS_Q        OSQTbl[OS_MAX_QS];         /* Table of QUEUE control blocks                        */
#endif


/*
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void OS_FAR OSTaskIdle(void *data);
static  void        OSDummy(void);
/*$PAGE*/
/*
*********************************************************************************************************
*                                         uC/OS INITIALIZATION
*********************************************************************************************************
*/

void OSInit(void)
{
    UBYTE i;


    OSTime        = 0L;
    OSTCBHighRdy  = (OS_TCB *)0;
    OSTCBCur      = (OS_TCB *)0;
    OSTCBList     = (OS_TCB *)0;
    OSIntNesting  = 0;
    OSLockNesting = 0;
    OSRunning     = FALSE;                                 /* Indicate that multitasking not started   */
    OSIdleCtr     = 0L;
    OSCtxSwCtr    = 0;
    OSRdyGrp      = 0;                                     /* Clear the ready list                     */
    for (i = 0; i < 8; i++) {
        OSRdyTbl[i] = 0;
    }
    for (i = 0; i < 64; i++) {                             /* Clear the priority table                 */
        OSTCBPrioTbl[i] = (OS_TCB *)0;
    }
    for (i = 0; i < OS_MAX_TASKS; i++) {                   /* Init. list of free TCBs                  */
        OSTCBTbl[i].OSTCBNext = &OSTCBTbl[i+1];
    }
    OSTCBTbl[OS_MAX_TASKS].OSTCBNext = (OS_TCB *)0;        /* Last OS_TCB is for OSTaskIdle()          */
    OSTCBFreeList                    = &OSTCBTbl[0];
#if OS_MAX_EVENTS > 0
    for (i = 0; i < (OS_MAX_EVENTS - 1); i++) {            /* Init. list of free EVENT control blocks  */
        OSEventTbl[i].OSEventPtr = &OSEventTbl[i+1];
    }
    OSEventTbl[OS_MAX_EVENTS - 1].OSEventPtr = (OS_EVENT *)0;
    OSEventFreeList                          = &OSEventTbl[0];
#else
    OSEventFreeList                          = (OS_EVENT *)0;
#endif

#if OS_Q_EN && (OS_MAX_QS > 0)
    for (i = 0; i < (OS_MAX_QS - 1); i++) {                /* Init. list of free QUEUE control blocks  */
        OSQTbl[i].OSQPtr = &OSQTbl[i+1];
    }
    OSQTbl[OS_MAX_QS - 1].OSQPtr = (OS_Q *)0;
    OSQFreeList                  = &OSQTbl[0];
#endif

    OSTaskCreate(OSTaskIdle, (void *)0, (void *)&OSTaskIdleStk[OS_TASK_IDLE_STK_TOP], OS_LO_PRIO);
}

/*
*********************************************************************************************************
*                                              IDLE TASK
*********************************************************************************************************
*/

static void OS_FAR OSTaskIdle(void *data)
{
    data = data;
    while (1) {
        OS_ENTER_CRITICAL();
        OSIdleCtr++;
        OS_EXIT_CRITICAL();
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                          START MULTITASKING
*********************************************************************************************************
*/

void OSStart(void)
{
    UBYTE y;
    UBYTE x;
    UBYTE prio;


    y             = OSUnMapTbl[OSRdyGrp];        /* Find highest priority's task priority number       */
    x             = OSUnMapTbl[OSRdyTbl[y]];
    prio          = (y << 3) + x;
    OSTCBHighRdy  = OSTCBPrioTbl[prio];          /* Point to highest priority task ready to run        */
    OSTCBCur      = OSTCBHighRdy;
    OSRunning     = TRUE;
    OSStartHighRdy();                            /* Execute target specific code to start task         */
}


/*
*********************************************************************************************************
*                                            uC/OS SCHEDULER
*********************************************************************************************************
*/

void OSSched(void)
{
    UBYTE y;


    OS_ENTER_CRITICAL();
    if ((OSLockNesting | OSIntNesting) == 0) {   /* Task scheduling must be enabled and not ISR level  */
        y             = OSUnMapTbl[OSRdyGrp];    /* Get pointer to highest priority task ready to run  */
        OSTCBHighRdy  = OSTCBPrioTbl[(y << 3) + OSUnMapTbl[OSRdyTbl[y]]];
        if (OSTCBHighRdy != OSTCBCur) {          /* Make sure this is not the current task running     */
            OSCtxSwCtr++;                        /* Increment context switch counter                   */
            OS_TASK_SW();                        /* Perform a context switch                           */
        }
    }
    OS_EXIT_CRITICAL();
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                          PREVENT SCHEDULING
*********************************************************************************************************
*/

void OSSchedLock(void)
{
    if (OSRunning == TRUE) {                     /* Make sure multitasking is running                  */
        OS_ENTER_CRITICAL();
        OSLockNesting++;                         /* Increment lock nesting level                       */
        OS_EXIT_CRITICAL();
    }
}


/*
*********************************************************************************************************
*                                          ENABLE SCHEDULING
*********************************************************************************************************
*/

void OSSchedUnlock(void)
{
    if (OSRunning == TRUE) {                           /* Make sure multitasking is running            */
        OS_ENTER_CRITICAL();
        if (OSLockNesting != 0) {                      /* Do not decrement if already 0                */
            OSLockNesting--;                           /* Decrement lock nesting level                 */
            if ((OSLockNesting | OSIntNesting) == 0) { /* See if scheduling re-enabled and not an ISR  */
                OS_EXIT_CRITICAL();
                OSSched();                             /* See if a higher priority task is ready       */
            } else {
                OS_EXIT_CRITICAL();
            }
        } else {
            OS_EXIT_CRITICAL();
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                            INITIALIZE TCB
*********************************************************************************************************
*/

UBYTE OSTCBInit(UBYTE prio, void OS_FAR *stk)
{
    OS_TCB *ptcb;


    OS_ENTER_CRITICAL();
    ptcb = OSTCBFreeList;                                  /* Get a free TCB from the free TCB list    */
    if (ptcb != (OS_TCB *)0) {
        OSTCBFreeList           = ptcb->OSTCBNext;         /* Update pointer to free TCB list          */
        OS_EXIT_CRITICAL();
        ptcb->OSTCBStkPtr       = stk;                     /* Load Stack pointer in TCB                */
        ptcb->OSTCBPrio         = (UBYTE)prio;             /* Load task priority into TCB              */
        ptcb->OSTCBStat         = OS_STAT_RDY;             /* Task is ready to run                     */
        ptcb->OSTCBDly          = 0;
        ptcb->OSTCBDelReq       = OS_NO_ERR;
        ptcb->OSTCBY            = prio >> 3;
        ptcb->OSTCBBitY         = OSMapTbl[ptcb->OSTCBY];
        ptcb->OSTCBX            = prio & 0x07;
        ptcb->OSTCBBitX         = OSMapTbl[ptcb->OSTCBX];

#if     OS_SEM_EN || OS_MBOX_EN || (OS_Q_EN && (OS_MAX_QS > 0))
        ptcb->OSTCBEventPtr     = (OS_EVENT *)0;           /* Task is not pending on an event          */
#endif

#if     OS_MBOX_EN || (OS_Q_EN && (OS_MAX_QS > 0))
        ptcb->OSTCBMsg          = (void *)0;               /* No message received                      */
#endif

        OS_ENTER_CRITICAL();
        OSTCBPrioTbl[prio]      = ptcb;
        ptcb->OSTCBNext         = OSTCBList;               /* Link into TCB chain                      */
        ptcb->OSTCBPrev         = (OS_TCB *)0;
        if (OSTCBList != (OS_TCB *)0) {
            OSTCBList->OSTCBPrev = ptcb;
        }
        OSTCBList               = ptcb;
        OSRdyGrp               |= ptcb->OSTCBBitY;         /* Make task ready to run                   */
        OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
        OS_EXIT_CRITICAL();
        return (OS_NO_ERR);
    } else {
        OS_EXIT_CRITICAL();
        return (OS_NO_MORE_TCB);
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               ENTER ISR
*********************************************************************************************************
*/

void OSIntEnter(void)
{
    OS_ENTER_CRITICAL();
    OSIntNesting++;                              /* Increment ISR nesting level                        */
    OS_EXIT_CRITICAL();
}


/*
*********************************************************************************************************
*                                               EXIT ISR
*********************************************************************************************************
*/

void OSIntExit(void)
{
    OS_ENTER_CRITICAL();
    if ((--OSIntNesting | OSLockNesting) == 0) { /* Reschedule only if all ISRs completed & not locked */
        OSIntExitY   = OSUnMapTbl[OSRdyGrp];
        OSTCBHighRdy = OSTCBPrioTbl[(OSIntExitY << 3) + OSUnMapTbl[OSRdyTbl[OSIntExitY]]];
        if (OSTCBHighRdy != OSTCBCur) {          /* No context switch if current task is highest ready */
            OSCtxSwCtr++;
            OSIntCtxSw();                        /* Perform interrupt level context switch             */
        }
    }
    OS_EXIT_CRITICAL();
}
/*$PAGE*/
#if OS_TASK_CHANGE_PRIO_EN
/*
*********************************************************************************************************
*                                       CHANGE PRIORITY OF A TASK
*********************************************************************************************************
*/

UBYTE OSTaskChangePrio(UBYTE oldprio, UBYTE newprio)
{
    OS_TCB   *ptcb;
    OS_EVENT *pevent;
    UBYTE     x;
    UBYTE     y;
    UBYTE     bitx;
    UBYTE     bity;



    if (oldprio >= OS_MAX_TASKS || newprio >= OS_MAX_TASKS) {
        return (OS_PRIO_INVALID);
    }
    OS_ENTER_CRITICAL();
    if (OSTCBPrioTbl[newprio] != (OS_TCB *)0) {                 /* New priority must not already exist */
        OS_EXIT_CRITICAL();
        return (OS_PRIO_EXIST);
    } else {
        OS_EXIT_CRITICAL();
        y    = newprio >> 3;                                    /* Precompute to reduce INT. latency   */
        bity = OSMapTbl[y];
        x    = newprio & 0x07;
        bitx = OSMapTbl[x];
        OS_ENTER_CRITICAL();
        if ((ptcb = OSTCBPrioTbl[oldprio]) != (OS_TCB *)0) {    /* Task to change must exist           */
            OSTCBPrioTbl[oldprio] = (OS_TCB *)0;                /* Remove TCB from old priority        */
            if (OSRdyTbl[ptcb->OSTCBY] & ptcb->OSTCBBitX) {     /* If task is ready make it not ready  */
                if ((OSRdyTbl[ptcb->OSTCBY] &= ~ptcb->OSTCBBitX) == 0) {
                    OSRdyGrp &= ~ptcb->OSTCBBitY;
                }
                OSRdyGrp    |= bity;                            /* Make new priority ready to run      */
                OSRdyTbl[y] |= bitx;
            } else {
                if ((pevent = ptcb->OSTCBEventPtr) != (OS_EVENT *)0) { /* Remove from event wait list  */
                    if ((pevent->OSEventTbl[ptcb->OSTCBY] &= ~ptcb->OSTCBBitX) == 0) {
                        pevent->OSEventGrp &= ~ptcb->OSTCBBitY;
                    }
                    pevent->OSEventGrp    |= bity;
                    pevent->OSEventTbl[y] |= bitx;
                }
            }
            OSTCBPrioTbl[newprio] = ptcb;                       /* Place pointer to TCB @ new priority */
            ptcb->OSTCBPrio       = newprio;                    /* Set new task priority               */
            ptcb->OSTCBY          = y;
            ptcb->OSTCBX          = x;
            ptcb->OSTCBBitY       = bity;
            ptcb->OSTCBBitX       = bitx;
            OS_EXIT_CRITICAL();
            OSSched();                                          /* Run highest priority task ready     */
            return (OS_NO_ERR);
        } else {
            OS_EXIT_CRITICAL();
            return (OS_PRIO_ERR);                               /* Task to change didn't exist         */
        }
    }
}
#endif
/*$PAGE*/
#if OS_TASK_DEL_EN
/*
*********************************************************************************************************
*                                            DELETE A TASK
*
* Notes:
*    1) To reduce interrupt latency, OSTaskDel() 'disables' the task:
*           a) by making it not ready
*           b) by removing it from any wait lists
*           c) by preventing OSTimeTick() from making the task ready to run.
*       The task can then be 'unlinked' from the miscellaneous structures in uC/OS.
*    2) The function OSDummy() is called after OS_EXIT_CRITICAL() because, on most processors, the next
*       instruction following the enable interrupt instruction is ignored.  You can replace OSDummy()
*       with a macro that basically executes a NO OP (i.e. OS_NOP()).  The NO OP macro would avoid the
*       execution time of the function call and return.
*********************************************************************************************************
*/
/*$PAGE*/
UBYTE OSTaskDel(UBYTE prio)
{
    OS_TCB   *ptcb;
    OS_EVENT *pevent;
    UBYTE     priocur;


    if (prio == OS_LO_PRIO) {                                   /* Not allowed to delete idle task     */
        return (OS_TASK_DEL_IDLE);
    }
    if (prio >= OS_MAX_TASKS && prio != OS_PRIO_SELF) {         /* Make sure task priority is valid    */
        return (OS_PRIO_INVALID);
    }
    OS_ENTER_CRITICAL();
    priocur = OSTCBCur->OSTCBPrio;                              /* Obtain the current priority         */
    if (prio == OS_PRIO_SELF) {                                 /* See if requesting to delete self    */
        prio = priocur;                                         /* Set priority to delete to current   */
    }
    if ((ptcb = OSTCBPrioTbl[prio]) != (OS_TCB *)0) {           /* Task to delete must exist           */
        OSTCBPrioTbl[prio] = (OS_TCB *)0;                       /* Clear old priority entry            */
        if ((OSRdyTbl[ptcb->OSTCBY] &= ~ptcb->OSTCBBitX) == 0) {/* Make task not ready                 */
            OSRdyGrp &= ~ptcb->OSTCBBitY;
        }
        if ((pevent = ptcb->OSTCBEventPtr) != (OS_EVENT *)0) {  /* If task is waiting on event         */
            if ((pevent->OSEventTbl[ptcb->OSTCBY] &= ~ptcb->OSTCBBitX) == 0) { /* ... remove task from */
                pevent->OSEventGrp &= ~ptcb->OSTCBBitY;                        /* ... event ctrl block */
            }
        }
        ptcb->OSTCBDly = 0;                                     /* Prevent OSTimeTick() from updating  */
        OS_EXIT_CRITICAL();                                     /* Enabling INT. ignores next instruc. */
        OSDummy();                                              /* ... Dummy ensures that INTs will be */
        OS_ENTER_CRITICAL();                                    /* ... disabled HERE!                  */
        if (ptcb->OSTCBPrev == (OS_TCB *)0) {                   /* Remove from TCB chain               */
            ptcb->OSTCBNext->OSTCBPrev = (OS_TCB *)0;
            OSTCBList                  = ptcb->OSTCBNext;
        } else {
            ptcb->OSTCBPrev->OSTCBNext = ptcb->OSTCBNext;
            ptcb->OSTCBNext->OSTCBPrev = ptcb->OSTCBPrev;
        }
        ptcb->OSTCBNext = OSTCBFreeList;                        /* Return TCB to free TCB list         */
        OSTCBFreeList   = ptcb;
        if (prio == priocur) {                                  /* Resched. only if deleting self!     */
            OS_EXIT_CRITICAL();
            OSSched();                                          /* Find new highest priority task      */
        } else {
            OS_EXIT_CRITICAL();
        }
        return (OS_NO_ERR);
    } else {
        OS_EXIT_CRITICAL();
        return (OS_TASK_DEL_ERR);
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                   REQUEST THAT A TASK DELETE ITSELF
*
* Description : This function is used to:
*                   a) notify a task to delete itself.
*                   b) to see if a task requested that the current task delete itself.
*               This function is a little tricky to understand.  Basically, you have a task that needs
*               to be deleted however, this task has resources that it has allocated (memory buffers,
*               semaphores, mailboxes, queues etc.).  The task cannot be deleted otherwise these
*               resources would not be freed.  The requesting task calls OSTaskDelReq() to indicate that
*               the task needs to be deleted.  Deleting of the task is however, deferred to the task to
*               be deleted.  For example, suppose that task #10 needs to be deleted.  The requesting task
*               example, task #5, would call OSTaskDelReq(10).  When task #10 gets to execute, it calls
*               this function by specifying OS_PRIO_SELF and monitors the returned value.  If the return
*               value is OS_TASK_DEL_REQ, another task requested a task delete.  Task #10 would look like
*               this:
*
*                   void OS_FAR Task(void *data)
*                   {
*                       .
*                       .
*                       while (1) {
*                           OSTimeDly(1);
*                           if (OSTaskDelReq(OS_PRIO_SELF) == OS_TASK_DEL_REQ) {
*                               Release any owned resources;
*                               De-allocate any dynamic memory;
*                               OSTaskDel(OS_PRIO_SELF);
*                           }
*                       }
*                   }
*
* Arguments : 'prio'   is the priority of the task to request the delete from
*
* Returns   : if 'prio' is OS_PRIO_SELF:
*                 returns OS_TASK_DEL_REQ if a  task requested to delete SELF
*                 returns OS_NO_ERR       if no task requested to delete SELF
*             if 'prio' is other than OS_PRIO_SELF:
*                 request that the task be deleted
*********************************************************************************************************
*/
/*$PAGE*/
UBYTE OSTaskDelReq(UBYTE prio)
{
    BOOLEAN  stat;
	UBYTE    err;
    OS_TCB  *ptcb;


    if (prio == OS_LO_PRIO) {                                   /* Not allowed to delete idle task     */
        return (OS_TASK_DEL_IDLE);
    }
    if (prio >= OS_MAX_TASKS && prio != OS_PRIO_SELF) {         /* Make sure task priority is valid    */
        return (OS_PRIO_INVALID);
    }
    if (prio == OS_PRIO_SELF) {                                 /* See if task to delete is requesting */
        OS_ENTER_CRITICAL();
        stat = OSTCBCur->OSTCBDelReq;                           /* Return request status to caller     */
        OS_EXIT_CRITICAL();
        return (stat);
    } else {
        OS_ENTER_CRITICAL();
        if ((ptcb = OSTCBPrioTbl[prio]) != (OS_TCB *)0) {       /* Task to delete must exist           */
            ptcb->OSTCBDelReq = OS_TASK_DEL_REQ;                /* Set flag indicating task to be DEL. */
        }
		err = OSTCBPrioTbl[prio] == (OS_TCB *)0 ? OS_TASK_NOT_EXIST : OS_NO_ERR;
        OS_EXIT_CRITICAL();
        return (err);
    }
}
#endif
/*$PAGE*/
#if OS_TASK_SUSPEND_EN
/*
*********************************************************************************************************
*                                            SUSPEND A TASK
*
* Description : This function is called to suspend a task.  The task can be the calling task if if the
*               priority passed to OSTaskSuspend() is the priority of the calling task.
* Arguments   : 'prio'     is the priority of the task to suspend.  If you specify OS_PRIO_SELF, the
*                          calling task will suspend itself and rescheduling will occur.
*
* Returns     : OS_NO_ERR                if the requested task is suspended
*               OS_TASK_SUSPEND_PRIO     if the task to suspend does not exist
* Note        : You should use this function with great care.  If you suspend a task that is waiting for
*               a message you will prevent this task from running when the message arrives.
*********************************************************************************************************
*/

UBYTE OSTaskSuspend(UBYTE prio)
{
    BOOLEAN   self;
    OS_TCB   *ptcb;


    if (prio == OS_LO_PRIO) {                                   /* Not allowed to suspend idle task    */
        return (OS_TASK_SUSPEND_IDLE);
    }
    if (prio >= OS_MAX_TASKS && prio != OS_PRIO_SELF) {         /* Make sure task priority is valid    */
        return (OS_PRIO_INVALID);
    }
    OS_ENTER_CRITICAL();
    if (prio == OS_PRIO_SELF) {                                 /* See if suspend SELF                 */
        prio = OSTCBCur->OSTCBPrio;
        self = TRUE;
    } else {
        self = FALSE;
    }
    if ((ptcb = OSTCBPrioTbl[prio]) == (OS_TCB *)0) {                /* Task to suspend must exist     */
        OS_EXIT_CRITICAL();
        return (OS_TASK_SUSPEND_PRIO);
    } else {
        if ((OSRdyTbl[ptcb->OSTCBY] &= ~ptcb->OSTCBBitX) == 0) {     /* Make task not ready            */
            OSRdyGrp &= ~ptcb->OSTCBBitY;
        }
        ptcb->OSTCBStat |= OS_STAT_SUSPEND;                          /* Status of task is 'SUSPENDED'  */
        OS_EXIT_CRITICAL();
        if (self == TRUE) {                                          /* Context switch only if SELF    */
            OSSched();
        }
        return (OS_NO_ERR);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                      RESUME A SUSPENDED TASK
*
* Description : This function is called to resume a previously suspended task.
* Arguments   : 'prio'     is the priority of the task to resume.
*
* Returns     : OS_NO_ERR                if the requested task is resumed
*               OS_TASK_RESUME_PRIO      if the task to resume does not exist
*               OS_TASK_NOT_SUSPENDED    if the task to resume has not been suspended
*********************************************************************************************************
*/

UBYTE OSTaskResume(UBYTE prio)
{
    OS_TCB   *ptcb;


    OS_ENTER_CRITICAL();
    if ((ptcb = OSTCBPrioTbl[prio]) == (OS_TCB *)0) {                /* Task to suspend must exist     */
        OS_EXIT_CRITICAL();
        return (OS_TASK_RESUME_PRIO);
    } else {
        if (ptcb->OSTCBStat & OS_STAT_SUSPEND) {                          /* Task must be suspended    */
            if ((ptcb->OSTCBStat &= ~OS_STAT_SUSPEND) == OS_STAT_RDY) {   /* Remove suspension         */
                OSRdyGrp               |= ptcb->OSTCBBitY;                /* Make task ready to run    */
                OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                OS_EXIT_CRITICAL();
                OSSched();
            } else {
                OS_EXIT_CRITICAL();
            }
            return (OS_NO_ERR);
        } else {
            OS_EXIT_CRITICAL();
            return (OS_TASK_NOT_SUSPENDED);
        }
    }
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                             DELAY TASK 'n' TICKS   (n from 1 to 65535)
*
* Description : This function is called to delay execution of the currently running task until some time
*               expires.  If the specified delay is greater than 0 then a context switch will result.
* Arguments   : 'ticks'     is the time delay that the task will be suspended in 'ticks'.  Note that by
*                           specifying 0, the task will not be delayed.
*********************************************************************************************************
*/

void OSTimeDly(UWORD ticks)
{
    if (ticks > 0) {                                                      /* 0 means no delay!         */
        OS_ENTER_CRITICAL();
        if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {  /* Delay current task        */
            OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
        }
        OSTCBCur->OSTCBDly = ticks;                                       /* Load ticks in TCB         */
        OS_EXIT_CRITICAL();
        OSSched();                                                        /* Find next task to run!    */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                         RESUME A DELAYED TASK
*********************************************************************************************************
*/

UBYTE OSTimeDlyResume(UBYTE prio)
{
    OS_TCB *ptcb;


    OS_ENTER_CRITICAL();
    ptcb = (OS_TCB *)OSTCBPrioTbl[prio];                   /* Make sure that task exist                */
    if (ptcb != (OS_TCB *)0) {
        if (ptcb->OSTCBDly != 0) {                         /* See if task is delayed                   */
            ptcb->OSTCBDly  = 0;                           /* Clear the time delay                     */
            if (!(ptcb->OSTCBStat & OS_STAT_SUSPEND)) {    /* See if task is ready to run              */
                OSRdyGrp               |= ptcb->OSTCBBitY; /* Make task ready to run                   */
                OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                OS_EXIT_CRITICAL();
                OSSched();                                 /* See if this is new highest priority      */
            } else {
                OS_EXIT_CRITICAL();                        /* Task may be suspended                    */
            }
            return (OS_NO_ERR);
        } else {
            OS_EXIT_CRITICAL();
            return (OS_TIME_NOT_DLY);                      /* Indicate that task was not delayed       */
        }
    } else {
        OS_EXIT_CRITICAL();
        return (OS_TASK_NOT_EXIST);                        /* The task does not exist                  */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                         PROCESS SYSTEM TICK
*********************************************************************************************************
*/

void OSTimeTick(void)
{
    OS_TCB *ptcb;


    ptcb = OSTCBList;                                      /* Point at first TCB in TCB list           */
    while (ptcb->OSTCBPrio != OS_LO_PRIO) {                /* Go through all TCBs in TCB list          */
        OS_ENTER_CRITICAL();
        if (ptcb->OSTCBDly != 0) {                         /* Delayed or waiting for event with TO     */
            if (--ptcb->OSTCBDly == 0) {                   /* Decrement nbr of ticks to end of delay   */
                if (!(ptcb->OSTCBStat & OS_STAT_SUSPEND)) {    /* Is task suspended?                   */
                    OSRdyGrp               |= ptcb->OSTCBBitY; /* No,  Make task Rdy to Run (timed out)*/
                    OSRdyTbl[ptcb->OSTCBY] |= ptcb->OSTCBBitX;
                } else {                                       /* Yes, Leave 1 tick to prevent ...     */
                    ptcb->OSTCBDly = 1;                        /* ... loosing the task when the ...    */
                }                                              /* ... suspension is removed.           */
            }
        }
        ptcb = ptcb->OSTCBNext;                            /* Point at next TCB in TCB list            */
        OS_EXIT_CRITICAL();
    }
    OS_ENTER_CRITICAL();                                   /* Update the 32-bit tick counter           */
    OSTime++;
    OS_EXIT_CRITICAL();
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                           SET SYSTEM CLOCK
*********************************************************************************************************
*/

void OSTimeSet(ULONG ticks)
{
    OS_ENTER_CRITICAL();
    OSTime = ticks;
    OS_EXIT_CRITICAL();
}


/*
*********************************************************************************************************
*                                        GET CURRENT SYSTEM TIME
*********************************************************************************************************
*/

ULONG OSTimeGet(void)
{
    ULONG ticks;


    OS_ENTER_CRITICAL();
    ticks = OSTime;
    OS_EXIT_CRITICAL();
    return (ticks);
}
/*$PAGE*/
#if OS_SEM_EN
/*
*********************************************************************************************************
*                                         INITIALIZE SEMAPHORE
*********************************************************************************************************
*/

OS_EVENT *OSSemCreate(UWORD cnt)
{
    OS_EVENT *pevent;


    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                              /* Get next free event control block        */
    if (OSEventFreeList != (OS_EVENT *)0) {                /* See if pool of free ECB pool was empty   */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {                         /* Get an event control block               */
        pevent->OSEventCnt    = cnt;                       /* Set semaphore value                      */
        pevent->OSEventGrp    = 0x00;                      /* Initialize rest of event control block   */
        pevent->OSEventTbl[0] = 0x00;
        pevent->OSEventTbl[1] = 0x00;
        pevent->OSEventTbl[2] = 0x00;
        pevent->OSEventTbl[3] = 0x00;
        pevent->OSEventTbl[4] = 0x00;
        pevent->OSEventTbl[5] = 0x00;
        pevent->OSEventTbl[6] = 0x00;
        pevent->OSEventTbl[7] = 0x00;
        return (pevent);
    } else {
        return ((OS_EVENT *)0);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           ACCEPT SEMAPHORE
*
* Description: This function checks the semaphore to see if the resource is available or if an event
*              occurred.  Unlike OSSemPend(), OSSemAccept() does not suspend the calling task if the
*              resource is not available or the event did not occur.
* Arguments  : 'pevent'   is a pointer to the event control block
* Returns    : >  0       if the resource is available or the event did not occur
*                         the semaphore is decremented so the next time OSSemAccept() is called, the
*                         resource will no longer be available.
*              == 0       if the resource is not available or the event did not occur
*********************************************************************************************************
*/

UWORD OSSemAccept(OS_EVENT *pevent)
{
    UWORD cnt;


    OS_ENTER_CRITICAL();
    cnt = pevent->OSEventCnt;
    if (cnt > 0) {                               /* See if resource is available                       */
        pevent->OSEventCnt--;                    /* Yes, decrement semaphore and notify caller         */
    }
    OS_EXIT_CRITICAL();
    return (cnt);                                /* Return semaphore count                             */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           PEND ON SEMAPHORE
*********************************************************************************************************
*/

void OSSemPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err)
{
    OS_ENTER_CRITICAL();
    if (pevent->OSEventCnt > 0) {                     /* If sem. is positive, resource available ...   */
        pevent->OSEventCnt--;                         /* ... decrement semaphore only if positive.     */
        OS_EXIT_CRITICAL();
        *err = OS_NO_ERR;
    } else {                                          /* Otherwise, must wait until event occurs       */
        OSTCBCur->OSTCBStat    |= OS_STAT_SEM;        /* Resource not available, pend on semaphore     */
        OSTCBCur->OSTCBDly      = timeout;            /* Store pend timeout in TCB                     */
        OSTCBCur->OSTCBEventPtr = pevent;             /* Store pointer to ECB in TCB                   */
        if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {  /* Task no longer ready      */
            OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
        }
        pevent->OSEventTbl[OSTCBCur->OSTCBY] |= OSTCBCur->OSTCBBitX;      /* Put task in waiting list  */
        pevent->OSEventGrp                   |= OSTCBCur->OSTCBBitY;
        OS_EXIT_CRITICAL();
        OSSched();                                    /* Find next highest priority task ready         */
        OS_ENTER_CRITICAL();
        if (OSTCBCur->OSTCBStat & OS_STAT_SEM) {      /* Must have timed out if still waiting for event*/
            if ((pevent->OSEventTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {
                pevent->OSEventGrp &= ~OSTCBCur->OSTCBBitY;
            }
            OSTCBCur->OSTCBStat     = OS_STAT_RDY;    /* Set status to ready                           */
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;  /* Task is no longer waiting for the event       */
            OS_EXIT_CRITICAL();
            *err = OS_TIMEOUT;                        /* Indicate that didn't get event within TO      */
        } else {
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;
            OS_EXIT_CRITICAL();
            *err = OS_NO_ERR;
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                         POST TO A SEMAPHORE
*********************************************************************************************************
*/

UBYTE OSSemPost(OS_EVENT *pevent)
{
    OS_TCB *ptcb;
    UBYTE   x;
    UBYTE   y;
    UBYTE   bitx;
    UBYTE   bity;
    UBYTE   p;


    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp) {                         /* See if any task waiting for semaphore         */
        y    = OSUnMapTbl[pevent->OSEventGrp];        /* Find highest priority task pending on event   */
        bity = OSMapTbl[y];
        x    = OSUnMapTbl[pevent->OSEventTbl[y]];
        bitx = OSMapTbl[x];
        p    = (y << 3) + x;
        if ((pevent->OSEventTbl[y] &= ~bitx) == 0) {  /* Remove this task from the waiting list        */
            pevent->OSEventGrp &= ~bity;
        }
        ptcb                 =  OSTCBPrioTbl[p];      /* Point to this task's OS_TCB                   */
        ptcb->OSTCBDly       =  0;                    /* Prevent OSTimeTick() from readying this task  */
        ptcb->OSTCBEventPtr  = (OS_EVENT *)0;         /* Unlink event control block from this task     */
        ptcb->OSTCBStat     &= ~OS_STAT_SEM;          /* Clear bit associated with event type          */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {         /* See if task is ready (could be suspended)     */
            OSRdyGrp        |=  bity;                 /* Put task in the ready to run list             */
            OSRdyTbl[y]     |=  bitx;
        }
        OS_EXIT_CRITICAL();
        OSSched();                                    /* Find highest priority task ready to run       */
        return (OS_NO_ERR);
    } else {
        if (pevent->OSEventCnt < 65535) {             /* Make sure semaphore will not overflow         */
            pevent->OSEventCnt++;                     /* Increment semaphore count to register event   */
            OS_EXIT_CRITICAL();
            return (OS_NO_ERR);
        } else {                                      /* Semaphore value has reached its maximum       */
            OS_EXIT_CRITICAL();
            return (OS_SEM_OVF);
        }
    }
}
#endif
/*$PAGE*/
#if OS_MBOX_EN
/*
*********************************************************************************************************
*                                      INITIALIZE MESSAGE MAILBOX
*********************************************************************************************************
*/

OS_EVENT *OSMboxCreate(void *msg)
{
    OS_EVENT *pevent;


    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                    /* Get next free event control block                  */
    if (OSEventFreeList != (OS_EVENT *)0) {      /* See if pool of free ECB pool was empty             */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {
        pevent->OSEventPtr    = msg;             /* Deposit message in event control block             */
        pevent->OSEventGrp    = 0x00;            /* Initialize rest of event control block             */
        pevent->OSEventTbl[0] = 0x00;
        pevent->OSEventTbl[1] = 0x00;
        pevent->OSEventTbl[2] = 0x00;
        pevent->OSEventTbl[3] = 0x00;
        pevent->OSEventTbl[4] = 0x00;
        pevent->OSEventTbl[5] = 0x00;
        pevent->OSEventTbl[6] = 0x00;
        pevent->OSEventTbl[7] = 0x00;
    }
    return (pevent);                             /* Return pointer to event control block              */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                     ACCEPT MESSAGE FROM MAILBOX
*
* Description: This function checks the mailbox to see if a message is available.  Unlike OSMboxPend(),
*              OSMboxAccept() does not suspend the calling task if a message is not available.
* Arguments  : 'pevent'      is a pointer to the event control block
* Returns    : != (void *)0  is the message in the mailbox if one is available.  The mailbox is cleared
*                            so the next time OSMboxAccept() is called, the mailbox will be empty.
*              == (void *)0  if the mailbox is empty
*********************************************************************************************************
*/

void *OSMboxAccept(OS_EVENT *pevent)
{
    void  *msg;


    OS_ENTER_CRITICAL();
    if ((msg = pevent->OSEventPtr) != (void *)0) {    /* See if there is already a message             */
        pevent->OSEventPtr = (void *)0;               /* Clear the mailbox                             */
    }
    OS_EXIT_CRITICAL();
    return (msg);                                     /* Return the message received (or NULL)         */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                      PEND ON MAILBOX FOR A MESSAGE
*********************************************************************************************************
*/

void *OSMboxPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err)
{
    void  *msg;


    OS_ENTER_CRITICAL();
    if ((msg = pevent->OSEventPtr) != (void *)0) {    /* See if there is already a message             */
        pevent->OSEventPtr = (void *)0;               /* Clear the mailbox                             */
        OS_EXIT_CRITICAL();
        *err = OS_NO_ERR;
    } else {
        OSTCBCur->OSTCBStat    |= OS_STAT_MBOX;       /* Message not available, task will pend         */
        OSTCBCur->OSTCBDly      = timeout;            /* Load timeout in TCB                           */
        OSTCBCur->OSTCBEventPtr = pevent;             /* Store pointer to event control block in TCB   */
        if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {  /* Task no longer ready      */
            OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
        }
        pevent->OSEventTbl[OSTCBCur->OSTCBY] |= OSTCBCur->OSTCBBitX;      /* Put task in waiting list  */
        pevent->OSEventGrp                   |= OSTCBCur->OSTCBBitY;
        OS_EXIT_CRITICAL();
        OSSched();                                    /* Find next highest priority task ready to run  */
        OS_ENTER_CRITICAL();
        if ((msg = OSTCBCur->OSTCBMsg) != (void *)0) {     /* See if we were given the message         */
            OSTCBCur->OSTCBMsg      = (void *)0;           /* Yes, clear message received              */
            OSTCBCur->OSTCBStat     = OS_STAT_RDY;
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;       /* No longer waiting for event              */
            OS_EXIT_CRITICAL();
            *err                    = OS_NO_ERR;
        } else if (OSTCBCur->OSTCBStat & OS_STAT_MBOX) {   /* If status is not OS_STAT_RDY, timed out  */
            if ((pevent->OSEventTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {
                pevent->OSEventGrp &= ~OSTCBCur->OSTCBBitY;
            }
            OSTCBCur->OSTCBStat     = OS_STAT_RDY;         /* Set status to ready                      */
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;
            OS_EXIT_CRITICAL();
            msg                     = (void *)0;           /* Set message contents to NULL             */
            *err                    = OS_TIMEOUT;          /* Indicate that a timeout occured          */
        } else {
            msg                     = pevent->OSEventPtr;  /* Message received                         */
            pevent->OSEventPtr      = (void *)0;           /* Clear the mailbox                        */
            OS_EXIT_CRITICAL();
            *err                    = OS_NO_ERR;
        }
    }
    return (msg);                                          /* Return the message received (or NULL)    */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                       POST MESSAGE TO A MAILBOX
*********************************************************************************************************
*/

UBYTE OSMboxPost(OS_EVENT *pevent, void *msg)
{
    OS_TCB *ptcb;
    UBYTE   x;
    UBYTE   y;
    UBYTE   bitx;
    UBYTE   bity;
    UBYTE   p;


    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp) {                         /* See if any task pending on mailbox            */
        y    = OSUnMapTbl[pevent->OSEventGrp];        /* Find highest prio. task waiting for message   */
        bity = OSMapTbl[y];
        x    = OSUnMapTbl[pevent->OSEventTbl[y]];
        bitx = OSMapTbl[x];
        p    = (y << 3) + x;                          /* Find the priority of the task                 */
        if ((pevent->OSEventTbl[y] &= ~bitx) == 0) {  /* Remove this task from the waiting list        */
            pevent->OSEventGrp &= ~bity;
        }
        ptcb                 =  OSTCBPrioTbl[p];      /* Point to this task's OS_TCB                   */
        ptcb->OSTCBDly       =  0;                    /* Prevent OSTimeTick() from readying task       */
        ptcb->OSTCBEventPtr  = (OS_EVENT *)0;         /* Unlink ECB from this task                     */
        ptcb->OSTCBMsg       =  msg;                  /* Send message directly to waiting task         */
        ptcb->OSTCBStat     &= ~OS_STAT_MBOX;         /* Clear bit associated with event type          */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {         /* See if task is ready (could be suspended)     */
            OSRdyGrp        |=  bity;                 /* Put task in the ready to run list             */
            OSRdyTbl[y]     |=  bitx;
        }
        OS_EXIT_CRITICAL();
        OSSched();                                    /* Find highest priority task ready to run       */
        return (OS_NO_ERR);
    } else {
        if (pevent->OSEventPtr != (void *)0) {        /* Make sure mailbox doesn't already have a msg  */
            OS_EXIT_CRITICAL();
            return (OS_MBOX_FULL);
        } else {
            pevent->OSEventPtr = msg;                 /* Place message in mailbox                      */
            OS_EXIT_CRITICAL();
            return (OS_NO_ERR);
        }
    }
}
#endif
/*$PAGE*/
#if OS_Q_EN && (OS_MAX_QS > 0)
/*
*********************************************************************************************************
*                                       INITIALIZE MESSAGE QUEUE
*********************************************************************************************************
*/

OS_EVENT *OSQCreate(void **start, UBYTE size)
{
    OS_EVENT *pevent;
    OS_Q     *pq;


    OS_ENTER_CRITICAL();
    pevent = OSEventFreeList;                    /* Get next free event control block                  */
    if (OSEventFreeList != (OS_EVENT *)0) {      /* See if pool of free ECB pool was empty             */
        OSEventFreeList = (OS_EVENT *)OSEventFreeList->OSEventPtr;
    }
    OS_EXIT_CRITICAL();
    if (pevent != (OS_EVENT *)0) {               /* See if we have an event control block              */
        OS_ENTER_CRITICAL();                     /* Get a free queue control block                     */
        pq = OSQFreeList;
        if (OSQFreeList != (OS_Q *)0) {
            OSQFreeList = OSQFreeList->OSQPtr;
        }
        OS_EXIT_CRITICAL();
        if (pq != (OS_Q *)0) {                   /* See if we were able to get a queue control block   */
            pq->OSQStart          = start;       /* Yes, initialize the queue                          */
            pq->OSQEnd            = &start[size];
            pq->OSQIn             = start;
            pq->OSQOut            = start;
            pq->OSQSize           = size;
            pq->OSQEntries        = 0;
            pevent->OSEventPtr    = pq;
            pevent->OSEventGrp    = 0x00;        /* Initialize rest of event control block             */
            pevent->OSEventTbl[0] = 0x00;
            pevent->OSEventTbl[1] = 0x00;
            pevent->OSEventTbl[2] = 0x00;
            pevent->OSEventTbl[3] = 0x00;
            pevent->OSEventTbl[4] = 0x00;
            pevent->OSEventTbl[5] = 0x00;
            pevent->OSEventTbl[6] = 0x00;
            pevent->OSEventTbl[7] = 0x00;
        } else {                                 /* No,  since we couldn't get a queue control block   */
            OS_ENTER_CRITICAL();                 /* Return event control block on error                */
            pevent->OSEventPtr = (void *)OSEventFreeList;
            OSEventFreeList    = pevent;
            OS_EXIT_CRITICAL();
            pevent = (OS_EVENT *)0;
        }
    }
    return (pevent);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                      ACCEPT MESSAGE FROM QUEUE
*
* Description: This function checks the queue to see if a message is available.  Unlike OSQPend(),
*              OSQAccept() does not suspend the calling task if a message is not available.
* Arguments  : 'pevent'      is a pointer to the event control block
* Returns    : != (void *)0  is the message in the queue if one is available.  The message is removed
*                            from the so the next time OSQAccept() is called, the queue will contain
*                            one less entry.
*              == (void *)0  if the queue is empty
*********************************************************************************************************
*/

void *OSQAccept(OS_EVENT *pevent)
{
    void  *msg;
    OS_Q  *pq;


    OS_ENTER_CRITICAL();
    pq = pevent->OSEventPtr;                     /* Point at queue control block                       */
    if (pq->OSQEntries != 0) {                   /* See if any messages in the queue                   */
        msg = *pq->OSQOut++;                     /* Yes, extract oldest message from the queue         */
        pq->OSQEntries--;                        /* Update the number of entries in the queue          */
        if (pq->OSQOut == pq->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pq->OSQOut = pq->OSQStart;
        }
    } else {
        msg = (void *)0;                         /* Queue is empty                                     */
    }
    OS_EXIT_CRITICAL();
    return (msg);                                /* Return message received (or NULL)                  */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                     PEND ON A QUEUE FOR A MESSAGE
*********************************************************************************************************
*/

void *OSQPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err)
{
    void  *msg;
    OS_Q  *pq;


    OS_ENTER_CRITICAL();
    pq = pevent->OSEventPtr;                     /* Point at queue control block                       */
    if (pq->OSQEntries != 0) {                   /* See if any messages in the queue                   */
        msg = *pq->OSQOut++;                     /* Yes, extract oldest message from the queue         */
        pq->OSQEntries--;                        /* Update the number of entries in the queue          */
        if (pq->OSQOut == pq->OSQEnd) {          /* Wrap OUT pointer if we are at the end of the queue */
            pq->OSQOut = pq->OSQStart;
        }
        OS_EXIT_CRITICAL();
        *err = OS_NO_ERR;
    } else {
        OSTCBCur->OSTCBStat    |= OS_STAT_Q;     /* Task will have to pend for a message to be posted  */
        OSTCBCur->OSTCBDly      = timeout;       /* Load timeout into TCB                              */
        OSTCBCur->OSTCBEventPtr = pevent;        /* Store pointer to event control block in TCB        */
        if ((OSRdyTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {  /* Task no longer ready      */
            OSRdyGrp &= ~OSTCBCur->OSTCBBitY;
        }
        pevent->OSEventTbl[OSTCBCur->OSTCBY] |= OSTCBCur->OSTCBBitX;      /* Put task in waiting list  */
        pevent->OSEventGrp                   |= OSTCBCur->OSTCBBitY;
        OS_EXIT_CRITICAL();
        OSSched();                               /* Find next highest priority task ready to run       */
        OS_ENTER_CRITICAL();
        if ((msg = OSTCBCur->OSTCBMsg) != (void *)0) {/* Did we get a message?                         */
            OSTCBCur->OSTCBMsg      = (void *)0;      /* Extract message from TCB (Put there by QPost) */
            OSTCBCur->OSTCBStat     = OS_STAT_RDY;
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;  /* No longer waiting for event                   */
            OS_EXIT_CRITICAL();
            *err                    = OS_NO_ERR;
        } else if (OSTCBCur->OSTCBStat & OS_STAT_Q) { /* Timed out if status indicates pending on Q    */
            if ((pevent->OSEventTbl[OSTCBCur->OSTCBY] &= ~OSTCBCur->OSTCBBitX) == 0) {
                pevent->OSEventGrp &= ~OSTCBCur->OSTCBBitY;
            }
            OSTCBCur->OSTCBStat     = OS_STAT_RDY;    /* Set status to ready                           */
            OSTCBCur->OSTCBEventPtr = (OS_EVENT *)0;  /* No longer waiting for event                   */
            OS_EXIT_CRITICAL();
            msg                     = (void *)0;      /* No message received                           */
            *err                    = OS_TIMEOUT;     /* Indicate a timeout occured                    */
        } else {
            msg = *pq->OSQOut++;                      /* Extract message from queue                    */
            pq->OSQEntries--;                         /* Update the number of entries in the queue     */
            if (pq->OSQOut == pq->OSQEnd) {           /* Wrap OUT pointer if we are at the end of Q    */
                pq->OSQOut = pq->OSQStart;
            }
            OS_EXIT_CRITICAL();
            *err = OS_NO_ERR;
        }
    }                                                 /* Return message received (or NULL)              */
    return (msg);
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                        POST MESSAGE TO A QUEUE
*********************************************************************************************************
*/

UBYTE OSQPost(OS_EVENT *pevent, void *msg)
{
    OS_Q   *pq;
    OS_TCB *ptcb;
    UBYTE   x;
    UBYTE   y;
    UBYTE   bitx;
    UBYTE   bity;
    UBYTE   p;


    OS_ENTER_CRITICAL();
    if (pevent->OSEventGrp) {                         /* See if any task pending on queue              */
        y    = OSUnMapTbl[pevent->OSEventGrp];        /* Find highest prio. task waiting for message   */
        bity = OSMapTbl[y];
        x    = OSUnMapTbl[pevent->OSEventTbl[y]];
        bitx = OSMapTbl[x];
        p    = (y << 3) + x;                          /* Find priority of task getting the msg         */
        if ((pevent->OSEventTbl[y] &= ~bitx) == 0) {  /* Remove this task from the waiting list        */
            pevent->OSEventGrp &= ~bity;
        }
        ptcb                 =  OSTCBPrioTbl[p];      /* Point to this task's OS_TCB                   */
        ptcb->OSTCBDly       =  0;                    /* Prevent OSTimeTick() from readying task       */
        ptcb->OSTCBEventPtr  = (OS_EVENT *)0;         /* Unlink ECB from this task                     */
        ptcb->OSTCBMsg       = msg;                   /* Send message directly to waiting task         */
        ptcb->OSTCBStat     &= ~OS_STAT_Q;            /* Clear bit associated with event type          */
        if (ptcb->OSTCBStat == OS_STAT_RDY) {         /* See if task is ready (could be susp'd)        */
            OSRdyGrp        |=  bity;                 /* Put task in the ready to run list             */
            OSRdyTbl[y]     |=  bitx;
        }
        OS_EXIT_CRITICAL();
        OSSched();                                    /* Find highest priority task ready to run       */
        return (OS_NO_ERR);
    } else {
        pq = pevent->OSEventPtr;                      /* Point to queue control block                  */
        if (pq->OSQEntries >= pq->OSQSize) {          /* Make sure queue is not full                   */
            OS_EXIT_CRITICAL();
            return (OS_Q_FULL);
        } else {
            *pq->OSQIn++ = msg;                       /* Insert message into queue                     */
            pq->OSQEntries++;                         /* Update the nbr of entries in the queue        */
            if (pq->OSQIn == pq->OSQEnd) {            /* Wrap IN ptr if we are at end of queue         */
                pq->OSQIn = pq->OSQStart;
            }
            OS_EXIT_CRITICAL();
        }
        return (OS_NO_ERR);
    }
}
#endif
/*$PAGE*/
/*
*********************************************************************************************************
*                                            DUMMY FUNCTION
*********************************************************************************************************
*/

static void  OSDummy(void)
{
}
