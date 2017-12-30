/*****************************************************************************

                           P R O B S T Y L . H


This include file standard definitions used in programs written by personnel
at Probitas Corporation.


Developed by PROBITAS CORPORATION, 3235 Kifer Road, Suite 300,
Santa Clara, CA  95051.  A non-transferrable, non-exclusive license is granted
to all Probitas clients without charge for their own use of this information.



                     C H A N G E   R E C O R D

  Date   Initials          Description
-------- -------- ------------------------------------------------------
03/22/93   AJN    Don't define BOOL if windows.h included
07/11/95   AJN    Added definitions of ASSERT and ASSERT_BOOL.
08/07/96   RWV		Added numof() macro.
01/15/97   AJN    Added definition of BOOL8
****************************************************************************/


#ifndef _PROBSTYL
#define _PROBSTYL


/* Special data types  */

#ifndef BOOL
#ifndef _INC_WINDOWS
#define BOOL unsigned char
#endif
#endif

#ifndef BOOL8
#define BOOL8 unsigned char
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef BITS
#define BITS unsigned short
#endif

/* Standard constants */

#ifndef NULLP
#define NULLP		((void*)0)			/* NULL pointer */
#endif

#ifndef OK
#define OK			(0)					/* For functions that return 0 if OK */
#endif

#ifndef TRUE
#define TRUE		(1)					/* Truth */
#endif

#ifndef FALSE
#define FALSE		(0)					/* Falsity */
#endif



/* Logical operators */

#ifndef IS
#define IS ==								/* Useful in statements like: if (x IS y) */
#endif

#ifndef IS_NOT								/* Useful in statements like: if (x IS_NOT y) */
#define IS_NOT !=
#endif

#ifndef AND
#define AND &&								/* Logical ANDing of two statements */
#endif

#ifndef OR
#define OR ||								/* Logical ORing of two statements */
#endif



/* Macro to calculate an array dimension */
#define numof(ARRAY)	(sizeof(ARRAY) / sizeof((ARRAY)[0]))


/* Linked structure operators */

#ifndef NEXT
#define NEXT(x)		((x) = (x)->pNext)		/* Go to next structure */
#endif

#ifndef PREV
#define PREV(x)		((x) = (x)->pPrev)		/* Go to previous structure */
#endif


/* BYTE to WORD conversions and manipulations */

#ifndef REV_WORD
#define REV_WORD(x) ((WORD) ((((x) & 0x00ff) << 8) | (((x) & 0xff00) >> 8)))
#endif

#ifndef BYTE_HIGH_FROM_WORD
#define BYTE_HIGH_FROM_WORD(x) ((BYTE) ((x) >> 8))
#endif

#ifndef BYTE_LOW_FROM_WORD
#define BYTE_LOW_FROM_WORD(x) ((BYTE) (x))
#endif

#ifndef WORD_MAKE_FROM_BYTES
#define WORD_MAKE_FROM_BYTES(hi,lo) ((WORD) ( (((WORD) hi) << 8) + ((WORD) lo) ) )
#endif

/* To define an ASSERT macro that is only called in _DEBUG mode */
#ifndef ASSERT
	#ifdef _DEBUG
		#include <assert.h>
		#define ASSERT(x) assert(x)
	#else
		#define ASSERT(x)
	#endif
#endif

/* To ASSERT that a BOOL has valid values */
#ifndef ASSERT_BOOL
	#define ASSERT_BOOL(f) ASSERT((f == 1) || (f == 0))
#endif

#endif  /* _PROBSTYL */
