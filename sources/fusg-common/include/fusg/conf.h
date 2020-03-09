/*
 * config.h
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_CONF_H_
#define FUSG_CONF_H_


#include <limits.h>


#define FUSG_CONF_DEFAULT "/etc/fusg/fusg.conf"

#define FUSG_LOGPATH_DEFAULT "/var/log/fusg/fusgd.log"
#define FUSG_TRACEPATH_DEFAULT ""
#define FUSG_DBPATH_DEFAULT "/var/fusg/db"

typedef struct {
	char fusgd_log[PATH_MAX];
	char fusgd_trace[PATH_MAX];
	char db_path[PATH_MAX];
} fusg_conf_t;

int fusg_conf_read(fusg_conf_t* conf, const char* path);

#endif /* FUSG_CONF_H_ */
