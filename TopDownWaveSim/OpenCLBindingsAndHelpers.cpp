#include "OpenCLBindingsAndHelpers.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define CHECK_FUNC_VALIDITY(func) if (!(func)) { return false; }																							// Simple helper define that reports error (returns false) if one of the functions doesn't bind correctly.

bool initOpenCLBindings() {
	HINSTANCE DLLProcID = LoadLibraryA("OpenCL.dll");																										// Load the OpenCL DLL.
	if (!DLLProcID) { return false; }																														// If it doesn't load correctly, fail.

	CHECK_FUNC_VALIDITY(clGetPlatformIDs = (clGetPlatformIDs_func)GetProcAddress(DLLProcID, "clGetPlatformIDs"));											// Go through all of the various functions and bind them (get function pointers to them and store those pointers in variables).
	CHECK_FUNC_VALIDITY(clGetPlatformInfo = (clGetPlatformInfo_func)GetProcAddress(DLLProcID, "clGetPlatformInfo"));										// If any one of these binds fails, everything stops and the whole functions fails.
	CHECK_FUNC_VALIDITY(clGetDeviceIDs = (clGetDeviceIDs_func)GetProcAddress(DLLProcID, "clGetDeviceIDs"));
	CHECK_FUNC_VALIDITY(clGetDeviceInfo = (clGetDeviceInfo_func)GetProcAddress(DLLProcID, "clGetDeviceInfo"));
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

cl_int initOpenCLVarsForBestDevice(const char* targetPlatformVersion, cl_platform_id& bestPlatform, cl_device_id& bestDevice, cl_context& context, cl_command_queue& commandQueue) {
	// Find the best device on the system.

	cl_uint platformCount;																																	// Get the amount of platforms that are available on the system.
	cl_int err = clGetPlatformIDs(0, nullptr, &platformCount);
	if (err != CL_SUCCESS) { return err; }
	if (!platformCount) { return CL_EXT_NO_PLATFORMS_FOUND; }

	cl_platform_id* platforms = new cl_platform_id[platformCount];																							// Get the actual array of platforms after we know how big it is supposed to be.
	err = clGetPlatformIDs(platformCount, platforms, nullptr);
	if (err != CL_SUCCESS) { return err; }

	bool bestPlatformInvalid;																																// Simple algorithm to go through each platform that matches the target version and select the best device out of all possibilities across all platforms.
	size_t bestDeviceMaxWorkGroupSize = 0;
	cl_device_id cachedBestDevice;																															// We cache the best device in this local variable to avoid unnecessarily dereferencing the bestDevice pointer.
	for (int i = 0; i < platformCount; i++) {
		cl_platform_id currentPlatform = platforms[i];																										// This is probably done automatically by the compiler, but I like having it here. Makes accessing the platform easier because you don't have to use an add instruction.

		size_t versionStringSize;																															// Get size of version string.
		err = clGetPlatformInfo(currentPlatform, CL_PLATFORM_VERSION, 0, nullptr, &versionStringSize);
		if (err != CL_SUCCESS) { return err; }

		char* buffer = new char[versionStringSize];																											// Get actual version string.
		err = clGetPlatformInfo(currentPlatform, CL_PLATFORM_VERSION, versionStringSize, buffer, nullptr);
		if (err != CL_SUCCESS) { return err; }

		if (!strcmp(buffer, targetPlatformVersion)) {																										// If the platform version matches the target version, continue.
			delete[] buffer;																																// Delete version string buffer as to not waste space.
			bestPlatformInvalid = true;																														// Invalidate bestPlatform so that, if a new best device appears, bestPlatform will be updated as well.

			cl_uint deviceCount;																															// Get the amount of devices on the current platform.
			err = clGetDeviceIDs(currentPlatform, CL_DEVICE_TYPE_GPU, 0, nullptr, &deviceCount);
			if (err != CL_SUCCESS) { return err; }
			if (!deviceCount) { return CL_EXT_NO_DEVICES_FOUND_ON_PLATFORM; }

			cl_device_id* devices = new cl_device_id[deviceCount];																							// Get the actual device IDs of the devices on the current platform.
			err = clGetDeviceIDs(currentPlatform, CL_DEVICE_TYPE_GPU, deviceCount, devices, nullptr);
			if (err != CL_SUCCESS) { return err; }

			for (int j = 0; j < deviceCount; j++) {																											// Go through all the devices on the current platform and see if you can find one that tops the already existing best.
				cl_device_id currentDevice = devices[i];

				size_t deviceMaxWorkGroupSize;																												// Get the theoretical maximum work group size of the current device. This value is how we measure which device has the most computational power, thereby qualifying as the best.
				err = clGetDeviceInfo(currentDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &deviceMaxWorkGroupSize, nullptr);
				if (err != CL_SUCCESS) { return err; }

				if (deviceMaxWorkGroupSize > bestDeviceMaxWorkGroupSize) {																					// If current device is better than best device, set best device to current device.
					bestDeviceMaxWorkGroupSize = deviceMaxWorkGroupSize;
					cachedBestDevice = currentDevice;
					if (bestPlatformInvalid) { bestPlatform = currentPlatform; bestPlatformInvalid = false; }												// Only update the platform if the new best device is on a different platform than the previous best device. This saves us from dereferencing pointers frequently for high device counts.
				}
			}
			continue;
		}
		delete[] buffer;																																	// Still delete the buffer even if platform doesn't match the target version.
	}
	if (!bestDeviceMaxWorkGroupSize) { return CL_EXT_NO_DEVICES_FOUND; }																					// If no devices were found, stop executing and return an error.
	bestDevice = cachedBestDevice;																															// Update actual bestDevice using the cachedBestDevice variable.

	// Establish other needed vars using the best device on the system.

	context = clCreateContext(nullptr, 1, &bestDevice, nullptr, nullptr, &err);																				// Create context using the best device.
	if (err != CL_SUCCESS) { return err; }

	commandQueue = clCreateCommandQueue(context, cachedBestDevice, 0, &err);																				// Create command queue using the newly created context and the best device. Make use of cachedBestDevice, which is still equal to dereferenced bestDevice pointer.
	if (err != CL_SUCCESS) { return err; }

	return CL_SUCCESS;																																		// If no error occurred up until this point, return CL_SUCCESS.
}


// TODO: Update the comments to reflect that we're doing this with references now instead of with pointers.
// Reason: The compiler might better be able to optimize if we use references. If the compiler inlines the function, the dereferencing problem completely goes away, but in case the compiler doesn't inline and uses pointers behind the scenes, we still use caching to make it efficient.
