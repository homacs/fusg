/*
 * main.c
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */


#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "fusg/conf.h"
#include "fusg/err.h"
#include "fusg/logging.h"
#include "fusg/version.h"

#include "search.h"



typedef enum {
	CMD_HELP,
	CMD_VERS,
	CMD_SEARCH_FILES,
	CMD_SEARCH_EXECS,
	CMD_DUMP,
} fusg_cmd_t;

static char* progname;
static fusg_cmd_t command;
static char** entries;
static int num_entries;
static const char* conf_file = FUSG_CONF_DEFAULT;

int read_args(int argc, char** argv)
{
	progname = argv[0];
	int rc = -1;
	int i;
	for (i = 1; i < argc; i++)
	{
		char* arg = argv[i];
		if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
		{
			command = CMD_HELP;
			rc = 0;
			break;
		}
		else if (!strcmp(arg, "-v") || !strcmp(arg, "--version"))
		{
			command = CMD_VERS;
			rc = 0;
			break;
		}
		else if (!strcmp(arg, "-c") || !strcmp(arg, "--conf"))
		{
			i++;
			if (i < argc)
			{
				conf_file = argv[i];
			}
			rc = 0;
		}
		else if (!strcmp(arg, "-f") || !strcmp(arg, "--file"))
		{
			command = CMD_SEARCH_EXECS;
			if (argc < 3)
			{
				log_error("missing files");
				command = CMD_HELP;
			}
			else
			{
				i++;
				rc = 0;
			}
			break;
		}
		else if (!strcmp(arg, "-e") || !strcmp(arg, "--exec"))
		{
			command = CMD_SEARCH_FILES;
			if (argc < 3)
			{
				log_error("missing executables");
				command = CMD_HELP;
			}
			else
			{
				i++;
				rc = 0;
			}
			break;
		}
		else if (!strcmp(arg, "-a") || !strcmp(arg, "--all"))
		{
			command = CMD_DUMP;
			rc = 0;
			break;
		}
		else
		{
			log_error("argument not supported: '%s'", arg);
			command = CMD_HELP;
			rc = -1;
			break;
		}
	}
	entries = argv+i;
	num_entries = argc-i;

	return rc;
}


int print_help()
{
	printf("\nFUSG v%s\n", FUSG_VER_STR);
	printf("  Command to query File USaGe statistics.\n");
	printf("  Part of FUSG tools.\n");
	printf("\nSYNOPSIS:\n");
	printf("fusg [OPTIONS] COMMAND FILE [FILE ...]\n");
	printf("\nCOMMANDS:\n");
	printf("  -f|--file FILE [FILE]...\n"
		   "    For each FILE list all executables, which used it.\n");
	printf("  -e|--exec EXE [EXE]...\n"
		   "    For each EXEcutable list all files, which have been\n"
		   "    used by it.\n");
	printf("  -a|--all\n"
		   "    Dump all file usage statistics found in the database.\n");
	printf("  -v|--vers:\n"
		   "    Print version and exit.\n");
	printf("  -h|--help:\n"
		   "    Print short description and exit.\n");
	printf("\nFLAGS:\n");
	printf("  -c|--conf:\n"
		   "    Provide a different config file than '/etc/fusg/fusg.conf'.\n");
	printf("\nEXAMPLES:\n");
	printf("  List executables, which used given <file>\n");
	printf("    > %s <flags> (-f|--file) <file>\n\n", progname);
	printf("  List files, which where used by given <executable>\n");
	printf("    > %s <flags> (-e|--exec) <executable>\n\n", progname);
	printf("  List the whole data base content\n");
	printf("    > %s <flags> (-a|--all)\n\n", progname);
	return 0;
}

int print_version()
{
	printf("fusg %s\n", FUSG_VER_STR);
	return 0;
}

void error_usage()
{
	print_help();
}


int do_command(void)
{
	switch(command)
	{
	case CMD_HELP:
		return print_help();
	case CMD_VERS:
		return print_version();
	case CMD_SEARCH_FILES:
		return search_files(conf_file, num_entries, entries);
	case CMD_SEARCH_EXECS:
		return search_execs(conf_file, num_entries, entries);
	case CMD_DUMP:
		return search_dump_all(conf_file);
	default:
		return ERR_UNKNOWN;
	}

}



int main(int argc, char** argv)
{
	int rc = EXIT_SUCCESS;

	logging_init(argv[0]);

	if (-1 == read_args(argc, argv))
	{
		error_usage();
		rc = ERR_USAGE;
	}
	else
	{
		rc = do_command();
	}

	logging_finalize();
	return rc;
}
