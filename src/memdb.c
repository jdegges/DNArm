#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include <db.h>

struct db
{
  uint32_t num_keys;
  uint32_t *keys;
  uint32_t *num_values;
  uint32_t **values;
  pthread_mutex_t lock;
};

struct db *
db_open (const char *path, const int parallel)
{
  struct db *d = malloc (sizeof *d);

  (void) path;
  (void) parallel;

  if (NULL != d)
    {
      d->num_keys = 0;
      d->keys = NULL;
      d->num_values = NULL;
      d->values = 0;
    }

  pthread_mutex_init (&d->lock, NULL);

  return d;
}

bool
db_close (struct db *d)
{
  if (NULL == d)
    return false;

  while (d->num_keys--)
    free (d->values[d->num_keys]);

  free (d->values);
  free (d->num_values);
  free (d->keys);
  free (d);

  pthread_mutex_destroy (&d->lock);

  return true;
}

bool
db_insert (struct db *d, const uint32_t key, const uint32_t value)
{
  uint32_t i;
  bool retval = false;

  pthread_mutex_lock (&d->lock);

  if (NULL == d)
    goto exit;

  for (i = 0; i < d->num_keys; i++)
    if (key == d->keys[i])
      break;

  /* create spot for new key */
  if (d->num_keys <= i)
    {
      if (NULL == (d->keys = realloc (d->keys, (sizeof *d->keys)
                                             * (++d->num_keys))))
        goto exit;
      d->keys[i] = key;

      if (NULL == (d->num_values = realloc (d->num_values,
                                            (sizeof *d->num_values)
                                           * d->num_keys)))

        goto exit;
      d->num_values[i] = 0;

      if (NULL == (d->values = realloc (d->values, (sizeof *d->values)
                                                  * d->num_keys)))
        goto exit;
      d->values[i] = NULL;
    }

  /* create spot for new value and insert into list */
  if (NULL == (d->values[i] = realloc (d->values[i], (sizeof *d->values[i])
                                                   * (d->num_values[i] + 1))))
    goto exit;
  d->values[i][d->num_values[i]++] = value;

  retval = true;

exit:
  pthread_mutex_unlock (&d->lock);
  return retval;
}

int32_t
db_query (struct db *d, const uint32_t key, uint32_t **values)
{
  uint32_t i, j;
  int32_t retval = -1;

  pthread_mutex_lock (&d->lock);

  if (NULL == d)
    goto exit;

  for (i = 0; i < d->num_keys; i++)
    if (key == d->keys[i])
      break;

  if (d->num_keys <= i)
    {
      retval = 0;
      goto exit;
    }

  if (NULL == (*values = malloc ((sizeof **values) * d->num_values[i])))
    goto exit;

  for (j = 0; j < d->num_values[i]; j++)
    *values[j] = d->values[i][j];

  retval = d->num_values[i];

exit:
  pthread_mutex_unlock (&d->lock);
  return retval;
}
