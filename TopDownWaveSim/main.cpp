#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>																												// Extentions to the main Windows.h header. I only have this here for the GET_X_LPARAM and GET_Y_LPARAM macros in the message handler.

#include "OpenCLBindingsAndHelpers.h"

#include <iostream>
#include <fstream>

#include <thread>

#include "debugOutput.h"																											// Exposes debug output (using OutputDebugStringA) in an easy-to-use, std::cout-like fashion.




// TODO: Make sure you have enough delete[]'s and you dispose everything properly and stuff.
// TODO: Make use of GPU asynchronous processing and do things while your kernel is running.
// The same can maybe be made use of for reading and writing to/from buffer.

// TODO: Include anti-aliasing somehow in the kernel source code in order to try to minimize artifacts. Make a seperate branch for that.


// graphicsLoop related things.
bool isAlive = true;
void graphicsLoop(HWND hWnd);

// Global variables for use with OpenCL.
cl_platform_id computePlatform;
cl_device_id computeDevice;
cl_context computeContext;
cl_command_queue computeCommandQueue;

// Global variables to do with the wave kernel specifically.
cl_program computeWaveProgram;
cl_kernel computeWaveKernel;
size_t computeWaveKernelWorkGroupSize;

// Global variables to do with the colorizer kernel specifically.
cl_program computeColorizerProgram;
cl_kernel computeColorizerKernel;
size_t computeColorizerKernelWorkGroupSize;

// Global variable to hold the state of the image swapper algorithm.
bool swapState;

// OpenCL compute image objects.
cl_mem lastFieldValues_computeImage;
cl_mem lastFieldVels_computeImage;
cl_mem fieldValues_computeImage;
cl_mem fieldVels_computeImage;

cl_mem frameOutput_computeImage;

char* readFromSourceFile(const char* sourceFile) {
	std::ifstream kernelSourceFile(sourceFile, std::ios::beg);																										// Open the source file.
	if (!kernelSourceFile.is_open()) {
		debuglogger::out << debuglogger::error << "couldn't open kernel source file" << debuglogger::endl;
		return nullptr;
	}
#pragma push_macro(max)																																				// Count the characters inside the source file, construct large enough buffer, read from file into buffer.
#undef max																																							// The reason for this push, pop and undef stuff is because max is a macro defined in one of the windows headers and it interferes with our code.
	kernelSourceFile.ignore(std::numeric_limits<std::streamsize>::max());																							// We shortly undefine it and then pop it's original definition back into the empty slot when we're done with our code.
#pragma pop_macro(max)
	std::streamsize kernelSourceSize = kernelSourceFile.gcount();
	char* kernelSource = new char[kernelSourceSize + 1];
	kernelSourceFile.seekg(0, std::ios::beg);
	kernelSourceFile.read(kernelSource, kernelSourceSize);
	kernelSourceFile.close();
	kernelSource[kernelSourceSize] = '\0';
	return kernelSource;																																			// This is theoretically dangerous because the calling code has to take care of deleting kernelSource. It's ok for this project.
}

bool setupComputeKernel(const char* sourceFile, const char* kernelName, cl_program& computeProgram, cl_kernel& computeKernel, size_t& computeKernelWorkGroupSize) {
	cl_program cachedComputeProgram;
	cl_kernel cachedComputeKernel;
	size_t cachedKernelWorkGroupSize;

	char* computeKernelSource = readFromSourceFile(sourceFile);																									// Read source code from file.
	if (!computeKernelSource) {
		debuglogger::out << debuglogger::error << "couldn't read compute program source code from file" << debuglogger::endl;
		return true;
	}
	cl_int err;
	cachedComputeProgram = clCreateProgramWithSource(computeContext, 1, (const char**)&computeKernelSource, nullptr, &err);														// Create compute program with the source code.
	delete[] computeKernelSource;																																			// Delete kernelSource because readFromSourceFile returned a raw, unsafe pointer that we need to take care of.
	if (!cachedComputeProgram) {
		debuglogger::out << debuglogger::error << "failed to create compute program with source code" << debuglogger::endl;
		return true;
	}

	err = clBuildProgram(cachedComputeProgram, 0, nullptr, nullptr, nullptr, nullptr);																					// Build compute program.
	if (err != CL_SUCCESS) {																																		// If build fails, alert user and display build log.
		debuglogger::out << debuglogger::error << "failed to build compute program" << debuglogger::endl;
		size_t buildLogSize;
		err = clGetProgramBuildInfo(cachedComputeProgram, computeDevice, CL_PROGRAM_BUILD_LOG, 0, nullptr, &buildLogSize);												// Get size of build log.
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "could not get compute program build log size" << debuglogger::endl;
			return true;
		}
		char* buffer = new char[buildLogSize];
		err = clGetProgramBuildInfo(cachedComputeProgram, computeDevice, CL_PROGRAM_BUILD_LOG, buildLogSize, buffer, nullptr);											// Get actual build log.
		if (err != CL_SUCCESS) {
			delete[] buffer;
			debuglogger::out << debuglogger::error << "could not retrieve compute program build log" << debuglogger::endl;
			return true;
		}
		debuglogger::out << "BUILD LOG:" << debuglogger::endl << buffer << debuglogger::endl;
		delete[] buffer;
		return true;
	}

	cachedComputeKernel = clCreateKernel(cachedComputeProgram, kernelName, &err);																							// Create compute kernel using a specific kernel function in the compute program.
	if (!cachedComputeKernel) {
		debuglogger::out << debuglogger::error << "failed to create compute kernel" << debuglogger::endl;
		return true;
	}

	err = clGetKernelWorkGroupInfo(cachedComputeKernel, computeDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &cachedKernelWorkGroupSize, nullptr);						// Get kernel work group size.
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't get compute kernel work group size" << debuglogger::endl;
		return true;
	}

	size_t computeKernelPreferredWorkGroupSizeMultiple;
	err = clGetKernelWorkGroupInfo(cachedComputeKernel, computeDevice, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &computeKernelPreferredWorkGroupSizeMultiple, nullptr);					// Get kernel preferred work group size multiple.
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't get preferred kernel work group size multiple for compute kernel" << debuglogger::endl;
		return true;
	}

	if (cachedKernelWorkGroupSize > computeKernelPreferredWorkGroupSizeMultiple) { cachedKernelWorkGroupSize -= cachedKernelWorkGroupSize % computeKernelPreferredWorkGroupSizeMultiple; }										// Compute the optimal work group size for the kernel based on the raw kernel optimum and the kernel preferred work group size multiple.

	computeProgram = cachedComputeProgram;
	computeKernel = cachedComputeKernel;
	computeKernelWorkGroupSize = cachedKernelWorkGroupSize;

	return false;
}

// Sets up some of the OpenCL vars asynchronously while the main thread handles other things.
bool OpenCLSetupFailure;
void OpenCLSetup() {
	if (!initOpenCLBindings()) {																																	// Dynamically link to OpenCL DLL and set up function bindings.
		debuglogger::out << debuglogger::error << "failed while initializing OpenCL bindings" << debuglogger::endl;
		OpenCLSetupFailure = true;
		return;
	}

	cl_int err = initOpenCLVarsForBestDevice("OpenCL 2.1 ", computePlatform, computeDevice, computeContext, computeCommandQueue);								// Initialize some necessary OpenCL vars. The space at the end of the version string is necessary.
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "failed while initializing OpenCL vars for best device" << debuglogger::endl;
		OpenCLSetupFailure = true;
		return;
	}

	if (setupComputeKernel("wavePropagator.cl", "wavePropagator", computeWaveProgram, computeWaveKernel, computeWaveKernelWorkGroupSize)) {
		debuglogger::out << debuglogger::error << "failed to set up wave propagator kernel" << debuglogger::endl;
		OpenCLSetupFailure = true;
		return;
	}

	if (setupComputeKernel("colorizer.cl", "colorizer", computeColorizerProgram, computeColorizerKernel, computeColorizerKernelWorkGroupSize)) {
		debuglogger::out << debuglogger::error << "failed to set up colorizer kernel" << debuglogger::endl;
		OpenCLSetupFailure = true;
		return;
	}
}

bool setDefaultKernelImageArguments() {
	cl_int err = clSetKernelArg(computeWaveKernel, 0, sizeof(cl_mem), &lastFieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set lastFieldValues argument in wave kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 1, sizeof(cl_mem), &lastFieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set lastFieldVels argument in wave kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 2, sizeof(cl_mem), &fieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set fieldValues argument in wave kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 3, sizeof(cl_mem), &fieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set fieldVels argument in wave kernel" << debuglogger::endl;
		return true;
	}

	err = clSetKernelArg(computeColorizerKernel, 0, sizeof(cl_mem), &fieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set fieldValues argument in colorizer kernel" << debuglogger::endl;
		return true;
	}

	swapState = false;

	return false;
}

// Global variables to keep track of window size.
cl_uint windowWidth;
cl_uint windowHeight;

// Global const which defines the image format that is to be used for compute images.
const cl_image_format computeImageFormat = { CL_R, CL_FLOAT };
const cl_image_format computeFrameFormat = { CL_RGBA, CL_UNSIGNED_INT8 };

bool createComputeImages() {
	size_t windowArea = windowWidth * windowHeight;																																					// Create an array of floats and zero them out. This memory will be copied to compute device so compute buffers are zeroed out.
	float* startValues = new float[windowArea];
	ZeroMemory(startValues, windowArea * sizeof(float));

	cl_int err;

	lastFieldValues_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &computeImageFormat, windowWidth, windowHeight,
		0, startValues, &err);
	if (!lastFieldValues_computeImage) {
		debuglogger::out << debuglogger::error << "failed to create lastFieldValues compute image" << debuglogger::endl;
		return true;
	}
	lastFieldVels_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &computeImageFormat, windowWidth, windowHeight,
		0, startValues, &err);
	if (!lastFieldVels_computeImage) {
		debuglogger::out << debuglogger::error << "failed to create lastFieldVels compute image" << debuglogger::endl;
		return true;
	}

	delete[] startValues;

	fieldValues_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE, &computeImageFormat, windowWidth, windowHeight,
		0, nullptr, &err);
	if (!fieldValues_computeImage) {
		debuglogger::out << debuglogger::error << "failed to create fieldValues compute image" << debuglogger::endl;
		return true;
	}
	fieldVels_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE, &computeImageFormat, windowWidth, windowHeight,
		0, nullptr, &err);
	if (!fieldVels_computeImage) {
		debuglogger::out << debuglogger::error << "failed to create fieldVels compute image" << debuglogger::endl;
		return true;
	}



	frameOutput_computeImage = clCreateImage2D(computeContext, CL_MEM_WRITE_ONLY, &computeFrameFormat, windowWidth, windowHeight, 0, nullptr, &err);
	if (!frameOutput_computeImage) {
		debuglogger::out << debuglogger::error << "failed to create frameOutput compute image" << debuglogger::endl;
		return true;
	}


	if (setDefaultKernelImageArguments()) { return true; }

	
	err = clSetKernelArg(computeColorizerKernel, 1, sizeof(cl_mem), &frameOutput_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set frameOutput argument in colorizer kernel" << debuglogger::endl;
		return true;
	}

	return false;
}

bool setKernelWindowSizeArguments() {
	cl_int err = clSetKernelArg(computeWaveKernel, 4, sizeof(cl_uint), &windowWidth);
	if (err != CL_SUCCESS) { return true; }
	err = clSetKernelArg(computeWaveKernel, 5, sizeof(cl_uint), &windowHeight);
	if (err != CL_SUCCESS) { return true; }

	err = clSetKernelArg(computeColorizerKernel, 2, sizeof(cl_uint), &windowWidth);
	if (err != CL_SUCCESS) { return true; }
	err = clSetKernelArg(computeColorizerKernel, 3, sizeof(cl_uint), &windowHeight);
	if (err != CL_SUCCESS) { return true; }

	return false;
}

// Global variables to keep track of mouse position.
bool mouseClicked = false;
int mouseX;
int mouseY;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		mouseX = GET_X_LPARAM(lParam);
		mouseY = GET_Y_LPARAM(lParam);
		mouseClicked = true;
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow) {
	debuglogger::out << "started program from wWinMain entry point (UNICODE)" << debuglogger::endl;
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	debuglogger::out << "started program from WinMain entry point (ANSI)" << debuglogger::endl;
#endif

	debuglogger::out << "starting OpenCLSetupThread..." << debuglogger::endl;
	std::thread OpenCLSetupThread(OpenCLSetup);																															// Set up every OpenCL var that is possible to set up before creating a window. Do this asynchronously while creating a window.

	TCHAR CLASS_NAME[] = TEXT("WAVE_SIM_WINDOW");

	WNDCLASS windowClass = { };																																			// Create WNDCLASS struct for later window creation.
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = CLASS_NAME;

	debuglogger::out << "registering window class..." << debuglogger::endl;
	RegisterClass(&windowClass);				// TODO: Figure out if you can do error handling on this one.

	debuglogger::out << "creating window..." << debuglogger::endl;
	HWND hWnd = CreateWindowEx(0, CLASS_NAME, TEXT("Wave Simulation"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
	if (hWnd == nullptr) {																																				// Check if creation of window was successful.
		debuglogger::out << debuglogger::error << "encountered error while creating window, terminating..." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "showing window..." << debuglogger::endl;
	ShowWindow(hWnd, nCmdShow);

	debuglogger::out << "joining with the OpenCL setup thread..." << debuglogger::endl;
	OpenCLSetupThread.join();																																			// Join with the OpenCLSetupThread to make sure everything is set up before moving on to the next step.
	if (OpenCLSetupFailure) {																																			// Check if OpenCLSetupThread reported an error while running and exited prematurely.
		debuglogger::out << debuglogger::error << "OpenCL setup thread reported an error, terminating..." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "getting client rect of window..." << debuglogger::endl;
	RECT clientRect;
	if (!GetClientRect(hWnd, &clientRect)) {
		debuglogger::out << debuglogger::error << "couldn't get client rect of window, terminating..." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	windowWidth = clientRect.right;																																		// Don't worry about subtracting left and top here, per documentation, they are always 0 and right and bottom are always width and height.
	windowHeight = clientRect.bottom;

	debuglogger::out << "creating compute images..." << debuglogger::endl;
	if (createComputeImages()) {
		debuglogger::out << debuglogger::error << "couldn't create compute images, terminating..." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "setting the windowWidth and windowHeight compute kernel arguments..." << debuglogger::endl;
	if (setKernelWindowSizeArguments()) {
		debuglogger::out << debuglogger::error << "failed to set window size arguments in kernel, terminating..." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "starting graphics thread..." << debuglogger::endl;
	std::thread graphicsThread(graphicsLoop, hWnd);

	debuglogger::out << "running message loop..." << debuglogger::endl;
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	debuglogger::out << "joining with graphics thread and exiting..." << debuglogger::endl;
	isAlive = false;
	graphicsThread.join();
}

const size_t region[3] = { 1, 1, 1 };
const float mouseGenerationValue = 10000;
bool handleMouseClick() {
	if (swapState) {
		size_t origin[3] = { mouseX, mouseY, 0 };
		cl_int err = clEnqueueWriteImage(computeCommandQueue, fieldValues_computeImage, true, origin, region, 0, 0, &mouseGenerationValue, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "couldn't write mouse update to compute device memory" << debuglogger::endl;
			return true;
		}
		return false;
	}
	size_t origin[3] = { mouseX, mouseY, 0};
	cl_int err = clEnqueueWriteImage(computeCommandQueue, lastFieldValues_computeImage, true, origin, region, 0, 0, &mouseGenerationValue, 0, nullptr, nullptr);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't write mouse update to compute device memory" << debuglogger::endl;
		return true;
	}
	return false;
}

bool swapKernelImageArguments() {
	if (swapState) { return setDefaultKernelImageArguments(); }

	cl_int err = clSetKernelArg(computeWaveKernel, 0, sizeof(cl_mem), &fieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set fieldValues argument in colorizer kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 1, sizeof(cl_mem), &fieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set fieldVels argument in colorizer kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 2, sizeof(cl_mem), &lastFieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set lastFieldValues argument in colorizer kernel" << debuglogger::endl;
		return true;
	}
	err = clSetKernelArg(computeWaveKernel, 3, sizeof(cl_mem), &lastFieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set lastFieldVels argument in wave kernel" << debuglogger::endl;
		return true;
	}

	
	err = clSetKernelArg(computeColorizerKernel, 0, sizeof(cl_mem), &lastFieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't set lastFieldValues argument in colorizer kernel" << debuglogger::endl;
	}


	swapState = true;

	return false;
}

void graphicsLoop(HWND hWnd) {
	HDC finalG = GetDC(hWnd);																									// Set up everything for drawing frames instead of shapes to the screen.
	HDC g = CreateCompatibleDC(finalG);
	HBITMAP bmp = CreateCompatibleBitmap(finalG, windowWidth, windowHeight);											// TODO: Make sure this clump is the most efficient way to do this.
	SelectObject(g, bmp);

#define FRAME_SIZE (windowWidth * windowHeight * 4)																								// Do the allocation and setup for the frame data buffer, which we copy into the bitmap after every frame.
	char* frame = new char[FRAME_SIZE];
	for (int i = 0; i < FRAME_SIZE; i += 4) {
		frame[i] =  0;
		frame[i + 3] = 255;
	}

	cl_int err;

	while (isAlive) {																											// Start the actual graphics loop.
		if (mouseClicked) {
			if (handleMouseClick()) {
				debuglogger::out << debuglogger::error << "mouse click handler reported an error" << debuglogger::endl;
			}
			mouseClicked = false;
		}

		size_t globalSize[2] = { windowWidth + (computeWaveKernelWorkGroupSize - (windowWidth % computeWaveKernelWorkGroupSize)), windowHeight };// TODO: In order to add antialiasing, you need multiply the actual fields by 4 and have the frameoutput match the window size.
																																				// TODO: How are you going to generate waves if every pixel is actually 4? 4 pixels at a time? will that cause interference?
		size_t localSize[2] = { computeWaveKernelWorkGroupSize, 1 };
		err = clEnqueueNDRangeKernel(computeCommandQueue, computeWaveKernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "failed to enqueue kernel" << debuglogger::endl;
			// TODO: Implement some way of exiting from inside the graphics thread loop.
		}

		err = clEnqueueNDRangeKernel(computeCommandQueue, computeColorizerKernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "failed to enqueue colorizer kernel" << debuglogger::endl;
		}

		err = clFinish(computeCommandQueue);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "failed to wait for compute kernel to finish" << debuglogger::endl;
			// TODO: Find a way to exit program from here.
		}

		if (swapKernelImageArguments()) {
			debuglogger::out << debuglogger::error << "failed to swap kernel image arguments" << debuglogger::endl;
		}

		size_t origin[3] = { 0, 0, 0 };
		size_t region[3] = { windowWidth, windowHeight, 1 };
		err = clEnqueueReadImage(computeCommandQueue, frameOutput_computeImage, true, (const size_t*)&origin, (const size_t*)&region, 0, 0, frame, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "couldn't read frame output from compute device" << debuglogger::endl;
		}

		SetBitmapBits(bmp, FRAME_SIZE, frame);																				// Copy the current frame data into the bitmap, so that it is displayed on the window.
		BitBlt(finalG, 0, 0, windowWidth, windowHeight, g, 0, 0, SRCCOPY);
	}
}
