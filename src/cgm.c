#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include</usr/include/stdint.h>
#include<stdbool.h>
#include "db.h"


int editValues(uint32_t* list, uint32_t length, int offset)
{
	int i, j = 0;
	
	/* adjust values to position where read would start */
	for(i = 0; i < length; i++){
		if(list[i] > offset){
			list[j] = list[i] - offset;
			j++;
		}
	}

	return j;
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
	return 0;

}

int doubleMatch(uint32_t* a, uint32_t* b, int aLength, int bLength, int secLength, uint32_t** matches)
{
	uint32_t* sing = NULL;
	uint32_t* c = NULL;
	int sLength, cLength = 0;

	int dLength = doubleMatchAid(a, b, aLength, bLength, secLength, matches, 0, 0);

	if(dLength > 0)
		return dLength;

	bLength = editValues(b, bLength, secLength);
	sLength = mergeLists(a, b, c, aLength, bLength, cLength, &sing);

	matches = &sing;
	return sLength;

}


int tripleMatch(uint32_t* a, uint32_t* b, uint32_t* c, int aLength, int bLength, int cLength, int secLength, uint32_t** matches)
{
	int i = 0, j = 0, k = 0, mLength = 0, d12Length = 0, d23Length = 0, d13Length = 0, dLength, sLength;
	uint32_t* trips = NULL;
	uint32_t* dub12 = NULL;
	uint32_t* dub23 = NULL;
	uint32_t* dub13 = NULL;
	uint32_t* dubs = NULL;
	uint32_t* sing = NULL;

	/* maximum length is length of smallest list */
	int mMax = aLength;
	int dMax = aLength;

	if(bLength < mMax){
		mMax = bLength;
		dMax = bLength;
	}
	if(cLength < mMax)
		mMax = cLength;

	trips = (uint32_t*) malloc(sizeof(uint32_t)*mMax);
	dub12 = (uint32_t*) malloc(sizeof(uint32_t)*dMax);
	if(trips == NULL || dub12 == NULL){
		printf("Unable to allocate memory!\n");
		exit(-1);
	}

	/* loop through the lists to find triple matches and double matches between 1st and 2nd list */
	while(i < aLength && j < bLength && k < cLength){
		int temp = mLength;

		while(j < bLength && k < cLength){
			if(b[j] < a[i] + secLength)
				j++;
			else if(b[j] > a[i] + secLength)
				break;
			else{
				dub12[d12Length] = a[i];
				d12Length++;

				while(k < cLength){
					if(c[k] < b[j] + secLength)
						k++;
					else if(c[k] > b[j] + secLength)
						break;
					else if(c[k] == b[j] + secLength){
						trips[mLength] = a[i];
						mLength++;
						break;
					}
				}
				
				j++;
				if(mLength > temp)
					break;
				
			}
		}
		
		i++;

	}
	
	/* if triple matches were found return them, and free the double matches */
	if(mLength > 0){
		free(dub12);
		*matches = trips;
		return mLength;
	}

	/* otherwise free the triple matches memory, find remaining double matches, and merge results */
	free(trips);

	d23Length = doubleMatchAid(b, c, bLength, cLength, secLength, &dub23, 0, secLength);
	d13Length = doubleMatchAid(a, c, aLength, cLength, secLength, &dub13, secLength, 0);

	if(d12Length != 0 && d13Length != 0 && d23Length != 0)
	{
		dLength = mergeLists(dub12, dub23, dub13, d12Length, d23Length, d13Length, &dubs);
		
		if(dub12 != NULL)
			free(dub12);
		if(dub23 != NULL)
			free(dub23);
		if(dub13 != NULL)
			free(dub13);
		
		*matches = dubs;
		return dLength;
	}

	/* if still no matches, combine the input lists and return them as results */
	bLength = editValues(b, bLength, secLength);
	cLength = editValues(c, cLength, 2*secLength);
	sLength = mergeLists(a, b, c, aLength, bLength, cLength, &sing);

	*matches = sing;
	return sLength;


}

#if 0
int cgm(char* read, int readLength, int keySize, uint32_t** matches, struct db* database)
{
	int sections, secLength, aLength, bLength, cLength, count;
	char* a;
	char* b;
	char* c;
	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* cList = NULL;

	/* determine the number of sections (0-3) and allocate memory for each key */
	if(readLength < keySize)
		return 0;
	if(readLength >= keySize){
		sections = 1;
		a = (char*) malloc(sizeof(char)*keySize/4);
	}
	if(readLength/keySize >= 2){
		sections = 2;
		b = (char*) malloc(sizeof(char)*keySize/4);
	}
	if(readLength/keySize >= 3){
		sections = 3;
		c = (char*) malloc(sizeof(char)*keySize/4);
	}

	if(a == NULL || sections >= 2 && b == NULL || sections >= 3 && c == NULL){
		printf("Unable to allocate memory!\n");
		exit(-1);
	}

	/* copy the reads into the key strings and query database */
	strncpy(a, read, keySize/4);
	aLength = db_query(database, a, &aList);
	
	if(sections >= 2){
		strncpy(b, &read[keySize/4], keySize/4);
		bLength = db_query(database, b, &bList);
	}
	
	if(sections == 3){
		strncpy(c, &read[keySize/2], keySize/4);
		cLength = db_query(database, c, &cList);

		/* if 3 sections attempt to find a triple match */
		count = tripleMatch(aList, bList, cList, aLength, bLength, cLength, keySize, matches);

		free(aList);
		free(bList);
		free(cList);
	}
	else if(sections == 2){
		/* if there are 2 sections attempt to find a double match */
		count = doubleMatch(aList, bList, aLength, bLength, keySize, matches);
		free(aList);
		free(bList);
	}
	else{
		/* if there's only 1 section, can't narrow results */
		matches = &aList;
		count = aLength;
	}

	/* free any allocated memory and return the number items in matches */
	free(a);
	if(sections >= 2){
		free(b);
		if(sections == 3){
			free(c);
		}
	}

	return count;
}
#endif

/* lets assume a few things
 *   1. readLength = 12 bytes (48 bases)
 *      uint32_t read[] = { key1, key2, key3 };
 *   2. keySize = 4 bytes (16 bases)
 */

int cgm48(uint32_t* read, uint32_t** matches, struct db* database)
{
	const int keySize = 16;
  int aLength, bLength, cLength, count;
  uint32_t* aList = NULL;
  uint32_t* bList = NULL;
  uint32_t* cList = NULL;

  /* query database */
  aLength = db_query(database, read[0], &aList);
  bLength = db_query(database, read[1], &bList);
  cLength = db_query(database, read[2], &cList);

  /* if 3 sections attempt to find a triple match */
  count = tripleMatch(aList, bList, cList, aLength, bLength, cLength, keySize, matches);

  free(aList);
  free(bList);
  free(cList);

  return count;
}

#if 0
int main (void)
{
	return 0;
}
#endif
