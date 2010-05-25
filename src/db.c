#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "db.h"

void *
db_open(char *name, const int read)
{
// extern GDBM_FILE gdbm_open __P((char *, int, int, int, void (*)()));
  if(read==0)
    return gdbm_open(name, 0, GDBM_READER, 0666, NULL);
  else
    return gdbm_open(name, 0, GDBM_NEWDB, 0666, NULL);
}

void 
db_close(void* dbf)
{
  gdbm_close((GDBM_FILE)dbf);
} 

int db_query(GDBM_FILE dbf, uint32_t key, uint32_t **value)
{
  datum k;
  k.dptr=&key;
  k.dsize=sizeof key;
  datum data;
  data  = gdbm_fetch(dbf, k);
  if(data.dptr == NULL)
    return NULL;
  *value = data.dptr;
  int32_t ret_val = data.dsize / (sizeof **value);
  return ret_val;
}

int
db_insert(GDBM_FILE dbf, uint32_t key, uint32_t value)
{
  datum k;
  int result;
  k.dptr = &key;
  k.dsize = sizeof key;

  datum data = gdbm_fetch(dbf, k);
  if(data.dptr!=NULL){
    int size=data.dsize;
    if (NULL == (data.dptr = realloc (data.dptr, data.dsize + sizeof value)))
      perror("Failed creating inserting key/value pair");
    data.dsize += sizeof value;
    memcpy (data.dptr+size, &value, sizeof value);
  }
  else{
    data.dptr = &value;
    data.dsize = sizeof value;
  }
  
  result= gdbm_store(dbf, k, data, GDBM_REPLACE);
  if(result==-1)
     perror("You tried inserting a key as a reader"); 
  return result;
}

