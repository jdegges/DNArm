//FILE: fgm.c (fine-grain matcher)
//AUTHOR: Joel C. Miller
//UCLA 2010, Spring Quarter
//Produced for CS 133/CS CM124 Joint Final Project - DNArm (fast DNA read mapper)
//
//MIT LICENCE...
//

//#include "fgm.h"
#define THRESHOLD 5
#define INDEL_THRESHOLD 27


typedef struct mutationData
{
	char * mods;	//what the nucleotide should be changed to...
	unsigned int * locs;	//the location at which the data should be changed.
	int * ins;	//indicates if this should be an insertion.
			//note: an insertion will list the same location multiple times, while a deletion will have a "D" in the corresponding nucleotide modification location.
	int len;
} mData;

//so, find locations...

//regardless of match accuracy, we'll be given a read sequence (probably of length 96) that has 3 16-length (32b) index tag sections.
//for error/mutation tracking:
//shall we just use chars? if so, A/T/G/C/I(insert after this position)/D(delete this location)
//the difference between errors and mutuations will be investigated in the mutation-tracking algorithm.
//i.e. repeatable differences are mutations, nonrepeatable differences are errors. (MUST WE DEAL WITH DIPLOID OR ONLY HAPLOID?)

//we will be given a location (or list of locations) to investigate.
//so, we need to find the best match - either verify the single map location, detect the most valid matching location, or declare a non-match if we can't get below threshold, in which case the read segment gets bounced back to the CGM (coarse-grain matcher).

//INTERFACE: this function should be called from the CGM. If there is not a valid match to be made, NULL is returned, signifying that the CGM should try a different index zone of the read segment (this case is indicative of an error, mutation, or insertion/deletion in the index zone). if a valid match is found, a data structure will be returned (linked list? pair vector? TBD) detailing any and all discrepancies detected, below the threshold (a perfect match will return a structure with a perfect match flag set, or something, because NULL will signify no match!). This structure, after being passed back to the CGM, should then be passed into the error tracker.

//args are: the read sequence list (?), the length of the read seq (counted in multiples of 16 bits), the list of probable location of the match, the read sequence itself
//match location is the leftmost bit we're looking at.
//helper... this just assists with the match list assignment,ins/del detection (+/- 3 shift), and some prep work (bit alignment, surrounding data retrieval, etc). the real work is done in diffCalc().
//list is the complete reference genome (i think)...



//this is a small helper function that just implements the bit-counting algo we used in diffCalc(), but only on one pair of uint32_t. this will nominally be used with diff sequences.
__kernel void smallCount(unsigned int * rtn, uint32_t diff)
{
	//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary (note: "0_" is the structure here.)
	diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
	rtn[0] = ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
	return;
}
//-----------------------------------READ LENGTH IS 32 or 64 bits (let's say 64. thats a nice number.)!


//this will return the location at which an insertion or deletion probably occured. 
__kernel void breakDetect(unsigned int * rtn, uint32_t * leadSeq, uint32_t * followSeq, int len)
{
	int x, i;
	uint32_t trueSeq;//, follow;	//temp
	int temp, lCnt, fCnt;
	int primeCount = 16;	//the lowest number of disagreements we've found so far... initialize to a high value.
	int primeLoc;	//the best location we've found so far.
	uint32_t leadMask, followMask;
	uint32_t lead, follow;	//the lead/follow sequences that we'll be using for comparison. these are of length 16 nucleotides, but in each iteration of the loop we want to shift over by only 8... so we have to screw with them a bit.
	int aLen = 2 * len - 1;
//	* dCount = 0;
	unsigned int rVal;
	
//	for(x = len - 1; x >= 0; x--)	//for all of the bins...
	for(x = 0; x < aLen; x++)	//for all the bins...
	{	
	
		if(x%2)	//if this is an odd iteration, we'll want to do a compound evaluation.
		{
			lead = ((leadSeq[x/2] & 0x00005555) << 16) | ((leadSeq[x/2 + 1] & 0x55550000) >> 16);	//create the compound diff sequences.
			follow = ((followSeq[x/2] & 0x00005555) << 16) | ((followSeq[x/2 + 1] & 0x55550000) >> 16);	
		}
		else
		{
			lead = leadSeq[x/2];	//assign the unaltered diff sequences.
			follow = followSeq[x/2];
		}
		//commentary: compound diff sequence evaluations are done in order to reliably detect indels that occur on or near boundaries of uint32_t's.
		
		//on each bin, count all of the set bits. if it's above THRESHOLD/2, then we want to look here.
//		temp = smallCount(lead);
//		*dCount += temp;
		smallCount(&lCnt, lead);
		smallCount(&fCnt, follow);
		if(lCnt < (THRESHOLD/2) || fCnt < (THRESHOLD/2))
			continue;

		leadMask = 0x00000000;		//start with the follow mask as the entire sequence...
		followMask = 0x55555555;
		for(i = 0; i < 16; i++)	//mask shifting...
		{
			trueSeq = leadMask & lead | followMask & follow;	//integrate the two sequences...
			smallCount(&temp, trueSeq);
			if(temp <= primeCount)	//if we've found a better match...
			{
				primeLoc = i + 8 * x;// + 1;////////////////////////////
				primeCount = temp;	//save the min count and the location.
			}
			
			leadMask = (leadMask >> 2) | 0x40000000;	//shift the mask.
			followMask >>= 2;	//shift the mask.
		}

	}	
	rtn[0] = primeLoc;
	return;
}

__kernel void mergeCount(unsigned int * rtn, uint32_t * leadSeq, uint32_t * followSeq, uint32_t cPoint, int len)	//changepoint, and read seq length in uint32_t's.
{
	int i, j;
	uint32_t * temp = leadSeq;
	uint32_t trueDiff;	//the actual diff sequence we'll analyze, either grabbed from lead, follow, or compounded.
	unsigned int dCount = 0;
	uint32_t changeover = (cPoint%16)*2;
	int tCnt;
	
	
	for(i = 0; i < len; i++)
	{
		if(temp[i] == 0)
			continue;	//skip this section if there's nothing to look at...
		
		if(cPoint%16 && cPoint/16 == i )	//if the changePoint occurs in THIS uint32_t and doesn't occur on the border... then we'll want to create a compound diff sequence.
		{
			trueDiff = (leadSeq[i] & (0x55555555 << (32 - changeover))) | (followSeq[i] & (0x55555555 >> changeover));	//construct the compounded diff sequence...
			temp = followSeq;
		}
		else if(followSeq[i] == 0)
		{
			temp = followSeq;
			continue;
		}
		else
		{	//this will start as the lead sequence, then we will shift to the follow sequence when appropriate.
			trueDiff = temp[i];
		}
		smallCount(&tCnt, trueDiff);
		dCount += tCnt;	//find the number of differences in the constructed diff sequence.
	}
	rtn[0] = dCount;
	return;
//return the total count.
}

//args are the aligned reference sequence, the read sequence, and the number of uint32_t's we need to compare.
//* passbackdata 



__kernel void diffCalc(unsigned int * rtn, uint32_t * refSeq, uint32_t * rdSeq, int readLen, uint32_t * diffSeq)
{	//in here, surround data is present, and aligned to have displacement 0 at beginning of , read sequence is bit aligned as well.
		int x = 0;
		uint32_t diff; //[readLen] diff;	//the temp and final diff variable.
		unsigned int dCount = 0;
		for(x; x < readLen; x++)	//for all of the bins...
		{
			//we need this so we don't artificially inflate the number of differences we have.
			diff = refSeq[x] ^ rdSeq[x];	//XORing the ref and read seqences will detect the differences... but we still have two possible difference sites per nucleotide. we need this reduced to 1 difference site.
			diff = (diff | (diff >> 1)) & 0x55555555;	//this will reduce the possible difference sites for each nucleotide to only one, and zero fill the rest.
			
			diffSeq[x] = diff;	//save this to our output argument.
			
			//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
			diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary
			diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
			dCount += ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
		}
		rtn[0] = dCount;
		return;
//this is the total number of differences in the sequence.
}

// * Base | lower case | upper case
// * ------------------------------
// * A    | 0110 0001  | 0100 0001
// * C    | 0110 0011  | 0100 0011
// * G    | 0110 0111  | 0100 0111
// * T    | 0111 0100  | 0101 0100
// *              ^^           ^^


__kernel void mutationDetect(mData * returnData, uint32_t *  diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc)	//the only thing detected here will be mutations or errors
{
	int i, j;
	int m = 0;	//counts the number of modifications
	uint32_t dTemp, rTemp;
	
	for(i = 0; i < rdLen; i++)//loop through each of the bins...
	{
		if(diffSeq[i] == 0)	//if there's absolutely no differences in this segment...
			continue;
		//otherwise do some calculation.
		dTemp = diffSeq[i];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];	//copy the read sequence to a temp variable for manipulation.
		for(j = 0; j < 16; j++)	//run through a single uint32_t
		{
			if((dTemp & 0x40000000) == 0x40000000)	//if the first nucleotide was different...
			{
				switch (rTemp & 0xC0000000)	//mask the two most significant bits, and insert the data where needed.
				{
					case 0x00000000:
						returnData -> mods[m] = 'A';
						break;
					case 0x40000000:
						returnData -> mods[m] = 'C';
						break;
					case 0x80000000:
						returnData -> mods[m] = 'T';
						break;
					case 0xC0000000:
						returnData -> mods[m] = 'G';
						break;
				}
				returnData -> locs[m] = startLoc + i*16 + j;	//store the absolute location that the mutation should be inserted into the reference genome.
				returnData -> ins[m] = 0;
				m++;	//increment the error storage index.
			}
			rTemp = rTemp << 2;	//shift over.
			dTemp = dTemp << 2;
		}
	}
	
//	returnData -> len = m;	//save the number of mutations.
}

__kernel void insDetect(mData * returnData, uint32_t * diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc, int insLen, int insLoc, int ldOffset)
{
	int i, j, k;
	int m = 0;	//counts the number of modifications
	uint32_t dTemp, rTemp;
	int comp = 0; //compensation factor...
//	uint32_t * diffSeq;	//this is used to fluently switch between the lead and follow sequences.
//	diffSeq = dSeqLead;
//	returnData -> len = insLen +;
	k = 0;	//init to zero here...
	
	for(i = 0; i < rdLen; i++)
	{
		dTemp = diffSeq[i+comp];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];
		for(j = 0; j < 16; j++)	//run through a single uint32_t
		{
			if ((i * 16 + j) >= insLoc && (j+k+(16*i)) < (insLen + insLoc))	//if we've found the insertion location...
			{
			
				for(k; k < insLen && (j + k) < 16; k++)
				{
					switch (rTemp & 0xC0000000)	//mask the two most significant bits, and insert the data where needed.
					{
						case 0x00000000:
							returnData -> mods[m] = 'A';
							break;
						case 0x40000000:
							returnData -> mods[m] = 'C';
							break;
						case 0x80000000:
							returnData -> mods[m] = 'T';
							break;
						case 0xC0000000:
							returnData -> mods[m] = 'G';
							break;
					}
					returnData -> locs[m] = startLoc + i*16 + j + ldOffset;	//store the absolute location that the mutation should be inserted into the reference genome.
					returnData -> ins[m] = 1;
					m++;	//increment the error storage index.
					rTemp <<= 2;	//shift over.
//					dTemp << 2;
//					diffSeq -= (sizeof(uint32_t) * rdLen);	//traverse to the proper diff sequence...
					comp -= 3;
				}
				dTemp = diffSeq[i+comp] << (2*(j+k));	//shift over to return to the same relative location we were at before the traverse.
				//j += (insLen - 1);
			}
			else if((dTemp & 0x40000000) == 0x40000000)	//if the first nucleotide was different...
			{
				switch (rTemp & 0xC0000000)	//mask the two most significant bits, and insert the data where needed.
				{
					case 0x00000000:
						returnData -> mods[m] = 'A';
						break;
					case 0x40000000:
						returnData -> mods[m] = 'C';
						break;
					case 0x80000000:
						returnData -> mods[m] = 'T';
						break;
					case 0xC0000000:
						returnData -> mods[m] = 'G';
						break;
				}
				returnData -> locs[m] = startLoc + i*16 + j + ldOffset;	//store the absolute location that the mutation should be inserted into the reference genome.
				returnData -> ins[m] = 0;
				m++;	//increment the error storage index.
			}
			rTemp = rTemp << 2;	//shift over.
			dTemp = dTemp << 2;
		}
	}
	returnData -> len = m;
}

__kernel void delDetect(mData * returnData, uint32_t * diffSeq, uint32_t * rdSeq, int rdLen, uint32_t startLoc, int delLen, int delLoc, int ldOffset)
{
	int i, j, k;
	int m = 0;	//counts the number of modifications
	uint32_t dTemp, rTemp;
	int comp = 0;	//compensation factor...
	k = 0;
//	uint32_t * diffSeq;	//this is used to fluently switch between the lead and follow sequences.
//	diffSeq = dSeqLead;
	
	for(i = 0; i < rdLen; i++)
	{
		dTemp = diffSeq[i+comp];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];
		for(j = 0; j < 16; j++)	//run through a single uint32_t
		{
			if((i * 16 + j) >= delLoc && (j+k+(16*i)) < (delLen + delLoc))	//if this location is the deletion point...
			{
				for(k = 0; k < delLen && (j + k) < 16; k++)	//as long as the delete sequence isnt complete...
				{
					returnData -> mods[m] = 'D';	//insert delete symbol...
					returnData -> locs[m] = startLoc + i*16 + j + k + ldOffset;	//and keep track of absolute location.
					returnData -> ins[m] = -1;
					m++;
					
//					diffSeq += (sizeof(uint32_t) * rdLen);	//traverse to the proper diff sequence...
//					diffSeq++;
					comp += 3;
				}
				
				dTemp = diffSeq[i+comp] << (2*j);	//shift over to return to the same relative location we were at before the traverse.
				//j += (delLen - 1);
			}
			else if((dTemp & 0x40000000) == 0x40000000)	//if the first nucleotide was different...
			{
				switch (rTemp & 0xC0000000)	//mask the two most significant bits, and insert the data where needed.
				{
					case 0x00000000:
						returnData -> mods[m] = 'A';
						break;
					case 0x40000000:
						returnData -> mods[m] = 'C';
						break;
					case 0x80000000:
						returnData -> mods[m] = 'T';
						break;
					case 0xC0000000:
						returnData -> mods[m] = 'G';
						break;
				}
				returnData -> locs[m] = startLoc + i*16 + j + ldOffset;	//store the absolute location that the mutation should be inserted into the reference genome.
				returnData -> ins[m] = 0;
				m++;	//increment the error storage index.
			}
			rTemp = rTemp << 2;	//shift over.
			dTemp = dTemp << 2;
		}
	}
	returnData -> len = m;
}

//=============================================================================
//--list is the complete base genome
//--readlen is the length of the read in nucleotides. THIS SHOULD BE IN MULTIPLES OF 16 FOR EXPECTED BEHAVIOR.
//--matchloclst is a list of match locations; the units are in nucleotides. this number indicates the START of the sequence, regardless of where the index was actually mapped to.
//--mllLen is the number of possible match locations.
//--rdSeq is the read sequence. 
//--listLen is the length of the reference genome.
__kernel void fgm(mData * returnData, uint32_t * list, uint32_t listLen, int readLen, uint32_t * matchLocLst, int mllLen, uint32_t * rdSeq)
{	
	int start;	//matchloclst is in nucleotides, correct into bits.
	int len = readLen>>4; //readlen is in nucleotides, correct into uint32_t sizing.
	int lBound; //correct from nucleotides into sizing of uint32_t
	int offset; //correct offset from nucleotides into bits.
	int min, secMin;
	int minIdx, secMinIdx;
	int c;
	int lowest, highest;
	int i;
	uint32_t lastInd = listLen >> 4;	//this will tell us what the index of the last bin is. this is important for error checking.
	uint32_t * refSeq = malloc (7*sizeof(uint32_t)*len);	//this is where we'll construct and align the reference sequence. keep everything because we need to compare later.
	uint32_t * diffSeq = malloc (7*sizeof(uint32_t)*len);	//doesnt really need to be initialized.
	uint32_t * lead, * follow;	//used for detecting indels.
//	uint32_t * matchSeq = &(diffSeq[len+3]);	//default our match sequence to be the zero-skew element.
	int compCount[7];	//the number of elements which disagree in each test. compCount[3] is zero-skew.
	int refRow;
	int skew;
	int correct;
	int curOffset;	//get absolute current offset.
	int insH, insL, delH, delL;
	int indelLen;	//length of the indel (we can track up to +/- 3. HOWEVER WE CAN EASILY EXPAND THIS ALGORITHM TO HANDLE A LOT LARGER INDELS)
	int indelLoc;	//the location where the indel starts (indicates either the location of the first inserted nucleotide, or the location at which nucleotides began to be deleted.
	int ins, del; //flag for detecting insertion or deletion
	int mCount;
	
//	mData * returnData;
//printf("FUNCTION...\n");
	//for each location in matchlist
	for(i = 0; i < mllLen; i++)
	{
		ins = 0; //default to no insertioin and no deletion.
		del = 0;
		start = matchLocLst[i] * 2;	//matchloclst is in nucleotides, correct into bits.
//		len = readLen>>4; //readlen is in nucleotides, correct into uint32_t sizing.
		
		lBound = matchLocLst[i] >>4; //correct from nucleotides into sizing of uint32_t
		offset = matchLocLst[i]%16*2; //correct offset from nucleotides into bits.
		min = 96;	//initialize these to out of range.
		secMin = 96;
		minIdx = 0;	//doesnt really matter... but initialize anyways.
		secMinIdx = 0;
		highest = -1;
		lowest = 7;	//init to out-of-range
		
		
		for (c = 0; c < len*7; c++)	//initialize our refsequence skew storage variable to all zeros, for all skew values..
		{
			refSeq[c] = 0;	//initialize.
		}

		refRow = 0;
		for (skew = -6; skew <= 6; skew += 2)	//for skewing L/R by 3 to detect indels. //##########make this a separate function?
		{
			
			correct = 0;	//correction term for over/underflow...
			curOffset = (offset+skew)%32;	//get absolute current offset.
			if(curOffset < 0) curOffset += 32;
			if(offset + skew < 0) correct = -1;	//this corrects for overflow/underflow when we're skewing.
			if(offset + skew >= 32) correct = 1; 
			
			if(lBound + correct < 0 || lBound + correct + len >= lastInd)	//if our compensation algorithm will skew us past the beginning or the end of the reference genome, this element should be skipped!
			{
				for (c = 0; c < len; c++)
					refSeq[refRow+c] = 0;
			}
			else 
				for (c = 0; c < len; c++)	//otherwise copy and align the data from the reference sequence.
				{
					if(curOffset == 0)	//for some reason, a non offset sequence produced an error with the original algorithm... something about shifting >>32, or whatever.
						refSeq[refRow+c] |= (list[lBound + correct + c]);	//no offset, so dont shift.
					else
					{
						refSeq[refRow+c] |= (list[lBound + correct + c]) << curOffset;	//first segment.
						refSeq[refRow+c] |= (list[lBound + correct + c + 1]) >> (32-curOffset); //remaining portion of fill in... 
//printf("%x\n", refSeq[refRow+c]);
					}
				}
			//we just aligned the reference sequence properly. now we can do a direct bit comparison and count bits...
		
			
			refRow += len;	//note: refRow]3] == skew of 0... also corresponds to refSeq[9], refSeq[10] and refSeq[11] (using 96-bit readsequence length) and to compCount[3]
		}
//printf("5\n");
		//run calculations for +/- 3 skewed reference sequences.
		diffCalc(&(compCount[0]), &(refSeq[0]), rdSeq, len, &(diffSeq[0]));	
		diffCalc(&(compCount[1]), &(refSeq[len*1]), rdSeq, len, &(diffSeq[len*1]));	
		diffCalc(&(compCount[2]), &(refSeq[len*2]), rdSeq, len, &(diffSeq[len*2]));	
		diffCalc(&(compCount[3]), &(refSeq[len*3]), rdSeq, len, &(diffSeq[len*3]));	
		diffCalc(&(compCount[4]), &(refSeq[len*4]), rdSeq, len, &(diffSeq[len*4]));	
		diffCalc(&(compCount[5]), &(refSeq[len*5]), rdSeq, len, &(diffSeq[len*5]));	
		diffCalc(&(compCount[6]), &(refSeq[len*6]), rdSeq, len, &(diffSeq[len*6]));	
//printf("6\n");
		for(c = 0; c < 7; c++)	//find the two sequences with the lowest number of disagreements with the reference sequence..
		{
			if(compCount[c] < min)
			{
				secMin = min;
				secMinIdx = minIdx;	//keep track of second lowest as well.
				min = compCount[c];
				minIdx = c;
			}
			else if (compCount[c] < secMin)
			{
				secMin = compCount[c];
				secMinIdx = c;
			}
		}
//printf("7\n");
		if(min < THRESHOLD && minIdx == 3)	//if the non-skewed reference sequence is a very good match...
		{
			returnData = malloc(sizeof(mData)); //allocate our return data.
			returnData -> mods = malloc(compCount[3] * sizeof(char));	//allocate space for the modification data tracking
			returnData -> locs = malloc(compCount[3] * sizeof(unsigned int));	//allocate space for modification location tracking
			returnData -> len = compCount[3];	//store the length of the mutation index for this read.
			returnData -> ins = malloc(compCount[3] * sizeof(int));
			
			mutationDetect(returnData, &(diffSeq[len*3]), rdSeq, len, matchLocLst[i]);	//the only thing detected here will be mutations or errors.
		
		//fill in our return data structure. pass in the return data pointer, the inconsistency list, the read sequence itself, and the length.
		
		
		
			return returnData;	//-------------------success!!!!!!
		}
//printf("8\n");
//printf("min: %d\n", min);
		if(min > INDEL_THRESHOLD)	//this just makes sure that we jump out if we're WAY off base in our estimation.
			continue;	
//printf("9\n");		
		
		//if we have a possible match... then we'll want to mesh the two segments together and see what sort of match we can get.
		//first, detect if it's an insertion or a deletion.
		//since each read sequence is going to use at least two uint32_t buckets...
		//	--	if we do a straight active-bit count of (the first uint32_t on the lower matching index) OR of (the last uint32_t on the higher matching index) and it is below threshold, then it must be an insertion.
		//  --  if we do a straight active-bit count of (the last uint32_t on the lower matching index) OR of (the first uint32_t on the higher matching index) and it is below threshold, then it must be an insertion.
		lowest = (minIdx < secMinIdx) ? minIdx : secMinIdx;	//find the min idx.
		highest = (minIdx >= secMinIdx) ? minIdx : secMinIdx;	//find the max idx.		

		smallCount(&delH, diffSeq[len*lowest]);	//i think these are right. but im tired. look at tomorrow...
		smallCount(&delL, diffSeq[len*(highest+1)-1]);
		smallCount(&insL, diffSeq[len*(lowest+1)-1]);
		smallCount(&insH, diffSeq[len*highest]);
		//two quantities of interest will be located at &(diffSeq[len*lowest]) and &(diffSeq[len*highest])
		
		indelLen = highest - lowest;	//obtain the length of the indel.
		

		
		
		if(insH <= THRESHOLD/2 || insL <= THRESHOLD/2)
		{
			lead = &(diffSeq[len*highest]);
			follow = &(diffSeq[len*lowest]);
			ins = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate an insertion.
		}
		else if (delH <= THRESHOLD/2 || delL <= THRESHOLD/2)
		{
			lead = &(diffSeq[len*lowest]);
			follow = &(diffSeq[len*highest]);
\			del = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate a deletion.
		}
		else	//if neither of these is true for some reason, then the previous subsequence match was probably a fluke, so jump out.
			continue;
			
		breakDetect(&indelLoc, lead, follow, len);	//detect where the break occurs.	
		//verified to work.
		mergeCount(&mCount, lead, follow, indelLoc, len);
		if(mCount > THRESHOLD + 3)	//we want to double check this. if the merged diff sequences are NOT below the threshold, then this is NOT a match! we should jump to the next location in the loop.

			continue;
		
		//the indelLoc must be added to the start position to obtain the absolute location of the indel, relative to the absolute beginning of the REFERENCE GENOME. 
		//
		returnData = malloc(sizeof(mData)); //allocate our return data.
		returnData -> mods = malloc(16 * sizeof(char));	//allocate space for the modification data tracking
		returnData -> locs = malloc(16 * sizeof(unsigned int));	//allocate space for modification location tracking
		returnData -> ins = malloc(16 * sizeof(int));	//keep track if this is an insertion or not
		
		
		//by the time we get here, we have the location of the indel, the length of the indel, and the proper diff sequences. we just need to calculate the actual differences. 
		//arguments we should pass to the error detector for a case WITH indels:actual read sequence,  lead and follow diff sequences, indel location, indel length, flag indicating insertion (0 == deletion).

//		mutationDetect(returnData, &(diffSeq[len*3]), rdSeq);	//the only thing detected here will be mutations or errors.
		if(ins == 1)
			insDetect(returnData, &(diffSeq[len*highest]), rdSeq, len, matchLocLst[i], indelLen, indelLoc, highest - 3);
		else if (del == 1)
			delDetect(returnData, &(diffSeq[len*lowest]), rdSeq, len, matchLocLst[i], indelLen, indelLoc, lowest - 3);

		
		return returnData;
		//in addition to passing in the return data pointer, the diff sequence, the read sequence, and the length, we also want to look at the insert flag (1 for insert, 0 for delete), the indel length, and the indel start location.
		
		
	}	//the end of calculation for one match possibility
	return NULL;	//if we're here, we had no luck. oh well.
}
//
//it should be noted that all positions returned will be relative to the beginning of the REFERENCE genome!!!
//

