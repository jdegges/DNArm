#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<stdbool.h>

int mergeLists(uint32_t* a, uint32_t* b, uint32_t* c, int aLength, int bLength, int cLength, uint32_t** result)
{
	int i = 0, j = 0, k = 0, count = 0;
	int max = aLength+bLength+cLength;

	*result = (uint32_t*) malloc(sizeof(uint32_t)*max);

	while(count < max)
	{
		if(i < aLength && (j >= bLength || a[i] < b[j]) && (k >= cLength || a[i] < c[k])){
			*result[count] = a[i];
			i++;
		}
		else if(j < bLength && (k >= cLength || b[j] > c[k])){
			*result[count] = b[j];
			j++;
		}
		else{
			*result[count] = c[k];
			k++;
		}
		count++;
	}

	return count;
}

int doubleMatch(uint32_t* a, uint32_t* b, int aLength, int bLength, int secLength, uint32_t** matches, int gap, int startOffset)
{
	int i = 0, j= 0, mLength = 0;
	uint32_t* dubs;

	int mMax = aLength;
	if(bLength < mMax)
		mMax = bLength;

	dubs = (uint32_t*) malloc(sizeof(uint32_t)*mMax);

	while(i < aLength && j < bLength){
		while(j < bLength){
			if(b[j] < a[i] + secLength + offset)
				j++;
			else if(b[j] > a[i] + secLength + offset)
				break;
			else{
				dubs[mLength] = a[i-startOffset];
				mLength++;
				break;
			}
		}
		i++;
	}

	if(mLength > 0){
		*matches = dubs;
		return mLength;
	}

	free(dubs);

}
int tripleMatch(uint32_t* a, uint32_t* b, uint32_t* c, int aLength, int bLength, int cLength, int secLength, uint32_t** matches)
{
	int i = 0, j = 0, k = 0, mLength = 0, d12Length = 0, d23Length = 0, d13Length = 0, dLength;
	uint32_t* trips, dub12, dub23, dub13, dubs;

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

	while(i < aLength && j < bLength && k < cLength){
		int ttemp = mLength;

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
	
	if(mLength > 0){
		free(dub12);
		*matches = trips;
		return mLength;
	}

	free(trips);

	d23Length = doubleMatch(b, c, bLength, cLength, secLength, &dub23, 0, secLength);
	d13Length = doubleMatch(a, c, aLength, cLength, secLength, &dub13, secLength, 0);

	dLength = mergeLists(dub12, dub23, dub13, d12Length, d23Length, d13Length, &dubs);
	
	free(dub12);
	free(dub23);
	free(dub13);
	
	*matches = dubs;
	return dLength;


}

int cgm(char* read, int readLength, int keySize)
{
	int sections, secLength;
	char* a, b, c;
	uint32_t* aList, bList, cList;
	int aLength, bLength, cLength;

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

	

	free(a);
	if(sections >= 2){
		free(b);
		if(sections == 3){
			free(c);
		}
	}

}