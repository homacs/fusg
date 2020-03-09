/*
 * store.c
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */
#include "../../../sources/fusgd/src/store.h"

#include <unistd.h>
#include <syscall.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <libaudit.h>

#include "../../../sources/fusgd/src/fusgd.h"
#include "../../fusg-common/include/fusg/err.h"
#include "../../fusg-common/include/fusg/logging.h"
#include "../../fusg-common/include/fusg/utils.h"





typedef struct {
	const char* executable;
	const char* filepath;
	file_usage_t flags;
	uint64_t timestamp;

	//
	// intermediate parsing results
	//
	uint64_t serial;    // event serial number
	int sc; 			// syscall number
	const char* cwd;	// current wd of executable


} fusg_event_t;


int event_valid(fusg_event_t* fusg)
{
	return (   fusg->executable
			&& fusg->filepath
			&& fusg->flags);
}



void store_error(fusg_event_t* fusg, const char* fmt, ...)
{
	va_list ap;
	const size_t size = PATH_MAX*2;
	char msg[size];

    va_start(ap, fmt);
	vsnprintf(msg, size, fmt, ap);
    va_end(ap);

	log_error("store: %lu, %s", fusg->serial, msg);
}


int check_field_value(fusg_event_t* fusg, auparse_state_t *au, const char* expect_value)
{
	const char* value = auparse_interpret_field(au);
	if (strcmp(value, expect_value))
	{
		store_error(fusg, "expected value '%' but got '%s'", expect_value, value);
		return ERR_AUPARSE;
	}
	return 0;
}




int parse_syscall(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;


	int machine = -1;

	if (!auparse_first_field(au))
	{
		log_error("missing first field");
		return ERR_AUPARSE;
	}

	const char* name;
	const char* value;
	do
	{
		name = auparse_get_field_name(au);
#ifndef NDEBUG
		if (!strcmp(name, "type"))
		{
			assert (0 == check_field_value(fusg, au, "SYSCALL"));
		}
		else
#endif // NDEBUG
		if (!strcmp(name, "arch"))
		{
			value = auparse_interpret_field(au);
			if (value) machine = audit_name_to_machine(value);
		}
		else if (!strcmp(name, "syscall"))
		{
			value = auparse_interpret_field(au);
			fusg->sc = audit_name_to_syscall(value, machine);
		}
		else if (!strcmp(name, "exe"))
		{
			value = auparse_interpret_field(au);
			fusg->executable = value;
		}
		else
		{
			// ignore
		}
	} while (auparse_next_field(au) > 0);

	return rc;
}


int parse_cwd(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;

	if (!auparse_first_field(au))
	{
		log_error("missing first field");
		return ERR_AUPARSE;
	}

	const char* name;
	do
	{
		name = auparse_get_field_name(au);
#ifndef NDEBUG
		if (!strcmp(name, "type"))
		{
			if (check_field_value(fusg, au, "CWD"))
				return ERR_AUPARSE;
		}
		else
#endif // NDEBUG
		if (!strcmp(name, "cwd"))
		{
			fusg->cwd = auparse_interpret_field(au);
			break;
		}
	}
	while (auparse_next_field(au) > 0);
	return rc;
}



int parse_path(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;


	if (!auparse_first_field(au))
	{
		log_error("missing first field");
		return ERR_AUPARSE;
	}


	const char* name;
	const char* value;
	do
	{
		name = auparse_get_field_name(au);
#ifndef NDEBUG
		if (!strcmp(name, "type"))
		{
			if (check_field_value(fusg, au, "PATH"))
				return ERR_AUPARSE;
		}
		else
#endif // NDEBUG
		if (!strcmp(name, "name"))
		{
			fusg->filepath = auparse_interpret_field(au);
		}
		else if (!strcmp(name, "nametype"))
		{
			value = auparse_interpret_field(au);
			if (!strcmp(value, "PARENT"))
			{
				fusg->flags |= FUSG_EXEC;
			}
			else if (!strcmp(value, "NORMAL"))
			{
				fusg->flags |= FUSG_READ;
			}
			else if (!strcmp(value, "CREATE"))
			{
				fusg->flags |= FUSG_CREAT;
			}
			else if (!strcmp(value, "DELETE"))
			{
				fusg->flags |= FUSG_DELET;
			}
			else if (!strcmp(value, "UNKNOWN"))
			{
				// seen filename "host:+port"
				// TODO: could be a pipe to a socket
			}
			else
			{
				store_error(fusg, "unknown nametype '%s'", value);
			}
		}
		else
		{
			// ignore
		}
	} while (auparse_next_field(au) > 0);

	if (!fusg->flags) fusg->flags = FUSG_READ;

	return rc;
}


int store_event(auparse_state_t *au)
{
	int rc = 0;

	dbref_t db = global.db;
	assert(db);

	fusg_event_t fusg;
	memset(&fusg, 0, sizeof(fusg));


	const au_event_t* e = auparse_get_timestamp(au);
	if (!e)
	{
		log_warn("corrupted event: no timestamp found.");
		return ERR_AUPARSE;
	}
	fusg.serial = e->serial;
	fusg.timestamp = e->sec;


	rc = auparse_first_record(au);
	if (rc != 1)
	{
		store_error(&fusg, "first record of event not found!");
		return ERR_AUPARSE;
	}

	// if type of first record of event isn't syscall
	// then discard it. (either corrupted or not interesting)
	int record_type = auparse_get_type(au);
	if (record_type != AUDIT_SYSCALL)
		return 0;



	char filepathbuf[PATH_MAX];
	do /* iterate over all records of the event */
	{

		record_type = auparse_get_type(au);
		switch(record_type)
		{
		case AUDIT_SYSCALL:
			rc = parse_syscall(&fusg, au);
			break;
		case AUDIT_CWD:
			rc = parse_cwd(&fusg, au);
			break;
		case AUDIT_PATH:
			rc = parse_path(&fusg, au);
			if (!rc)
			{
				fusg.filepath = fabsolute(fusg.cwd, fusg.filepath, filepathbuf);
				if (event_valid(&fusg))
				{
					rc = db_update(
							global.db,
							fusg.executable,
							fusg.filepath,
							fusg.flags,
							fusg.timestamp);
				}
				else
				{
					log_warn("incomplete or corrupted audit event (serial: %lu)", e->serial);
				}
				// reset variable entries
				fusg.filepath = 0;
				fusg.flags = 0;
			}
			break;
		}

	} while (!rc && auparse_next_record(au) > 0);

	return rc;
}
