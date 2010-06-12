__kernel void cgm_kernel(__constant uint *aList, __constant uint *bList,
						int aLength, int bLength,
						__local uint *aCache, __local uint *bCache,
						int gap, int offset,
						int keyLength, __global uint *matches)
{

	const size_t workSize_width = get_local_size(0);
	const size_t workSize_height = get_local_size(1);
	
	unsigned int x = get_global_id(0);
	unsigned int y = get_global_id(1);
	
	if(get_local_id(0) == 0 && get_local_id(1) == 0)
	{
		for(int i = 0; i < workSize_width; i++)
		{
			for(int j = 0; j < workSize_height; j++)
			{
				aCache[x + i] = aList[x + i];
				bCache[y + j] = bList[y + j];
			}
		}
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	if(bCache[y] == aCache[x] + keyLength + gap)
		matches[x] = aCache[x] - offset;
	
}