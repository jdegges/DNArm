//FILE: fgm.c (fine-grain matcher)
//AUTHOR: Joel C. Miller
//UCLA 2010, Spring Quarter
//Produced for CS 133/CS CM124 Joint Final Project - DNArm (fast DNA read mapper)
//
//MIT LICENCE...
//

#include<stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include</usr/include/stdint.h>
#include<CL/cl.h>
//#include "fgm.h"
#include "fgmcl.h"
#include <errno.h>

clData * fgmInit(uint * list, uint listLen, int rdLen, int batchSize) //return value is a success/fail indicator
{
  clData * device;	//for keeping track of stuff for our cl run.
  
  cl_platform_id *fg_platforms;
  cl_uint fg_num_platforms = 0;
  cl_device_id *fg_devices;
  cl_uint fg_num_devices = 0;
//  cl_context fg_context;
//  cl_command_queue fg_command_queue;
//  cl_mem a_buf;
//  cl_mem b_buf;
//  cl_mem c_buf;
  
//  cl_program fg_program;
//  cl_kernel fg_kernel;
//  cl_uint *results;
  FILE *fg_prgm_fptr;
  struct stat fg_prgm_sbuf;
  char *fg_prgm_data;
  size_t fg_prgm_size;
  size_t fg_offset;
  size_t fg_count;
//  const size_t fg_global_work_size[] = {32, 32};	
  
  cl_mem fg_res_buf;
  cl_mem fg_list_buf;
  cl_mem fg_mll_buf;
  cl_mem fg_mllLen_buf;
  cl_int ret;
  int i;
  
  device = malloc(sizeof(clData));
//  device->fg_global_work_size[0] = batchSize;	//set active work element/thread count...

	//batchsize 1024...
	device->fg_global_work_size[0] = 32;
	device->fg_global_work_size[1] = 32;
	

  /* figure out how many CL platforms are available */
//  platformNumCheck(ret);
/* figure out how many CL platforms are available */
  ret = clGetPlatformIDs (0, NULL, &fg_num_platforms);
  errorOnNotSuccess(ret, "Error getting the number of platform IDs: %d", ret);
  errorOnZero(fg_num_platforms, "No CL platforms were found.");

  
  /* allocate space for each available platform ID */
  fg_platforms = malloc ((sizeof *fg_platforms) * fg_num_platforms);
  errorOnNull(fg_platforms, "Out of memory");

  
  /* get all of the platform IDs */
  ret = clGetPlatformIDs (fg_num_platforms, fg_platforms, NULL);
  errorOnNotSuccess(ret, "Error getting platform IDs: %d", ret);

  
  /* find a platform that supports CL_DEVICE_TYPE_GPU */
  print_error ("Number of platforms found: %d", fg_num_platforms);
  for (i = 0; i < fg_num_platforms; i++)
    {
      ret = clGetDeviceIDs (fg_platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &fg_num_devices);
      if (CL_SUCCESS != ret)
        continue;

      if (0 < fg_num_devices)
        break;
    }
  print_error("CL_DEVICE_TYPE_GPU found. %d", ret);

  /* make sure at least one device was found */
  errorOnZero(fg_num_devices, "No CL device found that supports GPU Processing.");

  
  /* only one device is necessary... */
  fg_num_devices = 1;
  fg_devices = malloc ((sizeof *fg_devices) * fg_num_devices);
  errorOnNull(fg_devices, "Out of memory");

  
  /* get one device id */
  ret = clGetDeviceIDs (fg_platforms[i], CL_DEVICE_TYPE_GPU, fg_num_devices, fg_devices, NULL);
  errorOnNotSuccess(ret, "Error getting device IDs: %d", ret);

  
  /* create a context for the GPU device that was found earlier */
  device->fg_context = clCreateContext (NULL, fg_num_devices, fg_devices, NULL, NULL, &ret);
  errorOnNotSuccessNull(ret, device->fg_context, "Failed to create context: %d", ret);

  
  /* create a command queue for the GPU device */
  device->fg_command_queue = clCreateCommandQueue (device->fg_context, fg_devices[0], 0, &ret);
  errorOnNotSuccessNull(ret, device->fg_command_queue, "Failed to create a command queue: %d", ret);

  
  /* read the opencl program code into a string */
  fg_prgm_fptr = fopen ("fgmcl.cl", "r");
  errorOnNull(fg_prgm_fptr, "%s", strerror (errno));
  errorOnNonzero(stat ("fgmcl.cl", &fg_prgm_sbuf), "%s", strerror (errno));

  fg_prgm_size = fg_prgm_sbuf.st_size;
  fg_prgm_data = malloc (fg_prgm_size);
  errorOnNull(fg_prgm_data, "Out of memory");
  
  
  /* make sure all data is read from the file (just in case fread returns
   * short) */
  fg_offset = 0;
  while (fg_prgm_size - fg_offset != (fg_count = fread (fg_prgm_data + fg_offset, 1, fg_prgm_size - fg_offset, fg_prgm_fptr)))
    fg_offset += fg_count;
  errorOnNonzero(fclose (fg_prgm_fptr), "%s", strerror (errno));

  
  /* create a 'program' from the source */
  device->fg_program = clCreateProgramWithSource (device->fg_context, 1, (const char **) &fg_prgm_data, &fg_prgm_size, &ret);
  errorOnNotSuccessNull(ret, device->fg_program, "Failed to create program with source: %d", ret);

  
  /* compile the program.. (it uses llvm or something) */
  ret = clBuildProgram (device->fg_program, fg_num_devices, fg_devices, NULL, NULL, NULL);
  if (CL_SUCCESS != ret)
    {
      size_t size;
      char *log = calloc (1, 4000);
	  errorOnNull(log, "Out of memory");

      print_error ("Failed to build program: %d", ret);
      ret = clGetProgramBuildInfo (device->fg_program, fg_devices[0], CL_PROGRAM_BUILD_LOG,
                                   4096, log, &size);
      errorOnNotSuccess(ret, "Failed to get program build info: %d", ret);

      fprintf (stderr, "Begin log:\n%s\nEnd log.\n", log);
      exit (EXIT_FAILURE);
    }

  /* pull out a reference to your kernel */
  device->fg_kernel = clCreateKernel (device->fg_program, "fgm", &ret);
  errorOnNotSuccessNull(ret, device->fg_kernel, "Failed to create kernel: %d", ret);
  
  ////////////////////////////////////////////////////memory creation///////////////////////////////////////////////////
  
  /* create a buffer on the CL device *//////////////////////////////////////////////////////////////////////////////////////////////////////\
  
  //returnData
  device->fg_res_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (mData) * batchSize, NULL, &ret);	//allocate results array...
  errorOnNotSuccessNull(ret, device->fg_res_buf, "Failed to create buffer: %d", ret);
 
  //list
  device->fg_list_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (uint) * listLen >> 4 + 1, NULL, &ret);	//allocate the reference genome space...
  errorOnNotSuccessNull(ret, device->fg_list_buf, "Failed to create buffer: %d", ret);
  clEnqueueWriteBuffer(device->fg_command_queue, device->fg_list_buf, CL_TRUE, 0, (listLen >> 4)*sizeof(uint), (void*) list, 0, NULL, NULL);	//copy list to GPU...
  
  //listLen
  ret = clSetKernelArg (device->fg_kernel, 2, sizeof(cl_mem), &listLen);
  errorOnNotSuccess(ret, "Failed to set kernel argument: %d", ret);
  
  //matchLocLst
  // device->fg_mll_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (int) * batchSize * mlMax, NULL, &ret);
  //allocate match list buffer at kickoff
  //data gets copied on kickoff.
  
  //mllLen
  device->fg_mllLen_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (int) * batchSize , NULL, &ret);	//allocate match list length array...
  errorOnNotSuccessNull(ret, device->fg_mllLen_buf, "Failed to create buffer: %d", ret);
  //data gets copied on kickoff.
  
  //rdSeq
  device->fg_rs_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (uint) * batchSize * 3, NULL, &ret);	//allocate read sequence array...
  errorOnNotSuccessNull(ret, device->fg_rs_buf, "Failed to create buffer: %d", ret);
  //data gets copied on kickoff.
  
  //mlMax
  //ret = clSetKernelArg (kernel, 6, sizeof(uint), &mlMax);
  //errorOnNotSuccess(ret, "Failed to set kernel argument: %d", ret);
  //mlMax is set on kickoff
  
  //batchSize
  //ret = clSetKernelArg (kernel, 7, sizeof(uint), &setSize);
  //errorOnNotSuccess(ret, "Failed to set kernel argument: %d", ret);
  //batchSize is set on kickoff.
  
  //////////////////save other data...
  
  device->batchSize = batchSize;
  
  return device;
 }
  ////////////////////////////////////////////////////MEM CREATION/////////////////////////////////////////
  
int fgmStart(clData * device, struct cgmResult * data, int setSize)//uint32_t * matchLocLst, int mllLen, uint32_t * rdSeq) //return value is a success/fail indicator
{

	int i, j;
	uint * mllTemp;
	uint * rsTemp;
	uint * mllLenTemp;
	int mlMax = 0;
	cl_int ret;
	
	mData * results;
	
	cl_mem fg_mll_buf;
	
	results = malloc(sizeof(mData)*device->batchSize);
	mllLenTemp = malloc(sizeof(int) * (device->batchSize));	//temporary mll length storage...
	rsTemp = malloc(sizeof(uint) * (device->batchSize) * 3);	//temp read seq storage.
	
	for(i = 0; i < setSize; i++)
		if(data[i].length > mlMax)
			mlMax = data[i].length;	//we have to find the max length of the data...
  
	mllTemp = malloc(sizeof(uint) * (device->batchSize) * mlMax);	//allocate matchListLength temp storage...
	fg_mll_buf = clCreateBuffer (device->fg_context, CL_MEM_READ_WRITE, sizeof (uint) * device->batchSize * mlMax, NULL, &ret); //create cl buffer for the match lists.
	errorOnNotSuccessNull(ret, fg_mll_buf, "Failed to create buffer: %d", ret);
	
	for(i = 0; i < device->batchSize; i++)
	{
		if(i < setSize)//if the data exists here...
		{
			mllLenTemp[i] = data[i].length;	//copy length
			
			rsTemp[i*3] = data[i].read[0];	//copy readSequence.
			rsTemp[i*3 + 1] = data[i].read[1];
			rsTemp[i*3 + 2] = data[i].read[2];
			
			for(j = 0; j < data[i].length; j++)	//copy matchlist locations.
			{
				mllTemp[i*3 + j] = data[i].matches[j];	//copy a single element...
			}
		}
		else
			mllLenTemp[i] = 0;	//put null data in.
		
	}
	
	//by now we have all of the necessary data in our temp variables.
	//all cl buffers have also been created...
	//so we can now set the arguments in entirety.
	
	//returnData already done...
	
	//list already done...
	
	//listLen already done...
	
	//matchLocLst
	clEnqueueWriteBuffer(device->fg_command_queue, fg_mll_buf, CL_TRUE, 0, sizeof (uint) * device->batchSize * mlMax, (void*) mllTemp, 0, NULL, NULL);	
	
	//mllLen
	clEnqueueWriteBuffer(device->fg_command_queue, device->fg_mllLen_buf, CL_TRUE, 0, sizeof (uint32_t) * device->batchSize, (void*) mllLenTemp, 0, NULL, NULL);
  
	//rdSeq
	clEnqueueWriteBuffer(device->fg_command_queue, device->fg_rs_buf, CL_TRUE, 0, sizeof (uint) * device->batchSize * 3, (void*) rsTemp, 0, NULL, NULL);
  
  
	//mlMax
	ret = clSetKernelArg (device->fg_kernel, 6, sizeof(uint), &mlMax);
	errorOnNotSuccess(ret, "Failed to set kernel argument: %d", ret);
	
	//batchSize
	ret = clSetKernelArg (device->fg_kernel, 7, sizeof(uint), &setSize);
	errorOnNotSuccess(ret, "Failed to set kernel argument: %d", ret);

  
////////////////////////////////////////////////////////////////////////
//here, arguments and prep work for the run should be complete. we now enqueue for execution.
////////////////////////////////////////////////////////////////////////
  
  /* enque this kernel for execution... */
	ret = clEnqueueNDRangeKernel (device->fg_command_queue, device->fg_kernel, 2, NULL, device->fg_global_work_size, NULL, 0, NULL, NULL);
	errorOnNotSuccess(ret, "Failed to enqueue kernel: %d", ret);

  
  /* wait for the kernel to finish executing */
	ret = clEnqueueBarrier (device->fg_command_queue);
	errorOnNotSuccess(ret, "Failed to enqueue barrier: %d", ret);

  
//  /* copy the contents of fg_ret_buf from the CL device to the host (CPU) */
	ret = clEnqueueReadBuffer (device->fg_command_queue, device->fg_res_buf, CL_TRUE, 0, sizeof (mData) * device->batchSize, results, 0, NULL, NULL);
	errorOnNotSuccess(ret, "Failed to copy data from device to host: %d", ret);
//now we have the results on our local memory.

	for(i = 0; i < device->batchSize; i++)	//print out any mutations...
	{
		for(j = 0; j < results[i].len; j++)
			switch(results[i].ins[j])
			{
				case DEL:
					printf("Type: DEL                    Location: %u\n", results[i].ins[j], results[i].locs[j]);
					break;
					
				case SNP:
					printf("Type: SNP    Mutation: %c    Location: %u\n", results[i].ins[j], results[i].locs[j]);
					break;
					
				case INS:
					printf("Type: INS    Mutation: %c    Location: %u\n", results[i].ins[j], results[i].locs[j]);
					break;
					
				default:
					break;	//silently fail..
			}
	}

//we should release our matchList allocation here...
	ret = clReleaseMemObject (fg_mll_buf);
	errorOnNotSuccess(ret, "Failed to release memory object: %d", ret);
	
	free(results);
	free(mllLenTemp);
	free(mllTemp);
	free(rsTemp);
	
//	ret = clEnqueueBarrier (device->fg_command_queue);
//	errorOnNotSuccess(ret, "Failed to enqueue barrier: %d", ret);

	return 0;
}
  /* make sure the content of the buffer are what we expect */
//	for (i = 0; i < BUF_SIZE; i++)
//		if (i != device->fg_hst_buf[i])
//			print_error ("(i,hst_buf[i]) = (%d,%u)", i, device->fg_hst_buf[i]);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	  
  
  // typedef struct clInformation
// {
	// int batchSize;
	// cl_context fg_context;
	// cl_command_queue fg_command_queue;
	// cl_kernel fg_kernel;
	// cl_global_work_size[] = {32, 32};	//? check this?
	// cl_program = fg_program;
	// cl_mem fg_mllLen_buf;
	// cl_mem fg_res_buf;
	// cl_mem fg_rs_buf;
	// cl_mem fg_list_buf;
// } clData;
  


void fgmKill(clData * device) 
{
/* free up resources */
	cl_int ret;

	ret = clReleaseKernel (device->fg_kernel);
	errorOnNotSuccess(ret, "Failed to release kernel: %d", ret);

  
	ret = clReleaseProgram (device->fg_program);
	errorOnNotSuccess(ret, "Failed to release program: %d", ret);

//release buffers
	ret = clReleaseMemObject (device->fg_mllLen_buf);
	errorOnNotSuccess(ret, "Failed to release memory object: %d", ret);
	ret = clReleaseMemObject (device->fg_res_buf);
	errorOnNotSuccess(ret, "Failed to release memory object: %d", ret);
	ret = clReleaseMemObject (device->fg_rs_buf);
	errorOnNotSuccess(ret, "Failed to release memory object: %d", ret);
	ret = clReleaseMemObject (device->fg_list_buf);
	errorOnNotSuccess(ret, "Failed to release memory object: %d", ret);

  
	ret = clReleaseCommandQueue (device->fg_command_queue);
	errorOnNotSuccess(ret, "Failed to release command queue: %d", ret);

  
	ret = clReleaseContext (device->fg_context);
	errorOnNotSuccess(ret, "Failed to release context: %d", ret);

//	exit (EXIT_SUCCESS);
}


