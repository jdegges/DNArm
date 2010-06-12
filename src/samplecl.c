#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <CL/cl.h>

#define BUF_SIZE 1024

#define print_error(format, ...) { \
  fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  fprintf (stderr, format, ##__VA_ARGS__); \
  fprintf (stderr, "\n"); \
}

int
main (void)
{
  cl_platform_id *platforms;
  cl_uint num_platforms = 0;
  cl_device_id *devices;
  cl_uint num_devices = 0;
  cl_context context;
  cl_command_queue command_queue;
  cl_mem dev_buf;
  cl_program program;
  cl_kernel kernel;
  cl_uint *hst_buf = malloc (4096);
  FILE *prgm_fptr;
  struct stat prgm_sbuf;
  char *prgm_data;
  size_t prgm_size;
  size_t offset;
  size_t count;
  const size_t global_work_size[] = {32, 32};

  cl_int ret;
  cl_uint i;

  /* figure out how many CL platforms are available */
  ret = clGetPlatformIDs (0, NULL, &num_platforms);
  if (CL_SUCCESS != ret)
    {
      print_error ("Error getting the number of platform IDs: %d", ret);
      exit (EXIT_FAILURE);
    }

  if (0 == num_platforms)
    {
      print_error ("No CL platforms were found.");
      exit (EXIT_FAILURE);
    }

  /* allocate space for each available platform ID */
  if (NULL == (platforms = malloc ((sizeof *platforms) * num_platforms)))
    {
      print_error ("Out of memory");
      exit (EXIT_FAILURE);
    }

  /* get all of the platform IDs */
  ret = clGetPlatformIDs (num_platforms, platforms, NULL);
  if (CL_SUCCESS != ret)
    {
      print_error ("Error getting platform IDs: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* find a platform that supports CL_DEVICE_TYPE_CPU */
  print_error ("Number of platforms found: %d", num_platforms);
  for (i = 0; i < num_platforms; i++)
    {
      ret = clGetDeviceIDs (platforms[i], CL_DEVICE_TYPE_CPU, 0, NULL,
                            &num_devices);
      if (CL_SUCCESS != ret)
        continue;

      if (0 < num_devices)
        break;
    }

  /* make sure at least one device was found */
  if (num_devices == 0)
    {
      print_error ("No CL device found that supports CPU.");
      exit (EXIT_FAILURE);
    }

  /* only one device is necessary... */
  num_devices = 1;
  if (NULL == (devices = malloc ((sizeof *devices) * num_devices)))
    {
      print_error ("Out of memory");
      exit (EXIT_FAILURE);
    }

  /* get one device id */
  ret = clGetDeviceIDs (platforms[i], CL_DEVICE_TYPE_CPU, num_devices, devices,
                        NULL);
  if (CL_SUCCESS != ret)
    {
      print_error ("Error getting device IDs: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* create a context for the CPU device that was found earlier */
  context = clCreateContext (NULL, num_devices, devices, NULL, NULL, &ret);
  if (NULL == context || CL_SUCCESS != ret)
    {
      print_error ("Failed to create context: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* create a command queue for the CPU device */
  command_queue = clCreateCommandQueue (context, devices[0], 0, &ret);
  if (NULL == command_queue || CL_SUCCESS != ret)
    {
      print_error ("Failed to create a command queue: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* create a buffer on the CL device */
  dev_buf = clCreateBuffer (context, CL_MEM_READ_WRITE,
                            sizeof (cl_uint) * BUF_SIZE, NULL, &ret);
  if (NULL == dev_buf || CL_SUCCESS != ret)
    {
      print_error ("Failed to create buffer: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* read the opencl program code into a string */
  prgm_fptr = fopen ("samplecl.cl", "r");
  if (NULL == prgm_fptr)
    {
      print_error ("%s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  if (0 != stat ("samplecl.cl", &prgm_sbuf))
    {
      print_error ("%s", strerror (errno));
      exit (EXIT_FAILURE);
    }
  prgm_size = prgm_sbuf.st_size;

  prgm_data = malloc (prgm_size);
  if (NULL == prgm_data)
    {
      print_error ("Out of memory");
      exit (EXIT_FAILURE);
    }

  /* make suer all data is read from the file (just in case fread returns
   * short) */
  offset = 0;
  while (prgm_size - offset
        != (count = fread (prgm_data + offset, 1, prgm_size - offset,
                           prgm_fptr)))
    offset += count;

  if (0 != fclose (prgm_fptr))
    {
      print_error ("%s", strerror (errno));
      exit (EXIT_FAILURE);
    }

  /* create a 'program' from the source */
  program = clCreateProgramWithSource (context, 1, (const char **) &prgm_data,
                                       &prgm_size, &ret);
  if (NULL == program || CL_SUCCESS != ret)
    {
      print_error ("Failed to create program with source: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* compile the program.. (it uses llvm or something) */
  ret = clBuildProgram (program, num_devices, devices, NULL, NULL, NULL);
  if (CL_SUCCESS != ret)
    {
      size_t size;
      char *log = calloc (1, 4000);
      if (NULL == log)
        {
          print_error ("Out of memory");
          exit (EXIT_FAILURE);
        }

      print_error ("Failed to build program: %d", ret);
      ret = clGetProgramBuildInfo (program, devices[0], CL_PROGRAM_BUILD_LOG,
                                   4096, log, &size);
      if (CL_SUCCESS != ret)
        {
          print_error ("Failed to get program build info: %d", ret);
          exit (EXIT_FAILURE);
        }

      fprintf (stderr, "Begin log:\n%s\nEnd log.\n", log);
      exit (EXIT_FAILURE);
    }

  /* pull out a reference to your kernel */
  kernel = clCreateKernel (program, "samplecl_kernel", &ret);
  if (NULL == kernel || CL_SUCCESS != ret)
    {
      print_error ("Failed to create kernel: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* set your kernel's arguments (this kernel only has one argument) */
  ret = clSetKernelArg (kernel, 0, sizeof(cl_mem), &dev_buf);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to set kernel argument: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* enque this kernel for execution... */
  ret = clEnqueueNDRangeKernel (command_queue, kernel, 2, NULL,
                                global_work_size, NULL, 0, NULL,
                                NULL);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to enqueue kernel: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* wait for the kernel to finish executing */
  ret = clEnqueueBarrier (command_queue);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to enqueue barrier: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* copy the contents of dev_buf from the CL device to the host (CPU) */
  ret = clEnqueueReadBuffer (command_queue, dev_buf, true, 0,
                             sizeof (cl_uint) * BUF_SIZE, hst_buf, 0, NULL,
                             NULL);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to copy data from device to host: %d", ret);
      exit (EXIT_FAILURE);
    }

  ret = clEnqueueBarrier (command_queue);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to enqueue barrier: %d", ret);
      exit (EXIT_FAILURE);
    }

  /* make sure the content of the buffer are what we expect */
  for (i = 0; i < BUF_SIZE; i++)
    if (i != hst_buf[i])
      print_error ("(i,hst_buf[i]) = (%d,%u)", i, hst_buf[i]);

  /* free up resources */
  ret = clReleaseKernel (kernel);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to release kernel: %d", ret);
      exit (EXIT_FAILURE);
    }

  ret = clReleaseProgram (program);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to release program: %d", ret);
      exit (EXIT_FAILURE);
    }

  ret = clReleaseMemObject (dev_buf);
  if (CL_SUCCESS != ret)
    {
      print_error ("Failed to release memory object: %d", ret);
      exit (EXIT_FAILURE);
    }

  if (CL_SUCCESS != (ret = clReleaseCommandQueue (command_queue)))
    {
      print_error ("Failed to release command queue: %d", ret);
      exit (EXIT_FAILURE);
    }

  if (CL_SUCCESS != (ret = clReleaseContext (context)))
    {
      print_error ("Failed to release context: %d", ret);
      exit (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
