#ifndef _CLDB_H
#define _CLDB_H

#include <stdint.h>
#include <stdbool.h>

struct cldb;

/*
 * Open up a db in read only mode that can be accessed by threads concurrently.
 * Returns NULL if some error occurred.
 */
struct cldb *
cldb_open (char *path, int thread);

/*
 * Close db.
 * Returns false if some error occurred, true otherwise.
 */
bool
cldb_close (struct cldb *db);

/*
 * Get the values that corresponds with the provided key. The user must make
 * sure value points to allocated pointer storage. The returned list of values
 * will only be valid so long as the current L2 block is cached.
 */
int32_t
cldb_get (struct cldb *db, uint32_t key, uint32_t **value);

/*
 * Create a new CLDB at the specified path.
 */
bool
cldb_new (char *path, char *geometry, char *genome);

#endif
