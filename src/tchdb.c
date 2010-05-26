#include <tcutil.h>
#include <tcadb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <db.h>

struct db
{
  TCHDB *hdb;
};

/* make sure path ends with ".tch" */

struct db *
db_open (char *path, const int mode)
{
  struct db *db = malloc (sizeof *db);
  int omode;
  
  if (NULL == db)
    {
      fprintf (stderr, "out of memory\n");
      exit (EXIT_FAILURE);
    }

  db->hdb = tchdbnew ();
  if (!tchdbtune (db->hdb, 500000000, 5, 10, HDBTLARGE))
    {
      fprintf (stderr, "tune error\n");
      return false;
    }

  if (!tchdbsetcache (db->hdb, 5000000))
    {
      fprintf (stderr, "setting cache\n");
      return false;
    }

  if (!tchdbsetxmsiz (db->hdb, 2*67108864))
    {
      fprintf (stderr, "set xmsiz\n");
      return false;
    }

  if (mode == DB_MODE_READ_ONLY)
    omode = HDBOREADER;
  else if (mode == DB_MODE_WRITE_ONLY)
    omode = HDBOWRITER | HDBOCREAT;
  else
    {
      fprintf (stderr, "invalid open mode\n");
      return false;
    }

  if (!tchdbopen (db->hdb, path, omode))
    {
      
      fprintf (stderr, "open error\n");
      return false;
    }
  return db;
}

bool
db_close (struct db *db)
{
  if (!tchdbclose (db->hdb))
    {
      fprintf (stderr, "close error\n");
      return false;
    }
  tchdbdel (db->hdb);
  free (db);
  return true;
}

bool
db_insert (struct db *db, uint32_t key, uint32_t value)
{
  return tchdbputcat (db->hdb, &key, sizeof key, &value, sizeof value);
}

int32_t
db_query (struct db *db, uint32_t key, uint32_t **values)
{
  int count;
  *values = tchdbget (db->hdb, &key, sizeof key, &count);
  count /= sizeof **values;
  return count;
}
