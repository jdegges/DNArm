

#include "fgm.h"
#include "omp.h"



int fgmLaunch(uint32_t * list, uint32_t listLen, struct cgmResult * cgmData, int cgmDLen) //return value is a success/fail indicator
{
	int i, x;
	mData ** mutation;	//keeping track of our mutations...
	mutation = malloc(sizeof(mData*) * cgmDLen);	//make an array of pointers corresponding to the length of the cgmData passed in.
	
	omp_set_num_threads(8);
	
	#pragma omp parallel for
	for(x = 0; x < cgmDLen; x++)	//run through all elements of our batch.
	{
		fgm(&(mutation[x]),list, listLen, 48, cgmData[x]->matches, cgmData[x]->length, cgmData[x]->read);	//try to obtain a fine grain match of the given readSequence.
//		printf("list\t\tlistlen\t48\tmatches\tlength\tread\t\n%x\t%d\t%d\t%d\t%d\n", list[0], listLen, 48, cgmData[x]->matches[0], cgmData[x]->length, cgmData[x]->read);
	}
	
	#pragma omp single
	{

		for(x = 0; x < cgmDLen; x++)	//printout loop.
		{
			if(mutation[x]->len != -1)	//make sure there's actually data to print out...
			{
printf("%d\n", mutation[x]->len);
				for(i = 0; i < mutation[x]->len; i++)	//iterate over the mutation list, find all differences.
				{

					switch(mutation[x]->ins[i])
					{
						case DEL:
							printf("Type: DEL                    Location: %u\n", mutation[x]->ins[i], mutation[x]->locs[i]);
							break;
						
						case SNP:
							printf("Type: SNP    Mutation: %c    Location: %u\n", mutation[x]->ins[i], mutation[x]->mods[i], mutation[x]->locs[i]);
							break;
						
						case INS:
							printf("Type: INS    Mutation: %c    Location: %u\n", mutation[x]->ins[i], mutation[x]->mods[i], mutation[x]->locs[i]);
							break;
						default:
							break;	//silently fail..
					}
				}
				//clear up the data that was created by the fine grain matcher.
//				free(mutation[x]->mods);
//				free(mutation[x]->locs);
//				free(mutation[x]->ins);
//				free(mutation[x]);
				//if these are not cleared, there's a memory leak. each call to fgm will create a new mData.
			
			
			}
			//else DO NOTHING... if we can't generate a match, then we'll fail silently.
		}
		free(mutation);	//free the pointer array.
	}

	return 0;
}

