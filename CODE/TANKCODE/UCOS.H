/*
*********************************************************************************************************
*                                                 uC/OS
*                                          The Real-Time Kernel
*                                           SYSTEM DECLARATIONS
*
*                        (c) Copyright 1992-1996, Jean J. Labrosse, Plantation, FL
*                                           All Rights Reserved
*
*                                                  V1.11
*
* File : UCOS.H
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            MISCELLANEOUS
*********************************************************************************************************
*/

#ifdef  OS_GLOBALS
#define OS_EXT
#else
#define OS_EXT  extern
#endif

#define OS_PRIO_SELF           0xFF    /* Indicate SELF priority                                       */

/*
*********************************************************************************************************
*                                           uC/OS ERROR CODES
*********************************************************************************************************
*/

#define OS_NO_ERR                 0

#define OS_TIMEOUT               10
#define OS_TASK_NOT_EXIST        11

#define OS_MBOX_FULL             20
#define OS_MBOX_MSG_NOT_AVAIL    21

#if     OS_Q_EN && (OS_MAX_QS > 0)
#define OS_Q_FULL                30
#define OS_Q_MSG_NOT_AVAIL       31
#endif

#define OS_PRIO_EXIST            40
#define OS_PRIO_ERR              41
#define OS_PRIO_INVALID          42

#define OS_SEM_ERR               50
#define OS_SEM_OVF               51
#define OS_SEM_NOT_AVAIL         52

#define OS_TASK_DEL_ERR          60
#define OS_TASK_DEL_IDLE         61
#define OS_TASK_DEL_REQ          62

#define OS_NO_MORE_TCB           70

#define OS_TIME_NOT_DLY          80

#define OS_TASK_SUSPEND_PRIO     90
#define OS_TASK_SUSPEND_IDLE     91

#define OS_TASK_RESUME_PRIO     100
#define OS_TASK_NOT_SUSPENDED   101
/*$PAGE*/
/*
*********************************************************************************************************
*                                          EVENT CONTROL BLOCK
*********************************************************************************************************
*/

typedef struct os_event {
    UBYTE  OSEventGrp;                 /* Group corresponding to tasks waiting for event to occur      */
    UBYTE  OSEventTbl[8];              /* List of tasks waiting for event to occur                     */
    UWORD  OSEventCnt;                 /* Count of used when event is a semaphore                      */
    void  *OSEventPtr;                 /* Pointer to message or queue structure                        */
} OS_EVENT;


/*
*********************************************************************************************************
*                                        uC/OS TASK CONTROL BLOCK
*********************************************************************************************************
*/

typedef struct os_tcb {
    void   OS_FAR *OSTCBStkPtr;        /* Pointer to current top of stack                              */
    UBYTE          OSTCBStat;          /* Task status                                                  */
    UBYTE          OSTCBPrio;          /* Task priority (0 == highest, 63 == lowest)                   */
    UWORD          OSTCBDly;           /* Nbr ticks to delay task or, timeout waiting for event        */
    BOOLEAN        OSTCBDelReq;        /* Indicates whether a task needs to delete itself              */
    UBYTE          OSTCBX;             /* Bit position in group  corresponding to task priority (0..7) */
    UBYTE          OSTCBY;             /* Index into ready table corresponding to task priority        */
    UBYTE          OSTCBBitX;          /* Bit mask to access bit position in ready table               */
    UBYTE          OSTCBBitY;          /* Bit mask to access bit position in ready group               */
#if OS_SEM_EN || OS_MBOX_EN || (OS_Q_EN && (OS_MAX_QS > 0))
    OS_EVENT      *OSTCBEventPtr;      /* Pointer to event control block                               */
#endif
#if OS_MBOX_EN || (OS_Q_EN && (OS_MAX_QS > 0))
    void          *OSTCBMsg;           /* Message received from OSMboxPost() or OSQPost()              */
#endif
    struct os_tcb *OSTCBNext;          /* Pointer to next     TCB in the TCB list                      */
    struct os_tcb *OSTCBPrev;          /* Pointer to previous TCB in the TCB list                      */
} OS_TCB;


/*
*********************************************************************************************************
*                                          QUEUE CONTROL BLOCK
*********************************************************************************************************
*/

#if OS_Q_EN && (OS_MAX_QS > 0)
typedef struct os_q {
    struct os_q   *OSQPtr;             /* Link to next queue control block in list of free blocks      */
    void         **OSQStart;           /* Pointer to start of queue data                               */
    void         **OSQEnd;             /* Pointer to end   of queue data                               */
    void         **OSQIn;              /* Pointer to where next message will be inserted  in   the Q   */
    void         **OSQOut;             /* Pointer to where next message will be extracted from the Q   */
    UBYTE          OSQSize;            /* Size of queue (maximum number of entries)                    */
    UBYTE          OSQEntries;         /* Current number of entries in the queue                       */
} OS_Q;
#endif

/*
*********************************************************************************************************
*                                         uC/OS GLOBAL VARIABLES
*********************************************************************************************************
*/
                                       /* SYSTEM VARIABLES                                             */
OS_EXT  UWORD      OSCtxSwCtr;         /* Counter of number of context switches                        */
OS_EXT  ULONG      OSIdleCtr;          /* Idle counter                                                 */
OS_EXT  UBYTE      OSIntNesting;       /* Interrupt nesting level                                      */
OS_EXT  BOOLEAN    OSRunning;          /* Flag indicating that kernel is running                       */
OS_EXT  OS_TCB    *OSTCBCur;           /* Pointer to currently running TCB                             */
OS_EXT  OS_TCB    *OSTCBHighRdy;       /* Pointer to highest priority TCB ready to run                 */
OS_EXT  OS_TCB    *OSTCBPrioTbl[64];   /* Table of pointers to all created TCBs                        */
/*$PAGE*/
/*
*********************************************************************************************************
*                                       uC/OS FUNCTION PROTOTYPES
*                                     (Target Independant Functions)
*********************************************************************************************************
*/

void        OSInit(void);

void        OSStart(void);

void        OSSched(void);
void        OSSchedLock(void);
void        OSSchedUnlock(void);

#if         OS_TASK_CHANGE_PRIO_EN
UBYTE       OSTaskChangePrio(UBYTE oldprio, UBYTE newprio);
#endif
#if         OS_TASK_DEL_EN
UBYTE       OSTaskDel(UBYTE prio);
UBYTE       OSTaskDelReq(UBYTE prio);
#endif
#if         OS_TASK_SUSPEND_EN
UBYTE       OSTaskSuspend(UBYTE prio);
UBYTE       OSTaskResume(UBYTE prio);
#endif

UBYTE       OSTCBInit(UBYTE prio, void OS_FAR *stk);

void        OSIntEnter(void);
void        OSIntExit(void);

void        OSTimeDly(UWORD ticks);
UBYTE       OSTimeDlyResume(UBYTE prio);
void        OSTimeTick(void);
void        OSTimeSet(ULONG ticks);
ULONG       OSTimeGet(void);

#if         OS_SEM_EN
OS_EVENT   *OSSemCreate(UWORD value);
UWORD       OSSemAccept(OS_EVENT *pevent);
UBYTE       OSSemPost(OS_EVENT *pevent);
void        OSSemPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err);
#endif

#if         OS_MBOX_EN
OS_EVENT   *OSMboxCreate(void *msg);
void       *OSMboxAccept(OS_EVENT *pevent);
UBYTE       OSMboxPost(OS_EVENT *pevent, void *msg);
void       *OSMboxPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err);
#endif

#if         OS_Q_EN && (OS_MAX_QS > 0)
OS_EVENT   *OSQCreate(void **start, UBYTE size);
void       *OSQAccept(OS_EVENT *pevent);
UBYTE       OSQPost(OS_EVENT *pevent, void *msg);
void       *OSQPend(OS_EVENT *pevent, UWORD timeout, UBYTE *err);
#endif

/*
*********************************************************************************************************
*                                       uC/OS FUNCTION PROTOTYPES
*                                      (Target Specific Functions)
*********************************************************************************************************
*/

UBYTE       OSTaskCreate(void (OS_FAR *task)(void *pd), void *pdata, void *pstk, UBYTE prio);

void        OSStartHighRdy(void);
void OS_FAR OSCtxSw(void);
void        OSIntCtxSw(void);
void OS_FAR OSTickISR(void);
