/*
 * system.h
 *
 *  Created on: 14 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_COMMON_INCLUDE_FUSG_SYSTEM_H_
#define FUSG_COMMON_INCLUDE_FUSG_SYSTEM_H_

#include <sys/resource.h>

/**
 * Retrieves the currently set maximum size of core dumps
 * by checking entries of getrlimit(2) and returns it.
 * NOTE: A maximum size of 0, does not imply, that there will
 * be no core dumps processed. Core dumps can be instead piped
 * to a program, which may or may not write the core dump to
 * the file system. This can be determined by checking the
 * kernel.core_pattern entry in sysctl. Use function
 * system_coredump_pattern() to figure it out.
 *
 *
 * @return if size >= 0 ok, otherwise error. Size can be
 * RLIM_INFINITY, which stands for unlimited size.
 */
rlim_t system_coredump_size(void);

/**
 * Writes the kernel.core_pattern entry (see core and sysctl) into
 * given result_buffer and returns the size in number of bytes written.
 * The result is a null terminated string.
 *
 * NOTE: If a kernel.core_pattern starts with '|', the remainder of
 * the core_pattern is interpreted as command which is supposed to
 * receive the core via stdin. This happens even if the size limits
 * for core files are set to 0.
 *
 * @param result_buffer Buffer to receive the core pattern string.
 * @param result_buffer_size Size of the buffer.
 * @return size of bytes written to result_buffer (incl. terminating '\0') or -1 on error.
 */
ssize_t system_coredump_pattern(char *result_buffer, ssize_t result_buffer_size);



#endif /* FUSG_COMMON_INCLUDE_FUSG_SYSTEM_H_ */
