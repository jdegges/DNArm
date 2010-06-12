#include <stdlib.h>
#include <stdio.h>

#include <cldb.h>
#include <pthread.h>

static const uint64_t parallel = 4;

struct cldb *db;
static const uint64_t key_max = 4294967296;
static const uint64_t work_size = 4294967296 / 4;

static void *
prepare (void* vptr)
{
  uint64_t *id = vptr;
  uint32_t *values;
  uint64_t k;

  for (k = *id; k < key_max; k+=*id)
    {
      int32_t count = cldb_get (db, k, &values);
      if (count < 0)
        {
          fprintf (stderr, "invalid count: %d\n", count);
          exit (EXIT_FAILURE);
        }
    }
  free (id);
  return NULL;
}

int
main (void)
{
  pthread_t tid[parallel];
  int i;

  if (NULL == (db = cldb_open ("/mnt/gi.bin", 4)))
    {
      fprintf (stderr, "cldb_open\n");
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < parallel; i++)
    {
      uint64_t *id = malloc (sizeof *id);
      *id = i;

      pthread_create (&tid[i], NULL, prepare, id);
    }

  for (i = 0; i < parallel; i++)
    pthread_join (tid[i], NULL);

  cldb_close (db);
  exit (EXIT_SUCCESS);
}

