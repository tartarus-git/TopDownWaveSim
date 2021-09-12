#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>																												// Extentions to the main Windows.h header. I only have this in here for the GET_X_LPARAM and GET_Y_LPARAM macros in the message handler.

#include "selectOpenCLBindings.h"

#include <iostream>
#include <fstream>
#include <thread>

#include "debugOutput.h"

#define FIELD_PULL 0.01f																											// The strength with which points on the field are forced to return to their origin.

#define WINDOW_STARTING_WIDTH 300
#define WINDOW_STARTING_HEIGHT 300

int windowWidth = WINDOW_STARTING_WIDTH;
int windowHeight = WINDOW_STARTING_HEIGHT;



// TODO: Make sure you have enough delete[]'s and you dispose everything properly and stuff.



void graphicsLoop(HWND hWnd);
bool isAlive = true;

int mouseX = -1;
int mouseY = -1;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		mouseX = GET_X_LPARAM(lParam);
		mouseY = GET_Y_LPARAM(lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

float* lastFieldVels;
float* lastFieldValues;
float* fieldVels;
float* fieldValues;

size_t workGroupSize;
cl_command_queue computeCommandQueue;
cl_kernel computeKernel;

cl_mem lastFieldValues_computeImage;
cl_mem lastFieldVels_computeImage;
cl_mem fieldValues_computeImage;
cl_mem fieldVels_computeImage;

#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow) {
	std::cout << "Started program from wWinMain entry point (UNICODE)." << std::endl;
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	std::cout << "Started program from WinMain entry point (ANSI)." << std::endl;
#endif

	if (!initOpenCLBindings()) {
		debuglogger::out << debuglogger::error << "couldn't initialize OpenCL bindings" << debuglogger::endl;
		return EXIT_FAILURE;
	}

	#define FIELD_SIZE (WINDOW_STARTING_WIDTH * WINDOW_STARTING_HEIGHT)
	fieldValues = new float[FIELD_SIZE];
	fieldVels = new float[FIELD_SIZE];

	lastFieldValues = new float[FIELD_SIZE];
	ZeroMemory(lastFieldValues, FIELD_SIZE * sizeof(float));
	lastFieldVels = new float[FIELD_SIZE];
	ZeroMemory(lastFieldVels, FIELD_SIZE * sizeof(float));

	cl_int err;

	cl_uint platformCount;
	err = clGetPlatformIDs(0, NULL, &platformCount);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "OpenCL error." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	if (platformCount == 0) {
		debuglogger::out << debuglogger::error << "Couldn't find any OpenCL platforms." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "Available OpenCL platforms: " << platformCount << debuglogger::endl;

	cl_platform_id computePlatformID;
	err = clGetPlatformIDs(1, &computePlatformID, NULL);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't get the platform ID." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	debuglogger::out << "Using one of the possible platforms." << debuglogger::endl;

	size_t versionTextSize;
	err = clGetPlatformInfo(computePlatformID, CL_PLATFORM_VERSION, 0, NULL, &versionTextSize);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't get platform version text size." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	char* buffer = new char[versionTextSize];
	err = clGetPlatformInfo(computePlatformID, CL_PLATFORM_VERSION, versionTextSize, buffer, NULL);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Failed to get platform version text." << debuglogger::endl;
		delete[] buffer;
		return EXIT_FAILURE;
	}
	debuglogger::out << "Platform version: " << buffer << debuglogger::endl;
	delete[] buffer;

	cl_device_id computeDeviceID;
	err = clGetDeviceIDs(computePlatformID, CL_DEVICE_TYPE_GPU, 1, &computeDeviceID, NULL);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Failed to retrieve device ID." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	cl_context computeContext = clCreateContext(NULL, 1, &computeDeviceID, NULL, NULL, &err);
	if (!computeContext) {
		debuglogger::out << debuglogger::error << "Failed to create a compute context." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	computeCommandQueue = clCreateCommandQueue(computeContext, computeDeviceID, 0, &err);
	if (!computeCommandQueue) {
		debuglogger::out << debuglogger::error << "Failed to create a command queue for the compute device." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	std::ifstream kernelSourceFile("wavePropagator.cl", std::ios::beg);
	if (!kernelSourceFile.is_open()) {
		debuglogger::out << debuglogger::error << "Couldn't open kernel source file." << debuglogger::endl;
		return EXIT_FAILURE;
	}

#pragma push_macro(max)
#undef max
	kernelSourceFile.ignore(std::numeric_limits<std::streamsize>::max());
#pragma pop_macro(max)
	std::streamsize kernelSourceSize = kernelSourceFile.gcount();
	char* kernelSource = new char[kernelSourceSize + 1];
	kernelSourceFile.seekg(0, std::ios::beg);
	kernelSourceFile.read(kernelSource, kernelSourceSize);
	kernelSourceFile.close();
	kernelSource[kernelSourceSize] = '\0';

	cl_program computeProgram = clCreateProgramWithSource(computeContext, 1, (const char**)&kernelSource, NULL, &err);
	delete[] kernelSource;
	if (!computeProgram) {
		debuglogger::out << debuglogger::error << "Failed to create compute program with the kernel source." << debuglogger::endl;
		return EXIT_FAILURE;
		// TODO: Make sure you dispose of everything even when you exit the program in a hurry.
	}

	err = clBuildProgram(computeProgram, 0, NULL, NULL, NULL, NULL);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Failed to build compute program." << debuglogger::endl;
		size_t buildLogSize;
		err = clGetProgramBuildInfo(computeProgram, computeDeviceID, CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "Could not get program build log size." << debuglogger::endl;
			return EXIT_FAILURE;
		}
		char* buffer = new char[buildLogSize];
		err = clGetProgramBuildInfo(computeProgram, computeDeviceID, CL_PROGRAM_BUILD_LOG, buildLogSize, buffer, NULL);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "Could not retrieve program build log." << debuglogger::endl;
			return EXIT_FAILURE;
		}
		debuglogger::out << "BUILD LOG:" << debuglogger::endl << buffer << debuglogger::endl;
		delete[] buffer;
		return EXIT_FAILURE;
	}

	computeKernel = clCreateKernel(computeProgram, "wavePropagator", &err);
	if (!computeKernel) {
		debuglogger::out << debuglogger::error << "Failed to create kernel." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	cl_image_format computeImageFormat;				// TODO: See if you can use an initializer list for this instead and see if it is as efficient.
	computeImageFormat.image_channel_order = CL_R;
	computeImageFormat.image_channel_data_type = CL_FLOAT;

	lastFieldVels_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &computeImageFormat, windowWidth, windowHeight,
		0, lastFieldVels, &err);
	if (!lastFieldVels_computeImage) {
		debuglogger::out << debuglogger::error << "Failed to create compute image for lastFieldVels." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	fieldVels_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE, &computeImageFormat, windowWidth, windowHeight,
		0, NULL, &err);
	if (!fieldVels_computeImage) {
		debuglogger::out << debuglogger::error << "Failed to create compute image for fieldVels." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	lastFieldValues_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &computeImageFormat, windowWidth, windowHeight,
		0, lastFieldValues, &err);
	if (!lastFieldValues_computeImage) {
		debuglogger::out << debuglogger::error << "Failed to create compute image for lastFieldValues." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	fieldValues_computeImage = clCreateImage2D(computeContext, CL_MEM_READ_WRITE, &computeImageFormat, windowWidth, windowHeight,
		0, NULL, &err);
	if (!fieldValues_computeImage) {
		debuglogger::out << debuglogger::error << "Failed to create compute image for fieldValues." << debuglogger::endl;
		return EXIT_FAILURE;
	}

	err = clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &lastFieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't set the lastFieldVels argument in kernel." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	err = clSetKernelArg(computeKernel, 1, sizeof(cl_mem), &fieldVels_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't set the fieldVels argument in kernel." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	err = clSetKernelArg(computeKernel, 2, sizeof(cl_mem), &lastFieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't set the lastFieldValues argument in kernel." << debuglogger::endl;
		return EXIT_FAILURE;
	}
	err = clSetKernelArg(computeKernel, 3, sizeof(cl_mem), &fieldValues_computeImage);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "Couldn't set the fieldValues argument in kernel." << debuglogger::endl;
		return EXIT_FAILURE;
	}


	err = clGetKernelWorkGroupInfo(computeKernel, computeDeviceID, CL_KERNEL_WORK_GROUP_SIZE, sizeof(workGroupSize), &workGroupSize, nullptr);
	if (err != CL_SUCCESS) {
		debuglogger::out << debuglogger::error << "couldn't get optimal kernel work group size" << debuglogger::endl;
		return EXIT_FAILURE;
	}

	const TCHAR CLASS_NAME[] = TEXT("WAVE_SIM_WINDOW");

	WNDCLASS windowClass = { };																									// Create WNDCLASS struct for later window creation.
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = CLASS_NAME;

	std::cout << "Registering the window class..." << std::endl;
	RegisterClass(&windowClass);

	std::cout << "Creating the window..." << std::endl;
	HWND hWnd = CreateWindowEx(0, CLASS_NAME, TEXT("Wave Simulation"), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_STARTING_WIDTH, WINDOW_STARTING_HEIGHT, NULL, NULL, hInstance, NULL);

	if (hWnd == NULL) {																											// Check if creation of window was successful.
		std::cout << "Error encountered while creating the window, terminating..." << std::endl;								// TODO: Figure out if maye std::cerr would be better for this.
		return 0;
	}

	std::cout << "Showing the window..." << std::endl;
	ShowWindow(hWnd, nCmdShow);

	std::cout << "Starting the graphics thread..." << std::endl;
	std::thread graphicsThread(graphicsLoop, hWnd);

	std::cout << "Running the message loop..." << std::endl;
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	std::cout << "Joining with graphics thread and exiting..." << std::endl;
	isAlive = false;
	graphicsThread.join();
}



#define TO_FIELD_INDEX(x, y) (y * windowWidth + x)				// TODO: Instead of relying on this, use 2D arrays, they do exactly this but behind the scenes so that your code looks nicer. Of course, maybe don't if you think you can be more efficient with the multiplying using this way.
#define NODE_EQUALIZATION_STRENGTH 0.1f

int srcX;
int srcY;
float* srcVel;
float srcValue;
void equalize(int offsetX, int offsetY) {
	int absoluteX = srcX + offsetX;
	int absoluteY = srcY + offsetY;
	if (absoluteX < 0 || absoluteX >= windowWidth || absoluteY < 0 || absoluteY >= windowHeight) { return; }
	float dist = sqrt(offsetX * offsetX + offsetY * offsetY);															// TODO: Make sure the compiler optimizes this out and puts it into the accel calculations.
	float accel = (srcValue - fieldValues[TO_FIELD_INDEX(absoluteX, absoluteY)]) * NODE_EQUALIZATION_STRENGTH / dist;
	*srcVel -= accel;
}

const int kernelX[] { 1,  -1,  0,  0,  1, -1,  1, -1,  0, -2,  2,  0 };
const int kernelY[] { 0,   0, -1,  1, -1, -1,  1,  1, -2,  0,  0,  2 };
#define KERNEL_LENGTH (sizeof(kernelX) / sizeof(int))

void calculate(int x, int y) {
	srcX = x;
	srcY = y;
	int index = TO_FIELD_INDEX(x, y);
	float* value = fieldValues + index;
	srcValue = *value;

	srcVel = fieldVels + index;
	if (srcValue > 0) { *srcVel = lastFieldVels[index] - FIELD_PULL; }
	else if (srcValue < 0) { *srcVel = lastFieldVels[index] + FIELD_PULL; }
	else { *srcVel = lastFieldVels[index]; }				// TODO: Having to do this is stupid, we just need to initialize the memory from the get-go.
	for (int i = 0; i < KERNEL_LENGTH; i++) { equalize(kernelX[i], kernelY[i]); }

	

	//*value += *srcVel;
}

float clamp(float x, float upperBound) {
	if (x > upperBound) { return upperBound; }
	return x;
}

void graphicsLoop(HWND hWnd) {
	HDC finalG = GetDC(hWnd);																									// Set up everything for drawing frames instead of shapes to the screen.
	HDC g = CreateCompatibleDC(finalG);
	HBITMAP bmp = CreateCompatibleBitmap(finalG, WINDOW_STARTING_WIDTH, WINDOW_STARTING_HEIGHT);
	SelectObject(g, bmp);



	/*for (int i = 0; i < 900; i++) {
		fieldValues[i] = 100;
	}

	for (int i = FIELD_SIZE - 1; i >= FIELD_SIZE - 900; i--) {
		fieldValues[i] = 100;
	}*/

	fieldValues[windowWidth * 150 + 150 + 10] = 10000;
	fieldValues[windowWidth * 150 + 150 - 10] = 10000;


#define FRAME_SIZE (FIELD_SIZE * 4)																								// Do the allocation and setup for the frame data buffer, which we copy into the bitmap after every frame.
	char* frame = new char[FRAME_SIZE];
	for (int i = 0; i < FRAME_SIZE; i += 4) {
		frame[i] =  0;
		frame[i + 3] = 255;
	}

	cl_int err;

	bool state = true;

	while (isAlive) {																											// Start the actual graphics loop.

		if (mouseX != -1) {
			if (state) {
				float thing = 10000;
				size_t origin[3] = { mouseX, mouseY, 0 };
				size_t region[3] = { 1, 1, 1 };
				err = clEnqueueWriteImage(computeCommandQueue, lastFieldValues_computeImage, true, origin, region, 0, 0, &thing, 0, nullptr, nullptr);
				if (err != CL_SUCCESS) {
					debuglogger::out << debuglogger::error << "couldn't write mouse update to compute device memory" << debuglogger::endl;
					// TODO: Figure out a way to exit from here.
				}
			}
			else {
				float thing = 10000;
				size_t origin[3] = { mouseX, mouseY, 0};
				size_t region[3] = { 1, 1, 1 };
				err = clEnqueueWriteImage(computeCommandQueue, fieldValues_computeImage, true, origin, region, 0, 0, &thing, 0, nullptr, nullptr);
				if (err != CL_SUCCESS) {
					debuglogger::out << debuglogger::error << "couldn't write mouse update to compute device memory" << debuglogger::endl;
					// TODO: Figure out a way to exit from here.
				}
			}
			mouseX = -1;
		}

		size_t globalSize[2] = { 512, windowHeight };
		size_t localSize[2] = { workGroupSize, 1 };
		err = clEnqueueNDRangeKernel(computeCommandQueue, computeKernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "failed to enqueue kernel" << debuglogger::endl;
			// TODO: Implement some way of exiting from inside the graphics thread loop.
		}

		

		err = clFinish(computeCommandQueue);
		if (err != CL_SUCCESS) {
			debuglogger::out << debuglogger::error << "failed to wait for compute kernel to finish" << debuglogger::endl;
			// TODO: Find a way to exit program from here.
		}

		if (state) {
			err = clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &fieldVels_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 1, sizeof(cl_mem), &lastFieldVels_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 2, sizeof(cl_mem), &fieldValues_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 3, sizeof(cl_mem), &lastFieldValues_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
		}
		else {
			err = clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &lastFieldVels_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 1, sizeof(cl_mem), &fieldVels_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 2, sizeof(cl_mem), &lastFieldValues_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
			err = clSetKernelArg(computeKernel, 3, sizeof(cl_mem), &fieldValues_computeImage);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed while swapping GPU buffers" << debuglogger::endl;
				// TODO: Implement exit strat here.
			}
		}

		state = !state;


/*#define THING_STRENGTH 1
		for (int i = 0; i < FIELD_SIZE; i++) {
			if (fieldVels[i] < 0) {
				fieldVels[i] += THING_STRENGTH;
			}
			else if (fieldVels[i] > 0) {
				fieldVels[i] -= THING_STRENGTH;
			}
		}*/

		float* endFieldValues = new float[FIELD_SIZE];
		if (state) {
			size_t origin[3] = { 0, 0, 0 };
			size_t region[3] = { windowWidth, windowHeight, 1 };
			err = clEnqueueReadImage(computeCommandQueue, fieldValues_computeImage, true, origin, region, 0, 0, endFieldValues, 0, nullptr, nullptr);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed to read image data from compute device" << debuglogger::endl;
				// TODO: Figure out escape strat here.
			}
		}
		else {
			size_t origin[3] = { 0, 0, 0 };
			size_t region[3] = { windowWidth, windowHeight, 1 };
			err = clEnqueueReadImage(computeCommandQueue, lastFieldValues_computeImage, true, origin, region, 0, 0, endFieldValues, 0, nullptr, nullptr);
			if (err != CL_SUCCESS) {
				debuglogger::out << debuglogger::error << "failed to read image data from compute device" << debuglogger::endl;
				// TODO: Figure out escape strat here.
			}
		}

		for (int i = 0; i < FIELD_SIZE; i++) {
			int frameIndex = i * 4;
			if (endFieldValues[i] > 0) {
				frame[frameIndex + 1] = clamp(endFieldValues[i], 100) / 100 * 255;
				frame[frameIndex + 2] = 0;
			}
			else if (endFieldValues[i] == 0) {
				frame[frameIndex + 1] = 0;
				frame[frameIndex + 2] = 0;
			}
			else {
				frame[frameIndex + 1] = 0;
				frame[frameIndex + 2] = clamp((-endFieldValues[i]), 100) / 100 * 255;
			}
		}

		delete[] endFieldValues;

		SetBitmapBits(bmp, FRAME_SIZE, frame);																				// Copy the current frame data into the bitmap, so that it is displayed on the window.
		BitBlt(finalG, 0, 0, WINDOW_STARTING_WIDTH, WINDOW_STARTING_HEIGHT, g, 0, 0, SRCCOPY);
	}
}
