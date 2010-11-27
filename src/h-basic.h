/** \file h-basic.h 
    \brief Lowest-level include file 

 *
 * The most basic "include" file.
 *
 * This file simply includes other low level header files.
 */

#ifndef INCLUDED_H_BASIC_H
#define INCLUDED_H_BASIC_H

/* Autoconf Support */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif /* HAVE_CONFIG_H */

/* System Configuration */
#include "h-config.h"

/* System includes/externs */
#include "h-system.h"

/* Basic types */
#include "h-type.h"

/* Basic constants and macros */
#include "h-define.h"

#endif


