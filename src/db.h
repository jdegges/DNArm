#ifndef _H_DB_FOO
#define _H_DB_FOO

#include <stdint.h>
#include <stdbool.h>

#define DB_MODE_READ_ONLY   1
#define DB_MODE_WRITE_ONLY  2

/*
 * Open up a db that can be accessed by parallel threads concurrently.
 * Returns NULL if some error occurred.
 */
struct db *
db_open (char *path, int mode);

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
db_insert (struct db *d, uint32_t key, uint32_t value);

/*
 * Query for all values associated with key.
 * db_query() allocates space for all the items in values and returns the total
 * number of items. Upon error -1 is returned.
 */
int32_t
db_query (struct db *d, uint32_t key, uint32_t **values);

#endif
