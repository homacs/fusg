/*
 * db.h
 *
 *  Created on: 4 Mar 2020
 *      Author: homac
 */

#ifndef FUSG_DB_H_
#define FUSG_DB_H_


#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <gdbm.h>


typedef enum
{
	DB_READ  = 1<<0,
	DB_WRITE = 1<<1,
	DB_SYNC  = 1<<2,
}
db_flags_t;





#pragma pack(8)
typedef struct
{
	uint64_t exec_id;
	uint64_t file_id;
} fusg_stats_key_t;
#pragma pack()

#pragma pack(8)
typedef struct
{
	uint64_t read;
	uint64_t write;
	uint64_t create;
	uint64_t exec;
	uint64_t time;
	uint64_t _reserved;
} fusg_stats_t;
#pragma pack()

static inline
void fugs_stats_add(fusg_stats_t* stats_total,fusg_stats_t* stats)
{
	stats_total->create += stats->create;
	stats_total->exec += stats->exec;
	stats_total->read += stats->read;
	stats_total->write += stats->write;
	stats_total->time = stats_total->time < stats->time ? stats->time : stats_total->time;
}


typedef struct __db_t* dbref_t;

typedef struct __fusg_stats_iterator_t {
	dbref_t dbc;
	int have_lock;
	GDBM_FILE dbf;
	datum db_key;
} fusg_stats_iterator_t;

/**
 * Opens a data base.
 * @param dbpath path to the db (folder on file system)
 * @param flags FUSG_READ (readonly) | FUSG_WRITE (R/W) | DB_SYNC
 * @return db-reference or NULL on error.
 */
dbref_t db_open(const char* dbpath, db_flags_t flags);

/**
 * close .. no matter what
 */
void db_close(dbref_t db);

int db_flush(dbref_t db);

/**
 * Deletes the entire database from file system.
 */
int db_delete(const char* dbpath);


/**
 * Mostly used internally.
 *
 * Every public db-method is executed in between
 *
 *   db_lock()
 *   ...
 *   db_unlock()
 *
 * Locking supports recursive locking. Thus multiple
 * db-accesses can be executed in a larger locking scope
 * on application level.
 *
 *   db_lock()
 *
 *   db_fetch()  --> db_lock()
 *                   ...
 *                   db_unlock()
 *
 *   db_update() --> db_lock()
 *                   ...
 *                   db_unlock()
 *
 *   db_unlock();
 *
 * DB iterators (see db_fusg_stats_first()) call db_lock()
 * when a reference on the first entry was received.
 * The lock is released, when the db_iterator_release()
 * function was called.
 *
 */
int db_lock(dbref_t db);
int db_unlock(dbref_t db);
/**
 * Type of file usage.
 */
typedef enum {
	FUSG_READ  = 1<<0,
	FUSG_WRITE = 1<<1,
	FUSG_EXEC  = 1<<2,
	FUSG_CREAT = 1<<3,
	FUSG_DELET = 1<<4,
} file_usage_t;

#define FUSG_RW   (FUSG_READ | FUSG_WRITE)
#define FUSG_RWX  (FUSG_READ | FUSG_WRITE | FUSG_EXEC)
#define FUSG_RWXC (FUSG_READ | FUSG_WRITE | FUSG_EXEC  | FUSG_CREAT)

/**
 * @param executable executable issuing the action (syscall)
 * @param filepath path to the file receiving this action
 * @param flags: FUSG_EXEC | FUSG_WRITE | FUSG_READ | FUSG_CREAT
 * @param timestamp: event time stamp.
 * @return 0 on success -1 otherwise
 */
int db_update(dbref_t dbc, const char* executable, const char* filepath, file_usage_t flags, uint64_t timestamp);

/**
 * get usage stats of a given executable + filepath combination.
 */
int db_fetch(dbref_t dbc, const char* executable, const char* filepath, fusg_stats_t* fusg_stats);

/**
 * Get unique id of executable.
 * @return ((uint64_t)-1) if not found
 */
uint64_t db_exec_get_id(dbref_t dbc, const char* executable);

/**
 * Get executable for id.
 * @return NULL if not found
 */
int db_exec_get_executable(dbref_t dbc, uint64_t key_long, char* buffer, size_t size);


/**
 * Get unique id of filepath.
 * @return ((uint64_t)-1) if not found
 */
uint64_t db_file_get_id(dbref_t dbc, const char* filepath);

/**
 * Get filepath for id.
 * @return NULL if not found
 */
int db_file_get_file(dbref_t dbc, uint64_t key_long, char* buffer, size_t size);

/**
 * Initialises an iterator to walk through fusg_stats entries.
 * Iteration runs in a db transaction until
 * db_iterator_release() has been called!
 *
 * NOTE: Iterator has to be released after use.
 *
 *
 * @see db_iterator_release()
 */
int db_fusg_stats_first(dbref_t dbc, fusg_stats_iterator_t* iterator);

int db_iterator_fetch(fusg_stats_iterator_t* iterator, fusg_stats_t* fusg_stats);

int db_iterator_next(fusg_stats_iterator_t* iterator);

void db_iterator_release(fusg_stats_iterator_t iterator);


static inline
fusg_stats_key_t db_iterator_get_fugs_stats_key(fusg_stats_iterator_t* iterator)
{
	return *((fusg_stats_key_t*)iterator->db_key.dptr);
}


#endif /* FUSG_DB_H_ */
