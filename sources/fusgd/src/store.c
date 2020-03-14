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
	int syscall_number; 			// syscall number
	int syscall_success; 		// whether syscall was successful
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
	if (value && strcmp(value, expect_value))
	{
		store_error(fusg, "expected value '%' but got '%s'", expect_value, value);
		return ERR_AUPARSE;
	}
	return 0;
}




int parse_syscall(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;

	fusg->executable = NULL;


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
		if (!strcmp(name, "syscall"))
		{
			fusg->syscall_number = auparse_get_field_int(au);
		}
		else if (!strcmp(name, "success"))
		{
			value = auparse_get_field_str(au);
			if (!value) return ERR_AUPARSE;
			else if (!strcmp(value, "yes"))
			{
				fusg->syscall_success = 1;
			}
			else if (!strcmp(value, "no"))
			{
				fusg->syscall_success = 0;
			}
			else
			{
				return ERR_AUPARSE;
			}
		}
		else if (!strcmp(name, "exe"))
		{
			// NOTE: auparse tries realpath(path) but returns original in case of errno!=0
			value = auparse_interpret_field(au);
			fusg->executable = value;
		}
		else
		{
			// ignore
		}
	} while (auparse_next_field(au) > 0);

	return fusg->executable ? rc : ERR_AUPARSE;
}


int parse_cwd(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;

	fusg->cwd = NULL;

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
			// NOTE: auparse tries realpath(path) but returns original in case of errno!=0
			fusg->cwd = auparse_interpret_field(au);
			break;
		}
	} while (auparse_next_field(au) > 0);

	return fusg->cwd ? rc : ERR_AUPARSE;
}



int parse_path(fusg_event_t* fusg, auparse_state_t *au)
{
	int rc = 0;

	fusg->filepath = NULL;

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
			// NOTE: auparse tries realpath(path) but returns original in case of errno!=0
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

	return fusg->filepath ? rc : ERR_AUPARSE;
}


int store_event(auparse_state_t *au)
{
	int rc = 0;
	int stored = 0; // true if anything of this event was stored

	dbref_t db = global.db;
	assert(db);

	fusg_event_t fusg;
	memset(&fusg, 0, sizeof(fusg_event_t));


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

	int skip = 0;

	char filepathbuf[PATH_MAX];
	do /* iterate over all records of the event */
	{

		record_type = auparse_get_type(au);
		switch(record_type)
		{
		case AUDIT_SYSCALL:
			rc = parse_syscall(&fusg, au);
			if (!fusg.syscall_success)
			{
				skip = 1;
			}
			break;
		case AUDIT_CWD:
			rc = parse_cwd(&fusg, au);
			break;
		case AUDIT_PATH:
			rc = parse_path(&fusg, au);
			if (!rc)
			{

				// So we have to be very careful about the data to be expected.
				fusg.filepath = fabsolute(fusg.cwd, fusg.filepath, filepathbuf);
				if (event_valid(&fusg))
				{
					rc = db_update(
							global.db,
							fusg.executable,
							fusg.filepath,
							fusg.flags,
							fusg.timestamp);
					stored = 1;
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

	} while (!rc && !skip && auparse_next_record(au) > 0);

	if (!rc && stored) global.events_stored++;

	return rc;
}
