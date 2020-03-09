



#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "fusg/db.h"
#include "fusg/err.h"
#include "fusg/logging.h"
#include "fusg/version.h"

#include "fusgd.h"
#include "trace.h"
#include "work.h"


#define FUSGD_NAME "fusgd"


static void term_handler(int sig);
static void hup_handler(int sig);

static int read_args(int argc, char** argv);
static void print_usage(void);



fusgd_global_t global;

// FIXME: need a different approach: too slow to keep up in some cases.
//        Examples: find and eclipse startup

int main(int argc, char *argv[]) {
	int rc = 0;
	FILE* fusgd_trace = 0;
	FILE* fusgd_log = 0;

	// init global variables
	memset(&global, 0, sizeof(global));
	global.fin = stdin;
	global.fd = 0;
	time(&global.last_stats_report);

	// setup initial logging facility (either console or syslog)
	logging_set_log_levels(LL_INFO, LL_DEBUG, LL_INFO);
	logging_init(FUSGD_NAME);

	// read arguments and config
	rc = read_args(argc, argv);
	if (!rc) fusg_conf_read(&global.conf, global.conf_file);

	//
	// shortcut if we just want to output help
	//
	if (global.mode == OP_HELP)
	{
		print_usage();
		return rc;
	}

	// open log file
	if (global.conf.fusgd_log[0])
	{
		fusgd_log = fopen(global.conf.fusgd_log, "w");
		if (fusgd_log)
		{
			logging_setup_file(fusgd_log, 1);
			log_info("%s v%s", FUSGD_NAME, FUSG_VER_STR);
			log_info("adjusting log levels");
			log_info("for detailed log messages see: '%s'",  global.conf.fusgd_log);
			logging_set_log_levels(LL_ERROR, LL_DEBUG, LL_ERROR);
		}
		else
		{
			log_info("%s v%s", FUSGD_NAME, FUSG_VER_STR);
			log_warn("open '%s': %s", global.conf.fusgd_log, strerror(errno));
		}
	}

	log_info("db_path: '%s'", global.conf.db_path);
	log_info("fusg_log: '%s'", global.conf.fusgd_log);
	log_info("fusg_trace: '%s'", global.conf.fusgd_trace);


	if (global.mode == OP_PARSE_FILE)
	{
		global.fin = fopen(global.source, "rm");
		if (!global.fin) {
			log_error("can't open input log file '%s'\n\t%s", global.source, strerror(errno));
			rc = ERR_CONF;
			goto bail;
		} else {
			global.fd = fileno(global.fin);
		}
	}

	log_info("starting up");


	//
	// setup event tracing
	//
	if (global.conf.fusgd_trace[0])
	{
		fusgd_trace = fopen(global.conf.fusgd_trace, "w");
		if (fusgd_trace)
		{
			trace_set(fusgd_trace);
			log_info("event tracing enabled: '%s'", global.conf.fusgd_trace);
		}
		else log_error("can't setup debug log");
	}
	else
	{
		trace_set(stdout);
	}

	//
	// setup signal handlers
	//
	struct sigaction sa;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = term_handler;
	sigaction(SIGTERM, &sa, NULL);
	sa.sa_handler = hup_handler;
	sigaction(SIGHUP, &sa, NULL);

	//
	// open db
	//
	global.db = db_open(global.conf.db_path, DB_WRITE);


	//
	// start serving
	//
	log_info("start processing events");
	work();
	log_info("finished processing events");

	//
	// shutdown
	//
	if (global.stop)
		log_info("exiting on stop request");
	else
		log_info("exiting on EOF");


bail:

	db_close(global.db);

	if (fusgd_trace)
		fclose(fusgd_trace);


	log_info("exit code: %d", rc);


	if (fusgd_log)
		fclose(fusgd_log);


	return rc;
}



static int read_args(int argc, char** argv)
{
	// set defaults
	global.progname = argv[0];
	global.mode = OP_PARSE_STDIN;
	global.conf_file = FUSG_CONF_DEFAULT;

	// parse arguments
	int rc = 0;
	int i;
	for (i = 1; i < argc; i++)
	{
		char* arg = argv[i];
		if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
		{
			global.mode = OP_HELP;
		}
		else if (!strcmp(arg, "-c") || !strcmp(arg, "--conf"))
		{
			i++;
			if (i < argc)
			{
				global.conf_file = argv[i];
			}
		}
		else if (!strcmp(arg, "-r") || !strcmp(arg, "--read"))
		{
			global.mode = OP_PARSE_FILE;
			i++;
			if (i >= argc)
			{
				log_error("missing log file to read");
				global.mode = OP_HELP;
			}
			else
			{
				snprintf(global.source, PATH_MAX, "%s", argv[i]);
			}
			break;
		}
		else
		{
			log_error("missing arguments");
			global.mode = OP_HELP;
			rc = ERR_USAGE;
		}
	}


	return rc;
}

static void print_usage(void)
{
	printf("> %s <flags>\n", global.progname);
	printf("-c | --conf <fusg.conf>\n"
			"\tread config from given path <fusg.conf>.\n");
	printf("-r | --read <file>\n"
			"\tread file instead of stdin.\n");
}


static void term_handler(int sig)
{
	global.stop = 1;
}

static void hup_handler(int sig)
{
	global.received_sighup = 1;
}

void reload_config(void)
{
	// TODO: reload config here
	global.received_sighup = 0;
}

