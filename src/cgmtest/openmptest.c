#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include </usr/include/stdint.h>

#include <omp.h>

#define BUF_SIZE 1024

#define print_error(format, ...) { \
  fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  fprintf (stderr, format, ##__VA_ARGS__); \
  fprintf (stderr, "\n"); \
}

int cgm_omp(uint32_t* aList, uint32_t* bList, int aLength, int bLength, int gap, int offset, int keyLength, uint32_t** matches)
{
	uint32_t* results = NULL;
	int i = 0;

	int mMax = aLength;
	if(bLength < mMax)
		mMax = bLength;

	results = (uint32_t*) malloc(sizeof(uint32_t)*mMax);
	if(results == NULL){
		printf("Unable to allocate memory!\n");
		exit(-1);
	}
	
	#pragma omp parallel for schedule(dynamic, 1)
	for(i = 0; i < aLength; i++)
	{
		results[i] = 0;
		
		int lower = 0;
		int upper = bLength-1;
				
		while(lower <= upper){
			int j = (upper + lower)/2;
			if(bList[j] == aList[i] + keyLength + gap){
				results[i] = aList[i] - offset;
				break;
			}
			else if(bList[j] > aList[i] + keyLength + gap)
				upper = j - 1;
			else
				lower = j + 1;
		
		}
		
	}

	matches = &results;
}

int doubleMatchAid(uint32_t* a, uint32_t* b, int aLength, int bLength, int secLength, uint32_t** matches, int gap, int startOffset)
{
	int i = 0, j= 0, mLength = 0;
	uint32_t* dubs = NULL;

	/* maximum length is length of smaller list */
	int mMax = aLength;
	if(bLength < mMax)
		mMax = bLength;

	dubs = (uint32_t*) malloc(sizeof(uint32_t)*mMax);
	if(dubs == NULL){
		printf("Unable to allocate memory!\n");
		exit(-1);
	}

	/* loop through the items in the first list looking for matching items in the second list */
	while(i < aLength && j < bLength){
		while(j < bLength){
			if(b[j] == a[i] + secLength + gap){
				dubs[mLength] = a[i]-startOffset;
				mLength++;
				break;
			}
			j++;
		}
		i++;
	}

	/* if results were found, return them, otherwise free the memory */
	if(mLength > 0){
		*matches = dubs;
		return mLength;
	}

	free(dubs);
	return 0;

}

int uint32_t_cmp(const void *a, const void *b)
{
	const uint32_t* cp_a = (const int*) a;
	const uint32_t* cp_b = (const int*) b;

	return *cp_a - *cp_b;
}

int main(int argc, char* argv[])
{
	if(argc != 6){
		printf("USAGE: cgmtest MAXVALUE KEYSIZE LISTSIZE REPS MAXCOMPUTEUNITS\nNote: list size should be less than  the min of max work items[0] and max work items[1]\n");
		exit(-1);
	}

	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* matches = NULL;
	uint32_t* matches2 = NULL;
	
	int maxValue = atoi(argv[1]);
	int keySize = atoi(argv[2]);
	int listSize = atoi(argv[3]);
	int reps = atoi(argv[4]);
	int cpus = atoi(argv[5]);

	aList = (uint32_t*) malloc(sizeof(uint32_t)*listSize);
	bList = (uint32_t*) malloc(sizeof(uint32_t)*listSize);

	if(aList == NULL || bList == NULL){
		printf("Error allocating memory\n");
		exit(-1);
	}
	
	omp_set_num_threads(cpus);
	sleep(10);
	struct timeval t1, t2;
    double elapsedTime;
	double openmp = 0, seq = 0;

	srand( (unsigned)time(NULL));

	int i, j;

	for(i = 0; i < reps; i++)
	{
		printf("*****Trial %d*****\nGenerating lists...", i);
		for(j = 0; j < listSize; j++)
			aList[j] = rand() % maxValue;

		for(j = 0; j < listSize; j++)
			bList[j] = rand() % maxValue;
		printf("Done.\nSorting...");
		qsort(aList, listSize, sizeof(uint32_t), uint32_t_cmp);
		qsort(bList, listSize, sizeof(uint32_t), uint32_t_cmp);
		
		printf("Done.\nExecuting...\n");
		
		
		gettimeofday(&t1, NULL);

		doubleMatchAid(aList, bList, listSize, listSize, keySize, &matches, 0, 0);

		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
		seq += elapsedTime;
		printf("Sequential: %f\n", elapsedTime);



		gettimeofday(&t1, NULL);

		cgm_omp(aList, bList, listSize, listSize, 0, 0, keySize, &matches);

		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
		openmp += elapsedTime;
		printf("OpenMP: %f\n", elapsedTime);
		
	}

	printf("\n*****Results*****:\n");
	printf("Sequential: %f\n", seq/reps);
	printf("OpenMP: %f\n", openmp/reps);

}
