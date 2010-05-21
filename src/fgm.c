//FILE: fgm.c (fine-grain matcher)
//AUTHOR: Joel C. Miller
//UCLA 2010, Spring Quarter
//Produced for CS 133/CS CM124 Joint Final Project - DNArm (fast DNA read mapper)
//
//MIT LICENCE...
//
#define THRESHOLD 6
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
			breakDetect(&(diffSeq[len*lowest]), len);//&(diffSeq[len*highest]), len);	//ORDER MATTERS HERE. for ins, the low index goes first.
			ins = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate an insertion.
		}
		else if (delH <= THRESHOLD || delL <= THRESHOLD)
		{
			breakDetect(&(diffSeq[len*highest]), len);//&(diffSeq[len*lowest]), len);	//ORDER MATTERS HERE. for del, the high index goes first.
			del = 1;		//		######### PLACEHOLDER. this should be changed to modify some element of the data structure that gets passed back, to indicate a deletion.
			
		}
		else	//if neither of these is true for some reason, then the previous subsequence match was probably a fluke, so jump out.
			continue;
		
		indelLen = highest - lowest;	//obtain the length of the indel.
		
		
		
	}	//the end of calculation for one match possibility
	return NULL;	//if we're here, we had no luck. oh well.
}


//this will return the location at which an insertion or deletion probably occured.
unsigned int breakDetect(uint32_t * leadSeq, int len)//uint32_t * followSeq, int len)
{
	int transition = 0, prev = 0;; //flag for transition detection. this gets set to one when we get 
	int persistence;	//incremented with each match discovered, decremented (downto 0 with each match discovered)
	int total;	//incremented with each match discovered
	int x, i;
	uint32_t lead;//, follow;	//temp
	
	for(x = 0; x < len; x++)	//for all of the bins...
	{
		lead = leadSeq[x];
//		follow = followSeq[x];
		
		for(i = 0; i < 16; i++)	//16 neucleotide difference sites we're going to investigate.
		{
			transition = (lead & 0x80000000) ^ (transition ^ prev);//(//((lead & (lead << 2)) >> 30) &
			lead <<= 2;
//			follow <<= 2;
		}
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



//ALGORITHM

//this function will be called with a single read sequence (length N), and passed a list of locations where there's a possible match. (will it also be called with a locked location (beginning of index zone, and length of lock) displacement from the beginning of the read sequence?) we can check if its at the very beginning, the end, or somewhere in the middle. 
//if its at the beginning, the "head" will be hard-locked in place, while the tail is flexible +/- 3. 
//if its at the end, the "tail" will be hard-locked in place, while the head is flexible +/- 3. 
//if its at the middle, both the head and the tail are flexible (+/- 3 on BOTH ends for simplicity)

//this part is probably INACCURATE. 2 versions should suffice.
//TENTATIVE: there will be 7 versions of this! yes, 7. one for each shift-operation of detecting ins/del segments. fgm0, fgmR1, fgmR2, fgmR3, fgmL1, fgmL2, fgmL3. these will be called from a helper/setup function fgm().

//this part is probably INACCURATE. 1 version should suffice!
//there will be 2 versions of this. one will do a straight fast-match (run through sequence, throw back a sub-threshold match). the second will account for ins/del, by matching from the front of the sequence, and also from the theoretical end of the sequence +/- 3 locations.

//there will be only one version of this... we'll start with our zero displacement match effort for all possible match locations, which will use our anchor point but not shift anything. basically, this sequence will be XORed with the relevant section of the reference genome, to detect discrepancies. then, we will count all disagreeing bits (using shift ops). basically, the lowest sub-threshold match will be passed back as the successful match. an exception, of course, is that an exact match will instantly break out and be passed back (the returned data structure will note that the ). however, if we don't have a sub-threshold match, we'll go to our fallback strategy, which is more in depth. after retrieving any additonal data (surround data retrieval - nucleotide bins forward or behind the possible match location), we'll start by XORing the base genome with [read sequence >> 3, >>2, >>1, (no-shift data has been saved, so dont redo), <<1, <<2, and <<3]. we'll first see if we can detect the two that have the least number of mismatches (minimized number of 1's set in the data), and once these are obtained (and therefore having learned the size of the discrepancy), we'll extract the exact mismatch location using the inferred ins/del size we just extracted - we'll check the first low-discrepancy location against with the second low-discrepancy location that's been shifted the requisite number of places. by detecting where a switchover has taken place (where a close match on one sequence shifts to a string of bad matches, and a string of bad matches on the second one shifts to a close match), we can find a detailed location and size of an insertion or deletion.

//=========
//matrices?
//=========

//------------
//(some data alignment will be required here when data is retrieved from the database)
	//i.e. if a nucleotide string is stored as ints... (__ denotes a barier between two int storage bins) 00|10|01|11|10|10|01|10|00|01|11|11|01|10|10|11__00|10|01|11|10|10|01|10|00|01|11|11|01|10|10|11
	//but we want to capture a sequence that runs across a gap, like: 00|10|01|11|(*10|10|01|10|00|01|11|11|01|10|10|11__00|10|01|11*)|10|10|01|10|00|01|11|11|01|10|10|11
	//we'll have to shift our stuff over, etc...

//for each match location
//------------	
	
	
	//MAYBE NOT THIS-->//step through read sequence. keep track of total number of mismatches, and also number of mismatches in the last 5 nucleotides.
	//xor our sequence with our 

//end match location loop