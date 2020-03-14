/*
 * system.c
 *
 *  Created on: 14 Mar 2020
 *      Author: homac
 */

#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "fusg/system.h"


/**
 *
 * NOTE: a core dump size of 0 does not mean, that there will be no core dump.
 * If a core pattern is set, which starts with '|', the core dump is piped to
 * the program denoted in /proc/sys/kernel/core_pattern.
 *
 * @return Returns maximum size of a core dump in user space or -1 on error (errno is set).
 */
rlim_t system_coredump_size(void)
{
	rlim_t size = RLIM_INFINITY;

#define tuple_min(v1,v2)     ( (v1 < v2) ? v1 : v2 )
#define triple_min(v1,v2,v3) ( (v1 < tuple_min(v2,v3)) ? v1 : tuple_min(v2,v3) )


	struct rlimit limit;
	// check max size of core dumps
	int rc = getrlimit(RLIMIT_CORE, &limit);
	if (rc < 0)
		return rc;
	else
		size = tuple_min(limit.rlim_cur,limit.rlim_max);

	// maximum file size may be limited too
	rc = getrlimit(RLIMIT_FSIZE, &limit);
	if (rc < 0)
		return rc;
	else
		size = triple_min(size, limit.rlim_cur, limit.rlim_max);

#undef tuple_min
#undef triple_min
	return size;
}

ssize_t system_coredump_pattern(char *result_buffer, ssize_t result_buffer_size)
{
	int fd = open("/proc/sys/kernel/core_pattern", O_RDONLY);
	if (fd<0) return fd;
	ssize_t len = read(fd, result_buffer, result_buffer_size);
	if (len >= result_buffer_size) len = result_buffer_size-1;
	if (len >= 0)
	{
		if (len > 0 && result_buffer[len-1] == '\n')
		{
			len--;
		}
		result_buffer[len] = '\0';
		len++;
	}
	close(fd);
	return len;
}
