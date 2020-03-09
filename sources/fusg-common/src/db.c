/*
 * db.c
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */



#include <gdbm.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include "fusg/db.h"
#include "fusg/logging.h"
#include "fusg/utils.h"

#define DB_ID_ENTRY "id"

#define DB_FILE_EXEC "exec.db"
#define DB_FILE_EXER "execr.db"
#define DB_FILE_FILE "file.db"
#define DB_FILE_FILR "filer.db"
#define DB_FILE_EVNT "evnt.db"
#define DB_FILE_IDTB "idtb.db"


typedef struct __db_t {
	/** used to indicate that we already tried to close it */
	int open_flags;
	/** lock file descriptor */
	int lockfd;
	/** dirty == 1 -> has pending data to be flushed to disk */
	int dirty;

	/** executables */
	GDBM_FILE exec_db;
	/** executables reverse lookup */
	GDBM_FILE execr_db;
	/** files */
	GDBM_FILE file_db;
	/** files reverse lookup */
	GDBM_FILE filer_db;
	/** events */
	GDBM_FILE evnt_db;
	/** id table: used to generate dunique ids */
	GDBM_FILE idtb_db;

	int lock_depth;

	char path[0];
} db_t;


typedef enum {
	DB_HEALTHY = 0,
	DB_NOTFOUND,
	DB_NOACCESS,
	DB_CORRUPTED,
} db_health_t;


static inline db_health_t __db_check_health(const char* dbpath, db_flags_t flags);
void __db_perror(const char* context);
static inline int __db_id_init(dbref_t dbc);
static inline int __db_id_next(dbref_t dbc, uint64_t* next_id);

static inline int __db_store(GDBM_FILE db, datum key, datum content);

static inline int __db_store_str_long(GDBM_FILE db, const char* key_str, uint64_t value);
static inline uint64_t* __db_fetch_str_long(GDBM_FILE db, const char* key_str, uint64_t* value);
static inline int __db_store_long_str(GDBM_FILE db, uint64_t key_long, const char* value_str);
static inline int __db_fetch_long_str(GDBM_FILE db, uint64_t key_long, char* value, size_t size);

static inline datum __db_datum_evnt_key(fusg_stats_key_t* value);
static inline datum __db_datum_evnt_content(fusg_stats_t* value);
static inline int __db_get_or_create_unique_id(dbref_t dbc, GDBM_FILE str_id_db, const char* key, uint64_t* id);
static inline fusg_stats_t* __db_evnt_fetch(dbref_t dbc, fusg_stats_key_t* evnt_key, fusg_stats_t* evnt_val);
static inline int __db_evnt_store(dbref_t dbc, fusg_stats_key_t* evnt_key, fusg_stats_t* evnt_val);
static inline int __db_check_expected_notfound(void);
static inline int __db_get_fusg_key(dbref_t dbc, const char* executable, const char* filepath, fusg_stats_key_t* evnt_key);

void __db_sync(dbref_t dbc);


dbref_t db_open(const char* dbpath, db_flags_t flags)
{
	int dbinit = 0; // whether we have to initialise the database


	// check if dbpath is absolute path
	// -> caller has to take care of it!
	assert(fiscanonical(dbpath));

	// check if flags make sense
	assert(0 == (flags & ~(DB_SYNC | DB_READ | DB_WRITE)));

	size_t pathlen = strlen(dbpath) + 1;


	db_health_t health = __db_check_health(dbpath, flags);
	int rc = 0;
	switch (health)
	{
	case DB_HEALTHY:
		break;
	case DB_NOTFOUND:
		if (!(flags & DB_WRITE))
		{
			// have no write permission
			return NULL;
		}

		errno = 0;

		rc = mkdir(dbpath, 0700);
		if (rc == -1)
		{
			log_error("db_open: mkdir(%s): %s", dbpath, strerror(errno));
			log_error("db_open: check installation");
			return NULL;
		}
		dbinit = 1;
		break;
	case DB_NOACCESS:
		log_error("db_open: can't access db: '%s'", dbpath);
		return NULL;
	case DB_CORRUPTED:
		// FIXME: need auto-recovery in case of db-corruption.
		log_error("db_open: data base corrupted.");
		return NULL;
	}



	db_t* dbc = (db_t*)calloc(1, sizeof(db_t) + pathlen);
	strcpy(dbc->path, dbpath);

	dbc->open_flags = flags;
	dbc->lockfd = open(dbpath, O_RDONLY);
	if (!dbc->lockfd)
	{
		log_error("db_open: can't lock db: %s", strerror(errno));
		rc = -1;
		goto error;
	}
	db_lock(dbc);

	// defines the size of chunks to be transfered to db.
	// If less than file system block size, its set to
	// file system block size.
	int block_size = 0;
	// GDBM_WRCREAT: read/write access and create db if it does not exist
	// !GDBM_SYNC: don't sync I/O. We use manual periodic sync on unlock instead.
	// GDBM_NOLOCK: because we use proprietary locking.
	// !GDBM_NOMMAP: because we want it to be memory mapped.
	int gdbm_mode = GDBM_NOLOCK;
	if (flags & DB_WRITE)       gdbm_mode |= GDBM_WRCREAT;
	else /* flags == DB_READ */ gdbm_mode |= GDBM_READER;
	// file mode used when db is created.
	// We use user only R/W
	int mode = 0600;
	// redirecting of error logging
	// if set to NULL it DB will use a default.
	void (* fatal_func)(const char *);
	fatal_func = __db_perror; // FIXME: think about db-error handling

	char pathbuf[PATH_MAX];

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_EXEC);
	dbc->exec_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->exec_db) {
		__db_perror("gdbm_open(exec.db)");
		goto error;
	}

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_EXER);
	dbc->execr_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->execr_db) {
		__db_perror("gdbm_open(execr.db)");
		goto error;
	}

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_FILE);
	dbc->file_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->file_db) {
		__db_perror("gdbm_open(file.db)");
		goto error;
	}

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_FILR);
	dbc->filer_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->filer_db) {
		__db_perror("gdbm_open(filer.db)");
		goto error;
	}

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_EVNT);
	dbc->evnt_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->evnt_db) {
		__db_perror("gdbm_open(evnt.db)");
		goto error;
	}

	sprintf(pathbuf, "%s/%s", dbpath, DB_FILE_IDTB);
	dbc->idtb_db = gdbm_open(pathbuf, block_size, gdbm_mode, mode, fatal_func);
	if (!dbc->idtb_db) {
		__db_perror("gdbm_open(idtb.db)");
		goto error;
	}


	if (dbinit)
	{
		//
		// initialise id table
		//
		int rc = __db_id_init(dbc);
		if (rc) goto error;
		__db_sync(dbc);
	}
	db_unlock(dbc);
	return dbc;


error:
	db_unlock(dbc);
	db_close(dbc);
	return NULL;
}


void db_close(dbref_t dbc)
{
	if (dbc && dbc->open_flags)
	{
		dbc->open_flags = 0;
		if (dbc->lock_depth)
		{
			// there is something wrong with the lock depth

			// hit when dbref is corrupted
			assert(dbc->lock_depth >= 0);

			log_warn("database was not properly unlocked. Unlocking now.");
			dbc->lock_depth = 1;
			db_unlock(dbc);
		}
		if (dbc->exec_db)  gdbm_close(dbc->exec_db);
		if (dbc->execr_db) gdbm_close(dbc->execr_db);
		if (dbc->file_db)  gdbm_close(dbc->file_db);
		if (dbc->filer_db) gdbm_close(dbc->filer_db);
		if (dbc->evnt_db)  gdbm_close(dbc->evnt_db);
		if (dbc->idtb_db)  gdbm_close(dbc->idtb_db);

		if (dbc->lockfd)   close(dbc->lockfd);
		free(dbc);
	}
}

int db_delete(const char* dbpath)
{
	int rc = fremove_r(dbpath);
	if (rc && errno == ENOENT)
	{
		// did not exist
		rc = 0;
	}
	return rc;
}


void __db_sync(dbref_t dbc)
{
	if (dbc->open_flags & DB_WRITE)
	{
		if (dbc->exec_db)  gdbm_sync(dbc->exec_db);
		if (dbc->execr_db) gdbm_sync(dbc->execr_db);
		if (dbc->file_db)  gdbm_sync(dbc->file_db);
		if (dbc->filer_db) gdbm_sync(dbc->filer_db);
		if (dbc->evnt_db)  gdbm_sync(dbc->evnt_db);
		if (dbc->idtb_db)  gdbm_sync(dbc->idtb_db);
		log_debug("db synced to disk");
		dbc->dirty = 0;
	}
}

int db_flush(dbref_t dbc)
{
	if (dbc->dirty)
	{
		int rc;
		if ((rc = db_lock(dbc))) return rc;
		__db_sync(dbc);
		return db_unlock(dbc);
	}
	return 0;
}


int db_lock(dbref_t dbc)
{
	int rc = 0;
	if (!dbc->lock_depth)
	{
		// blocks until lock is free
		int lockop = dbc->open_flags & DB_WRITE ? LOCK_EX : LOCK_SH;
		rc = flock(dbc->lockfd, lockop);
		if (rc) log_error("db_lock(): %s", strerror(errno));
	}
	dbc->lock_depth++;
	return rc;
}

int db_unlock(dbref_t dbc)
{
	int rc = 0;
	if (dbc->lock_depth == 0)
	{
		log_error("db_unlock(): trying to unlock more than once -> ignored");
		return -1;
	}

	// hit when dbref is corrupted
	assert (dbc->lock_depth >= 0);

	dbc->lock_depth--;

	if (!dbc->lock_depth)
	{
		if ((dbc->open_flags & (DB_SYNC|DB_WRITE)) == (DB_SYNC|DB_WRITE))
		{
			__db_sync(dbc);
		}

		rc = flock(dbc->lockfd, LOCK_UN);
		if (rc) log_error("db_unlock(): %s", strerror(errno));
	}
	return rc;
}


int db_update(dbref_t dbc, const char* executable, const char* filepath, file_usage_t flags, uint64_t timestamp)
{
	db_lock(dbc);
	//
	// determine key into event db
	//

	int rc = 0;
	fusg_stats_key_t evnt_key;

	rc = __db_get_fusg_key(dbc, executable, filepath, &evnt_key);
	if (rc == -1) goto bail;



	//
	// update entry in event db
	//

	fusg_stats_t evnt_val;

	// fetch current state
	if (!__db_evnt_fetch(dbc, &evnt_key, &evnt_val)) {
		// does not exist
		memset(&evnt_val, 0, sizeof(fusg_stats_t));
	}

	// increment counters
	evnt_val.read   += ((flags & FUSG_READ)  > 0);
	evnt_val.write  += ((flags & FUSG_WRITE) > 0);
	evnt_val.create += ((flags & FUSG_CREAT) > 0);
	evnt_val.exec   += ((flags & FUSG_EXEC)  > 0);
	evnt_val.time    = timestamp;

	// update content in db
	rc = __db_evnt_store(dbc, &evnt_key, &evnt_val);
	dbc->dirty = 1;
bail:
	if (rc != 0) __db_perror("db_update");
	db_unlock(dbc);
	return rc;
}

int db_fetch(dbref_t dbc, const char* executable, const char* filepath, fusg_stats_t* fusg_stats)
{
	int rc = 0;
	db_lock(dbc);
	//
	// determine key into event db
	//

	fusg_stats_key_t evnt_key;

	rc = __db_get_fusg_key(dbc, executable, filepath, &evnt_key);
	if (rc == -1) goto bail;


	//
	// update entry in event db
	//


	// fetch current state
	if (!__db_evnt_fetch(dbc, &evnt_key, fusg_stats)) {
		rc = __db_check_expected_notfound();
	}
bail:
	db_unlock(dbc);
	return rc;
}


int db_fusg_stats_first(dbref_t dbc, fusg_stats_iterator_t* iterator)
{
	iterator->dbc = dbc;
	iterator->dbf = dbc->evnt_db;
	int rc = 	db_lock(dbc);
	if (rc) {
		db_unlock(iterator->dbc);
		goto bail;
	}
	iterator->have_lock = 1;
	iterator->db_key = gdbm_firstkey(iterator->dbf);
bail:
	return (iterator->db_key.dptr) ? 0 : -1;
}

void db_iterator_release(fusg_stats_iterator_t iterator)
{
	if (iterator.db_key.dptr)
	{
		free(iterator.db_key.dptr);
		iterator.db_key.dptr = NULL;
	}
	if (iterator.have_lock)
	{
		db_unlock(iterator.dbc);
		iterator.have_lock = 0;
	}
}

int db_iterator_fetch(fusg_stats_iterator_t* iterator, fusg_stats_t* fusg_stats)
{

	datum result = gdbm_fetch(iterator->dbf, iterator->db_key);
	if (result.dptr)
	{
		assert(result.dsize == sizeof(fusg_stats_t));
		*fusg_stats = *((fusg_stats_t*)result.dptr);
		free(result.dptr);
		return 0;
	}
	else
	{
		return __db_check_expected_notfound();
	}
}

int db_iterator_next(fusg_stats_iterator_t* iterator)
{
	char* oldkey = iterator->db_key.dptr;
	iterator->db_key = gdbm_nextkey(iterator->dbf, iterator->db_key);
	free(oldkey);
	return (iterator->db_key.dptr) ? 0 : -1;
}


int db_exec_get_executable(dbref_t dbc, uint64_t key_long, char* buffer, size_t size)
{
	db_lock(dbc);
	int rc = __db_fetch_long_str(dbc->execr_db, key_long, buffer, size);
	db_unlock(dbc);
	return rc;
}

uint64_t db_exec_get_id(dbref_t dbc, const char* executable)
{
	uint64_t id;
	db_lock(dbc);
	if (!__db_fetch_str_long(dbc->exec_db, executable, &id)) {
		id = -1;
	}
	db_unlock(dbc);
	return id;
}

int db_file_get_file(dbref_t dbc, uint64_t key_long, char* buffer, size_t size)
{
	db_lock(dbc);
	int rc = __db_fetch_long_str(dbc->filer_db, key_long, buffer, size);
	db_unlock(dbc);
	return rc;
}



uint64_t db_file_get_id(dbref_t dbc, const char* filepath)
{
	uint64_t id;
	db_lock(dbc);
	if (!__db_fetch_str_long(dbc->file_db, filepath, &id)) {
		id = -1;
	}
	db_unlock(dbc);
	return id;
}

static inline
db_health_t __db_check_health(const char* dbpath, db_flags_t flags)
{
	db_health_t health = DB_HEALTHY;

	struct stat statbuf;
	int rc = stat(dbpath, &statbuf);
	if (rc == -1)
	{
		if (errno == ENOENT)
		{

			return DB_NOTFOUND;
		}
		else
		{
			log_error("db_open: stat(%s): %s", dbpath, strerror(errno));
			log_error("db_open: check installation");
			return DB_NOACCESS;
		}
	}
	if (   !fdir_contains(dbpath, DB_FILE_EXEC)
			|| !fdir_contains(dbpath, DB_FILE_EXER)
			|| !fdir_contains(dbpath, DB_FILE_FILE)
			|| !fdir_contains(dbpath, DB_FILE_FILR)
			|| !fdir_contains(dbpath, DB_FILE_EVNT)
			|| !fdir_contains(dbpath, DB_FILE_IDTB)
		)
	{
		return DB_CORRUPTED;
	}
	return health;
}




static inline
datum __db_datum_evnt_key(fusg_stats_key_t* value)
{
	datum d;
	d.dptr = (char*)value;
	d.dsize = sizeof(fusg_stats_key_t);
	return d;
}


static inline
datum __db_datum_evnt_content(fusg_stats_t* value)
{
	datum d;
	d.dptr = (char*)value;
	d.dsize = sizeof(fusg_stats_t);
	return d;
}

/**
 * Get the id of an entry in a data base with key=id entries.
 *
 * If no such entry exists, it is inserted with a new unique id.
 * @param dbc the main database context.
 * @param str_id_db the gdbm db with key=id entries.
 * @param key
 * @param id
 * @return 0 on success and -1 on error
 */
static inline
int __db_get_or_create_unique_id(dbref_t dbc, GDBM_FILE str_id_db, const char* key, uint64_t* id)
{
	int rc = 0;
	if (!__db_fetch_str_long(str_id_db, key, id))
	{
		//
		// entry does not exist -> create
		//
		rc = __db_check_expected_notfound();
		if (rc == -1) goto bail;

		// fetch unique id
		rc = __db_id_next(dbc, id);
		if (rc == -1) goto bail;

		// insert new entry
		rc = __db_store_str_long(str_id_db, key, *id);
		if (rc == -1) goto bail;
	}
bail:
	return rc;
}


static inline
int __db_check_expected_notfound(void)
{
	if (gdbm_errno != GDBM_ITEM_NOT_FOUND)
	{
		__db_perror("fetch");
		return -1;
	}
	return 0;
}

static inline
int __db_get_fusg_key(dbref_t dbc, const char* executable, const char* filepath, fusg_stats_key_t* evnt_key)
{
	int rc = 0;
	rc = __db_get_or_create_unique_id(dbc, dbc->exec_db, executable, &evnt_key->exec_id);
	if (rc != 0) goto bail;
	rc = __db_store_long_str(dbc->execr_db, evnt_key->exec_id, executable);
	if (rc != 0) goto bail;
	rc = __db_get_or_create_unique_id(dbc, dbc->file_db, filepath, &evnt_key->file_id);
	if (rc != 0) goto bail;
	rc = __db_store_long_str(dbc->filer_db, evnt_key->file_id, filepath);
bail:
	return rc;
}

static inline
fusg_stats_t* __db_evnt_fetch(dbref_t dbc, fusg_stats_key_t* evnt_key, fusg_stats_t* evnt_val)
{
	datum key = __db_datum_evnt_key(evnt_key);
	datum result = gdbm_fetch(dbc->evnt_db, key);
	if (result.dptr)
	{
		assert(result.dsize == sizeof(fusg_stats_t));
		*evnt_val = *((fusg_stats_t*)result.dptr);
		free(result.dptr);
		return evnt_val;
	}
	else
	{
		__db_check_expected_notfound();
		return NULL;
	}
}

static inline
int __db_evnt_store(dbref_t dbc, fusg_stats_key_t* evnt_key, fusg_stats_t* evnt_val)
{
	datum key = __db_datum_evnt_key(evnt_key);
	datum val = __db_datum_evnt_content(evnt_val);
	// flag is
	// - GDBM_REPLACE : replace if exists
	// - GDBM_INSERT  : error if exists
	int flag = GDBM_REPLACE;
	return gdbm_store(dbc->evnt_db, key, val, flag);
}



void __db_perror(const char* context) {
	log_error("db-error: %s (errno: %s)", context, gdbm_strerror(gdbm_errno));
}


static inline
datum db_datum_str(const char* value)
{
	datum d;
	d.dptr  = (char*)value;
	d.dsize = strlen(value) + 1;
	return d;
}

static inline
datum db_datum_long(uint64_t* value)
{
	datum d;
	d.dptr = (char*)value;
	d.dsize = sizeof(uint64_t);
	return d;
}



static inline
int __db_store(GDBM_FILE db, datum key, datum content)
{
	// flag is
	// - GDBM_REPLACE : replace if exists
	// - GDBM_INSERT  : error if exists
	int flag = GDBM_REPLACE;
	int rc = gdbm_store(db, key, content, flag);
	if (rc == +1)
	{
		log_error("gdbm_store: caller was not an official writer or either key or content have a ‘NULL’ ‘dptr’ field.");
		return -1;
	}
	else if (rc == -1)
	{
		log_error("gdbm_store: The item was not stored because the argument flag was ‘GDBM_INSERT’ and the key was already in the database.");
		return -1;
	}
	return 0;
}


static inline
int __db_store_str_long(GDBM_FILE db, const char* key_str, uint64_t value)
{
	datum key = db_datum_str(key_str);
	datum content = db_datum_long(&value);
	return __db_store(db, key, content);
}

static inline
uint64_t* __db_fetch_str_long(GDBM_FILE db, const char* key_str, uint64_t* value)
{
	datum key = db_datum_str(key_str);

	datum result = gdbm_fetch(db, key);
	if (result.dptr)
	{
		assert(result.dsize == sizeof(uint64_t));
		*value = *((long*)result.dptr);
		free(result.dptr);
		return value;
	}
	else
	{
		__db_check_expected_notfound();
		return NULL;
	}
}

static inline
int __db_store_long_str(GDBM_FILE db, uint64_t key_long, const char* value_str)
{
	datum key = db_datum_long(&key_long);
	datum content = db_datum_str(value_str);
	return __db_store(db, key, content);
}

static inline
int __db_fetch_long_str(GDBM_FILE db, uint64_t key_long, char* value, size_t size)
{
	int rc = 0;
	datum key = db_datum_long(&key_long);
	datum result = gdbm_fetch(db, key);

	if (result.dptr)
	{
		if (result.dsize > size)
		{
			rc = size;
		}
		else
		{
			memcpy(value, result.dptr, result.dsize);
		}
		free(result.dptr);
	}
	else
	{
		__db_check_expected_notfound();
		rc = -1;
	}
	return rc;
}



static inline
int __db_id_init(dbref_t dbc)
{
	return __db_store_str_long(dbc->idtb_db, DB_ID_ENTRY, 0);
}


static inline
int __db_id_next(dbref_t dbc, uint64_t* next)
{
	uint64_t id;
	if (__db_fetch_str_long(dbc->idtb_db, DB_ID_ENTRY, &id))
	{
		*next = id;
		id++;
		return __db_store_str_long(dbc->idtb_db, DB_ID_ENTRY, id);
	}
	else
	{
		log_error("can't access id table entry '%s'", DB_ID_ENTRY);
		return -1;
	}
}

