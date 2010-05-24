#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <loomlib/thread_pool.h>

#define BUFLEN 1024
#define INPUTLEN 1024
#define KEYLEN 32
#define NUM_THREADS 8

struct find_args
{
    uint32_t *data;
    int d_len;
    long int f_pos;
};

static pthread_mutex_t print_mutex;

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
        //printf("i:%d  num:%llu\n", i, whole);
   
        int j;
        uint64_t temp;
//	pthread_mutex_lock(&print_mutex);
        for (j = 0; j < KEYLEN; j++){
	    // shift "whole" left by j places, then shift result right 32 places
	    temp = (whole << j) >> 32;
	    uint32_t key = temp;
	    //INSERT INTO DB
      	    //db_insert(struct db *d, key, pos);
	    printf("key: %llu, pos  %d\n", key, pos);
	    pos++;
        }
//	pthread_mutex_unlock(&print_mutex);
    }

}


int main(int argc, char **argv){
    struct thread_pool *pool;
    int ret_val;

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
        // allocate space to hold input from file2
        uint32_t *input = (uint32_t*) malloc(INPUTLEN*sizeof(uint32_t));
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

	struct find_args *args;
	if (NULL == (args = malloc (sizeof *args))){
	    printf("Out of memory\n");
	}
	args->data = input;
	args->d_len = numRead;
	args->f_pos = 8 * curr_pos;

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

    return 0;
}
