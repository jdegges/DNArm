__kernel void cgm_kernel(__global uint *aList, __global uint *bList,
						int aLength, int bLength,
						int gap, int offset,
						int keyLength, __global uint *matches)
{
	const size_t width = get_global_size(0);
	const size_t length = get_global_size(1);
	const size_t x = get_global_id (0);
	const size_t y = get_global_id (1);
	const size_t index = x + y * width;
	
	int i = index;
	
	while(i < aLength)
	{
		
		for(int j = 0; j < bLength; j++)
		{
			if(bList[j] == aList[i] + keyLength + gap){
				matches[i] = aList[i] - offset;
				break;
			}
			else if(bList[j] > aList[i] + keyLength + gap){
				matches[i] = 0;
				break;
			}
		}
	
		i += width*length;
	}
}