__kernel void
samplecl_kernel (__global uint *dev_buf)
{
  const size_t width = get_global_size (0);
  const size_t i = get_global_id (0);
  const size_t j = get_global_id (1);
  const size_t index = i + j * width;

  dev_buf[index] = index;
}
