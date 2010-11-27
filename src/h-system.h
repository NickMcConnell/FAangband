/** \file h-system.h 
    \brief Include the basic "system" files.
 
 * Make sure all "system" constants/macros are defined.
 * Make sure all "system" functions have "extern" declarations.
 *
 * This file is a big hack to make other files less of a hack.
 * This file has been rebuilt -- it may need a little more work.
 */

#ifndef INCLUDED_H_SYSTEM_H
#define INCLUDED_H_SYSTEM_H


/*** ANSI C headers ***/

#include <ctype.h>
#include <assert.h>

#ifdef _WIN32_WCE
/* Non-standard crap */
#else
#include <errno.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#if defined(NeXT) /* More non-standard crap */
# include <libc.h>
#else
# include <stdlib.h>
#endif

#include <string.h>

#ifdef _WIN32_WCE
/* Even more non-standard crap */
#else
#include <time.h>
#endif

/*** POSIX headers ***/

#if !defined(NeXT) && !defined(RISCOS)
#ifdef _WIN32_WCE
#else
# include <fcntl.h>
#endif
#endif


#if defined (SET_UID) || defined (MACH_O_CARBON)
# include <pwd.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

#ifdef SET_UID
# include <sys/types.h>
#endif

#if defined(__DJGPP__) || defined(__MWERKS__)
#include <unistd.h>
#endif /* __DJGPP__ || __MWERKS__ */

/*** Other headers ***/

#if defined(MACINTOSH) && defined(__MWERKS__)
# include <unix.h>
#endif

#if defined(WINDOWS) || defined(MSDOS) || defined(USE_EMX)
#ifdef _WIN32_WCE
#else
# include <io.h>
#endif
#endif


#ifdef SET_UID

# ifndef HAVE_USLEEP

/*
 * struct timeval in usleep requires sys/time.h
 *
 * System test removed since Unix systems that neither have usleep nor
 * sys/time.h are screwed anyway, since they have no way of delaying.
 */
#  include <sys/time.h>
# endif /* HAVE_USLEEP */

#endif /* SET_UID */

#endif /* INCLUDED_H_SYSTEM_H */


