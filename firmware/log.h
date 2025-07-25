#ifndef LOG_H
#define	LOG_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stddef.h>

#include "config/default/system/console/sys_console.h"

#define LOG_CONSOLE_ACTIVE
#define DEBUG_CONSOLE_ACTIVE
#define DUMP_CONSOLE_ACTIVE

typedef enum {
    DUMP_MODE_HEX = 0,
    DUMP_MODE_CHAR,
    DUMP_MODE_MIX,
} t_DUMP_MODES;

#ifdef	__cplusplus
extern "C" {
#endif

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */
    /* ************************************************************************** */


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */

#if defined(LOG_CONSOLE_ACTIVE) || defined(LOG_SYSLOG_ACTIVE)
#define _l(format, ...)     SYS_CONSOLE_Print(SYS_CONSOLE_DEFAULT_INSTANCE, format, ##__VA_ARGS__)
#else
#define _l(format, ...)
#endif

#if defined(DEBUG_CONSOLE_ACTIVE) || defined(DEBUG_SYSLOG_ACTIVE)
#define _d(format, ...)     SYS_CONSOLE_Print(SYS_CONSOLE_DEFAULT_INSTANCE, "{DEBUG} "format, ##__VA_ARGS__)
#else
#define _d(format, ...)
#endif

    void dump_data(char *rawBytes, size_t nBytes, t_DUMP_MODES mode);

#if defined(DUMP_CONSOLE_ACTIVE)
#define _dump(rawBytes, nBytes, mode)  dump_data((char*)rawBytes, nBytes, mode)
#else
#define _dump(rawBytes, nBytes, mode)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* LOG_H */

