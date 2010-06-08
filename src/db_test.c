#include <stdlib.h>
#include "db.h"
#include <stdint.h>
int main(void)
{

  //Open a database for writing
  void *gdbm = db_open("testdb", 1);
  uint32_t test;
uint32_t *values;
uint32_t key = 23;
uint32_t value=45;
uint32_t value2=245;
int i;
  db_insert(gdbm, key, value);
  db_insert(gdbm, key, value2);
  db_insert(gdbm, key, value2);
  db_insert(gdbm, key, value2);
  db_close(gdbm);
  gdbm = db_open("testdb", 0);
int32_t num_values = db_query (gdbm, key, &values);
for (i = 0; i < num_values; i++)
 printf("%u\n", values[i]);
 // if(test!=NULL)
  db_close(gdbm);
  return 0;
}
