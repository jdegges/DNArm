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
	char mods [16];	//what the nucleotide should be changed to...
	unsigned int locs [16];	//the location at which the data should be changed.
	int ins [16];	//indicates if this should be an insertion.
			//note: an insertion will list the same location multiple times, while a deletion will have a "D" in the corresponding nucleotide modification location.
	int len;
} mData;


//this is a small helper function that just implements the bit-counting algo we used in diffCalc(), but only on one uint. this will nominally be used with diff sequences.
unsigned int smallCount(uint diff)
{
	//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary (note: "0_" is the structure here.)
	diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
	return ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}
//-----------------------------------READ LENGTH IS 32 or 96 bits (let's say 96. thats a nice number.)!



//this will return the location at which an insertion or deletion probably occured. 
unsigned int breakDetect(uint * leadSeq, uint * followSeq, int len)
{
	int x, i;
	uint trueSeq;//, follow;	//temp
	int temp;
	int primeCount = 16;	//the lowest number of disagreements we've found so far... initialize to a high value.
	int primeLoc;	//the best location we've found so far.
	uint leadMask, followMask;
	uint lead, follow;	//the lead/follow sequences that we'll be using for comparison. these are of length 16 nucleotides, but in each iteration of the loop we want to shift over by only 8... so we have to screw with them a bit.
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
		else	//if this is an even iteration, we want to use a normal unaltered diff sequence.
		{
			lead = leadSeq[x/2];	//assign the unaltered diff sequences.
			follow = followSeq[x/2];
		}
		//commentary: compound diff sequence evaluations are done so we can reliably detect indels that occur on or near boundaries of uint's.
		
		//on each bin, count all of the set bits. if it's above THRESHOLD/2, then we want to look here.

		if(smallCount(lead) < (THRESHOLD/2) || smallCount(follow) < (THRESHOLD/2))	//if the lead or follow sequence have a very small number of differences, then we have not found the changeover point. we should keep looking.
			continue;

			leadMask = 0x00000000;		//start with the follow mask as the entire sequence...
			followMask = 0x55555555;	
			for(i = 0; i < 16; i++)	//mask shifting...
			{
				trueSeq = leadMask & lead | followMask & follow;	//integrate the two sequences into a single "true diff sequence"...
				if((temp = smallCount(trueSeq)) <= primeCount)	//if we've found a better match...
				{
					primeLoc = i + 8 * x;
					primeCount = temp;	//save the min count and the location.
				}
				
				leadMask = (leadMask >> 2) | 0x40000000;	//shift the mask.
				followMask >>= 2;	//shift the mask.
			}
			//by this point, we will have found the masking location that gives the minimum disagreement.

//		}
	}	
	return primeLoc;
}

//this is much like diffCalc, except that it can deal with lead/follow sequences, merging and counting the differences as appropriate.
unsigned int mergeCount(uint * leadSeq, uint * followSeq, uint cPoint, int len)	//changepoint, and read seq length in uint's.
{
	int i, j;
	uint * temp = leadSeq;
	uint trueDiff;	//the actual diff sequence we'll analyze, either grabbed from lead, follow, or compounded.
	unsigned int dCount = 0;
	uint changeover = (cPoint%16)*2;
	
	
	for(i = 0; i < len; i++)	//for each bin...
	{
		if(temp[i] == 0)
			continue;	//skip this section if there's nothing to look at...
		
		if(cPoint%16 && cPoint/16 == i )	//if the changePoint occurs in THIS uint and doesn't occur on the border... then we'll want to create a compound diff sequence.
		{
			trueDiff = (leadSeq[i] & (0x55555555 << (32 - changeover))) | (followSeq[i] & (0x55555555 >> changeover));	//construct the compounded diff sequence...
			temp = followSeq;	//switch the pointer for the diff sequence to the follow-sequence address.
		}
		else if(followSeq[i] == 0)	//if the changePoint occured on a bin boundary...
		{
			temp = followSeq;	//then we should swap the pointer to the follow-sequence, and continue...
			continue;
		}
		else
		{	//this will start as the lead sequence, then we will shift to the follow sequence when appropriate.
			trueDiff = temp[i];
		}
		dCount += smallCount(trueDiff);	//find the number of differences in the constructed diff sequence.
	}
	return dCount;	//return the total count.
}


//this will calculate the number of single differences from a single ref-sequence/read-sequence pair, and create a diff sequence for the pair, which is passed back through a pointer reference. the return value is the total number of differences that existed between the sequences.
unsigned int diffCalc(uint * refSeq, uint * rdSeq, int readLen, uint * diffSeq)
{
		int x = 0;
		uint diff;	//the temp and final diff variable.
		unsigned int dCount = 0;
		for(x; x < readLen; x++)	//for all of the bins...
		{
			//we need this so we don't artificially inflate the number of differences we have.
			diff = refSeq[x] ^ rdSeq[x];	//XORing the ref and read seqences will detect the differences... but we still have two possible difference sites per nucleotide. we need this reduced to 1 difference site.
			diff = (diff | (diff >> 1)) & 0x55555555;	//this will reduce the possible difference sites for each nucleotide to only one, and zero fill the rest.
			//the sequence created here will be 0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_0_, where '_' denotes a difference site.
			diffSeq[x] = diff;	//save this to our output argument.
			
			//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
			diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary
			diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
			dCount += ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
		}
		return dCount;	//this is the total number of differences in the sequence.
}

// * Base | lower case | upper case
// * ------------------------------
// * A    | 0110 0001  | 0100 0001
// * C    | 0110 0011  | 0100 0011
// * G    | 0110 0111  | 0100 0111
// * T    | 0111 0100  | 0101 0100
// *              ^^           ^^

//this will detect mutations in sequences that have neither insertions or deletions.
void mutationDetect(mData * returnData, uint *  diffSeq, uint * rdSeq, int rdLen, uint startLoc)	//the only thing detected here will be mutations or errors
{
	int i, j;
	int m = 0;	//counts the number of modifications
	uint dTemp, rTemp;
	
	for(i = 0; i < rdLen; i++)//loop through each of the bins...
	{
		if(diffSeq[i] == 0)	//if there's absolutely no differences in this segment...
			continue;
		//otherwise do some calculation.
		dTemp = diffSeq[i];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];	//copy the read sequence to a temp variable for manipulation.
		for(j = 0; j < 16; j++)	//run through a single uint
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

void insDetect(mData * returnData, uint * diffSeq, uint * rdSeq, int rdLen, uint startLoc, int insLen, int insLoc, int ldOffset)
{	//ldOffset corrects the location report for lead sequences that are offset from the unskewed reference sequence.
	int i, j, k;
	int m = 0;	//counts the number of modifications
	uint dTemp, rTemp;
	int comp = 0; //compensation factor... this accounts for shifting the relative index when we are at the insertion point. since diffSeq is actually pointing at the lead sequence, which will have a higher index than the follow sequence, a negative index address will actually give the correct shifted diff sequence.
//	uint * diffSeq;	//this is used to fluently switch between the lead and follow sequences.
//	diffSeq = dSeqLead;
//	returnData -> len = insLen +;
	k = 0;	//init to zero here...
	
	for(i = 0; i < rdLen; i++)
	{
		dTemp = diffSeq[i+comp];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];
		for(j = 0; j < 16; j++)	//run through a single uint
		{
			if ((i * 16 + j) >= insLoc && (j+k+(16*i)) < (insLen + insLoc))	//if we've found the insertion location...
			{
			
				for(k; k < insLen && (j + k) < 16; k++)	//in addition to checking against the insertion length, we also need to check if this iteration will run us off the end of a storage bin.
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
					rTemp <<= 2;	//shift over so we can investigate the next nucleotide.
					comp -= 3;	//decrement the compensation factor...
				}
				dTemp = diffSeq[i+comp] << (2*(j+k));	//shift over to return to the same relative location we were at before the traverse.
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

void delDetect(mData * returnData, uint * diffSeq, uint * rdSeq, int rdLen, uint startLoc, int delLen, int delLoc, int ldOffset)
{
	int i, j, k;
	int m = 0;	//counts the number of modifications
	uint dTemp, rTemp;
	int comp = 0;	//compensation factor... same concept as in insDetect.
	k = 0;
	
	for(i = 0; i < rdLen; i++)
	{
		dTemp = diffSeq[i+comp];	//copy the diff sequence to a temp variable for manipulation...
		rTemp = rdSeq[i];
		for(j = 0; j < 16; j++)	//run through a single uint
		{
			if((i * 16 + j) >= delLoc && (j+k+(16*i)) < (delLen + delLoc))	//if this location is the deletion point...
			{
				for(k = 0; k < delLen && (j + k) < 16; k++)	//as long as the delete sequence isnt complete... (and we're not off the edge)
				{
					returnData -> mods[m] = 'D';	//insert delete symbol...
					returnData -> locs[m] = startLoc + i*16 + j + k + ldOffset;	//and keep track of absolute location.
					returnData -> ins[m] = -1;
					m++;
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
__kernel void fgm(__global mData * returnData, __global uint * list, uint listLen, 
			__global uint * g_matchLocLst, __global uint * g_mllLen, 
			__global uint * g_rdSeq, uint mlMax, uint batchSize)
{	
	int start;	//matchloclst is in nucleotides, correct into bits.
	int len = rdLen>>4; //readlen is in nucleotides, correct into uint sizing.
	int lBound; //correct from nucleotides into sizing of uint
	int offset; //correct offset from nucleotides into bits.
	int min, secMin;
	int minIdx, secMinIdx;
	int c;
	int lowest, highest;
	int i;
	uint lastInd = listLen >> 4;	//this will tell us what the index of the last bin is. this is important for error checking.
	uint rs [21];
	uint ds [21];

//	uint * refSeq = malloc (7*sizeof(uint)*len);	//this is where we'll construct and align the reference sequence. keep everything because we need to compare later.
//	uint * diffSeq = malloc (7*sizeof(uint)*len);	//doesnt really need to be initialized.
	uint * lead, * follow;	//used for detecting indels.
//	uint * matchSeq = &(diffSeq[len+3]);	//default our match sequence to be the zero-skew element.
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
	uint * refSeq = rs;//&(rs[0]);
	uint * diffSeq = ds;//&(ds[0]);
	
	int tn;
//	tx = get_global_id(0);
//	ty = get_global_id(1);
	tn = get_global_id(0)*32 + get_global_id(1);	//x and y dimensions for threads...
	
	if (tn >= batchSize)
		return;			//this is a jump-out point... basically only needed at the end of processing, when there isnt enough data to do a full batch process.
		
		
//	mData * returnData = g_returnData + tn;//&(g_returnData[tn]);
	uint * rdSeq = g_rdSeq + (tn*3);//&(g_rdSeq[tn * 3]);	//the readsequence that we'll be looking at.
	uint * matchLocLst = g_matchLocLst + (tn*mlMax);//&(g_matchLocLst[tn * mlMax]);
	uint * mllLen = g_mllLen + tn;//&(g_mllLen[tn]);

	
	
	//for each location in matchlist
	for(i = 0; i < mllLen; i++)
	{
		ins = 0; //default to no insertion and no deletion.
		del = 0;
		start = matchLocLst[i] * 2;	//matchloclst is in nucleotides, correct into bits.
//		len = readLen>>4; //readlen is in nucleotides, correct into uint sizing.
		
		lBound = matchLocLst[i] >>4; //correct from nucleotides into sizing of uint
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
				for (c = 0; c < len; c++)	//zero fill...
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

					}
				}
			//we just aligned the reference sequence properly. now we can do a direct bit comparison and count bits...
		
			
			refRow += len;	//note: refRow 3 == skew of 0... also corresponds to refSeq[9], refSeq[10] and refSeq[11] (using 96-bit readsequence length) and to compCount[3]
		}

		//run calculations for +/- 3 skewed reference sequences.
		compCount[0] = diffCalc(&(refSeq[0]), rdSeq, len, &(diffSeq[0]));	
		compCount[1] = diffCalc(&(refSeq[len*1]), rdSeq, len, &(diffSeq[len*1]));	
		compCount[2] = diffCalc(&(refSeq[len*2]), rdSeq, len, &(diffSeq[len*2]));	
		compCount[3] = diffCalc(&(refSeq[len*3]), rdSeq, len, &(diffSeq[len*3]));	
		compCount[4] = diffCalc(&(refSeq[len*4]), rdSeq, len, &(diffSeq[len*4]));	
		compCount[5] = diffCalc(&(refSeq[len*5]), rdSeq, len, &(diffSeq[len*5]));	
		compCount[6] = diffCalc(&(refSeq[len*6]), rdSeq, len, &(diffSeq[len*6]));	

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

		if(min < THRESHOLD && minIdx == 3)	//if the non-skewed reference sequence is a very good match...
		{
//			returnData[tn] = malloc(sizeof(mData)); //allocate our return data.
//			returnData[tn] -> mods = malloc(compCount[3] * sizeof(char));	//allocate space for the modification data tracking
//			returnData[tn] -> locs = malloc(compCount[3] * sizeof(unsigned int));	//allocate space for modification location tracking
			returnData[tn] -> len = compCount[3];	//store the length of the mutation index for this read.
//			returnData[tn] -> ins = malloc(compCount[3] * sizeof(int));
			
			mutationDetect(&(returnData[tn]), &(diffSeq[len*3]), rdSeq, len, matchLocLst[i]);	//the only thing detected here will be mutations or errors.
		
		//fill in our return data structure. pass in the return data pointer, the inconsistency list, the read sequence itself, and the length.
		
		
			return;
//			return returnData;	//-------------------success!!!!!!
		}

		if(min > INDEL_THRESHOLD)	//this just makes sure that we jump out if we're WAY off base in our estimation.
			continue;	
		
		
		//if we have a possible match... then we'll want to mesh the two segments together and see what sort of match we can get.
		//first, detect if it's an insertion or a deletion.
		//since each read sequence is going to use at least two uint buckets...
		//	--	if we do a straight active-bit count of (the first uint on the lower matching index) OR of (the last uint on the higher matching index) and it is below threshold, then it must be an insertion.
		//  --  if we do a straight active-bit count of (the last uint on the lower matching index) OR of (the first uint on the higher matching index) and it is below threshold, then it must be an insertion.
		lowest = (minIdx < secMinIdx) ? minIdx : secMinIdx;	//find the min idx.
		highest = (minIdx >= secMinIdx) ? minIdx : secMinIdx;	//find the max idx.		

		delH = smallCount(diffSeq[len*lowest]);	//as described above...
		delL = smallCount(diffSeq[len*(highest+1)-1]);
		insL = smallCount(diffSeq[len*(lowest+1)-1]);
		insH = smallCount(diffSeq[len*highest]);
		//two quantities of interest will be located at &(diffSeq[len*lowest]) and &(diffSeq[len*highest])
		
		indelLen = highest - lowest;	//obtain the length of the indel.
		
		if(insH <= THRESHOLD/2 || insL <= THRESHOLD/2)	//if the start of the high or the end of the low sequence are less than the threshold
		{
			lead = &(diffSeq[len*highest]);	//then the lead is high, and the follow is low.
			follow = &(diffSeq[len*lowest]);
			ins = 1;		
		}
		else if (delH <= THRESHOLD/2 || delL <= THRESHOLD/2)	//otherwise, if the start of the low or the end of the high sequence are less than the threshold
		{
			lead = &(diffSeq[len*lowest]);	//then lead is low and follow is high.
			follow = &(diffSeq[len*highest]);
			del = 1;		
		}
		else	//if neither of these is true for some reason, then the previous subsequence match was probably a fluke, so jump out.
			continue;
			
		indelLoc = breakDetect(lead, follow, len);	//detect where the break occurs.	
		//verified to work.
		
		if((mCount = mergeCount(lead, follow, indelLoc, len)) > THRESHOLD + 3)	//we want to double check this. if the merged diff sequences are NOT below the threshold, then this is NOT a match! we should jump to the next location in the loop.
			continue;
		
		//the indelLoc must be added to the start position to obtain the absolute location of the indel, relative to the absolute beginning of the REFERENCE GENOME. 
		//
//		returnData[tn] = malloc(sizeof(mData)); //allocate our return data.
		
		
		//by the time we get here, we have the location of the indel, the length of the indel, and the proper diff sequences. we just need to calculate the actual differences. 

		if(ins == 1)	//this part is straightforward...
			insDetect(&(returnData[tn]), &(diffSeq[len*highest]), rdSeq, len, matchLocLst[i], indelLen, indelLoc, highest - 3);
		else if (del == 1)
			delDetect(&(returnData[tn]), &(diffSeq[len*lowest]), rdSeq, len, matchLocLst[i], indelLen, indelLoc, lowest - 3);

		return;
//		return returnData;		
		
	}	//the end of calculation for one match possibility
	returnData[tn].len = -1;
	return;
	//return NULL;	//if we're here, we had no luck. oh well.
}
//
//it should be noted that all positions returned will be relative to the beginning of the REFERENCE genome!!!
//

