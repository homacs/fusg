

#include "fusg/db.h"
#include "fusg/logging.h"
#include "fusg/utils.h"
#include "fusg/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>

#define DB_BASE_PATH "/tmp/fugsdb-test"


void test_coredump_size(void)
{
	rlim_t coresize = system_coredump_size();
	assert(coresize >= 0);
}

void test_coredump_pattern(void)
{
	char pattern[PATH_MAX];
	int result = system_coredump_pattern(pattern, sizeof(pattern));
	assert(result >= 0);
}

void testsub_fwhich(const char* cmd, const char* expect)
{
	char buffer[PATH_MAX+1];
	const char* resolved = fwhich(buffer, cmd);
	assert(resolved != NULL);
	assert(resolved == buffer);
	assert(!strcmp(resolved, expect));
}

void test_fwhich(void)
{
	testsub_fwhich("find", "/usr/bin/find");
}

void test_iscanonical(void)
{
	assert(0 == fiscanonical("tmp"));
	assert(0 == fiscanonical(".."));
	assert(0 == fiscanonical("."));
	assert(1 == fiscanonical("/"));
	assert(1 == fiscanonical("/tmp"));
	assert(0 == fiscanonical("/tmp/"));
	assert(0 == fiscanonical("/tmp/."));
	assert(0 == fiscanonical("/tmp/.."));
	assert(1 == fiscanonical("/tmp/..."));
	assert(1 == fiscanonical("/tmp/.a"));
	assert(1 == fiscanonical("/tmp/..a"));
	assert(1 == fiscanonical("/tmp/...a"));
	assert(0 == fiscanonical("/tmp/./"));
	assert(0 == fiscanonical("/tmp/../"));
	assert(0 == fiscanonical("/tmp/.../"));
	assert(0 == fiscanonical("/tmp/./a"));
	assert(0 == fiscanonical("/tmp/../a"));
	assert(1 == fiscanonical("/tmp/.../a"));
}



void testsub_fabsolute(const char* cwd, const char* path, const char* expect)
{
	char pathbuf[PATH_MAX];
	const char* result = fabsolute(cwd, path, pathbuf);
	assert(result != NULL);
	assert(!strcmp(result, pathbuf));
	assert(!strcmp(result, expect));
}

void test_fabsolute()
{
	testsub_fabsolute("/tmp", "test", "/tmp/test");
	testsub_fabsolute("/tmp/", "test", "/tmp/test");
	testsub_fabsolute("/tmp//", "test", "/tmp/test");
	testsub_fabsolute("/tmp", "test/", "/tmp/test");
	testsub_fabsolute("/tmp", "test//", "/tmp/test");
	testsub_fabsolute("/tmp", "test/./", "/tmp/test");
	testsub_fabsolute("/tmp", "test/../", "/tmp");
	testsub_fabsolute("/tmp", "test/..", "/tmp");
	testsub_fabsolute("/tmp", "test/driven/..", "/tmp/test");
	testsub_fabsolute("/tmp", "test.", "/tmp/test.");
	testsub_fabsolute("/tmp", ".test", "/tmp/.test");
	testsub_fabsolute("/tmp", "..test", "/tmp/..test");
	testsub_fabsolute("/tmp", "...", "/tmp/...");
	testsub_fabsolute("/...", "...", "/.../...");
	testsub_fabsolute("/", "", "/");
}


void test_db_create(void)
{
	int rc;
	rc = db_delete(DB_BASE_PATH);
	assert(rc == 0);
	dbref_t db = db_open(DB_BASE_PATH, FUSG_WRITE);
	assert(db != NULL);
	db_close(db);
}

void test_db_reopen(void)
{
	int rc = db_delete(DB_BASE_PATH);
	assert(rc == 0);
	dbref_t db = db_open(DB_BASE_PATH, DB_WRITE);
	assert(db != NULL);
	db_close(db);

	db = db_open(DB_BASE_PATH, DB_WRITE);
	assert(db != NULL);
	db_close(db);

}

void test_db_update(void)
{
	dbref_t db = db_open(DB_BASE_PATH, DB_WRITE);
	assert(db != NULL);

	const char* exe = "/usr/bin/firefox";

	uint64_t timestamp = 0;
	int rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_RW | FUSG_CREAT, timestamp);
	assert(rc == 0);

	timestamp++;
	rc = db_update(db, exe, "/home/homac/.mozilla/settings", FUSG_WRITE | FUSG_CREAT, timestamp);
	assert(rc == 0);

	timestamp++;
	rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_READ | FUSG_EXEC, timestamp);
	assert(rc == 0);

	fusg_stats_t stats;
	rc = db_fetch(db, exe, "/home/homac/.mozilla", &stats);
	assert(rc == 0);
	assert(stats.create == 1);
	assert(stats.read   == 2);
	assert(stats.write  == 1);
	assert(stats.exec   == 1);
	assert(stats.time   == 2);

	rc = db_fetch(db, exe, "/home/homac/.mozilla/settings", &stats);
	assert(rc == 0);
	assert(stats.create == 1);
	assert(stats.read   == 0);
	assert(stats.write  == 1);
	assert(stats.exec   == 0);
	assert(stats.time   == 1);



	exe = "/usr/bin/thunderbird";
	timestamp++;
	rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_READ | FUSG_EXEC, timestamp);
	assert(rc == 0);

	timestamp++;
	rc = db_update(db, exe, "/home/homac/.mozilla/thunderbird", FUSG_WRITE | FUSG_EXEC | FUSG_CREAT, timestamp);
	assert(rc == 0);


	rc = db_fetch(db, exe, "/home/homac/.mozilla", &stats);
	assert(rc == 0);
	assert(stats.create == 0);
	assert(stats.read   == 1);
	assert(stats.write  == 0);
	assert(stats.exec   == 1);
	assert(stats.time   == 3);

	rc = db_fetch(db, exe, "/home/homac/.mozilla/thunderbird", &stats);
	assert(rc == 0);
	assert(stats.create == 1);
	assert(stats.read   == 0);
	assert(stats.write  == 1);
	assert(stats.exec   == 1);
	assert(stats.time   == 4);

	db_close(db);

	// reopen read only
	db = db_open(DB_BASE_PATH, DB_READ);

	fusg_stats_iterator_t it;
	size_t maxpath = PATH_MAX + 1;
	char filebuf[maxpath];
	char exebuf[maxpath];

	for (rc = db_fusg_stats_first(db, &it); rc == 0; rc = db_iterator_next(&it))
	{
		rc = db_iterator_fetch(&it, &stats);
		assert(rc == 0);
		fusg_stats_key_t key = db_iterator_get_fugs_stats_key(&it);
		rc = db_exec_get_executable(db, key.exec_id, exebuf, maxpath);
		assert(rc == 0);
		rc = db_file_get_file(db, key.file_id, filebuf, maxpath);
		assert(rc == 0);
		printf("%3lu %3lu %3lu %3lu %5lu '%s' -> '%s'\n",
				stats.create, stats.read, stats.write, stats.exec, stats.time,
				exebuf, filebuf);
	}
	db_iterator_release(it);

	db_close(db);
}


void test_db_search(void)
{
	dbref_t db = db_open(DB_BASE_PATH, DB_WRITE);
	assert(db != NULL);

	const char* exe = "/usr/bin/firefox";

	uint64_t timestamp = time(NULL);
	int rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_RW | FUSG_CREAT, timestamp);
	assert(rc == 0);

	timestamp = time(NULL);
	rc = db_update(db, exe, "/home/homac/.mozilla/settings", FUSG_WRITE | FUSG_CREAT, timestamp);
	assert(rc == 0);

	timestamp = time(NULL);
	rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_READ | FUSG_EXEC, timestamp);
	assert(rc == 0);


	exe = "/usr/bin/thunderbird";
	timestamp = time(NULL);
	rc = db_update(db, exe, "/home/homac/.mozilla", FUSG_READ | FUSG_EXEC, timestamp);
	assert(rc == 0);

	timestamp = time(NULL);
	rc = db_update(db, exe, "/home/homac/.mozilla/thunderbird", FUSG_WRITE | FUSG_EXEC | FUSG_CREAT, timestamp);
	assert(rc == 0);

	db_close(db);

	// reopen read only
	db = db_open(DB_BASE_PATH, DB_READ);

	fusg_stats_iterator_t it;
	size_t maxpath = PATH_MAX + 1;
	char filebuf[maxpath];
	char exebuf[maxpath];
	fusg_stats_t stats;
	fusg_stats_t stats_total;

	memset(&stats_total, 0, sizeof(fusg_stats_t));

	const char* file = "/home/homac/.mozilla";
	uint64_t file_id = db_file_get_id(db, file);
	assert(file_id != (uint64_t)-1); // not found
	char tmbuf[256];
	int count = 0;
	printf("executables using file '%s'\n", file);
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

	printf("summary %3lu %3lu %3lu %3lu %s %5d\n",
			stats_total.create, stats_total.read, stats_total.write, stats_total.exec, ptime(stats_total.time, tmbuf), count);

	memset(&stats_total, 0, sizeof(fusg_stats_t));

	count = 0;
	exe = "/usr/bin/firefox";
	uint64_t exec_id = db_exec_get_id(db, exe);
	printf("files used by executable '%s'\n", exe);
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

	printf("summary %3lu %3lu %3lu %3lu %s %5d\n",
			stats_total.create, stats_total.read, stats_total.write, stats_total.exec, ptime(stats_total.time, tmbuf), count);

	db_close(db);
}


int main(void) {
	test_coredump_pattern();
	test_coredump_size();
	test_fabsolute();
	test_fwhich();
	test_iscanonical();

	test_db_create();
	test_db_reopen();
	test_db_update();

	test_db_search();

	return EXIT_SUCCESS;
}
