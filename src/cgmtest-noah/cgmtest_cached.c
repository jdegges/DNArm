#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <CL/cl.h>
//#include <omp.h>

#include "cgmhelper.h"

#define BUF_SIZE 1024

#define DEBUG 0

#define print_error(format, ...) { \
	fprintf (stderr, "[%s:%d in %s] ", __FILE__, __LINE__, __func__); \
	fprintf (stderr, format, ##__VA_ARGS__); \
	fprintf (stderr, "\n"); \
} \

int doubleMatchAid(uint32_t* a, uint32_t* b, int aLength, int bLength,
		int secLength, uint32_t** matches, int gap, int startOffset) {
	int i = 0, j = 0, mLength = 0;
	uint32_t* dubs = NULL;

	/* maximum length is length of smaller list */
	int mMax = aLength;
	if (bLength < mMax)
		mMax = bLength;

	dubs = (uint32_t*) malloc(sizeof(uint32_t) * mMax);
	if (dubs == NULL) {
		printf("Unable to allocate memory!\n");
		exit(-1);
	}

	/* loop through the items in the first list looking for matching items in the second list */
	while (i < aLength && j < bLength) {
		while (j < bLength) {
			if (b[j] < a[i] + secLength + gap)
				j++;
			else if (b[j] > a[i] + secLength + gap)
				break;
			else {
				dubs[mLength] = a[i] - startOffset;
				mLength++;
				break;
			}
		}
		i++;
	}

	/* if results were found, return them, otherwise free the memory */
	if (mLength > 0) {
		*matches = dubs;
		return mLength;
	}

	free(dubs);
	return 0;

}

double gpu_cgm_cacheing2(uint32_t* aList, uint32_t* bList, int aLength, int bLength,
		int keyLength, uint32_t** matches, char* clFile, int x, int y) {
	int gap = 0, myoffset = 0;
	cl_platform_id *platforms;
	cl_uint num_platforms = 0;
	cl_device_id *devices;
	cl_uint num_devices = 0;
	cl_context context;
	cl_command_queue command_queue;
	cl_mem a_buf;
	cl_mem b_buf;
	cl_mem res_buf;
	cl_program program;
	cl_kernel kernel;
	cl_uint *results;
	FILE *prgm_fptr;
	struct stat prgm_sbuf;
	char *prgm_data;
	size_t prgm_size;
	size_t offset;
	size_t count;
	const size_t global_work_size[] = { x, y };

	cl_int ret;
	cl_uint i;

	struct timeval t1, t2;
	double elapsedTime;

	results = malloc(sizeof(cl_uint) * aLength);

	/* figure out how many CL platforms are available */
	ret = clGetPlatformIDs(0, NULL, &num_platforms);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting the number of platform IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (0 == num_platforms) {
		print_error ("No CL platforms were found.");
		exit(EXIT_FAILURE);
	}

	/* allocate space for each available platform ID */
	if (NULL == (platforms = malloc((sizeof *platforms) * num_platforms))) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* get all of the platform IDs */
	ret = clGetPlatformIDs(num_platforms, platforms, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting platform IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* find a platform that supports given device type */
	//	print_error ("Number of platforms found: %d", num_platforms);
	for (i = 0; i < num_platforms; i++) {
		ret = clGetDeviceIDs(platforms[i], getDeviceType(), 0, NULL,
				&num_devices);
		if (CL_SUCCESS != ret)
			continue;

		if (0 < num_devices)
			break;
	}

	/* make sure at least one device was found */
	if (num_devices == 0) {
		print_error ("No CL device found that supports device type: %s.", ((getDeviceType() == CL_DEVICE_TYPE_CPU) ? "CPU" : "GPU"));
		exit(EXIT_FAILURE);
	}

	/* only one device is necessary... */
	num_devices = 1;
	if (NULL == (devices = malloc((sizeof *devices) * num_devices))) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* get one device id */
	ret = clGetDeviceIDs(platforms[i], getDeviceType(), num_devices,
			devices, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting device IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create a context for the CPU device that was found earlier */
	context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &ret);
	if (NULL == context || CL_SUCCESS != ret) {
		print_error ("Failed to create context: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create a command queue for the CPU device */
	command_queue = clCreateCommandQueue(context, devices[0], 0, &ret);
	if (NULL == command_queue || CL_SUCCESS != ret) {
		print_error ("Failed to create a command queue: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create buffers on the CL device */
	a_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
			sizeof(cl_uint) * aLength, NULL, &ret);
	if (NULL == a_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create a buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	b_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
			sizeof(cl_uint) * bLength, NULL, &ret);
	if (NULL == b_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create b buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	int res_bufSize = aLength;

	res_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint)
			* res_bufSize, NULL, &ret);
	if (NULL == res_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create b buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* read the opencl program code into a string */
	prgm_fptr = fopen(clFile, "r");
	if (NULL == prgm_fptr) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}

	if (0 != stat(clFile, &prgm_sbuf)) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}
	prgm_size = prgm_sbuf.st_size;

	prgm_data = malloc(prgm_size);
	if (NULL == prgm_data) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* make sure all data is read from the file (just in case fread returns
	 * short) */
	offset = 0;
	while (prgm_size - offset != (count = fread(prgm_data + offset, 1,
			prgm_size - offset, prgm_fptr)))
		offset += count;

	if (0 != fclose(prgm_fptr)) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}

	/* create a 'program' from the source */
	program = clCreateProgramWithSource(context, 1, (const char **) &prgm_data,
			&prgm_size, &ret);
	if (NULL == program || CL_SUCCESS != ret) {
		print_error ("Failed to create program with source: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* compile the program.. (it uses llvm or something) */
	ret = clBuildProgram(program, num_devices, devices, NULL, NULL, NULL);
	if (CL_SUCCESS != ret) {
		size_t size;
		char *log = calloc(1, 4000);
		if (NULL == log) {
			print_error ("Out of memory");
			exit(EXIT_FAILURE);
		}

		print_error ("Failed to build program: %d", ret);
		ret = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG,
				4096, log, &size);
		if (CL_SUCCESS != ret) {
			print_error ("Failed to get program build info: %d", ret);
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "Begin log:\n%s\nEnd log.\n", log);
		exit(EXIT_FAILURE);
	}

	/* pull out a reference to your kernel */
	kernel = clCreateKernel(program, "cgm_kernel", &ret);
	if (NULL == kernel || CL_SUCCESS != ret) {
		print_error ("Failed to create kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	gettimeofday(&t1, NULL);

	/* write data to these buffers */
	clEnqueueWriteBuffer(command_queue, a_buf, CL_FALSE, 0, aLength
			* sizeof(uint32_t), (void*) aList, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, b_buf, CL_FALSE, 0, bLength
			* sizeof(uint32_t), (void*) bList, 0, NULL, NULL);

	/* set your kernel's arguments */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 2, sizeof(int), &aLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 3, sizeof(int), &bLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 4, sizeof(uint32_t) * aLength, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 5, sizeof(uint32_t) * bLength, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 6, sizeof(int), &gap);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 7, sizeof(int), &myoffset);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 8, sizeof(int), &keyLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 9, sizeof(cl_mem), &res_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* make sure buffers have been written before executing */
	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* enque this kernel for execution... */
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,
			global_work_size, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* wait for the kernel to finish executing */
	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* copy the contents of dev_buf from the CL device to the host (CPU) */
	ret = clEnqueueReadBuffer(command_queue, res_buf, true, 0, sizeof(cl_uint)
			* aLength, results, 0, NULL, NULL);

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0; // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms

	if (CL_SUCCESS != ret) {
		print_error ("Failed to copy data from device to host: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* make sure the content of the buffer are what we expect */
	if(DEBUG)
	{
		for (i = 0; i < aLength; i++)
			printf("%d\n", results[i]);
	}

	/* free up resources */
	ret = clReleaseKernel(kernel);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseProgram(program);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release program: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseMemObject(a_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clReleaseMemObject(b_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseMemObject(res_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (CL_SUCCESS != (ret = clReleaseCommandQueue(command_queue))) {
		print_error ("Failed to release command queue: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (CL_SUCCESS != (ret = clReleaseContext(context))) {
		print_error ("Failed to release context: %d", ret);
		exit(EXIT_FAILURE);
	}

	matches = &results;
	return elapsedTime;
}

double gpu_cgm_cacheing(uint32_t* aList, uint32_t* bList, int aLength, int bLength,
		int keyLength, uint32_t** matches, char* clFile, int x, int y) {
	int gap = 0, myoffset = 0;
	cl_platform_id *platforms;
	cl_uint num_platforms = 0;
	cl_device_id *devices;
	cl_uint num_devices = 0;
	cl_context context;
	cl_command_queue command_queue;
	cl_mem a_buf;
	cl_mem b_buf;
	cl_mem res_buf;
	cl_program program;
	cl_kernel kernel;
	cl_uint *results;
	FILE *prgm_fptr;
	struct stat prgm_sbuf;
	char *prgm_data;
	size_t prgm_size;
	size_t offset;
	size_t count;
	const size_t global_work_size[] = { x, y };

	cl_int ret;
	cl_uint i;

	struct timeval t1, t2;
	double elapsedTime;

	results = malloc(sizeof(cl_uint) * aLength);

	/* figure out how many CL platforms are available */
	ret = clGetPlatformIDs(0, NULL, &num_platforms);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting the number of platform IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (0 == num_platforms) {
		print_error ("No CL platforms were found.");
		exit(EXIT_FAILURE);
	}

	/* allocate space for each available platform ID */
	if (NULL == (platforms = malloc((sizeof *platforms) * num_platforms))) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* get all of the platform IDs */
	ret = clGetPlatformIDs(num_platforms, platforms, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting platform IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* find a platform that supports given device type */
	//	print_error ("Number of platforms found: %d", num_platforms);
	for (i = 0; i < num_platforms; i++) {
		ret = clGetDeviceIDs(platforms[i], getDeviceType(), 0, NULL,
				&num_devices);
		if (CL_SUCCESS != ret)
			continue;

		if (0 < num_devices)
			break;
	}

	/* make sure at least one device was found */
	if (num_devices == 0) {
		print_error ("No CL device found that supports device type: %s.", ((getDeviceType() == CL_DEVICE_TYPE_CPU) ? "CPU" : "GPU"));
		exit(EXIT_FAILURE);
	}

	/* only one device is necessary... */
	num_devices = 1;
	if (NULL == (devices = malloc((sizeof *devices) * num_devices))) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* get one device id */
	ret = clGetDeviceIDs(platforms[i], getDeviceType(), num_devices,
			devices, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Error getting device IDs: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create a context for the CPU device that was found earlier */
	context = clCreateContext(NULL, num_devices, devices, NULL, NULL, &ret);
	if (NULL == context || CL_SUCCESS != ret) {
		print_error ("Failed to create context: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create a command queue for the CPU device */
	command_queue = clCreateCommandQueue(context, devices[0], 0, &ret);
	if (NULL == command_queue || CL_SUCCESS != ret) {
		print_error ("Failed to create a command queue: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* create buffers on the CL device */
	a_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
			sizeof(cl_uint) * aLength, NULL, &ret);
	if (NULL == a_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create a buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	b_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
			sizeof(cl_uint) * bLength, NULL, &ret);
	if (NULL == b_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create b buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	int res_bufSize = aLength;

	res_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_uint)
			* res_bufSize, NULL, &ret);
	if (NULL == res_buf || CL_SUCCESS != ret) {
		print_error ("Failed to create b buffer: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* read the opencl program code into a string */
	prgm_fptr = fopen(clFile, "r");
	if (NULL == prgm_fptr) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}

	if (0 != stat(clFile, &prgm_sbuf)) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}
	prgm_size = prgm_sbuf.st_size;

	prgm_data = malloc(prgm_size);
	if (NULL == prgm_data) {
		print_error ("Out of memory");
		exit(EXIT_FAILURE);
	}

	/* make sure all data is read from the file (just in case fread returns
	 * short) */
	offset = 0;
	while (prgm_size - offset != (count = fread(prgm_data + offset, 1,
			prgm_size - offset, prgm_fptr)))
		offset += count;

	if (0 != fclose(prgm_fptr)) {
		print_error ("%s", strerror (errno));
		exit(EXIT_FAILURE);
	}

	/* create a 'program' from the source */
	program = clCreateProgramWithSource(context, 1, (const char **) &prgm_data,
			&prgm_size, &ret);
	if (NULL == program || CL_SUCCESS != ret) {
		print_error ("Failed to create program with source: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* compile the program.. (it uses llvm or something) */
	ret = clBuildProgram(program, num_devices, devices, NULL, NULL, NULL);
	if (CL_SUCCESS != ret) {
		size_t size;
		char *log = calloc(1, 4000);
		if (NULL == log) {
			print_error ("Out of memory");
			exit(EXIT_FAILURE);
		}

		print_error ("Failed to build program: %d", ret);
		ret = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG,
				4096, log, &size);
		if (CL_SUCCESS != ret) {
			print_error ("Failed to get program build info: %d", ret);
			exit(EXIT_FAILURE);
		}

		fprintf(stderr, "Begin log:\n%s\nEnd log.\n", log);
		exit(EXIT_FAILURE);
	}

	/* pull out a reference to your kernel */
	kernel = clCreateKernel(program, "cgm_kernel", &ret);
	if (NULL == kernel || CL_SUCCESS != ret) {
		print_error ("Failed to create kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	gettimeofday(&t1, NULL);

	/* write data to these buffers */
	clEnqueueWriteBuffer(command_queue, a_buf, CL_FALSE, 0, aLength
			* sizeof(uint32_t), (void*) aList, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, b_buf, CL_FALSE, 0, bLength
			* sizeof(uint32_t), (void*) bList, 0, NULL, NULL);

	/* set your kernel's arguments */
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 2, sizeof(int), &aLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 3, sizeof(int), &bLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 4, sizeof(int), &gap);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 5, sizeof(int), &myoffset);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clSetKernelArg(kernel, 6, sizeof(int), &keyLength);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clSetKernelArg(kernel, 7, sizeof(cl_mem), &res_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to set kernel argument: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* make sure buffers have been written before executing */
	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* enque this kernel for execution... */
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,
			global_work_size, NULL, 0, NULL, NULL);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* wait for the kernel to finish executing */
	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* copy the contents of dev_buf from the CL device to the host (CPU) */
	ret = clEnqueueReadBuffer(command_queue, res_buf, true, 0, sizeof(cl_uint)
			* aLength, results, 0, NULL, NULL);

	gettimeofday(&t2, NULL);
	elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0; // sec to ms
	elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms

	if (CL_SUCCESS != ret) {
		print_error ("Failed to copy data from device to host: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clEnqueueBarrier(command_queue);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to enqueue barrier: %d", ret);
		exit(EXIT_FAILURE);
	}

	/* make sure the content of the buffer are what we expect */
	if(DEBUG)
	{
		for (i = 0; i < aLength; i++)
			printf("%d\n", results[i]);
	}

	/* free up resources */
	ret = clReleaseKernel(kernel);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release kernel: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseProgram(program);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release program: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseMemObject(a_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}
	ret = clReleaseMemObject(b_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}

	ret = clReleaseMemObject(res_buf);
	if (CL_SUCCESS != ret) {
		print_error ("Failed to release memory object: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (CL_SUCCESS != (ret = clReleaseCommandQueue(command_queue))) {
		print_error ("Failed to release command queue: %d", ret);
		exit(EXIT_FAILURE);
	}

	if (CL_SUCCESS != (ret = clReleaseContext(context))) {
		print_error ("Failed to release context: %d", ret);
		exit(EXIT_FAILURE);
	}

	matches = &results;
	return elapsedTime;
}

int uint32_t_cmp(const void *a, const void *b) {
	const uint32_t* cp_a = (const int*) a;
	const uint32_t* cp_b = (const int*) b;

	return *cp_a - *cp_b;
}

int main(int argc, char* argv[]) {
	if (argc < 6) {
		printf(
				"USAGE: cgmtest_shared MAXVALUE KEYSIZE LISTSIZE REPS MAXCOMPUTEUNITS [GPU_TOGGLE]\nNote: list size should be less than	the min of max work items[0] and max work items[1]\n");
		exit(-1);
	}

	uint32_t* aList = NULL;
	uint32_t* bList = NULL;
	uint32_t* matches = NULL;
	uint32_t* matches2 = NULL;
	
	int matchLen;

	int maxValue = atoi(argv[1]);
	int keySize = atoi(argv[2]);
	int listSize = atoi(argv[3]);
	int reps = atoi(argv[4]);
	int cpus = atoi(argv[5]);
	if(argc == 7)
		setGPUEnabled(atoi(argv[6]));

	aList = (uint32_t*) malloc(sizeof(uint32_t) * listSize);
	bList = (uint32_t*) malloc(sizeof(uint32_t) * listSize);

	if (aList == NULL || bList == NULL) {
		printf("Error allocating memory\n");
		exit(-1);
	}

	struct timeval t1, t2;
	double elapsedTime;
	double seq = 0, cache_const = 0, cache_local = 0;

	srand((unsigned) time(NULL));

	int i, j, m;

	for (i = 0; i < reps; i++) {
		if(DEBUG) printf("*****Trial %d*****\nGenerating lists...", i);
		for (j = 0; j < listSize; j++)
			aList[j] = rand() % maxValue;

		for (j = 0; j < listSize; j++)
			bList[j] = rand() % maxValue;
		if(DEBUG) printf("Done.\nSorting...");
		qsort(aList, listSize, sizeof(uint32_t), uint32_t_cmp);
		qsort(bList, listSize, sizeof(uint32_t), uint32_t_cmp);

		if(DEBUG) printf("Done.\nExecuting...\n");
		gettimeofday(&t1, NULL);

		matchLen = doubleMatchAid(aList, bList, listSize, listSize, keySize, &matches, 0,
				0);
				
		if(DEBUG)
		{
			for(m = 0; m < matchLen; m++)
				printf(" %d", matches[m]);
			printf("\n");
		}

		gettimeofday(&t2, NULL);
		elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0; // sec to ms
		elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms
		seq += elapsedTime;
		if(DEBUG) printf("Sequential: %f\n", elapsedTime);

		elapsedTime = gpu_cgm_cacheing(aList, bList, listSize, listSize, keySize,
				&matches, "cgmcl_cached.cl", listSize, listSize);

		cache_const += elapsedTime;
		if(DEBUG) printf("Constant Cacheing: %f\n", elapsedTime);

		elapsedTime = gpu_cgm_cacheing2(aList, bList, listSize, listSize, keySize,
						&matches, "cgmcl_cached2.cl", listSize, listSize);

		cache_local += elapsedTime;
		if(DEBUG) printf("Local Cacheing: %f\n", elapsedTime);

	}

	if(DEBUG) printf("Sequential: %f\n", seq / reps);
	printf("Constant Cacheing: %f\n", cache_const / reps);
	printf("Local Cacheing: %f\n", cache_local / reps);
}
