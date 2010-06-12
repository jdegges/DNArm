

#include "fgm.h"


//int fgmInit(uint32_t * list, uint32_t listLen, int rdLen) //return value is a success/fail indicator
//{
	
	
	
//}

// typedef struct cgmResult
// {
     // int read[3]; // 16 bases in each int
     // uint32_t* matches; // pointer to list of match locations
     // int length; // length of match list
// } cgmRes;

// typedef struct mutationData
// {
	// char * mods;	//what the nucleotide should be changed to...
	// unsigned int * locs;	//the location at which the data should be changed.
	// int * ins;	//indicates if this should be an insertion.
		////	note: an insertion will list the same location multiple times, while a deletion will have a "D" in the corresponding nucleotide modification location.
	// int len;
// } mData;

//int fgmStart(struct cgmResult * cgmData, int cgmDLen);
int fgmLaunch(uint32_t * list, uint32_t listLen, cgmRes * cgmData, int cgmDLen) //return value is a success/fail indicator
{
	int i, x;
	mData * mutation;	//keeping track of our mutations...
	
	
	for(x = 0; x < cgmDLen; x++)	//run through all elements of our batch.
	{
		mutation = fgm(list, listLen, 48, cgmData[x].matches, cgmData[x].length, cgmData[x].read);	//try to obtain a fine grain match of the given readSequence.
		if(mutation != NULL)	//make sure there's actually data to print out...
		{
			for(i = 0; i < mutation->len; i++)	//iterate over the mutation list, find all differences.
			{
				switch(mutation->ins[i])
				{
					case DEL:
						printf("Type: DEL                    Location: %u\n", mutation->ins[i], mutation->locs[i]);
						break;
						
					case SNP:
						printf("Type: SNP    Mutation: %c    Location: %u\n", mutation->ins[i], mutation->locs[i]);
						break;
						
					case INS:
						printf("Type: INS    Mutation: %c    Location: %u\n", mutation->ins[i], mutation->locs[i]);
						break;
					default:
						break;	//silently fail..
				}
			}
			//clear up the data that was created by the fine grain matcher.
			free(mutation->mods);
			free(mutation->locs);
			free(mutation->ins);
			free(mutation);
			//if these are not cleared, there's a memory leak. each call to fgm will create a new mData.
			
			
		}
		//else DO NOTHING... if we can't generate a match, then we'll fail silently.
	}
	return 0;
}
//void main()
//{
//return;
//};

//fgmReturn(); //this will return a list of mutationData structures (or just one, with all of the data consolidated, if you prefer - let me know), or NULL if the computations are not complete.
//{

//}

