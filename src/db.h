#ifndef _DB_H
#define _DB_H

#include <stdint.h>
#include <stdbool.h>

struct db;

/*
 * Open up a db that can be accessed by parallel threads concurrently.
 * Returns NULL if some error occurred.
 */
struct db *
db_open (char *path, const int parallel);

/*
 * Close db.
 * Returns false if some error occurred, true otherwise.
 */
bool
db_close (struct db *d);

/*
 * Insert the key-value pair into the db.
 * Returns true if successfully inserted, false otherwise.
 */
bool
db_insert (struct db *d, const uint32_t key, const uint32_t value);

/*
 * Query for all values associated with key.
 * db_query() allocates space for all the items in values and returns the total
 * number of items. Upon error -1 is returned.
 */
int32_t
db_query (struct db *d, const uint32_t key, uint32_t **values);

#endif
