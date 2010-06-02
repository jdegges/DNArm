#include <tcutil.h>
#include <tcadb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <db.h>

struct db
{
  TCADB *adb;
};

/* make sure path ends with ".tch" */

struct db *
db_open (char *path, const int parallel)
{
  struct db *db = malloc (sizeof *db);
  TCADB *adb;
  
  if (NULL == db)
    {
      fprintf (stderr, "out of memory\n");
      exit (EXIT_FAILURE);
    }

  adb = tcadbnew ();
  if (!tcadbopen (adb, path))
    {
      fprintf (stderr, "open error\n");
      exit (EXIT_FAILURE);
    }

  db->adb = adb;

  return db;
}

bool
db_close (struct db *db)
{
  TCADB *adb = db->adb;

  if (!tcadbclose (adb))
    {
      fprintf (stderr, "close error\n");
      return false;
    }

  tcadbdel (adb);
  free (db);
  return true;
}

bool
db_insert (struct db *db, const uint32_t key, const uint32_t value)
{
  TCADB *adb = db->adb;
  uint32_t k = key;
  uint32_t v = value;

  return tcadbputcat (adb, &k, sizeof k, &v, sizeof v);
}

int32_t
db_query (struct db *db, const uint32_t key, uint32_t **values)
{
  TCADB *adb = db->adb;
  uint32_t k = key;
  int count;

  *values = tcadbget (adb, &k, sizeof k, &count);
  count /= sizeof **values;
  return count;
}
