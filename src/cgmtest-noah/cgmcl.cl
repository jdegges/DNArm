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
		matches[i] = 0;
		
		int lower = 0;
		int upper = bLength-1;
				
		while(lower <= upper){
			int j = (upper + lower)/2;
			if(bList[j] == aList[i] + keyLength + gap){
				matches[i] = aList[i] - offset;
				break;
			}
			else if(bList[j] > aList[i] + keyLength + gap)
				upper = j - 1;
			else
				lower = j + 1;
		
		}
		
		i += width*length;
	}
}