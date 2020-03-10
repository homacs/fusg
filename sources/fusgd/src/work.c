/*
 * work.c
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>
#include <libaudit.h>
#include <auparse.h>

#include "../../fusg-common/include/fusg/logging.h"
#include "../../fusg-common/include/fusg/db.h"
#include "../../../sources/fusgd/src/fusgd.h"
#include "../../fusg-common/include/fusg/err.h"

#include "../../../sources/fusgd/src/trace.h"
#include "../../../sources/fusgd/src/store.h"

static auparse_state_t *au = NULL;

static time_t time_last_flush = 0;
static time_t time_flush_period = 1; // db-flush every n secs

/* Local declarations */
static void handle_event(auparse_state_t *au, auparse_cb_event_t cb_event_type,
		void *user_data);




static void periodic_db_flush()
{

	time_t now;
	time(&now);
	time_t duration = now - time_last_flush;
	if (duration > time_flush_period)
	{
		time_last_flush = now;
		db_flush(global.db);
	}
}



int work()
{
	char tmp[MAX_AUDIT_MESSAGE_LENGTH + 1];

	int rc = 0;
	au = auparse_init(AUSOURCE_FEED, 0);
	if (au == NULL) {
		log_fatal("exiting due to auparse init errors");
		return ERR_AUPARSE;
	}
	// homac: indicate that events are raw
	auparse_set_escape_mode(au, AUPARSE_ESC_RAW);
	auparse_add_callback(au, handle_event, NULL, NULL);


	do {
		fd_set read_mask;
		struct timeval tv;
		int retval = -1;

		/* Load configuration */
		if (global.received_sighup)
		{
			// SIGHUP means: reload config
			reload_config();
		}

		if (global.mode == OP_PARSE_FILE)
		{
			// we have always data, when reading files
			retval = 1;
		}
		else // --> reads stdin
		{
			// the regular path, which does not work with regular files:
			// - wait for input at fd (usually stdin)
			// - every 3 seconds, check incomplete but queued events to have
			//   reached complete time. If they have reached complete time
			//   they will be processed through the callback handler.
			int retry = 0;
			do {

				tv.tv_sec = time_flush_period;
				tv.tv_usec = 0;
				FD_ZERO(&read_mask);
				FD_SET(global.fd, &read_mask);
				retval = select(1, &read_mask, NULL, NULL, &tv);

				// retry if select was just interrupted by a signal
				// handler and neither sighup nor stop is set.
				retry = (retval == -1 && errno == EINTR && !global.received_sighup && !global.stop);
			} while (retry);
		}

		if (!global.stop && !global.received_sighup && retval > 0) {
			//
			// we have got a new record -> process
			//


			// NOTE: this has to be fgets, because the given len in the header is
			//       wrong.
			if (fgets_unlocked(tmp, MAX_AUDIT_MESSAGE_LENGTH, global.fin))
			{
				auparse_feed(au, tmp, strnlen(tmp, MAX_AUDIT_MESSAGE_LENGTH));
			}
			else
			{
				if (!feof(global.fin)) log_error("audit message stream corrupted?");
			}
		} else if (retval == 0) {
			//
			// select() timed out.
			//

			// if we still have pending events
			// they might never get finished.
			// Aging mechanism avoids them getting
			// stuck in memory.
			if (auparse_feed_has_data(au))
				auparse_feed_age_events(au);

			auparse_flush_feed(au);

			// since we are waiting check if we can
			// flush changes in db to file system.
			periodic_db_flush();
		}
	} while (!feof(global.fin) && !global.stop);

	// flush any accumulated events from queue
	auparse_flush_feed(au);
	auparse_destroy(au);

	// do a final explicit flush
	db_flush(global.db);

	return rc;
}



/* This function receives a single complete event at a time from the auparse
 * library. This is where the main analysis code would be added. */
static void handle_event(auparse_state_t *au, auparse_cb_event_t cb_event_type,
		void *user_data) {
	int rc;
	if (cb_event_type != AUPARSE_CB_EVENT_READY)
		return;

	global.events_processed++;

	trace_whole_event_interpreted(au);
	rc = store_event(au);
	if (rc)
	{
		log_error("rc=%d, errno: %s", rc, strerror(errno));
	}

	if (0 == global.events_processed % 1000)
	{
		time_t now; time(&now);
		time_t duration = now - global.last_stats_report;
		global.last_stats_report = now;
		log_info("statistics report:");
		log_info("\ttime since last report: %lu s", duration);
		log_info("\tprocessed events: %d", global.events_processed);
		log_info("\tstored events: %d", global.events_stored);
	}

}

