/*
 * Cache Value DB
 * 
 */

#define _POSIX_SOURCE
#define _XOPEN_SOURCE 600

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <cvdb.h>

/*
 * file layout: 4 byte aligned 32 bit values, addressable by a 32 bit key
 *   key = 0,  key = 1,  key = 2,  ..., key = 2**32-1
 * +---------------------------------------------+
 * | 32 bits | 32 bits | 32 bits | ... | 32 bits |
 * +---------------------------------------------+
 */


struct cvdb
{
  FILE      *dbp;
  uint8_t   omode;
  uint32_t  *cache;
  uint64_t  offset;
};

#define print_error(msg){ \
    fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
    fprintf (stderr, "%s\n", (msg)); \
}

/* write the bytes specified to the offset specified in the file stream. this
 * function shall not return upon failure. */
static void
xfseekwrite (const void *ptr, size_t size, size_t nmemb, long offset,
             int whence, FILE *stream)
{
  if (0 != fseek (stream, offset, whence))
    {
      print_error (strerror (errno));
      exit (EXIT_FAILURE);
    }
  if (nmemb != fwrite (ptr, size, nmemb, stream))
    {
      if (feof (stream))
        print_error ("EOF");
      if (ferror (stream))
        print_error ("Stream error");
      exit (EXIT_FAILURE);
    }
}

/* read the number of bytes specified from the offset specified in the file
 * stream. this function shall not return upon failure. */
static void
xfseekread (void *ptr, size_t size, size_t nmemb, long offset, int whence,
            FILE *stream)
{
  if (0 != fseek (stream, offset, whence))
    {
      print_error (strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (nmemb != fread (ptr, size, nmemb, stream))
    {
      if (feof (stream))
        print_error ("EOF");
      if (ferror (stream))
        print_error ("Stream error");
      exit (EXIT_FAILURE);
    }
}

struct cvdb *
cvdb_open (char *path, const int mode)
{
  struct cvdb *db;
  struct stat st;

  /* make sure the mode is sane, only the following make sense:
   * READ
   * WRITE
   * READ | WRITE
   * WRITE | CREATE
   * READ | WRITE | CREATE
   */
  if (mode != DB_MODE_READ
      && mode != DB_MODE_WRITE
      && mode != (DB_MODE_READ | DB_MODE_WRITE)
      && mode != (DB_MODE_WRITE | DB_MODE_CREATE)
      && mode != (DB_MODE_READ | DB_MODE_WRITE | DB_MODE_CREATE))
    {
      print_error ("Invalid mode");
      return NULL;
    }

  if (NULL == (db = malloc (sizeof *db)))
    {
      print_error ("Out of memory");
      return NULL;
    }

  db->dbp = NULL;
  db->omode = mode;
  db->cache = NULL;
  db->offset = 0;

  if (NULL == (db->cache = calloc (1, CACHE_BYTES)))
    {
      print_error ("Out of memory");
      return NULL;
    }

  /* create file if it doesnt exist */
  if (0 != stat (path, &st))
    {
      /* make sure mode contains CREATE and WRITE */
      if (ENOENT == errno && (DB_MODE_CREATE | DB_MODE_WRITE) & mode)
        {
          int64_t size = 0x400000000 / CACHE_BYTES; /* must be exactly 16GB */

          if (NULL == (db->dbp = fopen (path, "wb+")))
            {
              print_error (strerror (errno));
              goto error;
            }

          /* zero out newly created file */
          while (size--)
            xfseekwrite (db->cache, 1, CACHE_BYTES, 0, SEEK_CUR, db->dbp);
        }
      else
        {
          print_error (strerror (errno));
          goto error;
        }
    }
  /* open existing file */
  else
    {
      /* if WRITE mode was specified then open for writing else just reading */
      if (NULL == (db->dbp = fopen (path, DB_MODE_WRITE & mode
                                          ? "rb+" : "rb")))
        {
          print_error (strerror (errno));
          goto error;
        }
    }

  /* tell OS that we are going to be accessing data sequentially */
  if (0 != posix_fadvise (fileno (db->dbp), 0, 0, POSIX_FADV_SEQUENTIAL))
    {
      print_error ("posix_fadvise");
      goto error;
    }

  /* if opened for reading then read in the first cache block */
  if (DB_MODE_READ & db->omode)
    xfseekread (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);

  return db;

error:
  if (NULL != db->dbp && 0 != fclose (db->dbp))
    print_error (strerror (errno));
  free (db->cache);
  free (db);
  return NULL;
}

bool
cvdb_close (struct cvdb *db)
{

  if (NULL == db)
    return true;

  /* if opened for writing then flush the last used cache block */
  if (DB_MODE_WRITE & db->omode)
    xfseekwrite (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);

  if (NULL != db->dbp && 0 != fclose (db->dbp))
    {
      print_error (strerror (errno));
      return false;
    }

  free (db->cache);
  free (db);
  return true;
}

bool
cvdb_put (struct cvdb *db, uint32_t key, uint32_t value)
{
  uint64_t index = ((uint64_t) key) * (sizeof value);

  if (NULL == db)
    {
      print_error ("Invalid argument ``db''");
      return false;
    }

  if (!(DB_MODE_WRITE & db->omode))
    {
      print_error ("You must open this db in write mode to PUT");
      return false;
    }

  /* if key hits cache then write value to cache */
  if (db->offset <= index && index < db->offset + CACHE_BYTES)
    db->cache[key % CACHE_SIZE] = value;
  /* if key misses cache, write current cache to disk, clear cache,
   * write value to cache, and update offset */
  else
    {
      xfseekwrite (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);
      db->offset = (index / CACHE_BYTES) * CACHE_BYTES;

      /* pull in the next cache block */
      xfseekread (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);
      db->cache[key % CACHE_SIZE] = value;
    }
  return true;
}

bool
cvdb_increment (struct cvdb *db, uint32_t key)
{
  uint64_t index = ((uint64_t) key) * sizeof (uint32_t);

  if (NULL == db)
    {
      print_error ("Invalid argument ``db''");
      return false;
    }

  if (!(DB_MODE_WRITE & db->omode))
    {
      print_error ("You must open this db in write mode to PUT");
      return false;
    }

  /* if key hits cache then write incremented value to cache */
  if (db->offset <= index && index < db->offset + CACHE_BYTES)
    db->cache[key % CACHE_SIZE]++;
  /* if key misses cache, write current cache to disk, clear cache,
   * write value to cache, and update offset */
  else
    {
      xfseekwrite (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);
      db->offset = (index / CACHE_BYTES) * CACHE_BYTES;

      /* pull in the next cache block */
      xfseekread (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);

      /* incrementn value */
      db->cache[key % CACHE_SIZE]++;
    }
  return true;
}

bool
cvdb_get (struct cvdb *db, uint32_t key, uint32_t *value)
{
  uint64_t index = ((uint64_t) key) * (sizeof *value);

  if (NULL == db)
    {
      print_error ("Invalid argument ``db''");
      return false;
    }

  if (!(DB_MODE_READ & db->omode))
    {
      print_error ("You must open this db in read mode to GET");
      return false;
    }

  /* if key hits the cache then quickly return the value */
  if (db->offset <= index && index < db->offset + CACHE_BYTES)
    *value = db->cache[key % CACHE_SIZE];
  /* if key misses cache then load the appropriate cache block from disk */
  else
    {
      /* flush current cache block */
      if (DB_MODE_WRITE & db->omode)
        xfseekwrite (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);

      db->offset = (index / CACHE_BYTES) * CACHE_BYTES;
      xfseekread (db->cache, 1, CACHE_BYTES, db->offset, SEEK_SET, db->dbp);
      *value = db->cache[key % CACHE_SIZE];
    }
  return true;
}
