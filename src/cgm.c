#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include</usr/include/stdint.h>
#include<stdbool.h>
#include "db.h"


int mergeLists(uint32_t* a, uint32_t* b, uint32_t* c, int aLength, int bLength, int cLength, uint32_t** result)
{
	int i = 0, j = 0, k = 0, count = 0;
	uint32_t* mem = NULL;

	/* result should be combined length of all lists */
	int max = aLength+bLength+cLength;
	
	mem = (uint32_t*) malloc(sizeof(uint32_t)*max);
	
	if(mem == NULL){
		printf("Unable to allocate memory!\n");
		exit(-1);
	}

	/* add items in increasing order */
	while(count < max)
	{
		if(i < aLength && (j >= bLength || a[i] <= b[j]) && (k >= cLength || a[i] <= c[k])){
			mem[count] = a[i];
			i++;
		}
		else if(j < bLength && (k >= cLength || b[j] <= c[k])){
			mem[count] = b[j];
			j++;
		}
		else{
			mem[count] = c[k];
			k++;
		}
		count++;
	}

	/* return the size of the results list */
	*result = mem;
	return count;
}


int doubleMatch(uint32_t* a, uint32_t* b, int aLength, int bLength, int secLength, uint32_t** matches, int gap, int startOffset)
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
			if(b[j] < a[i] + secLength + gap)
				j++;
			else if(b[j] > a[i] + secLength + gap)
				break;
			else{
				dubs[mLength] = a[i]-startOffset;
				mLength++;
				break;
			}
		}
		i++;
	}

	/* if results were found, return them, otherwise free the memory */
	if(mLength > 0){
		*matches = dubs;
		return mLength;
	}

	free(dubs);
	*matches = NULL;
	return 0;

}

int cgm(int a, int b, int c, int d, uint32_t** matches, struct db* database)
{
	int sections, secLength, aLength, bLength, cLength, dLength
	int double1, double2, double3, double4, double5, double6;
	int quad, triple1, triple2, triple3, triple4;
	int itempA, itempB, itempC;
	int count;
	uint32_t* dubMatches1, dubMatches2, dubMatches3, dubMatches4, dubMatches5, dubMatches6;
	uint32_t* quadMatches, tripMatches1, tripMatches2, tripMatches3, tripMatches4;
	uint32_t* tempA, tempB, tempC;

	int keySize = 16;

	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* cList = NULL;
	uint32_t* dList = NULL;

	aLength = db_query(database, a, aList);
	bLength = db_query(database, b, bList);
	cLength = db_query(database, c, cList);
	dLength = db_query(database, d, dList);

	omp_set_num_threads(8);
	
	#pragma omp parallel sections
	{
		double1 = doubleMatch(aList, bList, aLength, bLength, keySize, &dubMatches1, 0, 0);
		#pragma omp section
		double2 = doubleMatch(aList, cList, aLength, cLength, keySize, &dubMatches2, keySize, 0);
		#pragma omp section
		double3 = doubleMatch(aList, dList, aLength, dLength, keySize, &dubMatches3, keySize*2, 0);
		#pragma omp section
		double4 = doubleMatch(bList, cList, bLength, cLength, keySize, &dubMatches4, 0, keySize);
		#pragma omp section
		double5 = doubleMatch(bList, dList, bLength, dLength, keySize, &dubMatches5, keySize, keySize);
		#pragma omp section
		double6 = doubleMatch(cList, dList, cLength, dLength, keySize, &dubMatches6, 0, keySize*2);
	}

	#pragma omp parallel sections
	{
		quad = doubleMatch(dubMatches1, dubMatches6, double1, double2, 0, &quadMatches, 0, 0);
		#pragma omp section
		triple1 = doubleMatch(dubMatches1, dubMatches2, double1, double2, 0, &tripMatches1, 0, 0);
		#pragma omp section
		triple2 = doubleMatch(dubMatches1, dubMatches3, double1, double2, 0, &tripMatches2, 0, 0);
		#pragma omp section
		triple3 = doubleMatch(dubMatches2, dubMatches3, double1, double2, 0, &tripMatches3, 0, 0);
		#pragma omp section
		triple4 = doubleMatch(dubMatches4, dubMatches5, double1, double2, 0, &tripMatches4, 0, 0);
	}

	if(quad > 0){
		*matches = quadMatches;
		count = quad;
	}
	else if(triple1 + triple2 + triple3 + triple4 > 0)
	{
		#pragma omp parallel sections
		{
			itempA = mergeLists(tripMatches1, tripMatches2, NULL, triple1, triple2, 0, &tempA);
			#pragma omp section
			itempB = mergeLists(tripMatches3, tripMatches4, NULL, triple3, triple4, 0, &tempB);
		}
		
		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;
		
		free(tempA);
		free(tempB);
	}
	else if(double1 + double2 + double3 + double4 + double5 + double6 > 0)
	{
		#pragma omp parallel sections
		{
			itempA = mergeLists(dubMatches1, dubMatches2, dubMatches3, double1, double2, double3, &tempA);
			#pragma omp section
			itempB = mergeLists(dubMatches4, dubMatches5, dubMatches6, double4, double5, double6, &tempB);
		}
		
		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;

		free(tempA);
		free(tempB);

	}
	else
	{
		#pragma omp parallel sections
		{
			itempA = mergeLists(aList, bList, NULL, aLength, bLength, 0, &tempA);
			#pragma omp section
			itempB = mergeLists(cList, dList, NULL, cLength, dLength, 0, &tempB);
		}
		
		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;

		free(tempA);
		free(tempB);
	}

	/* free any allocated memory and return the number items in matches */
	if(aList != NULL)
		free(aList);
	if(bList != NULL)
		free(bList);
	if(cList != NULL)
		free(cList);
	if(dList != NULL)
		free(dList);

	return count;
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

	struct timeval t1, t2;
    double elapsedTime;
	double seq = 0, binSearch = 0, parallel = 0;

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
		
		
		elapsedTime = gpu_cgm(aList, bList, listSize, listSize, keySize, &matches2, "cgmcl.cl", cpus, 1);
	
		binSearch += elapsedTime;
		printf("Binary Search: %f\n", elapsedTime);
				

		elapsedTime = gpu_cgm(aList, bList, listSize, listSize, keySize, &matches, "cgmcl2.cl", listSize, listSize);

		parallel += elapsedTime;
		printf("Simple Parallel %f\n", elapsedTime);
		
	}

	printf("\n*****Results*****:\n");
	printf("Sequential: %f\n", seq/reps);
	printf("Binary Search: %f\n", binSearch/reps);
	printf("Simple Parallel: %f\n", parallel/reps);
	

}