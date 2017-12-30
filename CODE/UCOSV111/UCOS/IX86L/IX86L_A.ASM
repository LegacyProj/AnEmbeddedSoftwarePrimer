;********************************************************************************************************
;                                                uC/OS
;                                         The Real-Time Kernel
;
;                      (c) Copyright 1992, 1993, Jean J. Labrosse, Plantation, FL
;                                          All Rights Reserved
;
;
;                                       80x86/80x88 Specific code
;                                          LARGE MEMORY MODEL
;
; File : Ix86L_A.ASM
; By   : Jean J. Labrosse
;********************************************************************************************************

.MODEL LARGE
.186
.CODE

            PUBLIC _OSStartHighRdy
            PUBLIC _OSCtxSw
            PUBLIC _OSIntCtxSw
            PUBLIC _OSTickISR

            EXTRN  _OSIntEnter:FAR
            EXTRN  _OSIntExit:FAR
            EXTRN  _OSTimeTick:FAR

            EXTRN  _OSTCBCur:DWORD
            EXTRN  _OSTCBHighRdy:DWORD

            PAGE                                 ; /*$PAGE*/
;*********************************************************************************************************
;                                          START MULTITASKING
;                                      void OSStartHighRdy(void)
;
; Total execution time : 179 bus cycles
;*********************************************************************************************************

_OSStartHighRdy    PROC FAR

;
            MOV    AX, DGROUP                         ;  4~, Reload DS
            MOV    DS, AX                             ;  2~
;
            LES    BX, DWORD PTR DS:_OSTCBHighRdy     ; 18~, SS:SP = OSTCBHighRdy->OSTCBStkPtr;
            MOV    SS, ES:[BX+2]                      ;  9~
            MOV    SP, ES:[BX]                        ;  9~
;
            POP    DS                                 ;  8~, Pop task's stack
            POP    ES                                 ;  8~
            POPA                                      ; 51~
;
            IRET                                      ; 28~, Run task

_OSStartHighRdy    ENDP

            PAGE                                      ; /*$PAGE*/
;*********************************************************************************************************
;                                PERFORM A CONTEXT SWITCH (From task level)
;                                           void OSCtxSw(void)
;
; Total execution time : 277 bus cycles
;*********************************************************************************************************

_OSCtxSw    PROC   FAR

            PUSHA                                     ; 36~, Save current task's context
            PUSH   ES                                 ;  8~
            PUSH   DS                                 ;  8~
;
            MOV    AX, DGROUP                         ;  4~, Reload DS with DGROUP
            MOV    DS, AX                             ;  2~
;
            LES    BX, DWORD PTR DS:_OSTCBCur         ; 18~, OSTCBCur->OSTCBStkPtr = SS:SP
            MOV    ES:[BX+2], SS                      ; 12~
            MOV    ES:[BX], SP                        ; 12~
;
            MOV    AX, WORD PTR DS:_OSTCBHighRdy+2    ;  9~, OSTCBCur = OSTCBHighRdy
            MOV    DX, WORD PTR DS:_OSTCBHighRdy      ;  9~
            MOV    WORD PTR DS:_OSTCBCur+2, AX        ; 12~, 
            MOV    WORD PTR DS:_OSTCBCur, DX          ; 12~
;
            LES    BX, DWORD PTR DS:_OSTCBHighRdy     ; 18~, SS:SP = OSTCBHighRdy->OSTCBStkPtr;
            MOV    SS, ES:[BX+2]                      ;  9~
            MOV    SP, ES:[BX]                        ;  9~
;
            POP    DS                                 ;  8~, Restore context of task to run
            POP    ES                                 ;  8~
            POPA                                      ; 51~
;
            IRET                                      ; 28~, Return to new task

_OSCtxSw    ENDP

            PAGE                                      ; /*$PAGE*/
;*********************************************************************************************************
;                                PERFORM A CONTEXT SWITCH (From an ISR)
;                                        void OSIntCtxSw(void)
;
; Total execution time : 219 bus cycles
;*********************************************************************************************************

_OSIntCtxSw PROC   FAR
;
            ADD    SP,10                              ;  4~, Ignore calls to OSIntExit and OSIntCtxSw
;
            MOV    AX, DGROUP                         ;  4~, Reload DS with DGROUP
            MOV    DS, AX                             ;  2~
;
            LES    BX, DWORD PTR DS:_OSTCBCur         ; 18~, OSTCBCur->OSTCBStkPtr = SS:SP
            MOV    ES:[BX+2], SS                      ; 12~
            MOV    ES:[BX], SP                        ; 12~
;
            MOV    AX, WORD PTR DS:_OSTCBHighRdy+2    ;  9~, OSTCBCur = OSTCBHighRdy
            MOV    DX, WORD PTR DS:_OSTCBHighRdy      ;  9~
            MOV    WORD PTR DS:_OSTCBCur+2, AX        ; 12~
            MOV    WORD PTR DS:_OSTCBCur, DX          ; 12~
;
            LES    BX, DWORD PTR DS:_OSTCBHighRdy     ; 18~, SS:SP = OSTCBHighRdy->OSTCBStkPtr;
            MOV    SS, ES:[BX+2]                      ;  9~
            MOV    SP, ES:[BX]                        ;  9~
;
;
            POP    DS                                 ;  8~, Restore new task's context
            POP    ES                                 ;  8~
            POPA                                      ; 51~
;
            IRET                                      ; 28~, Return to new task

_OSIntCtxSw ENDP

            PAGE
;*********************************************************************************************************
;                                            HANDLE TICK ISR
;*********************************************************************************************************

_OSTickISR  PROC   FAR

            PUSHA                                     ; Save task's context
            PUSH   ES
            PUSH   DS
;
            INT    081H                               ; Chain into DOS's tick ISR
;
            MOV    AX, DGROUP                         ; Reload DS with DGROUP
            MOV    DS, AX
;
            CALL   FAR PTR _OSIntEnter                ; Notify uC/OS of ISR
            CALL   FAR PTR _OSTimeTick                ; Process system tick
            CALL   FAR PTR _OSIntExit                 ; Notify uC/OS of end of ISR
;
            POP    DS                                 ; Restore interrupted task's context
            POP    ES
            POPA
;
            IRET                                      ; Return to interrupted task

_OSTickISR  ENDP

            END
