#ifndef _H_DB_FOO
#define _H_DB_FOO

#include <stdlib.h>
#include <gdbm.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

void* db_open(char *name, const int read);

void db_close(void* dbf);
void dbase_store(GDBM_FILE dbf,uint32_t key, uint32_t value);

char* dbase_fetch(GDBM_FILE dbf, uint32_t key, uint32_t **value);

#endif
