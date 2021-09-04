#include "selectOpenCLBindings.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

const HINSTANCE DLLProcID = LoadLibrary(TEXT("OpenCL.dll"));

// TODO: Make this whole loading code be able to handle errors and show them and whatnot.

const clGetPlatformIDs_func clGetPlatformIDs = (clGetPlatformIDs_func)GetProcAddress(DLLProcID, "clGetPlatformIDs");
const clGetDeviceIDs_func clGetDeviceIDs = (clGetDeviceIDs_func)GetProcAddress(DLLProcID, "clGetDeviceIDs");
const clCreateContext_func clCreateContext = (clCreateContext_func)GetProcAddress(DLLProcID, "clCreateContext");
const clCreateCommandQueue_func clCreateCommandQueue = (clCreateCommandQueue_func)GetProcAddress(DLLProcID, "clCreateCommandQueue");
const clCreateProgramWithSource_func clCreateProgramWithSource = (clCreateProgramWithSource_func)GetProcAddress(DLLProcID, "clCreateProgramWithSource");
const clBildProgram_func clBuildProgram = (clBuildProgram_func)GetProcAddress(DLLProcID, "clBuildProgram");
