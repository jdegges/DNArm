#ifndef _FGM_H
#define _FGM_H

#include<stdio.h>
#include<stdlib.h>
#include</usr/include/stdint.h>

#define THRESHOLD 5
#define INDEL_THRESHOLD 27

//flags for mutation flags
#define SNP 0
#define INS 1
#define DEL -1

typedef struct mutationData
{
	char mods [16];	//what the nucleotide should be changed to...
	uint32_t locs [16];	//the location at which the data should be changed.
	int ins [16];	//indicates if this should be an insertion.
			//note: an insertion will list the same location multiple times, while a deletion will have a "D" in the corresponding nucleotide modification location.
	int len;
} mData;

unsigned int smallCount(uint32_t diff);

unsigned int breakDetect(uint32_t * leadSeq, uint32_t * followSeq, int len);

unsigned int mergeCount(uint32_t * leadSeq, uint32_t * followSeq, uint32_t cPoint, int len);

unsigned int diffCalc(uint32_t * refSeq, uint32_t * rdSeq, int readLen, uint32_t * diffSeq);

void mutationDetect(mData * returnData, uint32_t *  diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc);

void insDetect(mData * returnData, uint32_t * diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc, int insLen, int insLoc, int ldOffset);

void delDetect(mData * returnData, uint32_t * diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc, int delLen, int delLoc, int ldOffset);

mData * fgm(uint32_t * list, uint32_t listLen, int readLen, uint32_t * matchLocLst, int mllLen, uint32_t * rdSeq);

#endif
