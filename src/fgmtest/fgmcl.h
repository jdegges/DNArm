#ifndef _FGMCL_H
#define _FGMCL_H

#include<stdio.h>
#include<stdlib.h>
#include</usr/include/stdint.h>
#include<CL/cl.h>

#define THRESHOLD 5
#define INDEL_THRESHOLD 27

//flags for mutation flags
#define SNP 0
#define INS 1
#define DEL -1

#define print_error(format, ...) { \
  fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
  fprintf (stderr, format, ##__VA_ARGS__); \
  fprintf (stderr, "\n"); \
}

#define errorOnNotSuccess(nsCheck, format, ...){\
	if(CL_SUCCESS != nsCheck)\
	{\
		print_error (format, ##__VA_ARGS__);\
		exit (EXIT_FAILURE);\
	}\
}

#define errorOnZero(zCheck, format, ...){\
	if(0 == zCheck)\
	{\
		print_error (format, ##__VA_ARGS__);\
		exit (EXIT_FAILURE);\
	}\
}

#define errorOnNonzero(nzCheck, format, ...){\
	if(0 != nzCheck)\
	{\
		print_error (format, ##__VA_ARGS__);\
		exit (EXIT_FAILURE);\
	}\
}

#define errorOnNull(nCheck, format, ...){\
	if(NULL == nCheck)\
	{\
		print_error (format, ##__VA_ARGS__);\
		exit (EXIT_FAILURE);\
	}\
}

#define errorOnNotSuccessNull(nsCheck, nCheck, format, ...){\
	if (NULL == nCheck || CL_SUCCESS != nsCheck)\
	{\
		print_error (format, ##__VA_ARGS__);\
		exit (EXIT_FAILURE);\
	}\
}
/**/
struct cgmResult{
      int read[3]; // 16 bases in each int
 
      uint* matches; // pointer to list of match locations
      int length; // length of match list
};
/**/
typedef struct clInformation
{
	int batchSize;
	cl_context fg_context;
	cl_command_queue fg_command_queue;
	cl_kernel fg_kernel;
//	const size_t cl_global_work_size[] = {32, 32};	//? check this?
	size_t fg_global_work_size[2];
	cl_program fg_program;
	cl_mem fg_mllLen_buf;
	cl_mem fg_res_buf;
	cl_mem fg_rs_buf;
	cl_mem fg_list_buf;
} clData;

typedef struct mutationData
{
	char * mods;	//what the nucleotide should be changed to...
	unsigned int * locs;	//the location at which the data should be changed.
	int * ins;	//indicates if this should be an insertion.
			//note: an insertion will list the same location multiple times, while a deletion will have a "D" in the corresponding nucleotide modification location.
	int len;
} mData;

clData * fgmInit(uint * list, uint listLen, int rdLen, int batchSize);

int fgmStart(clData * device, struct cgmResult * data, int setSize);

void fgmKill(clData * device);

#endif
