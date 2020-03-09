/*
 * logging.c
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <syslog.h>

#include <assert.h>


#include "fusg/logging.h"

const char* LL_TAB[] =
{
	"debug",
	"info",
	"warn",
	"error",
	"fatal",
	"",
};

#define _S(__LL__) (LL_TAB[__LL__])

static loglevel_t syslog_loglevel  = LL_DEBUG;
static loglevel_t console_loglevel = LL_DEBUG;
static loglevel_t file_loglevel    = LL_DEBUG;


static const int LOG_MAX_MSG_LEN = PATH_MAX * 2 + NAME_MAX;


typedef void (*logging_function_v)(loglevel_t level, const char* fmt, va_list ap);
void log_to_chain_v(loglevel_t level, const char* fmt, va_list ap);
void log_to_syslog_v(loglevel_t level, const char* fmt, va_list ap);
void log_to_file_v(loglevel_t level, const char* fmt, va_list ap);
void log_to_console_v(loglevel_t level, const char* fmt, va_list ap);

typedef void (*logging_function)(loglevel_t level, const char* message);
void log_to_syslog(loglevel_t level, const char* msg);
void log_to_file(loglevel_t level, const char* msg);
void log_to_console(loglevel_t level, const char* msg);

logging_function logging_get_function(logging_function_v logger);
logging_function_v logging_get_function_v(logging_function logger);

static FILE* logging_file = 0;
static logging_function_v logging = log_to_console_v;

static size_t logging_chain_size = 0;
static const size_t logging_chain_max = 8;
static logging_function logging_chain[] = {0,0,0,0,0,0,0,0,};

void logging_chain_init(void);
void logging_chain_append(logging_function logger);
void logging_chain_clear(void);


// TODO: missing need time stamps in output (console/file)

void logging_set_log_levels(
		loglevel_t console_max_level,
		loglevel_t file_max_level,
		loglevel_t syslog_max_level)
{
	console_loglevel = console_max_level;
	file_loglevel = file_max_level;
	syslog_loglevel = syslog_max_level;
}

int logging_init(const char* progname)
{
	int have_tty = isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
	if (!have_tty)
	{
		return logging_setup_syslog(progname, 0);
	}
	else
	{
		return logging_setup_console(0);
	}
}



int logging_setup_file(FILE* log_file, int chain)
{
	if (chain) logging_chain_init();
	else logging_finalize();

	logging_file = log_file;

	if (chain) logging_chain_append(log_to_file);
	else logging = log_to_file_v;
	return 0;
}

int logging_setup_console(int chain)
{
	if (chain) logging_chain_init();
	else logging_finalize();

	if (chain) logging_chain_append(log_to_console);
	else logging = log_to_console_v;
	return 0;
}

int logging_setup_syslog(const char* ident, int chain)
{
	if (chain) logging_chain_init();
	else logging_finalize();

	openlog(ident, LOG_PERROR | LOG_PID, LOG_DAEMON);

	if (chain) logging_chain_append(log_to_syslog);
	else logging = log_to_syslog_v;
	return 0;
}

void logging_finalize_that_v(logging_function_v f);

void logging_finalize_that(logging_function f)
{
	logging_finalize_that_v(logging_get_function_v(f));
}

void logging_finalize_that_v(logging_function_v f)
{
	if (f == log_to_file_v)
	{
		fflush(logging_file);
	}
	else if (f == log_to_console_v)
	{
		fflush(stderr);
		fflush(stdout);
	}
	else if (f == log_to_syslog_v)
	{
		closelog();
	}
}


void logging_finalize(void)
{
	if (logging == log_to_chain_v)
	{
		for (int i = 0; i < logging_chain_size; i++)
		{
			if (!logging_chain[i]) break;
			else logging_finalize_that(logging_chain[i]);
		}
		logging_chain_clear();
	}
	else
	{
		logging_finalize_that_v(logging);
	}
	logging = log_to_console_v;
}

char* log_format_message(char* buffer, size_t buffer_size, const char* fmt, va_list ap)
{
	size_t size = 0;
    /* Determine required size */

    size = vsnprintf(buffer, buffer_size, fmt, ap);

    if (size < 0)
        return NULL;

    if (size >= buffer_size)
    {
    	// output has been truncated
    	buffer[buffer_size-1] = '\0';
    }
    return buffer;
}

void log_debug(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	logging(LL_DEBUG, fmt, ap);
    va_end(ap);
}

void log_info(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	logging(LL_INFO, fmt, ap);
    va_end(ap);
}

void log_warn(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	logging(LL_WARN, fmt, ap);
    va_end(ap);
}

void log_error(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	logging(LL_ERROR, fmt, ap);
    va_end(ap);
}

void log_fatal(const char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	logging(LL_FATAL, fmt, ap);
    va_end(ap);
}



int log_get_syslog_level(loglevel_t level) {
	switch (level) {
	case LL_DEBUG: return LOG_DEBUG;
	case LL_INFO:  return LOG_INFO;
	case LL_WARN:  return LOG_WARNING;
	case LL_ERROR: return LOG_ERR;
	case LL_FATAL: return LOG_CRIT;
	default:       return -1;
	}
}

void log_to_syslog(loglevel_t level, const char* message)
{
    if (level < syslog_loglevel) return;
	int syslog_facility = LOG_DAEMON; // system daemons without separate facility value
    int syslog_level = log_get_syslog_level(level);
    if (syslog_level < 0)
    {
    	syslog(LOG_WARNING | syslog_facility, "no mapping for log level '%s' defaulting to 'error'", _S(level));
    	syslog_level = LOG_ERR;
    }
   	syslog(syslog_level | syslog_facility, "%s", message);
}

void log_to_syslog_v(loglevel_t level, const char* fmt, va_list ap)
{
    if (level < syslog_loglevel) return;

    char message[LOG_MAX_MSG_LEN];
    log_format_message(message, LOG_MAX_MSG_LEN, fmt, ap);

    log_to_syslog(level, message);
}

void log_to_file(loglevel_t level, const char* message)
{
    if (level < file_loglevel) return;
    fprintf(logging_file, "%s: %s\n", _S(level), message);
    fflush(logging_file);
}

void log_to_file_v(loglevel_t level, const char* fmt, va_list ap)
{
    if (level < file_loglevel) return;

    char message[LOG_MAX_MSG_LEN];
    log_format_message(message, LOG_MAX_MSG_LEN, fmt, ap);
    log_to_file(level, message);
}


void log_to_console(loglevel_t level, const char* message)
{
    if (level < console_loglevel) return;
    if (level < LL_WARN)
    {
		fprintf(stdout, "%s: %s\n", _S(level), message);
    }
    else if (level <= LL_FATAL)
    {
		fprintf(stderr, "%s: %s\n", _S(level), message);
    }
    else
    {
    	fprintf(stderr, "no mapping for log level '%s' defaulting to stderr", _S(level));
		fprintf(stderr, "%s: %s\n", _S(level), message);
    }
}

void log_to_console_v(loglevel_t level, const char* fmt, va_list ap)
{
    if (level < console_loglevel) return;

    char message[LOG_MAX_MSG_LEN];
    log_format_message(message, LOG_MAX_MSG_LEN, fmt, ap);
    log_to_console(level, message);
}

void log_to_chain_v(loglevel_t level, const char* fmt, va_list ap)
{
    char message[LOG_MAX_MSG_LEN];
    log_format_message(message, LOG_MAX_MSG_LEN, fmt, ap);

	for (int i = 0; i < logging_chain_size; i++)
	{
		logging_chain[i](level, message);
	}
}

logging_function logging_get_function(logging_function_v logger)
{
	if (logger == log_to_console_v) return log_to_console;
	else if (logger == log_to_file_v) return log_to_file;
	else if (logger == log_to_syslog_v) return log_to_syslog;
	else return 0;
}

logging_function_v logging_get_function_v(logging_function logger)
{
	if (logger == log_to_console) return log_to_console_v;
	else if (logger == log_to_file) return log_to_file_v;
	else if (logger == log_to_syslog) return log_to_syslog_v;
	else return 0;
}



void logging_chain_init(void)
{
	if (logging != log_to_chain_v)
	{
		logging_chain[0] = logging_get_function(logging);
		logging = log_to_chain_v;
		logging_chain_size++;
	}
}

void logging_chain_append(logging_function logger)
{
	// if hit, then chain_init was not called
	assert(logging == log_to_chain_v);

	// cannot chain more than 8 loggers
	assert(logging_chain_size < logging_chain_max);

	logging_chain[logging_chain_size] = logger;
	logging_chain_size++;
}

void logging_chain_clear(void)
{
	logging_chain_size = 0;
	logging_chain[0] = 0;
}

