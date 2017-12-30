;********************************************************************************************************
;                                                uC/OS
;                                         The Real-Time Kernel
;
;                       (c) Copyright 1992, 1993, Jean J. Labrosse, Plantation, FL
;                                          All Rights Reserved
;
;
;                                       80x86/80x88 Specific code
;                                          SMALL MEMORY MODEL
;
; File : Ix86S_A.ASM
; By   : Jean J. Labrosse
;********************************************************************************************************

            PUBLIC _OSTickISR
            PUBLIC _OSStartHighRdy
            PUBLIC _OSCtxSw
            PUBLIC _OSIntCtxSw

            EXTRN  _OSIntEnter:NEAR
            EXTRN  _OSIntExit:NEAR
            EXTRN  _OSTimeTick:NEAR

            EXTRN  _OSTCBCur:WORD
            EXTRN  _OSTCBHighRdy:WORD

.MODEL      SMALL
.CODE
.186

;*********************************************************************************************************
;                                          START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Total execution time : 134 bus cycles
;*********************************************************************************************************

_OSStartHighRdy  PROC NEAR

            MOV    BX, [_OSTCBHighRdy]            ;  9~, Point to TCB of highest priority task ready to run
;
            MOV    SS, [BX+2]                     ;  9~, SS:SP = OSTCBHighRdy->OSTCBStkPtr
            MOV    SP, [BX]                       ;  9~,
;
            POP    DS                             ;  8~, Load task's context
            POP    ES                             ;  8~
            POPA                                  ; 51~
;
            IRET                                  ; 28~, Run task

_OSStartHighRdy  ENDP

            PAGE                                  ; /*$PAGE*/
;*********************************************************************************************************
;                                PERFORM A CONTEXT SWITCH (From task level)
;                                           void OSCtxSw(void)
;
; Total execution time : 225 bus cycles
;*********************************************************************************************************

_OSCtxSw    PROC   FAR
;
            PUSHA                                ; 36~, Save current task's context
            PUSH   ES                            ;  8~
            PUSH   DS                            ;  8~
;
            MOV    AX, SEG _OSTCBCur             ;  4~, Reload DS in case it was altered
            MOV    DS, AX                        ;  2~
;
            MOV    BX, [_OSTCBCur]               ;  9~, OSTCBCur->OSTCBStkPtr = SS:SP
            MOV    [BX+2], SS                    ; 12~
            MOV    [BX], SP                      ; 12~
;
            MOV    BX, [_OSTCBHighRdy]           ;  9~, OSTCBCur = OSTCBHighRdy
            MOV    [_OSTCBCur], BX               ; 12~
;
            MOV    SS, [BX+2]                    ;  9~, SS:SP = OSTCBHighRdy->OSTCBStkPtr;
            MOV    SP, [BX]                      ;  9~
;
            POP    DS                            ;  8~, Load new task's context
            POP    ES                            ;  8~
            POPA                                 ; 51~
;
            IRET                                 ; 28~, Return to new task
;
_OSCtxSw    ENDP

            PAGE                                 ; /*$PAGE*/
;*********************************************************************************************************
;                                PERFORM A CONTEXT SWITCH (From an ISR)
;                                        void OSIntCtxSw(void)
;
; Total execution time : 177 bus cycles
;*********************************************************************************************************

_OSIntCtxSw PROC   NEAR
;
            ADD    SP,6                          ;  4~, Ignore calls to OSIntExit and OSIntCtxSw
;
            MOV    AX, SEG _OSTCBCur             ;  4~, Reload DS in case it was altered
            MOV    DS, AX                        ;  2~
;
            MOV    BX, [_OSTCBCur]               ;  9~, OSTCBCur->OSTCBStkPtr = SS:SP
            MOV    [BX+2], SS                    ; 12~
            MOV    [BX], SP                      ; 12~
;
            MOV    BX, [_OSTCBHighRdy]           ;  9~, OSTCBCur = OSTCBHighRdy
            MOV    [_OSTCBCur], BX               ; 12~
;
            MOV    SS, [BX+2]                    ;  9~, SS:SP = OSTCBHighRdy->OSTCBStkPtr;
            MOV    SP, [BX]                      ;  9~
;
            POP    DS                            ;  8~, Restore context of task to run
            POP    ES                            ;  8~
            POPA                                 ; 51~
;
            IRET                                 ; 28~, Return to new task

;
_OSIntCtxSw ENDP

            PAGE
;*********************************************************************************************************
;                                            HANDLE TICK ISR
;*********************************************************************************************************
;
_OSTickISR  PROC   FAR
;
            PUSHA                                ; Save interrupted task's context
            PUSH   ES
            PUSH   DS
;
            INT    081H                          ; Chain into DOS's tick ISR
;
            MOV    AX, DGROUP                    ; Reload DS with DGROUP
            MOV    DS, AX
;
            CALL   _OSIntEnter                   ; Notify uC/OS about ISR
            CALL   _OSTimeTick                   ; Handle system tick
            CALL   _OSIntExit                    ; Exit uC/OS through scheduler if HPT ready
;
            POP    DS                            ; Restore interrupted task's context
            POP    ES
            POPA
;
            IRET                                 ; Return to interrupted task
;
_OSTickISR  ENDP
;
            END
