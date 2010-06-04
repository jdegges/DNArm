#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>

#include <db.h>

char dbn[] = "/path/to/db.txt";

#define SIZE 42900

int
main (void)
{
  struct db *d;
  struct timeval stv;
  struct timeval etv;
  uint64_t i;

  /* insert 2**32 values into the db */
  assert (NULL != (d = db_open (dbn, 1)));
  gettimeofday (&stv, NULL);
  for (i = 0; i < SIZE; i++)
    assert (true == db_insert (d, i, i));
  gettimeofday (&etv, NULL);

  /* print elapsed time (for benchmarking db implementations) */
  printf ("\n");
  printf ("inserting took: %ld s\n", etv.tv_sec - stv.tv_sec);

  /* query for all of the 2**32 values and verify that they are correct */
  gettimeofday (&stv, NULL);
  for (i = 0; i < SIZE; i++)
    {
      uint32_t *values;
      int32_t count = db_query (d, i, &values);

      assert (1 == count);
      assert (i == *values);
      free (values);
    }
  gettimeofday (&etv, NULL);

  /* print elapsed time (for benchmarking db implementations) */
  printf ("\n");
  printf ("querying took: %ld s\n", etv.tv_sec - stv.tv_sec);
  assert (true == db_close (d));

  return 0;
}
