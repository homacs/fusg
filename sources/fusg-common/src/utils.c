/*
 * utils.c
 *
 *  Created on: 8 Mar 2020
 *      Author: homac
 */


#include "fusg/utils.h"

const char* fwhich(char* buffer, const char* command)
{
	int is_exec = 0;


	assert (buffer && command && command[0]);


	const char* PATH = getenv("PATH");
	const char sep = ':';
	const char* p = PATH;
	char* b = buffer;
	while (p && *p)
	{
		// skip to start
		while (*p == sep) p++;

		// copy until end of path variable entry
		for (b = buffer; *p && *p != sep; b++, p++) *b = *p;
		snprintf(b, PATH_MAX - (buffer - b), "/%s", command);
		if (buffer[0] && fisfile(buffer) && 0 == eaccess(buffer, R_OK|X_OK))
		{
			// yes
			is_exec = 1;
			break;
		}
	}
	return (is_exec) ? buffer : NULL;
}
