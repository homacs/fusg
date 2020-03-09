/*
 * logging.h
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_LOGGING_H_
#define FUSG_LOGGING_H_

#include <stdio.h>

typedef enum
{
	LL_DEBUG = 0,
	LL_INFO,
	LL_WARN,
	LL_ERROR,
	LL_FATAL,
	LL_IGNORE,
} loglevel_t;



/**
 * Setup a fallback logging facility to
 * be available during startup.
 *
 * Function chooses a suitable log sink
 * considering whether we have a tty or not
 * (console output). Init syslog for no
 * tty and console otherwise.
 *
 * @param progname is used when logging to syslog
 */
int logging_init(const char* progname);

/**
 * set loglevels for the different logging adapters (console, file and syslog).
 *
 * log levels can be set before logging_init().
 *
 * If a loglevel is defined as LL_IGNORE, the previous value will be kept.
 *
 */
void logging_set_log_levels(loglevel_t console_max_level, loglevel_t file_max_level, loglevel_t syslog_max_level);

/**
 * setup logging into a log file.
 * @param chain (chain==0) replaces logger, (chain!=0) appends logger to a chain
 * @see logging_finalize()
 */
int logging_setup_file(FILE* log_file, int chain);

/**
 * setup logging to log to console only, using stdout and stderr respectively.
 *
 * Logging to console is the default log output when the process
 * was first started.
 *
 * @param chain (chain==0) replaces logger, (chain!=0) appends logger to a chain
 *
 * @see logging_finalize()
 */
int logging_setup_console(int chain);

/**
 * setup logging to log to syslog and console (if available).
 * @param chain (chain==0) replaces logger, (chain!=0) appends logger to a chain
 * @see logging_finalize()
 */
int logging_setup_syslog(const char* ident, int chain);

/**
 * shutdown logging facility.
 *
 * Call this function in case of an ordinary shutdown of
 * your process. It flushes output and closes syslog if
 * it was open.
 *
 * Any output logged after a call to logging_finalize()
 * will be redirected to console.
 *
 * logging can be reestablished by using any of the
 * logging_setup_xxx() functions.
 *
 */
void logging_finalize(void);

/** Log a debug message. */
void log_debug(const char *fmt, ...);
/** Log a info message. */
void log_info(const char *fmt, ...);
/** Log a warning message. */
void log_warn(const char *fmt, ...);
/** Log a error message. */
void log_error(const char *fmt, ...);
/** Log a fatal message. */
void log_fatal(const char *fmt, ...);


#endif /* FUSG_LOGGING_H_ */
