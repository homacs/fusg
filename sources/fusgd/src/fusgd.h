/*
 * global.h
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */

#ifndef FUSGD_H_
#define FUSGD_H_

#include "../../fusg-common/include/fusg/conf.h"
#include "../../fusg-common/include/fusg/db.h"

typedef enum
{
	OP_HELP,
	OP_PARSE_STDIN,
	OP_PARSE_FILE,
} fusgd_op_t;

typedef struct
{
	const char* progname;
	const char* conf_file;
	fusgd_op_t mode;

	fusg_conf_t conf;

	volatile int stop;
	volatile int received_sighup;

	char source[PATH_MAX];
	int fd;
	FILE* fin;

	dbref_t db;

	// some processing stats
	time_t last_stats_report;
	uint64_t events_processed;
	uint64_t events_stored;

} fusgd_global_t;


void reload_config(void);



extern fusgd_global_t global;



#endif /* FUSGD_H_ */
