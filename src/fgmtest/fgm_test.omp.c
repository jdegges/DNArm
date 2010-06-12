//FGM-TEST

#include <stdio.h>
#include "fgm.h"
//mData * fgm(uint * list, int readLen, uint * matchLocLst, int mllLen, uint * rdSeq)

//	aa	0
//	ac	1
//	at	2
//	ag	3
//	ca	4
//	cc	5
//	ct	6
//	cg	7
//	ta	8
//	tc	9
//	tt	A
//	tg	B
//	ga	C
//	gc	D
//	gt	E
//	gg	F

int main(int argc, char ** argv)
{
	int x, i;
//	mData results [34];//** results;// [34];	//for saving the results of our tests...
	
	
//	clData * gpu;

	struct cgmResult * data;// [34];	
	data = malloc(sizeof(struct cgmResult)*34);

	uint testRefSeq [10]= {0x2D89E192, 0xD9A01F72, 0x2B9B11EE, 0x519D4F53, 0x9E987936, 0x48C5B986, 0xB5848D60, 0xEB4C6938, 0x93B1B4E1, 0x021FBBD4};
	//the test ref sequence is only 160 base pairs long. but thats ok, since this algorithm only deals with fine grained local matching.
	uint listLen = 160;	//test ref seq of length 160.
	
	uint readLen = 48;	//read length 48 base pairs.
	uint testReads [34][3]= 
	{
	{0xD9A01F72,	0x8680B9B2,	0xB6775353},//1
	{0x53D4E7A6,	0x8680B9B2,	0xB6775353},
	{0xD9A01F72,	0x2B9B11EE,	0x519D4F53},
	{0x53D4E7A6,	0x1E4D9231,	0x6E61AD61},//4
	{0x2B9BD1EE,	0x519D4F53,	0x9E987936},
	{0x2B9BD1EE,	0x529D4F53,	0x9E987936},
	{0x2B9BD1EE,	0x529D4F53,	0x96987936},
	{0xC4BB9467,	0x53D4E7A6,	0x1E4D9231},//8
	{0xC4BB9467,	0x51D4E7A6,	0x1E4D9231},
	{0xC4BB9467,	0x51D4E7A6,	0x9E4D9231},
	{0x22B9B11E,	0xE5100753,	0xD4E7A61E},
	{0x8AE6C47B,	0x94677539,	0xE9879364},//12
	{0x48C5B986,	0xB5812358,	0x3AD31A4E},
	{0x48C5B986,	0xB58048D6,	0x0EB4C693},
	{0x48C5B986,	0xB5801235,	0x83AD31A4},
	{0x9E987936,	0x48C6E61A,	0xD6123583},//16
	{0x9E987936,	0x48C7986B,	0x5848D60E},
	{0x9E987936,	0x48C661AD,	0x6123583A},
	{0xE61AD612,	0x3580EB4C,	0x693893B1},	 //E61AD612	3580EB4C	693893B1
	{0xE61AD612,	0x35803AD3,	0x1A4E24EC},//20 //E61AD612	35803AD3	1A4E24EC
	{0xE61AD612,	0x35820EB4,	0xC693893B},	 //E61AD612	35820EB4	C693893B
	{0x48D60EB4,	0xC69224EC,	0x6D384087},
	{0x48D60EB4,	0xC69093B1,	0xB4E1021F},
	{0x48D60EB4,	0xC6924EC6,	0xD384087E},//24
	{0x2D89E192,	0xD9A07DC8,	0xAE6C47B9},
	{0xEB4C6938,	0x93B06D38,	0x4087EEF5},
	{0xA9F722B9,	0xB11EE519,	0xD4F539E9},
	{0x81F722B9,	0xB11EE519,	0xD4F539E9},//28
	{0x01F722B9,	0xB11EE519,	0xD4F539C0},
	{0x01F722B9,	0xB11EE519,	0xD4F539E8},
	{0x807D22B9,	0xB11EE519,	0xD4F539E9},
	{0x01F722B9,	0xB11EE519,	0xD4F5E7A6},//32
	{0x01F722B8,	0x1B11EE51,	0x9D4F539E},
	{0x01F722B9,	0xC47B9467,	0x53D4E7A6}
	};//there are 34 test cases. //each read sequence is 3 uint's long - 48 base pairs; 96 bits.
	
	uint locs [34] = {
	16, 55, 16, 55, 32,
	32, 32, 39, 39, 39, 
	30, 34, 80, 80, 80, 
	64, 64, 64, 89, 89, 
	89, 102, 102, 102, 0, 
	112, 22, 22, 22, 22, 
	22, 22, 22, 22
	};
	
//struct cgmResult{
//      int read[3]; // 16 bases in each int
// 
//      uint32_t* matches; // pointer to list of match locations
//      int length; // length of match list
//};
	//printf
	for(x = 0; x < 34; x++)
	{
		data[x].read[0] = testReads[x][0];
		data[x].read[1] = testReads[x][1];
		data[x].read[2] = testReads[x][2];
		data[x].matches = malloc(sizeof(int));
		data[x].matches[0] = locs[x];
		data[x].length = 1;

		
	}
	
	fgmLaunch(testRefSeq, 160, data, 34);
//	int x;	//for indices.
//	results = malloc(34*sizeof(mData*));

	

//	gpu = fgmInit(testRefSeq, listLen, readLen, 34);

//	fgmStart(gpu, data, 34);
//	fgmKill(gpu);


//	printf("working...\n");
	
	//mData * fgm(uint * list, uint listLen, int readLen, uint * matchLocLst, int mllLen, uint * rdSeq);
	
//	for(x = 0; x < 34; x++)	
//	{//12
//		printf("test index %d:\n", x);
//		results[x] = fgm(testRefSeq, listLen, readLen, &(locs[x]), 1, testReads[x]);
//		printf("====TEST NUMBER %d====\n", (x+1));
//		if(results[x] == NULL)
//			printf("NO MATCH FOUND!!!\n");
//		else
//		{
//			if(results[x]->len == 0)	//this is an exact match.
//				printf("EXACT MATCH! no mutations were discovered in this read sequence.\n");
//			for(i = 0; i < results[x]->len; i++)
///			{
//				printf("location: %u \tmutation: %c \tins: %i\n", results[x]->locs[i], results[x]->mods[i], results[x]->ins[i]);
//			}
//		}
//	}
	
	
	return 0;
}


