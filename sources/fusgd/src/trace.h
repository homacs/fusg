/*
 * debug-tools.h
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_TRACE_H_
#define FUSG_TRACE_H_

#include <stdio.h>

#include "libaudit.h"
#include "auparse.h"


FILE* trace_set(FILE* fout);

/* This function shows how to dump a whole event by iterating over records */
void trace_whole_event(auparse_state_t *au);

/* This function shows how to dump a whole record's text */
void trace_whole_record(auparse_state_t *au);

/* This function shows how to iterate through the fields of a record
 * and print its name and raw value and interpretted value. */
void trace_fields_of_record(auparse_state_t *au);

void trace_fields_interpreted(auparse_state_t *au);


/* This function shows how to dump a whole event by iterating over records */
void trace_fields_of_record_interpreted(auparse_state_t *au);

/* This function shows how to dump a whole event by iterating over records */
void trace_whole_event_interpreted(auparse_state_t *au);


#endif /* FUSG_TRACE_H_ */
