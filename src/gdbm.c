#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <gdbm.h>

#include <db.h>

struct db
{
  GDBM_FILE dbf;
};

struct db *
db_open(char *name, const int read)
{
  struct db *db = malloc (sizeof *db);
  if (NULL == db)
    return NULL;

  db->dbf = gdbm_open(name, 0, read ? GDBM_NEWDB : GDBM_READER, 0666, NULL);
  if (NULL == db->dbf)
  {
    free (db);
    return NULL;
  }
  return db;
}

bool
db_close(struct db *db)
{
  gdbm_close(db->dbf);
  free (db);
  return true;
} 

int32_t
db_query(struct db *db, uint32_t key, uint32_t **value)
{
  GDBM_FILE dbf = db->dbf;

  datum k = {&key, sizeof key};
  datum data;
  data  = gdbm_fetch(dbf, k);
  if(data.dptr == NULL)
    return 0;
  *value = data.dptr;
  int32_t ret_val = data.dsize / (sizeof **value);
  return ret_val;
}

bool
db_insert(struct db *db, uint32_t key, uint32_t value)
{
  GDBM_FILE dbf = db->dbf;

  datum k = {&key, sizeof key};
  int result;

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
  return result == 0;
}

