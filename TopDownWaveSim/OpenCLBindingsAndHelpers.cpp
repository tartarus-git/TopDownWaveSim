#include "OpenCLBindingsAndHelpers.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define CHECK_FUNC_VALIDITY(func) if (!(func)) { return false; }																						// Simple helper define that reports error (returns false) if one of the functions doesn't bind correctly.

bool initOpenCLBindings() {
	HINSTANCE DLLProcID = LoadLibraryA("OpenCL.dll");																									// Load the OpenCL DLL.
	if (!DLLProcID) { return false; }																													// If it doesn't load correctly, fail.

	CHECK_FUNC_VALIDITY(clGetPlatformIDs = (clGetPlatformIDs_func)GetProcAddress(DLLProcID, "clGetPlatformIDs"));										// Go through all of the various functions and bind them (get function pointers to them and store those pointers in variables).
	CHECK_FUNC_VALIDITY(clGetPlatformInfo = (clGetPlatformInfo_func)GetProcAddress(DLLProcID, "clGetPlatformInfo"));									// If any one of these binds fails, everything stops and the whole functions fails.
	CHECK_FUNC_VALIDITY(clGetDeviceIDs = (clGetDeviceIDs_func)GetProcAddress(DLLProcID, "clGetDeviceIDs"));
	CHECK_FUNC_VALIDITY(clCreateContext = (clCreateContext_func)GetProcAddress(DLLProcID, "clCreateContext"));
	CHECK_FUNC_VALIDITY(clCreateCommandQueue = (clCreateCommandQueue_func)GetProcAddress(DLLProcID, "clCreateCommandQueue"));
	CHECK_FUNC_VALIDITY(clCreateProgramWithSource = (clCreateProgramWithSource_func)GetProcAddress(DLLProcID, "clCreateProgramWithSource"));
	CHECK_FUNC_VALIDITY(clBuildProgram = (clBuildProgram_func)GetProcAddress(DLLProcID, "clBuildProgram"));
	CHECK_FUNC_VALIDITY(clGetProgramBuildInfo = (clGetProgramBuildInfo_func)GetProcAddress(DLLProcID, "clGetProgramBuildInfo"));
	CHECK_FUNC_VALIDITY(clCreateKernel = (clCreateKernel_func)GetProcAddress(DLLProcID, "clCreateKernel"));
	CHECK_FUNC_VALIDITY(clCreateImage2D = (clCreateImage2D_func)GetProcAddress(DLLProcID, "clCreateImage2D"));
	CHECK_FUNC_VALIDITY(clSetKernelArg = (clSetKernelArg_func)GetProcAddress(DLLProcID, "clSetKernelArg"));
	CHECK_FUNC_VALIDITY(clGetKernelWorkGroupInfo = (clGetKernelWorkGroupInfo_func)GetProcAddress(DLLProcID, "clGetKernelWorkGroupInfo"));
	CHECK_FUNC_VALIDITY(clEnqueueNDRangeKernel = (clEnqueueNDRangeKernel_func)GetProcAddress(DLLProcID, "clEnqueueNDRangeKernel"));
	CHECK_FUNC_VALIDITY(clFinish = (clFinish_func)GetProcAddress(DLLProcID, "clFinish"));
	CHECK_FUNC_VALIDITY(clEnqueueWriteBuffer = (clEnqueueWriteBuffer_func)GetProcAddress(DLLProcID, "clEnqueueWriteBuffer"));
	CHECK_FUNC_VALIDITY(clEnqueueWriteImage = (clEnqueueWriteImage_func)GetProcAddress(DLLProcID, "clEnqueueWriteImage"));
	CHECK_FUNC_VALIDITY(clEnqueueReadImage = (clEnqueueReadImage_func)GetProcAddress(DLLProcID, "clEnqueueReadImage"));
}
