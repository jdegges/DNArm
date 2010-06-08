//FILE: fgm.c (fine-grain matcher)
//AUTHOR: Joel C. Miller
//UCLA 2010, Spring Quarter
//Produced for CS 133/CS CM124 Joint Final Project - DNArm (fast DNA read mapper)
//
//MIT LICENCE...
//
#define THRESHOLD 5
#define INDEL_THRESHOLD 29

//so, find locations...

//regardless of match accuracy, we'll be given a read sequence (bit length 32? 48? 64?) that has 3 or 4 16-length index tag sections.
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





//=============================================================================
//--list is the complete base genome
//--readlen is the length of the read in nucleotides. THIS SHOULD BE IN MULTIPLES OF 16 FOR EXPECTED BEHAVIOR.
//--matchloclst is a list of match locations; the units are in nucleotides. this number indicates the START of the sequence, regardless of where the index was actually mapped to.
//--mllLen is the number of possible match locations.
//--rdSeq is the read sequence. 
* passbackdata fgm(uint32_t * list, int readLen, uint32_t * matchLocLst, int mllLen, uint32_t ** rdSeq)
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
	uint32_t * refSeq = malloc (7*sizeof(uint32_t)*len);	//this is where we'll construct and align the reference sequence. keep everything because we need to compare later.
	uint32_t * diffSeq = malloc (7*sizeof(uint32_t)*len);	//doesnt really need to be initialized.
	uint32_t * matchSeq = &(diffSeq[len+3]);	//default our match sequence to be the zero-skew element.
	int compCount[7];	//the number of elements which disagree in each test. compCount[3] is zero-skew.
	int refRow;
	int skew;
	int correct;
	int curOffset;	//get absolute current offset.
	int insH, insL, delH, delL;
	int indelLen;	//length of the indel (we can track up to +/- 3. HOWEVER WE CAN EASILY EXPAND THIS ALGORITHM TO HANDLE A LOT LARGER INDELS)
	int indelLoc;	//the location where the indel starts (indicates either the location of the first inserted nucleotide, or the location at which nucleotides began to be deleted.
	int ins, del; //flag for detecting insertion or deletion
	
	//for each location in matchlist
	for(i = 0; i < mllLen; i++)
	{
		ins = 0;
		del = 0;
		start = matchLocLst[i] * 2;	//matchloclst is in nucleotides, correct into bits.
//		len = readLen>>4; //readlen is in nucleotides, correct into uint32_t sizing.
		
		lBound = matchLocList[i] >>4; //correct from nucleotides into sizing of uint32_t
		offset = matchLocList[i]%16*2; //correct offset from nucleotides into bits.
		min = 64;
		secMin = 64;
		minIdx = 0;
		secMinIdx = 0;
		//c;
		lowest = 7;	//init to out-of-range
		
//		refSeq = malloc (7*sizeof(uint32_t)*len);	//this is where we'll construct and align the reference sequence. keep everything because we need to compare later.
//		diffSeq = malloc (7*sizeof(uint32_t)*len);	//doesnt really need to be initialized.
//		matchSeq = &(diffSeq[len+3]);	//default our match sequence to be the zero-skew element.
		
		for (c = 0; c < len*7; c++)
		{
			refSeq[c] = 0;	//initialize.
		}
				
		for(c = 0; c < len; c++)	//copy and align the data from the reference sequence.
		{
			refSeq[len*3+c] |= (list[lBound + correct]) << curOffset;	//first segment.
			refSeq[len*3+c] |= (list[lBound + correct+1]) >> (32-curOffset); //remaining fill-in from segment c+1.
		}
		
		compCount[3] = diffCalc(&(refSeq[len*3]), rdSeq, len, &(diffSeq[len*3]));	//this is the default simple case... no offset is used.
		
		if(compCount[3] <= THRESHOLD)	//if we got a good match on our first try
		{
		//BUILD DATA STRUCTURE...
		//...
		//...
			return 	//	##########
					//	##########
					//	##########	FIGURE OUT WTF WE'RE GOING TO RETURN.
					//	##########
					//	##########
		}
		
		//if we get here, we didn't find a quick easy match.
		
		refRow = 0;
		for (skew = -6; skew <= 6; skew += 2)	//for skewing L/R by 3 to detect indels. //##########make this a separate function?
		{
			if (skew == 0)	//since we already did this, no point in redoing it.
			{
				skew += 2;	//so skip forward...
				refRow += len;
			}
			
			correct = 0;
			curOffset = (offset+skew)%32;	//get absolute current offset.
			if(offset + skew < 0) correct = -1;	//this corrects for overflow/underflow when we're skewing.
			if(offset + skew > 32) correct = 1; 
			
			for (c = 0; c < len; c++)	//copy and align the data from the reference sequence.
			{
				refSeq[refRow+c] |= (list[lBound + correct]) << curOffset;	//first segment.
				refSeq[refRow+c] |= (list[lBound + correct+1]) >> (32-curOffset); //remaining portion of fill in...
			}
			//we just aligned the reference sequence properly. now we can do a direct bit comparison and count bits...
		
			
			refRow += len;	//refRow3 == skew of 0... also corresponds to refSeq[6] and refSeq[7] (using 64-bit readsequence length) and to compCount[3]
		}
		
		//run calculations for +/- 3 skewed reference sequences.
		compCount[0] = diffCalc(&(refSeq[0]), rdSeq, len, &(diffSeq[0]));	
		compCount[1] = diffCalc(&(refSeq[len*1]), rdSeq, len, &(diffSeq[len*1]));	
		compCount[2] = diffCalc(&(refSeq[len*2]), rdSeq, len, &(diffSeq[len*2]));	
		compCount[4] = diffCalc(&(refSeq[len*4]), rdSeq, len, &(diffSeq[len*4]));	
		compCount[5] = diffCalc(&(refSeq[len*5]), rdSeq, len, &(diffSeq[len*5]));	
		compCount[6] = diffCalc(&(refSeq[len*6]), rdSeq, len, &(diffSeq[len*6]));	
		
		for(c = 0; c < 7; c++)	//find the two sequences with the lowest number of disagreements with the reference sequence..
			if(compCount[c] < min)
			{
				secMin = min;
				secMinIdx = minIdx;
				min = compCount[c];
				minIdx = c;
			}
		
		if(min > INDEL_THRESHOLD)	//this just makes sure that we jump out if we're WAY off base in our estimation.
			continue;	
			
		//if we have a possible match... then we'll want to mesh the two segments together and see what sort of match we can get.
		//first, detect if it's an insertion or a deletion.
		//since each read sequence is going to use at least two uint32_t buckets...
		//	--	if we do a straight active-bit count of (the first uint32_t on the lower matching index) OR of (the last uint32_t on the higher matching index) and it is below threshold, then it must be an insertion.
		//  --  if we do a straight active-bit count of (the last uint32_t on the lower matching index) OR of (the first uint32_t on the higher matching index) and it is below threshold, then it must be an insertion.
		lowest = (minIdx < secMinIdx) ? minIdx : secMinIdx;	//find the min idx.
		highest = (minIdx >= secMinIdx) ? minIdx : secMinIdx;	//find the max idx.		
		
		insH = smallCount(diffSeq[len*lowest]);	//i think these are right. but im tired. look at tomorrow...
		insL = smallCount(diffSeq[len*(highest+1)-1];
		delL = smallCount(diffSeq[len*(lowest+1)-1];
		delH = smallCount(diffSeq[len*highest];
		
		//two quantities of interest will be located at &(diffSeq[len*lowest]) and &(diffSeq[len*highest])
		
		if(insH <= THRESHOLD || insL <= THRESHOLD)
		{
			indelLoc = breakDetect(&(diffSeq[len*highest]), &(diffSeq[len*highest]), len);	//ORDER MATTERS HERE. for ins, the high index goes first.
			ins = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate an insertion.
		}
		else if (delH <= THRESHOLD || delL <= THRESHOLD)
		{
			indelLoc = breakDetect(&(diffSeq[len*lowest]), &(diffSeq[len*lowest]), len);	//ORDER MATTERS HERE. for del, the low index goes first.
			del = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate a deletion.
		}
		else	//if neither of these is true for some reason, then the previous subsequence match was probably a fluke, so jump out.
			continue;
		
		//the indelLoc must be added to the start position to obtain the absolute location of the indel, relative to the absolute beginning of the REFERENCE GENOME. 
		//
		indelLen = highest - lowest;	//obtain the length of the indel.
		
		//by the time we get here, we have the location of the indel, the length of the indel, and the proper diff sequences. we just need to calculate the actual differences. 
		//arguments we should pass to the error detector for a case WITH indels:actual read sequence,  lead and follow diff sequences, indel location, indel length, flag indicating insertion (0 == deletion).
		
		
	}	//the end of calculation for one match possibility
	return NULL;	//if we're here, we had no luck. oh well.
}
//
//it should be noted that all positions returned will be relative to the beginning of the REFERENCE genome!!!
//

//this will return the location at which an insertion or deletion probably occured. we search from the end of a sequence, because the bit manipulation is easier.
unsigned int breakDetect(uint32_t * leadSeq, uint32_t * followSeq, int len)
{
	int x, i;
	uint32_t trueSeq;//, follow;	//temp
	int temp;
	int primeCount = 16;	//the lowest number of disagreements we've found so far... initialize to a high value.
	int primeLoc;	//the best location we've found so far.
	uint32_t leadMask, followMask;
	uint32_t lead, follow;	//the lead/follow sequences that we'll be using for comparison. these are of length 16 nucleotides, but in each iteration of the loop we want to shift over by only 8... so we have to screw with them a bit.
	int aLen = 2 * len - 1;
	
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
		if(smallCount(&lead)) > (THRESHOLD/2))
		{

//			lead << (32 - totalSet*2 - 2*THRESHOLD/len);	//shift over until we're in the neighborhood of something interesting.
//maybe put this back in?
			leadMask = 0x00000000;		//start with the follow mask as the entire sequence...
			followMask = 0x55555555;
			for(i = 0; i < 16; i++)	//mask shifting...
			{
				trueSeq = leadMask & lead | followMask & follow;	//integrate the two sequences...
				if((temp = smallCount(&trueSeq)) < primeCount)	//if we've found a better match...
				{
					primeLoc = i;
					primeCount = temp;	//save the min count and the location.
				}
				
				leadMask = (leadMask >> 2) & 0x40000000;	//shift the mask.
				followMask >>= 2;	//shift the mask.
			}
			//by this point, we will have found the masking location that gives the minimum disagreement.
			return primeLoc + 8 * x;	//the proper location (in nucleotide places) will be here. 8*x is the offset (how much we shifted over in each loop)
	}
}

//this is a small helper function that just implements the bit-counting algo we used in diffCalc(), but only on one pair of uint32_t. this will nominally be used with diff sequences.
unsigned int smallCount(uint32_t diff)
{
	//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary (note: "0_" is the structure here.)
	diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
	return ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}
//-----------------------------------READ LENGTH IS 32 or 64 bits (let's say 64. thats a nice number.)!



//args are the aligned reference sequence, the read sequence, and the number of uint32_t's we need to compare.
//* passbackdata 



unsigned int diffCalc(uint32_t * refSeq, uint32_t * rdSeq, int readLen, uint32_t * diffSeq)
{	//in here, surround data is present, and aligned to have displacement 0 at beginning of , read sequence is bit aligned as well.
		int x = 0;
		uint32_t [readLen] diff;	//the temp and final diff variable.
		unsigned int dCount = 0;
		for(x; x < readLen; x++)	//for all of the bins...
		{
			//we need this so we don't artificially inflate the number of differences we have.
			diff = refSeq[x] ^ rdSeq[x];	//XORing the ref and read seqences will detect the differences... but we still have two possible difference sites per nucleotide. we need this reduced to 1 difference site.
			diff = (temp | (temp >> 1)) & 0x55555555;	//this will reduce the possible difference sites for each nucleotide to only one, and zero fill the rest.
			
			diffSeq[x] = diff;	//save this to our output argument.
			
			//now for bit counting. this is the most efficient horizontal addition algorithm i could find, and was borrowed from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
			diff = diff - ((diff >> 1) & 0x55555555);                    // reuse input as temporary
			diff = (diff & 0x33333333) + ((diff >> 2) & 0x33333333);     // temp
			dCount += ((diff + (diff >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
		}
		return dCount;	//this is the total number of differences in the sequence.
}


//* PASSBACKDATA matchdetail(int readlen, int32_t rdSeq[4], int32_t matchloc, uint32_t *list, 
