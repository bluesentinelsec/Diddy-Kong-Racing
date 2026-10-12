
/*====================================================================
 * os_libc.h
 *
 * Copyright 1995, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics,
 * Inc.; the contents of this file may not be disclosed to third
 * parties, copied or duplicated in any form, in whole or in part,
 * without the prior written permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to
 * restrictions as set forth in subdivision (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS
 * 252.227-7013, and/or in similar or successor clauses in the FAR,
 * DOD or NASA FAR Supplement. Unpublished - rights reserved under the
 * Copyright Laws of the United States.
 *====================================================================*/

/*---------------------------------------------------------------------*
        Copyright (C) 1998 Nintendo. (Originated by SGI)
        
        $RCSfile: os_libc.h,v $
        $Revision: 1.3 $
        $Date: 1999/07/13 01:43:47 $
 *---------------------------------------------------------------------*/

#ifndef _OS_LIBC_H_
#define	_OS_LIBC_H_

#include "os_pfs.h"

#ifdef _LANGUAGE_C_PLUS_PLUS
extern "C" {
#endif

#include <PR/ultratypes.h>

#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

/**************************************************************************
 *
 * Type definitions
 *
 */


#endif /* defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS) */

/**************************************************************************
 *
 * Global definitions
 *
 */


#if defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS)

/**************************************************************************
 *
 * Macro definitions
 *
 */


/**************************************************************************
 *
 * Extern variables
 *
 */


/**************************************************************************
 *
 * Function prototypes
 *
 */

/* byte string operations */

#ifdef TARGET_PC
#if defined(TARGET_PC)
/* Host builds: game code sees the N64 libc stubs (include/libc/string.h
 * declares these), platform code the real string.h, whose fortified macros
 * must not be shadowed by declarations here. */
#include <string.h>
#elif !defined(MODERN_CC)
extern void     bcopy(const void *, void *, int);
extern int      bcmp(const void *, const void *, int);
extern void     bzero(void *, int);
#else
extern void     bcopy(const void *, void *, size_t);
extern int      bcmp(const void *, const void *, size_t);
extern void     bzero(void *, size_t);
#endif
#else
#ifndef MODERN_CC
extern void     bcopy(const void *, void *, int);
extern int      bcmp(const void *, const void *, int);
extern void     bzero(void *, int);
#else
extern void     bcopy(const void *, void *, size_t);
extern int      bcmp(const void *, const void *, size_t);
extern void     bzero(void *, size_t);
#endif
#endif

/* Printf */

#ifdef TARGET_PC
#include <stdio.h> /* host sprintf may be a fortified macro; don't redeclare it */
#else
extern int sprintf(char *s, const char *format, ...);
#endif
extern void		osSyncPrintf(const char *fmt, ...);


#endif  /* defined(_LANGUAGE_C) || defined(_LANGUAGE_C_PLUS_PLUS) */

#ifdef _LANGUAGE_C_PLUS_PLUS
}
#endif

#endif /* !_OS_LIBC_H_ */
