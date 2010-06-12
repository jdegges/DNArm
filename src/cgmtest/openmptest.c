#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<stdbool.h>
#include "db.h"

int listSize;
uint32_t maxValue;
double seq;

struct db{
	int x;
};

int remove_dups(uint32_t* list, int length)
{
	int i, j = 0;

	for(i = 0; i < length; i++)
	{
		if(0 <= i-1 && list[i] == list[i-1])
			i++;
		else{
			list[j] = list[i];
			j++;
		}
	}

	return j;

}

int uint32_t_cmp(const void *a, const void *b)
{
	const uint32_t* cp_a = (const int*) a;
	const uint32_t* cp_b = (const int*) b;

	return *cp_a - *cp_b;
}

int32_t db_query (struct db *d, uint32_t key, uint32_t **values)
{
	int j;
	uint32_t* list = NULL;
	
	list = (uint32_t*) malloc(sizeof(uint32_t)*listSize);
	
	if(list == NULL){
		printf("Error allocating memory\n");
		exit(-1);
	}

	for(j = 0; j < listSize; j++)
		list[j] = rand() % maxValue;

	qsort(list, listSize, sizeof(uint32_t), uint32_t_cmp);

	*values = list;
//	printf("Returning created list...\n");

	return remove_dups(list, listSize);

}


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


int doubleMatch(uint32_t* a, uint32_t* b, int aLength, int bLength, uint32_t secLength, uint32_t** matches, uint32_t gap, uint32_t startOffset)
{
	int i = 0, j= 0, mLength = 0;
	uint32_t* dubs = NULL;

	if(aLength == 0 || bLength == 0)
	{
		*matches == NULL;
		return 0;
	}

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

int cgm(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t** matches, struct db* database)
{
	int sections, secLength, aLength, bLength, cLength, dLength;
	int double1, double2, double3, double4, double5, double6;
	int quad, triple1, triple2, triple3, triple4;
	int itempA, itempB;
	int count;
	
	struct timeval t1, t2;
    double elapsedTime;
	
	uint32_t* dubMatches1 = NULL;
	uint32_t* dubMatches2 = NULL;
	uint32_t* dubMatches3 = NULL;
	uint32_t* dubMatches4 = NULL;
	uint32_t* dubMatches5 = NULL;
	uint32_t* dubMatches6 = NULL;
	
	uint32_t* tripMatches1 = NULL;
	uint32_t* tripMatches2 = NULL;
	uint32_t* tripMatches3 = NULL;
	uint32_t* tripMatches4 = NULL;

	uint32_t* quadMatches = NULL;

	uint32_t* tempA;
	uint32_t* tempB;
	uint32_t* tempC;

	int keySize = 16;

	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* cList = NULL;
	uint32_t* dList = NULL;

	aLength = db_query(database, a, &aList);
	bLength = db_query(database, b, &bList);
	cLength = db_query(database, c, &cList);
	dLength = db_query(database, d, &dList);
	
	gettimeofday(&t1, NULL);

	double1 = doubleMatch(aList, bList, aLength, bLength, keySize, &dubMatches1, 0, 0);
	double2 = doubleMatch(aList, cList, aLength, cLength, keySize, &dubMatches2, keySize, 0);
	double3 = doubleMatch(aList, dList, aLength, dLength, keySize, &dubMatches3, keySize*2, 0);
	double4 = doubleMatch(bList, cList, bLength, cLength, keySize, &dubMatches4, 0, keySize);
	double5 = doubleMatch(bList, dList, bLength, dLength, keySize, &dubMatches5, keySize, keySize);
	double6 = doubleMatch(cList, dList, cLength, dLength, keySize, &dubMatches6, 0, keySize*2);

	quad = doubleMatch(dubMatches1, dubMatches6, double1, double6, 0, &quadMatches, 0, 0);
	triple1 = doubleMatch(dubMatches1, dubMatches2, double1, double2, 0, &tripMatches1, 0, 0);
	triple2 = doubleMatch(dubMatches1, dubMatches3, double1, double3, 0, &tripMatches2, 0, 0);
	triple3 = doubleMatch(dubMatches2, dubMatches3, double2, double3, 0, &tripMatches3, 0, 0);
	triple4 = doubleMatch(dubMatches4, dubMatches5, double4, double5, 0, &tripMatches4, 0, 0);

	if(quad > 0){
		*matches = quadMatches;
		count = quad;
	}
	else if(triple1 + triple2 + triple3 + triple4 > 0)
	{
		itempA = mergeLists(tripMatches1, tripMatches2, NULL, triple1, triple2, 0, &tempA);
		itempB = mergeLists(tripMatches3, tripMatches4, NULL, triple3, triple4, 0, &tempB);
		
		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;
		
		free(tempA);
		free(tempB);

	}
	else if(double1 + double2 + double3 + double4 + double5 + double6 > 0)
	{
		itempA = mergeLists(dubMatches1, dubMatches2, dubMatches3, double1, double2, double3, &tempA);
		itempB = mergeLists(dubMatches4, dubMatches5, dubMatches6, double4, double5, double6, &tempB);

		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;

		free(tempA);
		free(tempB);

	}
	else
	{
		itempA = mergeLists(aList, bList, NULL, aLength, bLength, 0, &tempA);
		itempB = mergeLists(cList, dList, NULL, cLength, dLength, 0, &tempB);
		
		count = mergeLists(tempA, tempB, NULL, itempA, itempB, 0, &tempC);
		*matches = tempC;

		free(tempA);
		free(tempB);
	}

	/* free any allocated memory and return the number items in matches */
		free(aList);
		free(bList);
		free(cList);
		free(dList);
		free(tripMatches1);
		free(tripMatches2);
		free(tripMatches3);
		free(tripMatches4);
		free(dubMatches1);
		free(dubMatches2);
		free(dubMatches3);
		free(dubMatches4);
		free(dubMatches5);
		free(dubMatches6);
	if(quad == 0)
		free(quadMatches);

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	seq += elapsedTime;
//	printf("Time: %f\n", elapsedTime);

	return count;
}

int main(int argc, char* argv[])
{
	if(argc != 6){
		printf("USAGE: cgmtest MAXVALUE LISTSIZE THREADS CHUNKSIZE REPS \n");
		exit(-1);
	}

	

	maxValue = (uint32_t) strtoul(argv[1], NULL, 10);
	listSize = atoi(argv[2]);
	int threads = atoi(argv[3]);
	int p = atoi(argv[4]);
	int reps = atoi(argv[5]);
	
	omp_set_num_threads(threads);

	uint32_t* results = NULL;
	struct db mydb;

	struct timeval t1, t2;
	seq = 0;
	double overall = 0;
	srand( (unsigned)time(NULL));

	gettimeofday(&t1, NULL);
	
	int i, j;

	#pragma omp parallel for reduction(+:seq) \
								schedule(dynamic, p) \
								private(results)
	for(i = 0; i < reps; i++)
	{
//		printf("Executing...\n");
		
		cgm(0,0,0,0, &results, &mydb);

		free(results);
		results = NULL;


	}
	
	gettimeofday(&t2, NULL);
	overall = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	overall += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

//	struct db d;
//	uint32_t u;
//	uint32_t* vals;
//	double elapsed = 0;
//	double dbTime = 0;
//
//	for(i = 0; i < reps; i++)
//	{
//		gettimeofday(&t1, NULL);
//		db_query (&d, u, &vals);
//
//		gettimeofday(&t2, NULL);
//		elapsed = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
//		elapsed += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
//
//		dbTime += elapsed;
//	}
	
	printf("\n*****Result*****:\n");
	printf("Average Time (w/o generating lists): %f\n", seq/reps);
	printf("Average Time (w/ generating lists): %f\n", overall/reps);
//	printf("Average List Generation Time: %f\n", dbTime/reps);
//	printf("Calculated Average Time (w/o generating lists): %f\n", overall/reps - dbTime/reps*4);

}

