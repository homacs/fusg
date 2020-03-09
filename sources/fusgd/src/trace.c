/*
 * debug-tools.h
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_DEBUG_TOOLS_H_
#define FUSG_TRACE_H_

#include <stdio.h>


#include "../../fusg-common/include/fusg/logging.h"

#include "libaudit.h"
#include "auparse.h"

FILE* trace_out = 0;

FILE* trace_set(FILE* fout)
{
	FILE* previous = trace_out;
	trace_out = fout;
	return previous;
}

/* This function shows how to dump a whole event by iterating over records */
void trace_whole_event(auparse_state_t *au) {
	if (!trace_out) return;
	auparse_first_record(au);
	do {
		fprintf(trace_out, "%s\n", auparse_get_record_text(au));
	} while (auparse_next_record(au) > 0);
	fprintf(trace_out, "\n");
}

/* This function shows how to dump a whole record's text */
void trace_whole_record(auparse_state_t *au) {
	if (!trace_out) return;
	fprintf(trace_out, "%s: %s\n", audit_msg_type_to_name(auparse_get_type(au)),
			auparse_get_record_text(au));
	fprintf(trace_out, "\n");
}

/* This function shows how to iterate through the fields of a record
 * and print its name and raw value and interpretted value. */
void trace_fields_of_record(auparse_state_t *au) {
	if (!trace_out) return;
	fprintf(trace_out, "record type %d(%s) has %d fields\n", auparse_get_type(au),
			audit_msg_type_to_name(auparse_get_type(au)),
			auparse_get_num_fields(au));

	fprintf(trace_out, "line=%d file=%s\n", auparse_get_line_number(au),
			auparse_get_filename(au) ? auparse_get_filename(au) : "stdin");

	const au_event_t *e = auparse_get_timestamp(au);
	if (e == NULL) {
		fprintf(trace_out, "Error getting timestamp - aborting\n");
		return;
	}
	/* Note that e->sec can be treated as time_t data if you want
	 * something a little more readable */
	fprintf(trace_out, "event time: %u.%u:%lu, host=%s\n", (unsigned) e->sec, e->milli,
			e->serial, e->host ? e->host : "?");
	auparse_first_field(au);

	do {
		fprintf(trace_out, "field: %s=%s (%s)\n", auparse_get_field_name(au),
				auparse_get_field_str(au), auparse_interpret_field(au));
	} while (auparse_next_field(au) > 0);
	fprintf(trace_out, "\n");
}


void trace_fields_interpreted(auparse_state_t *au) {
	if (!trace_out) return;
	if (auparse_first_field(au)) do
	{
		fprintf(trace_out, " %s=\"%s\"",
				auparse_get_field_name(au),
				auparse_interpret_field(au));
	}
	while (auparse_next_field(au) > 0);
}


/* This function shows how to dump a whole event by iterating over records */
void trace_fields_of_record_interpreted(auparse_state_t *au) {
	if (!trace_out) return;

	const au_event_t *e = auparse_get_timestamp(au);
	if (e == NULL) {
		log_warn("trace: getting timestamp - aborting");
		return;
	}

	/* Note that e->sec can be treated as time_t data if you want
	 * something a little more readable */
	fprintf(trace_out, "%u.%u:%lu:", (unsigned) e->sec, e->milli, e->serial);

	trace_fields_interpreted(au);

	fprintf(trace_out, "\n");
}

/* This function shows how to dump a whole event by iterating over records */
void trace_whole_event_interpreted(auparse_state_t *au) {
	if (!trace_out) return;
	int rc = auparse_first_record(au);

	if (rc != 1)
	{
		log_warn("trace: First record of event not found!");
		return;
	}

	const au_event_t *e = auparse_get_timestamp(au);
	if (e == NULL)
	{
		log_warn("trace: Couldn't find timestamp");
		return;
	}

	/* Note that e->sec can be treated as time_t data if you want
	 * something a little more readable */
	fprintf(trace_out, "%u.%u:%lu:\n", (unsigned) e->sec, e->milli, e->serial);
	fprintf(trace_out, "\n");

	do {
		fprintf(trace_out, "\t");
		trace_fields_interpreted(au);
		fprintf(trace_out, "\n");

	} while (auparse_next_record(au) > 0);
	fprintf(trace_out, "\n");
    fflush(trace_out);

}




#endif /* FUSG_DEBUG_TOOLS_H_ */
