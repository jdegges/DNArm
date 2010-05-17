#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFLEN 1024
#define INPUTLEN 1024
#define KEYLEN 32

int gpos = 0;

uint32_t findkeys(uint32_t *input, int inlen){

    int i;
    uint32_t *firstptr = input;
    uint32_t *secondptr = input + 1;

    for (i = 0; i < inlen - 1; i++){
        uint32_t next = *secondptr++;
        uint64_t whole = *firstptr++;
   
        whole = (whole << 32) | next;
        //printf("i:%d  num:%llu\n", i, whole);
   
        int j;
        uint64_t temp;
        for (j = 0; j < KEYLEN; j++){
	    // shift "whole" left by j places, then shift result right 32 places
	    temp = (whole << j) >> 32;
	    uint32_t key = temp;
	    //printf("key:%llu\n", key);
	    //INSERT INTO DB
      	    //db_insert(struct db *d, key, gpos);
	    //printf("key: %llu, pos  %d\n", key, gpos);
	    gpos++;
        }
    }
    // Return last element to add as first element of the next iteration
    return input[inlen];

}


int main(int argc, char **argv){
    // Open file 
    FILE *fp = fopen(argv[1], "r");
    if (NULL == fp)
    {
	printf("File open error!\n");
	return -1;
    }

    // allocate space to hold input from file
    uint32_t *input = (uint32_t*) malloc(INPUTLEN*sizeof(uint32_t));

    // store last int of each iteration
    uint32_t stored;
    int first = 1, numRead;

    for(;;){	

	// Decrement pointer to point at first element of array
	if (first){
	    numRead = fread(input, sizeof(uint32_t), INPUTLEN, fp);
	}else{
	    numRead = fread(input, sizeof(uint32_t), INPUTLEN-1, fp);    
	    input--;
	}

	// printf("NUMREAD = %d\n", numRead);

	if (numRead == 0){
	    //if (!first)
	    //db_insert(stored, gpos)
	    break;
	}else if (numRead <= INPUTLEN - 1){
	    if (first)
		stored = findkeys(input, numRead);
 	    else
	        stored = findkeys(input, numRead+1);
	    // db_insert(stored, gpos)
	    break;
        }else{
	    stored = findkeys(input, INPUTLEN);
	    input[0] = stored;
	    input++; /* increment pointer */
	}
	
	if (first) first = 0; /* switch flag */

    }

}
