/*
*********************************************************************************************************
*                                                uC/OS
*                                         The Real-Time Kernel
*
*                      (c) Copyright 1992, 1993, Jean J. Labrosse, Plantation, FL
*                                          All Rights Reserved
*
*                                       80x86/80x88 Specific code
*                                           LARGE MEMORY MODEL
*
* File : Ix86L.H
* By   : Jean J. Labrosse
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*/

typedef unsigned char  BOOLEAN;
typedef unsigned char  UBYTE;                    /* Unsigned  8 bit quantity                           */
typedef signed   char  BYTE;                     /* Signed    8 bit quantity                           */
typedef unsigned int   UWORD;                    /* Unsigned 16 bit quantity                           */
typedef signed   int   WORD;                     /* Signed   16 bit quantity                           */
typedef unsigned long  ULONG;                    /* Unsigned 32 bit quantity                           */
typedef signed   long  LONG;                     /* Signed   32 bit quantity                           */

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#ifndef  FALSE
#define  FALSE    0
#endif

#ifndef  TRUE
#define  TRUE     1
#endif

/*
*********************************************************************************************************
*                                Intel 80x86 (LARGE) Processor Specifics
*********************************************************************************************************
*/

#define  OS_ENTER_CRITICAL()  asm  CLI
#define  OS_EXIT_CRITICAL()   asm  STI
#define  OS_TASK_SW()         asm  INT   uCOS

#define  OS_FAR               far
#define  OS_STK_TYPE          UBYTE
