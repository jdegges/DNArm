#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<stdbool.h>
#include "db.h"

struct cgmResult{
	int read[3]; // 16 bases in each int

	uint32_t* matches; // pointer to list of match locations
	int length; // length of match list
};


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

	/* free any allocated memory and return the number items in matches */
		free(aList);
		free(bList);
		free(cList);
		free(dubMatches1);
		free(dubMatches2);
		free(dubMatches3);
	if(triple == 0)
		free(tripMatches);

	return count;
}

int cgm(int** reads, uint32_t numReads, int chunkSize, struct db* database)
{
	int i;

	struct cgmResult* results = (struct cgmResult*) malloc(sizeof(struct cgmResult)*numReads);
	if(results == NULL){
		printf("Unable to allocate memory!");
		exit(-1);
	}


	#pragma omp parallel for schedule(dynamic, chunkSize)
	for(i = 0; i < numReads; i++)
	{
		results[i].read[0] = reads[i][0];
		results[i].read[1] = reads[i][1];
		results[i].read[2] = reads[i][2];
		results[i].length = cgm_solver(reads[i][0], reads[i][1], reads[i][2], &results[i].matches, database);
	}

	int j = fgmStart(results, i);

	return i;
}
