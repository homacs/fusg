/*
 * search.c
 *
 *  Created on: 6 Mar 2020
 *      Author: homac
 */


#include <errno.h>
#include <string.h>
#include <assert.h>

#include "../../fusg-common/include/fusg/conf.h"
#include "../../fusg-common/include/fusg/db.h"
#include "../../fusg-common/include/fusg/err.h"
#include "../../fusg-common/include/fusg/logging.h"
#include "../../fusg-common/include/fusg/utils.h"

static fusg_conf_t conf;
dbref_t db = NULL;

int search_init(const char* conf_file)
{
	int rc = fusg_conf_read(&conf, conf_file);
	if (rc == -1)
	{
		log_error("can't read config at '%s': %s", conf_file, strerror(errno));
		return ERR_CONF;
	}

	db = db_open(conf.db_path, FUSG_READ);
	if (!db)
	{
		log_error("can't open db at '%s': %s", conf.db_path, strerror(errno));
		return ERR_DB;
	}
	return rc;
}

int search_done(void)
{
	if (db) db_close(db);
	return 0;
}



int search_files_single(const char* exe)
{
	int rc = 0;
	fusg_stats_iterator_t it;
	size_t maxpath = PATH_MAX + 1;
	char filebuf[maxpath];
	char exebuf[maxpath];
	char tmbuf[256];
	fusg_stats_t stats;
	fusg_stats_t stats_total;

	memset(&stats_total, 0, sizeof(fusg_stats_t));

	int file_exists;

	if (!realpath(exe, exebuf))
	{
		log_error("%s: '%s'", strerror(errno), exe);
		file_exists = 0;
	}
	else
	{
		exe = exebuf;
		file_exists = 1;
	}

	uint64_t exec_id = db_exec_get_id(db, exe);
	int entry_exists = (exec_id != (uint64_t)-1);
	if (!file_exists && !entry_exists)
	{
		log_error("no fs and no db entry: '%s'", exe);
		return ERR_USAGE;
	}

	int count = 0;
	printf("files used by executable '%s'\n", exe);
	if (entry_exists)
	{
		for (rc = db_fusg_stats_first(db, &it); rc == 0; rc = db_iterator_next(&it))
		{
			rc = db_iterator_fetch(&it, &stats);
			assert(rc == 0);
			fusg_stats_key_t key = db_iterator_get_fugs_stats_key(&it);
			if (key.exec_id != exec_id)
			{
				continue;
			}
			rc = db_file_get_file(db, key.file_id, filebuf, maxpath);
			assert(rc == 0);

			printf("\t%3lu %3lu %3lu %3lu %s '%s'\n",
					stats.create, stats.read, stats.write, stats.exec, ptime(stats.time, tmbuf),
					filebuf);
			fugs_stats_add(&stats_total, &stats);
			count++;
		}
		db_iterator_release(it);
	}
	printf("summary %3lu %3lu %3lu %3lu %s %5d\n",
			stats_total.create, stats_total.read, stats_total.write, stats_total.exec, ptime(stats_total.time, tmbuf), count);

	return rc;
}

int search_files(const char* conf_file, int num_execs, char** execs)
{
	int rc = search_init(conf_file);
	if (rc) return rc;

	for (int i = 0; i < num_execs; i++)
	{
		search_files_single(execs[i]);
	}

	return search_done();
}

int search_execs_single(char* file)
{
	int rc = 0;

	fusg_stats_iterator_t it;
	size_t maxpath = PATH_MAX + 1;
	char exebuf[maxpath];
	char filebuf[maxpath];
	char tmbuf[256];
	fusg_stats_t stats;
	fusg_stats_t stats_total;

	memset(&stats_total, 0, sizeof(fusg_stats_t));

	int file_exists;
	if (!realpath(file, filebuf))
	{
		log_error("%s: '%s'", strerror(errno), file);
		file_exists = 0;
	}
	else
	{
		file = filebuf;
		file_exists = 1;
	}

	uint64_t file_id = db_file_get_id(db, file);
	int entry_exists = (file_id != (uint64_t)-1);

	if (!file_exists && !entry_exists)
	{
		log_error("no fs and no db entry: '%s'", file);
		return ERR_USAGE;
	}


	int count = 0;
	printf("executables using file '%s'\n", file);
	if (entry_exists) {
		for (rc = db_fusg_stats_first(db, &it); rc == 0; rc = db_iterator_next(&it))
		{
			rc = db_iterator_fetch(&it, &stats);
			assert(rc == 0);
			fusg_stats_key_t key = db_iterator_get_fugs_stats_key(&it);
			if (key.file_id != file_id)
			{
				continue;
			}
			rc = db_exec_get_executable(db, key.exec_id, exebuf, maxpath);
			assert(rc == 0);

			printf("\t%3lu %3lu %3lu %3lu %s '%s'\n",
					stats.create, stats.read, stats.write, stats.exec, ptime(stats.time, tmbuf),
					exebuf);

			fugs_stats_add(&stats_total, &stats);
			count++;
		}
		db_iterator_release(it);
	}
	printf("summary %3lu %3lu %3lu %3lu %s %5d\n",
			stats_total.create, stats_total.read, stats_total.write, stats_total.exec, ptime(stats_total.time, tmbuf), count);

	return rc;
}

int search_execs(const char* conf_file, int num_files, char** files)
{
	int rc = search_init(conf_file);
	if (rc) return rc;

	for (int i = 0; i < num_files; i++)
	{
		search_execs_single(files[i]);
	}

	return search_done();
}


int search_dump_all(const char* conf_file)
{
	int rc = search_init(conf_file);
	if (rc) return rc;


	fusg_stats_iterator_t it;
	size_t maxpath = PATH_MAX + 1;
	char exebuf[maxpath];
	char filebuf[maxpath];
	char tmbuf[256];
	fusg_stats_t stats;
	fusg_stats_t stats_total;

	memset(&stats_total, 0, sizeof(fusg_stats_t));



	int count = 0;
	for (rc = db_fusg_stats_first(db, &it); rc == 0; rc = db_iterator_next(&it))
	{
		rc = db_iterator_fetch(&it, &stats);
		assert(rc == 0);
		fusg_stats_key_t key = db_iterator_get_fugs_stats_key(&it);
		rc = db_exec_get_executable(db, key.exec_id, exebuf, maxpath);
		assert(rc == 0);

		rc = db_file_get_file(db, key.file_id, filebuf, maxpath);
		assert(rc == 0);

		printf("\t%3lu %3lu %3lu %3lu %s '%s'\n\t\t'%s'\n",
				stats.create, stats.read, stats.write, stats.exec, ptime(stats.time, tmbuf),
				exebuf, filebuf);

		fugs_stats_add(&stats_total, &stats);
		count++;
	}
	db_iterator_release(it);



	return search_done();
}

