#include <stdlib.h>
#include <stdio.h>

#include <cldb.h>
#include <pthread.h>

struct arg_data{
  struct cldb *db;
  uint64_t key;
  uint32_t* values;
};  	

uint64_t total=0;
pthread_mutex_t total_lock;

void prepare(void* threadarg)
{
   struct arg_data *my_data;
   my_data = (struct arg_data *) threadarg;
   int32_t gi_count = cldb_get(my_data->db, my_data->key, &(my_data->values));
   if (gi_count < 0)
   {
    fprintf (stderr, "invalid count: %d\n", gi_count);
    exit (EXIT_FAILURE);
   }  
    total+=gi_count;
}

int
main (void)
{
  total=0;
  pthread_mutex_init(&total_lock, NULL);
  uint64_t key_max = 4294967296;
  uint64_t key;
  int i;
  struct cldb *db = cldb_open ("/mnt/gi.bin", 4);
  pthread_t pthreads[4];
  if (NULL == db)
    {
      fprintf (stderr, "cldb_open\n");
      exit (EXIT_FAILURE);
    }

  while(key < key_max)
  {
    for(i=0; i< 4; i++){
      uint32_t *gi_values;
      struct arg_data data;
      data.db = db;
      data.key = key;
      data.values = gi_values;

      pthread_create(&pthreads[i], NULL, prepare, (void*) &data);
      key++;
    }
    for(i=0; i< 4; i++){
      pthread_join(pthreads[i], NULL);
    }

  }
   

  cldb_close (db);
  exit (EXIT_SUCCESS);
}

