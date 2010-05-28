__kernel void cgm_kernel(__global uint *aList, __global uint *bList,
						int aLength, int bLength,
						int gap, int offset,
						int keyLength, __global uint *matches)
{

	const size_t x = get_global_id (0);
	const size_t y = get_global_id (1);

	if(bList[y] == aList[x] + keyLength + gap)
		matches[x] = aList[x] - offset;
	
}