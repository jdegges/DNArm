#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define BUFLEN 1024
#define INPUTLEN 1024
#define KEYLEN 32

int main(int argc, char **argv){
	// Open file 
	FILE *fp = fopen(argv[1], "r");
	if (NULL == fp){
		printf("File open error!\n");
		return -1;
	}

	uint32_t *input = (uint32_t*) malloc(INPUTLEN*sizeof(uint32_t));

  // read the first int from the file
	uint32_t last = 0;
	int numRead = fread (&last, sizeof *last, 1, fp);
	if (!numRead){
		// Not even 4 bytes in the file, exit
		return 1;
	}

	for(;;){
    // the first slot of input will be the last int read in the previous loop
		int numRead = fread(input + 1, sizeof(uint32_t), INPUTLEN - 1, fp);
		if (!numRead){
			// Done, exit
			return 1;
		}

		input[0] = last;

		int i;

		for (i = 0; i < INPUTLEN - 1; i++){
			uint32_t next = input[i+1];
			uint64_t whole = input[i];

			//printf("%llu\n", whole);
			whole = (whole << 32) | next;
			printf("%llu\n", whole);
			//printf("%llu\n", whole);
		
			int j;
			uint64_t temp;
			for (j = 0; j < KEYLEN; j++){
				// shift "whole" left by j places, then shift result right 32 places
				temp = (whole << j) >> 32;
				uint32_t key = temp;
				//printf("key:%llu\n", key);
				//INSERT INTO DB
				//db_insert(struct db *d, key, KEYLEN);
			}
		}

    // save the last int since it hasnt been processed yet
		last = input[i];
	}
}
