#ifndef _CVDB_H
#define _CVDB_H

#include <stdint.h>
#include <stdbool.h>

struct cvdb;

#define DB_MODE_READ    1   /* open in read mode */
#define DB_MODE_WRITE   2   /* open in write mode */
#define DB_MODE_CREATE  4   /* if db file does not exist, create it */

#define CACHE_SIZE    0x08000000     /* 2**27 */
#define CACHE_BYTES   (CACHE_SIZE*4) /* 512M */

/*
 * Open up a db that can be accessed by parallel threads concurrently.
 * Returns NULL if some error occurred.
 */
struct cvdb *
cvdb_open (char *path, int mode);

/*
 * Close db.
 * Returns false if some error occurred, true otherwise.
 */
bool
cvdb_close (struct cvdb *db);

/*
 * PUT / GET interface:
 *   - will use exactly 16GB of disk space
 *   - for optimal performance accesses should remain local to each cache block
 */

/*
 * Store the key-value pair, if a value is already associated with the key then
 * it will be overwritten.
 */
bool
cvdb_put (struct cvdb *db, uint32_t key, uint32_t value);

/*
 * Like put except increment key's value by one
 */
bool
cvdb_increment (struct cvdb *db, uint32_t key);

/*
 * Get the value that corresponds with the provided key. The user must make
 * sure value points to allocated storage. All values will be initalized to
 * zero even if no value was ever inserted at this key.
 */
bool
cvdb_get (struct cvdb *db, uint32_t key, uint32_t *value);

#endif
