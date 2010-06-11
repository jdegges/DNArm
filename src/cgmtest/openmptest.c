#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include</usr/include/stdint.h>
#include<stdbool.h>
#include "db.h"

int listSize;
uint32_t maxValue;
double seq;
int output = 0;



struct cgmResult{
	int read[3]; // 16 bases in each int

	uint32_t* matches; // pointer to list of match locations
	int length; // length of match list
};

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

uint32_t dbtable[3][5];
int num = 0;

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
				if(startOffset < a[i]){
					dubs[mLength] = a[i]-startOffset;
					mLength++;
				}
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

int cgm_solver(uint32_t a, uint32_t b, uint32_t c, uint32_t** matches, struct db* database)
{
	int sections, secLength, aLength, bLength, cLength;
	int double1, double2, double3;
	int triple;
	int count;

	struct timeval t1, t2;
    double elapsedTime;

	uint32_t* dubMatches1 = NULL;
	uint32_t* dubMatches2 = NULL;
	uint32_t* dubMatches3 = NULL;

	uint32_t* tripMatches = NULL;

	uint32_t* temp;

	int keySize = 16;

	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* cList = NULL;

	aLength = db_query(database, a, &aList);
	bLength = db_query(database, b, &bList);
	cLength = db_query(database, c, &cList);

	gettimeofday(&t1, NULL);

	double1 = doubleMatch(aList, bList, aLength, bLength, keySize, &dubMatches1, 0, 0);
	double2 = doubleMatch(aList, cList, aLength, cLength, keySize, &dubMatches2, keySize, 0);
	double3 = doubleMatch(bList, cList, bLength, cLength, keySize, &dubMatches3, 0, keySize);

	triple = doubleMatch(dubMatches1, dubMatches2, double1, double2, 0, &tripMatches, 0, 0);

	if(triple > 0){
		*matches = tripMatches;
		count = triple;
	}
	else if(double1 + double2 + double3 > 0)
	{
		count = mergeLists(dubMatches1, dubMatches2, dubMatches3, double1, double2, double3, &temp);
		*matches = temp;
	}
	else
	{
		count = mergeLists(aList, bList, cList, aLength, bLength, cLength, &temp);

		*matches = temp;
	}


	if(output == 1){
		#pragma omp critical
		{
			int i;
			printf("AList: ");
			for(i = 0; i < aLength; i++)
				printf("%d ", aList[i]);
			printf("\nBList (-16): ");
			for(i = 0; i < bLength; i++)
				printf("%d ", bList[i]-16);
			printf("\nCList (-32): ");
			for(i = 0; i < cLength; i++)
				printf("%d ", cList[i]-32);
			printf("\nMatches: ");
			for(i = 0; i < count; i++)
				printf("%d ", (*matches)[i]);
			printf("\n\n***********************************\n\n");
		}
	}

	/* free any allocated memory and return the number items in matches */
		free(aList);
		free(bList);
		free(cList);
		free(dubMatches1);
		free(dubMatches2);
		free(dubMatches3);
	if(triple == 0)
		free(tripMatches);

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
	seq += elapsedTime;
//	printf("Time: %f\n", elapsedTime);


	return count;
}


int cgm(int** reads, uint32_t numReads, int chunkSize, struct db* database)
{
	int i;
	seq = 0;
	struct timeval t1, t2;
	double overall = 0;
	
	gettimeofday(&t1, NULL);

//	uint32_t* matches = NULL;
	#pragma omp parallel for reduction(+:seq) \
							schedule(dynamic, chunkSize)
	for(i = 0; i < numReads; i++)
	{
		uint32_t* matches = NULL;
		cgm_solver(0, 0, 0, &matches, database);
		free(matches);
	}
//	int j = fgmStart(results, i);
	gettimeofday(&t2, NULL);
	overall = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
	overall += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms

	printf("\n*****Result*****:\n");
	printf("Average Time (w/o generating lists): %f\n", seq/numReads);
	printf("Average Time (w/ generating lists and overhead): %f\n", overall/numReads);
	printf("Total Time (w/ generating lists and overhead): %f\n", overall);
	return i;
}


int main(int argc, char* argv[])
{
	if(argc != 6 && argc != 7){
		printf("USAGE: cgm MAXVALUE LISTSIZE THREADS CHUNKSIZE REPS [-o]\n-o is used to print list values to cout");
		exit(-1);
	}

	maxValue = (uint32_t) strtoul(argv[1], NULL, 10);
	listSize = atoi(argv[2]);
	int threads = atoi(argv[3]);
	int p = atoi(argv[4]);
	int reps = atoi(argv[5]);

	if(argc == 7 && argv[6][0] == '-' && argv[6][1] == 'o')
		output = 1;

	omp_set_num_threads(threads);

	uint32_t* results = NULL;
	struct db mydb;


	srand( (unsigned)time(NULL));

	int** reads = NULL;



	cgm(reads,reps,p,&mydb);



}

