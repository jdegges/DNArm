__kernel void cgm_kernel(__global image2d_t *aImg, __global image2d_t *bImg,
						int gap, int offset,
						int keyLength, __global uint *matches)
{
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE
								| CLK_ADDRESS_CLAMP_TO_EDGE
								| CLK_FILTER_NEAREST;
	const size_t x = get_global_id (0);
	const size_t y = get_global_id (1);
	
	int a_x = x;
	int a_y = 1;
	int b_x = y;
	int b_y = 1;
	
	const int2 aCoord = (int2)(a_x, a_y);
	const int2 bCoord = (int2)(b_x, b_y);
	
	int4 a4 = read_imagei(aImg, sampler, aCoord);
	int4 b4 = read_imagei(bImg, sampler, bCoord);
	
	matches[x] = INT_MAX;

	if(b4.x == a4.x + keyLength + gap)
		matches[x] = a4.x - offset;
	if(b4.y == a4.y + keyLength + gap)
		matches[x] = a4.y - offset;
	if(b4.z == a4.z + keyLength + gap)
		matches[x] = a4.z - offset;
	if(b4.w == a4.w + keyLength + gap)
		matches[x] = a4.w - offset;
	if(b4.x == a4.y + keyLength + gap)
		matches[x] = a4.y - offset;
	if(b4.x == a4.z + keyLength + gap)
		matches[x] = a4.z - offset;
	if(b4.x == a4.w + keyLength + gap)
		matches[x] = a4.w - offset;
	if(b4.y == a4.x + keyLength + gap)
		matches[x] = a4.x - offset;
	if(b4.y == a4.z + keyLength + gap)
		matches[x] = a4.z - offset;
	if(b4.y == a4.w + keyLength + gap)
		matches[x] = a4.w - offset;
	if(b4.z == a4.x + keyLength + gap)
		matches[x] = a4.x - offset;
	if(b4.z == a4.y + keyLength + gap)
		matches[x] = a4.y - offset;
	if(b4.z == a4.w + keyLength + gap)
		matches[x] = a4.w - offset;
	if(b4.w == a4.x + keyLength + gap)
		matches[x] = a4.x - offset;
	if(b4.w == a4.y + keyLength + gap)
		matches[x] = a4.y - offset;
	if(b4.w == a4.z + keyLength + gap)
		matches[x] = a4.z - offset;
}