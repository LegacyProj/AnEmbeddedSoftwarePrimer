/*
*********************************************************************************************************
*                                        uC/OS, The Real-Time Kernel
*
*                                                EXAMPLE #1
*********************************************************************************************************
*/

#include "INCLUDES.H"

/*
*********************************************************************************************************
*                                             CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     768                /* Size of each task's stacks (# of bytes)       */
#define          N_TASKS            62                /* Number of identical tasks                     */

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

UBYTE            TaskStk[N_TASKS][TASK_STK_SIZE];     /* Tasks stacks                                  */
UBYTE            StatStk[TASK_STK_SIZE];
char             TaskData[N_TASKS];                   /* Parameters to pass to each task               */
UBYTE            StatData;
OS_EVENT        *RandomSem;
UWORD            OldSP;
UWORD            OldBP;
void interrupt (*OldTickISR)(void);

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void far         Task(void *data);                    /* Function prototypes of tasks                  */
void far         Stat(void *data);                    /* Function prototypes of Statistics task        */
void             DispChar(UBYTE x, UBYTE y, char c);
void             DispStr(UBYTE x, UBYTE y, char *s);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main(void)
{
    clrscr();                                              /* Clear the screen                         */
    OldBP      = _BP;                                      /* Save current SP and BP                   */
    OldSP      = _SP;
    OldTickISR = getvect(0x08);                            /* Get MS-DOS's tick vector                 */
    setvect(0x81, OldTickISR);                             /* Store MS-DOS's tick to chain             */
    setvect(uCOS, (void interrupt (*)(void))OSCtxSw);      /* uC/OS's context switch vector            */
    OSInit();
    RandomSem = OSSemCreate(1);                            /* Random number semaphore                  */
    OSTaskCreate(Stat, (void *)&StatData, (void *)&StatStk[TASK_STK_SIZE], 62);
    OSStart();                                             /* Start multitasking                       */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              STATISTICS TASK
*********************************************************************************************************
*/

void far Stat(void *data)
{
    UBYTE  i;
    char   s[100];
    double max;
    double usage;


    data = data;                                           /* Prevent compiler warning                 */
    OS_ENTER_CRITICAL();
    setvect(0x08, (void interrupt (*)(void))OSTickISR);    /* Install uC/OS's clock tick ISR           */
    OS_EXIT_CRITICAL();
    DispStr(0, 22, "Determining  CPU's capacity ...");     /* Determine maximum count for OSIdleCtr    */
    OSTimeDly(1);                                          /* Synchronize to clock tick                */
    OSIdleCtr = 0L;                                        /* Determine MAX. idle counter value ...    */
    OSTimeDly(18);                                         /* ... for 18 clock ticks                   */
    max       = (double)OSIdleCtr;
    for (i = 0; i < N_TASKS; i++) {                        /* Create N_TASKS identical tasks           */
        TaskData[i] = ' ' + i;
        OSTaskCreate(Task, (void *)&TaskData[i], (void *)&TaskStk[i][TASK_STK_SIZE], i);
    }
    DispStr(0, 23, "Press any key to quit.");
    OSIdleCtr = 0L;
    while (1) {
        usage = 100.0 - (100.0 * (double)OSIdleCtr / max); /* Compute and display statistics           */
        sprintf(s, "Task Switches/sec: %d   CPU Usage: %5.2f %%   Idle Ctr: %7.0f / %7.0f   ",
                OSCtxSwCtr,
                usage,
                (double)OSIdleCtr,
                max);
        DispStr(0, 22, s);
        if (kbhit()) {                                     /* Exit if any key pressed                  */
            OS_ENTER_CRITICAL();
            setvect(0x08, OldTickISR);
            OS_EXIT_CRITICAL();
            clrscr();
            _BP = OldBP;                                   /* Restore old SP and BP                    */
            _SP = OldSP;
            exit(0);
        }
        OS_ENTER_CRITICAL();
        OSCtxSwCtr = 0;                                    /* Reset statistics counters                */
        OSIdleCtr  = 0L;
        OS_EXIT_CRITICAL();
        OSTimeDly(18);                                     /* Wait one second                          */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void far Task(void *data)
{
    UBYTE x;
    UBYTE y;
    UBYTE err;


    while (1) {
        OSTimeDly(1);                            /* Delay 1 clock tick                                 */
        OSSemPend(RandomSem, 0, &err);           /* Acquire semaphore to perform random numbers        */
        x = random(79);                          /* Find X position where task number will appear      */
        y = random(21);                          /* Find Y position where task number will appear      */
        OSSemPost(RandomSem);                    /* Release semaphore                                  */
        DispChar(x, y, *(char *)data);           /* Display the task number on the screen              */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                                DISPLAY FUNCTIONS
*********************************************************************************************************
*/
void DispChar(UBYTE x, UBYTE y, char c)
{
    UBYTE far *pscr;
    UWORD      offset;


    offset  = (UWORD)y * 160 + (UWORD)x * 2;     /* Calculate position of character on the screen      */
    pscr    = MK_FP(0xB800, offset);
    OS_ENTER_CRITICAL();
    *pscr++ = c;                                 /* Put character in video RAM                         */
    *pscr   = 0x07;                              /* Put video attribute in video RAM                   */
    OS_EXIT_CRITICAL();
}



void DispStr(UBYTE x, UBYTE y, char *s)
{
    UBYTE far *pscr;
    UWORD      offset;


    offset  = (UWORD)y * 160 + (UWORD)x * 2;     /* Calculate position of character on the screen      */
    pscr    = MK_FP(0xB800, offset);
    while (*s) {
        OS_ENTER_CRITICAL();
        *pscr++ = *s++;                          /* Put character in video RAM                         */
        *pscr++ = 0x07;                          /* Put video attribute in video RAM                   */
        OS_EXIT_CRITICAL();
    }
}
