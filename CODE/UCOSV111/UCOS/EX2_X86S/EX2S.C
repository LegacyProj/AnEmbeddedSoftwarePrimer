/*
*********************************************************************************************************
*                                     uC/OS, The Real-Time Kernel
*
*                                              EXAMPLE #2
*********************************************************************************************************
*/

#include "INCLUDES.H"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE    1024                /* Size of each task's stacks (# of bytes)       */
#define          KEY_Q_SIZE         10                /* Size of keyboard queue                        */

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

UBYTE            TaskStatStk[TASK_STK_SIZE];          /* Statistics task stack                         */
UBYTE            TaskClkStk[TASK_STK_SIZE];           /* Clock      task stack                         */
UBYTE            TaskKeyStk[TASK_STK_SIZE];           /* Keyboard   task stack                         */
UBYTE            Task1Stk[TASK_STK_SIZE];             /* Task #1    task stack                         */
UBYTE            Task2Stk[TASK_STK_SIZE];             /* Task #2    task stack                         */
UBYTE            Task3Stk[TASK_STK_SIZE];             /* Task #3    task stack                         */

OS_EVENT        *KeyQPtr;                             /* Pointer to keyboard queue                     */
OS_EVENT        *KeyMboxPtr;                          /* Pointer to keyboard mailbox                   */

void            *KeyQ[KEY_Q_SIZE];                    /* Keyboard queue                                */

void interrupt (*OldTickISR)(void);
UWORD            OldBP;
UWORD            OldSP;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void far         TaskStat(void *data);                /* Function prototypes of tasks                  */
void far         TaskKey(void *data);
void far         TaskClk(void *data);
void far         Task1(void *data);
void far         Task2(void *data);
void far         Task3(void *data);

void             DispChar(UBYTE x, UBYTE y, char  c);
void             DispStr(UBYTE x,  UBYTE y, char *s);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main(void)
{
    UBYTE i;


    clrscr();
    OldBP      = _BP;
    OldSP      = _SP;
    OldTickISR = getvect(0x08);                            /* Get MS-DOS's tick vector                 */
    setvect(0x81, OldTickISR);                             /* Store MS-DOS's tick to chain             */
    setvect(uCOS, (void interrupt (*)(void))OSCtxSw);      /* uC/OS's context switch vector            */
    OSInit();
    KeyQPtr    = OSQCreate(&KeyQ[0], KEY_Q_SIZE);          /* Keyboard queue                           */
    KeyMboxPtr = OSMboxCreate((void *)0);                  /* Keyboard mailbox                         */
    OSTaskCreate(TaskStat, (void *)0, (void *)&TaskStatStk[TASK_STK_SIZE], 62);
    OSStart();                                             /* Start multitasking                       */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                              STATISTICS TASK
*********************************************************************************************************
*/

void far TaskStat(void *data)
{
    UBYTE  i;
    char   s[80];
    double max;
    double usage;
    ULONG  idle;
    UWORD  ctxsw;


    data = data;                                           /* Prevent compiler warning                 */
    DispStr(0, 0, "uC/OS, The Real-Time Kernel");
    OS_ENTER_CRITICAL();                                   /* Install uC/OS's clock tick ISR           */
    setvect(0x08, (void interrupt (*)(void))OSTickISR);
    OS_EXIT_CRITICAL();
    DispStr(0, 22, "Determining  CPU's capacity ...");     /* Determine maximum count for OSIdleCtr    */
    OSTimeDly(1);                                          /* Synchronize to clock tick                */
    OSIdleCtr = 0L;                                        /* Determine MAX. idle counter value        */
    OSTimeDly(18);
    max       = (double)OSIdleCtr;
    OSTaskCreate(TaskKey, (void *)0, (void *)&TaskKeyStk[TASK_STK_SIZE], 10);
    OSTaskCreate(Task1,   (void *)0, (void *)&Task1Stk[TASK_STK_SIZE],   11);
    OSTaskCreate(Task2,   (void *)0, (void *)&Task2Stk[TASK_STK_SIZE],   12);
    OSTaskCreate(Task3,   (void *)0, (void *)&Task3Stk[TASK_STK_SIZE],   13);
    OSTaskCreate(TaskClk, (void *)0, (void *)&TaskClkStk[TASK_STK_SIZE], 14);
    while (1) {
        OS_ENTER_CRITICAL();
        ctxsw      = OSCtxSwCtr;
        idle       = OSIdleCtr;
        OSCtxSwCtr = 0;                                    /* Reset statistics counters                */
        OSIdleCtr  = 0L;
        OS_EXIT_CRITICAL();
        usage = 100.0 - (100.0 * (double)idle / max);      /* Compute and display statistics           */
        sprintf(s, "Task Switches: %d     CPU Usage: %5.2f %%", ctxsw, usage);
        DispStr(0, 22, s);
        sprintf(s, "Idle Ctr: %7.0f / %7.0f  ", (double)idle, max);
        DispStr(0, 23, s);
        OSTimeDly(18);                                     /* Wait one second                          */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                              KEYBOARD TASK
*********************************************************************************************************
*/

void far TaskKey(void *data)
{
    UWORD ctr;
    char  s[80];


    data = data;                                           /* Prevent compiler warning                 */
    ctr  = 0;
    DispStr(0, 5, "Keyboard Task:");
    DispStr(0, 6, "   Press 1 to send a message to task #1");
    DispStr(0, 7, "   Press 2 to send a message to task #2");
    DispStr(0, 8, "   Press X to quit");
    while (1) {
        OSTimeDly(1);
        sprintf(s, "%05d", ctr);
        DispStr(15, 5, s);
        ctr++;
        if (kbhit()) {
            switch (getch()) {
                case '1': OSMboxPost(KeyMboxPtr, (void *)1);
                          break;

                case '2': OSQPost(KeyQPtr, (void *)1);
                          break;

                case 'x':
                case 'X': OS_ENTER_CRITICAL();
                          setvect(0x08, OldTickISR);
                          OS_EXIT_CRITICAL();
                          clrscr();
                          _BP = OldBP;
                          _SP = OldSP;
                          exit(0);
                          break;
            }
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #1
*********************************************************************************************************
*/

void far Task1(void *data)
{
    UBYTE  err;
    UWORD  toctr;
    UWORD  msgctr;
    char   s[80];


    data   = data;
    msgctr = 0;
    toctr  = 0;
    while (1) {
        sprintf(s, "Task #1 Timeout Counter: %05d", toctr);
        DispStr(0, 10, s);
        sprintf(s, "        Message Counter: %05d", msgctr);
        DispStr(0, 11, s);
        OSMboxPend(KeyMboxPtr, 36, &err);
        switch (err) {
            case OS_NO_ERR:
                 msgctr++;
                 break;

            case OS_TIMEOUT:
                 toctr++;
                 break;
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #2
*********************************************************************************************************
*/

void far Task2(void *data)
{
    UBYTE  err;
    UWORD  toctr;
    UWORD  msgctr;
    char   s[80];


    data   = data;
    msgctr = 0;
    toctr  = 0;
    while (1) {
        sprintf(s, "Task #2 Timeout Counter: %05d", toctr);
        DispStr(0, 15, s);
        sprintf(s, "        Message Counter: %05d", msgctr);
        DispStr(0, 16, s);
        OSQPend(KeyQPtr, 72, &err);
        switch (err) {
            case OS_NO_ERR:
                 msgctr++;
                 break;

            case OS_TIMEOUT:
                 toctr++;
                 break;
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #3
*********************************************************************************************************
*/

void far Task3(void *data)
{
    UBYTE  err;
    UBYTE  x;
    UBYTE  y;
    UBYTE  z;


    data = data;
    DispStr(50, 0, "---------- Task #3 ----------");
    while (1) {
        OSTimeDly(1);
        x = random(29);                          /* Find X position where task number will appear      */
        y = random(20);                          /* Find Y position where task number will appear      */
        z = random(4);
        switch (z) {
            case 0: DispChar(x + 50, y + 1, '*');
                    break;

            case 1: DispChar(x + 50, y + 1, '$');
                    break;

            case 2: DispChar(x + 50, y + 1, '#');
                    break;

            case 3: DispChar(x + 50, y + 1, ' ');
                    break;
        }
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               CLOCK TASK
*********************************************************************************************************
*/

void far TaskClk(void *data)
{
    struct time now;
    struct date today;
    char        s[40];


    data = data;
    while (1) {
        OSTimeDly(18);
        gettime(&now);
        getdate(&today);
        sprintf(s, "%02d-%02d-%02d  %02d:%02d:%02d",
                   today.da_mon,
                   today.da_day,
                   today.da_year,
                   now.ti_hour,
                   now.ti_min,
                   now.ti_sec);
        DispStr(58, 23, s);
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
