/*
 * cgmhelper.h
 *
 *  Created on: Jun 9, 2010
 *      Author: Noah
 */

#include <CL/cl.h>

#ifndef CGMHELPER_H_
#define CGMHELPER_H_

int GPU_ENABLED = 0;

cl_device_type getDeviceType()
{
	if(GPU_ENABLED)
		return CL_DEVICE_TYPE_GPU;
	else
		return CL_DEVICE_TYPE_CPU;
}

void setGPUEnabled(int i)
{
	if (i != 0)
		GPU_ENABLED = 1;
	else
		GPU_ENABLED = 0;
}


#endif /* CGMHELPER_H_ */
