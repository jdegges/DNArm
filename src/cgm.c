#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
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

int cgm(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t** matches, struct db* database)
{
	int sections, secLength, aLength, bLength, cLength, dLength;
	int double1, double2, double3, double4, double5, double6;
	int quad, triple1, triple2, triple3, triple4;
	int itempA, itempB;
	int count;
	
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

