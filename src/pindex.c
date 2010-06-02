#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <loomlib/thread_pool.h>
#include <errno.h>
#include <string.h>
#include <db.h>

#define BUFLEN 1024
#define INPUTLEN 1024
#define KEYLEN 32
#define NUM_THREADS 8

struct find_args
{
    uint32_t *data;
    int d_len;
    long int f_pos;
    int state;
};

static pthread_mutex_t print_mutex;
static struct db *db;

static void findkeys(void *t){

	struct find_args *args = t;
	size_t inlen = args->d_len;
	uint32_t *firstptr = args->data;
	uint32_t *secondptr = args->data + 1;
	long int pos = args->f_pos;

	int i;

	for (i = 0; i < inlen - 1; i++){
		uint32_t next = *secondptr++;
		uint64_t whole = *firstptr++;
   
		whole = (whole << 32) | next;
   
		int j;
		uint64_t temp;
		for (j = 0; j < KEYLEN; j+=2){
			// shift "whole" left by j places, then shift result right 32 places
			temp = (whole << j) >> 32;
			uint32_t key = temp;
			//INSERT INTO DB
			db_insert(db, key, pos);
			//printf("key: %llu, pos  %d\n", key, pos);
			pos++;
			if (j == 0) {
				int k;
				uint32_t *values;
				int32_t count = db_query (db, key, &values);
				assert (0 < count);
				for (k = 0; k < count; k++) {
					if (values[k] == pos-1)
						break;
				}
				if (k < count)
					printf ("Inserted key\n");
			}
		}
	}
}

static void test_index(const char *path)
{
	FILE *fp = fopen (path, "rb");
	uint32_t *input;

	for (;;) {
    int i;
		input = malloc ((sizeof *input) * INPUTLEN);
		if (NULL == input)
		{
			printf ("Out of memory\n");
			exit (EXIT_FAILURE);
		}

		int curr_pos = ftell (fp);
		int numRead = fread (input, sizeof *input, INPUTLEN, fp);
		if (0 == numRead)
			break;

		for (i = 0; i < numRead; i++)
		{
			int pos = curr_pos + i*16;
			uint32_t *p = NULL;
			uint32_t *values = NULL;
			int32_t count = db_query (db, input[i], &values);

			if (count <= 0)
			{
				printf ("Error querying.\n");
				exit (EXIT_FAILURE);
			}

			p = values;

			do
			{
				if (pos == *p)
					break;
			} while (++p < values);

			if (values <= p)
				printf ("Didn't find data I put into the db\n");

			free (values);
		}
	}

  fclose (fp);
}

int state;

int main(int argc, char **argv){
	struct thread_pool *pool;
	int ret_val;
	uint32_t *input;

	assert (db = db_open (NULL, NUM_THREADS));
	assert ( pool = thread_pool_new(NUM_THREADS));

	if (0 != (ret_val = pthread_mutex_init (&print_mutex, NULL)))
	{
		printf("Failed to init mutex");
		return 1;
	}

	// Open file 
	FILE *fp = fopen(argv[1], "rb");
	if (NULL == fp)
	{
		printf("File open error!\n");
		return -1;
	}

	int  numRead;
	long int filepos;

	for(;;){
    int i;
		// allocate space to hold input from file2
		input = (uint32_t*) malloc(INPUTLEN*sizeof(uint32_t));
		if (NULL == input)
		{   
			printf("Unable to allocate input buf\n");
		}
        
		int curr_pos = ftell(fp);
		numRead = fread(input, sizeof(uint32_t), INPUTLEN, fp);

		if (0 == numRead)
		{
			break;
		}

    state = 0;
    for (i = 0; i < numRead; i++)
      {
        state += input[i];
      }

		struct find_args *args;
		if (NULL == (args = malloc (sizeof *args))){
			printf("Out of memory\n");
		}
		args->data = input;
		args->d_len = numRead;
		args->f_pos = 4 * curr_pos;
    args->state = state;

		assert (thread_pool_push (pool, findkeys, args));

		// adjust file pointer back one integer
		if (numRead < INPUTLEN)
			break;
		else
			fseek(fp, -sizeof(uint32_t), SEEK_CUR);
	}

	free(input);
	assert (thread_pool_terminate (pool));
	thread_pool_free (pool);

	if (0 != (ret_val = pthread_mutex_destroy (&print_mutex)))
	{
		printf("pthread_mutex_destroy: %d\n", ret_val);
	}

	if (0 != fclose(fp))
	{
		printf("%s", strerror(errno));
	}

	test_index (argv[1]);
	assert (db_close (db));
	return 0;
}
