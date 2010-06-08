//FILE: fgm.c (fine-grain matcher)
//AUTHOR: Joel C. Miller
//UCLA 2010, Spring Quarter
//Produced for CS 133/CS CM124 Joint Final Project - DNArm (fast DNA read mapper)
//
//MIT LICENCE...
//

#include<stdlib.h>
#include</usr/include/stdint.h>
#include<CL/cl.h>
//#include "fgm.h"
#include <errno.h>

#define print_error(format, ...) { \
  fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  fprintf (stderr, format, ##__VA_ARGS__); \
  fprintf (stderr, "\n"); \
}


int fgmInit(uint32_t * list, uint32_t listLen, int rdLen) //return value is a success/fail indicator
{
  cl_platform_id *fg_platforms;
  cl_uint fg_num_platforms = 0;
  cl_device_id *fg_devices;
  cl_uint fg_num_devices = 0;
  cl_context fg_context;
  cl_command_queue fg_command_queue;
//  cl_mem a_buf;
//  cl_mem b_buf;
//  cl_mem c_buf;
//  cl_mem res_buf;
  cl_program fg_program;
  cl_kernel fg_kernel;
//  cl_uint *results;
  FILE *fg_prgm_fptr;
  struct stat fg_prgm_sbuf;
  char *fg_prgm_data;
  size_t fg_prgm_size;
  size_t fg_offset;
  size_t fg_count;
  const size_t fg_global_work_size[] = {32, 32};	
}


int fgmStart(uint32_t * matchLocLst, int mllLen, uint32_t * rdSeq); //return value is a success/fail indicator
{

}

mData * fgmReturn(); //this will return a list of mutationData structures (or just one, with all of the data consolidated, if you prefer - let me know), or NULL if the computations are not complete.
{

}


