/*
*********************************************************************************************************
*                                                 uC/OS
*                                          The Real-Time Kernel
*
*                        (c) Copyright 1992-1995, Jean J. Labrosse, Plantation, FL
*                                           All Rights Reserved
*
*                                   Configuration for Intel 80x86 (Large)
*
* File : OS_CFG.H
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          uC/OS CONFIGURATION
*********************************************************************************************************
*/

#define OS_MAX_TASKS             63    /* Maximum number of tasks in your application                  */
#define OS_MAX_EVENTS            20    /* Maximum number of event control blocks in your application   */
#define OS_MAX_QS                 5    /* Maximum number of queue control blocks in your application   */

#define OS_TASK_IDLE_STK_SIZE  1024    /* Idle task stack size (WORDs)                                 */
#define OS_TASK_IDLE_STK_TOP   1023    /* Index into idle task top of stack                            */

#define OS_TASK_CHANGE_PRIO_EN    1    /* Include code for OSTaskChangePrio()                          */
#define OS_TASK_DEL_EN            1    /* Include code for OSTaskDel()                                 */
#define OS_SEM_EN                 1    /* Include code for SEMAPHORES                                  */
#define OS_MBOX_EN                1    /* Include code for MAILBOXES                                   */
#define OS_Q_EN                   1    /* Include code for QUEUES                                      */
#define OS_TASK_SUSPEND_EN        1    /* Include code for OSTaskSuspend() and OSTaskResume()          */

#define uCOS                   0x80
