/*
 * utils.h
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_UTILS_H_
#define FUSG_UTILS_H_

#define _GNU_SOURCE
#include <unistd.h>
#include <gdbm.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#include <assert.h>

static inline
int fexists(const char* path)
{
	struct stat statbuf;
	int rc = stat(path, &statbuf);
	return rc == 0;
}

static inline
int fisfile(const char* path)
{
	struct stat statbuf;
	int rc = stat(path, &statbuf);
	return rc == 0 && S_ISREG(statbuf.st_mode);
}

const char* fwhich(char* buffer, const char* command);



static inline
char* ptime(uint64_t time, char* buf)
{
    struct tm tmp;

    if (!localtime_r((time_t*)&time, &tmp)) {
        perror("localtime");
    	return ctime_r((time_t *)&time, buf);
    }
    strftime(buf, 255, "%Y-%m-%d %T", &tmp);
    return buf;
}

static inline
const char* fabsolute(const char* cwd, const char* path, char* absolute)
{
	if (path[0] != '/')
	{
		// append path to cwd
		snprintf(absolute, PATH_MAX, "%s", cwd);
		size_t len = strlen(cwd);

		// append '/' if not already there
		if (absolute[len-1] != '/')
		{
			absolute[len] = '/';
			len++;
		}

		// append path
		char* p = absolute + len;
		snprintf(p, PATH_MAX-len, "%s", path);

	}
	else
	{
		// copy path, so we can work on it
		snprintf(absolute, PATH_MAX, "%s", path);
	}

	// check if the absolute path contains '/..' or '/.'
	typedef enum {
		NONE = 0,
		SLASH = 1,
		SLASH_DOT = 2,
		SLASH_DOT_DOT = 3,
	} SEQ;

	path = absolute;
	char* r = absolute; // pointer in path reading ahead
	char* l = 0;    // pointer to remind start of a sequence '/' '/.' '/..'
	char* c = absolute; // pointer copying from p
	SEQ seq = 0;

	for (; *r; r++)
	{

		// TODO: this can be more compact if decisions are translated
		//       into commands to be carried out at the end of the loop.

		if (*r == '/')
		{
			if (l)
			{
				seq = r-l;
				if (seq < SLASH_DOT_DOT)
				{
					// found '//' or '/./'
					// --> skip ahead
					l = r;
				}
				else /* seq == SLASH_DOT_DOT */
				{
					// found '/../'
					// rewind to previous '/'
					if (c == path) return NULL;
					for (--c; c > path && *c != '/'; c--);
					l = r; // remind start of sequence
					       // but don't copy it for now
				}
			}
			else
			{
				l = r; // remind start of sequence
				       // but don't copy it for now
			}
		}
		else if (*r == '.')
		{
			if (l)
			{
				seq = r-l;
				if (seq == SLASH_DOT_DOT)
				{
					// '/...' is actually allowed
					for (;l <= r; c++, l++) *c = *l;
					l = 0;
				}
#ifndef NDEBUG
				else assert (seq < SLASH_DOT_DOT);
#endif
			}
			else
			{
				// just a regular dot inside of a file name:
				// - file.x
				// --> copy it
				*c = *r;
				c++;
			}
		}
		else
		{
			if (l)
			{
				seq = r-l;
				if (seq <= SLASH_DOT_DOT)
				{
					// found regular or hidden file of the following
					// patterns ('r' stands for the current character at *r):
					// - '/r'
					// - '/.r'
					// - '/..r'
					// --> copy seq
					for (;l <= r; c++, l++) *c = *l;
					l = 0;
				}
#ifndef NDEBUG
				// no such other case expected
				else assert(0);
#endif
			}
			else
			{
				// regular file character
				// --> copy it
				*c = *r;
				c++;
			}
		}
	}
	if (l)
	{
		seq = r-l;
		if (seq == SLASH_DOT_DOT)
		{
			// '/..' at the end
			// rewind to previous '/'
			if (c == path)
			{
				return NULL; // trying to go beyond FS root
			}
			else
			{
				for (--c; c > path && *c != '/'; c--);
			}
		}
	}
	if (c == path)
	{
		// path ends up being root "/" and c always points to the last slash
		// soooo ... step one to the right before terminating it.
		c += 1;
	}
	*c = '\0';

	return absolute;
}

/**
 * Turns the given path, which may be relative to cwd,
 * into an absolute path, starting with cwd.
 *
 * Any .. and . and // will be resolved.
 * Any soft links, that do still exist, will be resolved.
 *
 * The function always writes the resulting absolute path
 * into the given buffer 'resolved'.
 *
 * The return value indicates, whether the path actually
 * exists in the file system, or not. If it exists, the
 * function returns a pointer to the given buffer 'resolved'.
 * If the path does not exist, the function returns NULL.
 *
 * The buffer 'resolved' has to be at least (PATH_MAX+1) long.
 *
 * @param cwd Stands for current working directory and is taken
 *            as the base path, from which 'path' has to be
 *            interpreted.
 * @param path Either an absolute path or a path which is
 *             relative to given 'cwd'.
 * @param resolved A buffer of size (PATH_MAX+1) which receives
 *            the resolved path.
 * @return 'resolved' in case the resolved path exist in file system
 *            and NULL otherwise.
 */
static inline
const char* frealpath(const char* cwd, const char* path, char* resolved)
{
	path = fabsolute(cwd, path, resolved);

	// try to get the real path
	char canonical[PATH_MAX+1];
	path = realpath(path, canonical);
	if (path)
	{
		snprintf(resolved, PATH_MAX, "%s", canonical);
	}
	return path;
}

/**
 * Checks if given path is in canonical path
 * as those, that can be retrieved through
 * realpath(3) or canonicalize_file_name(3).
 *
 * Its intended purpose is to test compliance of an implementation
 * with a given API contract with an assert statement like
 * assert(fiscanonical(given_path));
 */
static inline
int fiscanonical(const char* path)
{

	if (path[0] != '/')
	{
		return 0; // not an absolute path
	}


	// check if the absolute path contains '/..' or '/.'
	int len = strlen(path);
	int score = len > 1 ? 1 : 0;
	for (int i = 1; i < len; i++)
	{
		if (path[i] == '/')
		{
			if (score)
			{
				return 0; // contains '//' or '/./' or /../'
			}
			else
			{
				score++;
			}
		}
		else if (path[i] == '.' && score)
		{
			if (score > 2)
			{
				score = 0; // allow '/...'
			}
			else
			{
				score++;
			}
		}
		else
		{
			score = 0;
		}
	}
	return score ? 0 : 1; // ends with '/' or '/.' or '/..'
}



/**
 * Remove a file or directory (recursively).
 * @return 0 on success and -1 on error (errno is set)
 */
static inline
int fremove_r(const char* path)
{
	DIR* dir = NULL;
	typedef char filename_t[NAME_MAX+1];
	typedef char pathname_t[PATH_MAX+1];
	filename_t* files = NULL;
	int rc = remove(path);
	if (rc == -1 && errno == ENOTEMPTY)
	{
		// recursively clear nonempty directory before removing it
		dir = opendir(path);
		if (dir)
		{
			// reset error state
			rc = 0;
			errno = 0;

			//
			// gather list of file names to be removed
			//
			int files_cap = 2;
			filename_t* files = (filename_t*)malloc(sizeof(filename_t) * files_cap);
			if (!files)
			{
				rc = -1;
				goto bail;
			}
			// create a list directory entries to be removed
			int f = 0;
			for (struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
			{
				if (entry->d_name[0] == '.'
						&& (entry->d_name[1] == '\0'
								|| (entry->d_name[1] == '.' && entry->d_name[2] == '\0')
							))
				{
					// ignore "." and ".."
					continue;
				}
				if (f >= files_cap)
				{
					files_cap <<= 1;
					files = realloc(files, sizeof(filename_t) * files_cap);
				}
				memcpy(files[f], entry->d_name, NAME_MAX+1);
				f++;
			}
			if (errno)
			{
				rc = -1;
				goto bail;
			}
			pathname_t filepath;

			// remove all files
			for (int i = 0; i < f && rc == 0; i++)
			{
				rc = snprintf(filepath, sizeof(pathname_t), "%s/%s", path, files[i]);
				if (rc > PATH_MAX) {
					rc = -1;
					errno = ENAMETOOLONG;
				} else if (rc < 0) {
					rc = -1;
					errno = EIO;
				} else {
					// success
					rc = 0;
				}
				if (rc) goto bail;

				rc = fremove_r(filepath);
			}

			if (rc == 0)
			{
				// finally remove the original path
				rc = remove(path);
			}
		}
	}
bail:
	if (dir) closedir(dir);
	if (files) free(files);
	return rc;
}

static inline
int fdir_contains(const char* dbpath, const char* file)
{
	char pathbuf[PATH_MAX+1];
	snprintf(pathbuf, PATH_MAX, "%s/%s", dbpath, file);
	return fexists(pathbuf);
}



/**
 * recursive mkdir ((not tested)
 */
static inline
int mkdir_p(const char* dirpath, size_t len, int mode)
{
	int rc;
    char* tmp = (char*)dirpath;
    char *p = NULL;
	if(tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++)
	if(*p == '/') {
		*p = 0;
		rc = mkdir(tmp, mode);
		*p = '/';
		if (rc == -1 && errno != EEXIST)
		{
			return -1;
		}
	}
	return 0;
}




#endif /* UTILS_H_ */
