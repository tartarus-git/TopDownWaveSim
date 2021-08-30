#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <thread>

#define FIELD_PULL 0.01f																											// The strength with which points on the field are forced to return to their origin.

#define WINDOW_STARTING_WIDTH 1000
#define WINDOW_STARTING_HEIGHT 1000

int windowWidth = WINDOW_STARTING_WIDTH;
int windowHeight = WINDOW_STARTING_HEIGHT;

void graphicsLoop(HWND hWnd);
bool isAlive = true;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow) {
	std::cout << "Started program from wWinMain entry point (UNICODE)." << std::endl;
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	std::cout << "Started program from WinMain entry point (ANSI)." << std::endl;
#endif

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


// TODO:
// This is what I think might be the solution. The wave looks strange when you put the STRENGTH define higher, maybe that is because the forces are stronger so they poke through the noise better and you can see the diamond shape, whereas with littler numbers you're almost fooled into thinking
// that it's circle. I think that maybe in nature, every node influences every other node, not just the neighboring nodes, but it does so with a dropoff in influence so that the whole thing is gradual. That might resolve your weird shapes and give you perfect circles.
// Start by trying with the surrounding 9 nodes, you don't have much processing power for more than that until you parallelize all of this for a gigantic speed boost. I assume the dropoff is caused by the hypotonus force vector getting more and more horizontal, which makes it harder
// for two nodes to adjust with eachother. That might be one way of thinking about it and simulating it, maybe it is different though, give it a think.

void calculate(float currentValue, float currentVel, float* nextCurrentValue, float* nextCurrentVel, float topValue, float leftValue, float bottomValue, float rightValue, float* top, float* left, float* bottom, float* right) {			// Calculates the actual wave propagation.

	if (currentValue == 0) {
		//*nextCurrentVel = currentVel;
	}
	else if (currentValue > 0) {
		//*nextCurrentVel = currentVel - FIELD_PULL;
		*nextCurrentVel -= FIELD_PULL;
	}
	else if (currentValue < 0) {
		//*nextCurrentVel = currentVel + FIELD_PULL;
		*nextCurrentVel += FIELD_PULL;
	}
	*nextCurrentValue = currentValue;

#define STRENGTH 0.1f

	if (top) {
			float dist = abs(currentValue - topValue);
	float thing = dist * STRENGTH;

	if (currentValue > topValue) {
		*nextCurrentVel -= thing;
		//*top += thing;										// TODO: Instead of doing this to fix it, you can go through the whole list and only calculate the right and left neighbor interactions, but do those fully and doubly. That way is better I think for multiple reasons.
	}
	else {
		*nextCurrentVel += thing;
	//	*top -= thing;
	}
	}

	if (left) {
		float dist = abs(currentValue - leftValue);
	float thing = dist * STRENGTH;

	if (currentValue > leftValue) {
		*nextCurrentVel -= thing;
	//	*left += thing;
	}
	else {
		*nextCurrentVel += thing;
	//	*left -= thing;
	}
	}

	if (bottom) {
		float dist = abs(currentValue - bottomValue);
	float thing = dist * STRENGTH;

	if (currentValue > bottomValue) {
		*nextCurrentVel -= thing;
	//	*bottom += thing;
	}
	else {
		*nextCurrentVel += thing;
	//	*bottom -= thing;
	}
	}

	if (right) {
		float dist = abs(currentValue - rightValue);
	float thing = dist * STRENGTH;

	if (currentValue > rightValue) {
		*nextCurrentVel -= thing;
	//	*right += thing;
	}
	else {
		*nextCurrentVel += thing;
		//*right -= thing;
	}
	}

	*nextCurrentValue += *nextCurrentVel;
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

#define FIELD_SIZE (WINDOW_STARTING_WIDTH * WINDOW_STARTING_HEIGHT)
	float* field = new float[FIELD_SIZE];
	float* fieldVels = new float[FIELD_SIZE];
	ZeroMemory(field, FIELD_SIZE * sizeof(float));
	ZeroMemory(fieldVels, FIELD_SIZE * sizeof(float));

	float* lastField = new float[FIELD_SIZE];
	float* lastFieldVels = new float[FIELD_SIZE];
	ZeroMemory(lastField, FIELD_SIZE * sizeof(float));
	ZeroMemory(lastFieldVels, FIELD_SIZE * sizeof(float));

	/*for (int i = 0; i < 900; i++) {
		lastField[i] = 100;
	}*/

	lastField[windowWidth * 150 + 150] = 10000;

#define FRAME_SIZE (FIELD_SIZE * 4)																								// Do the allocation and setup for the frame data buffer, which we copy into the bitmap after every frame.
	char* frame = new char[FRAME_SIZE];
	for (int i = 0; i < FRAME_SIZE; i += 4) {
		frame[i] =  0;
		frame[i + 3] = 255;
	}

	while (isAlive) {																											// Start the actual graphics loop.


		for (int i = 0; i < FIELD_SIZE; i++) {

			float topValue;																										// Check if a pixel above us exists.
			float* topVel;
			if (i >= windowWidth) {
				int top = i - windowWidth;
				topValue = lastField[top];
				topVel = fieldVels + top;
			}
			else { topVel = nullptr; }

			int x = i % windowWidth;

			float leftValue;																									// Check if a pixel to the left of us exists.
			float* leftVel;
			if (x > 0) {
				int left = i - 1;
				leftValue = lastField[left];
				leftVel = fieldVels + left;
			}
			else { leftVel = nullptr; }

			float bottomValue;																									// Check if a pixel below us exists.
			float* bottomVel;
			if (i < FIELD_SIZE - windowWidth) {																					// TODO: FIELD_SIZE needs to be replaced for the modular size to work properly.
				int bottom = i + windowWidth;
				bottomValue = lastField[bottom];
				bottomVel = fieldVels + bottom;
			}
			else { bottomVel = nullptr; }

			float rightValue;																									// Check if a pixel to the right of us exists.
			float* rightVel;
			if (x < windowWidth - 1) {
				int right = i + 1;
				rightValue = lastField[right];
				rightVel = fieldVels + right;
			}
			else { rightVel = nullptr; }

			calculate(lastField[i], lastFieldVels[i], field + i, fieldVels + i, topValue, leftValue, bottomValue, rightValue, topVel, leftVel, bottomVel, rightVel);					// Send all the data off to calculation function.
			
			//calculate(lastField[i], lastFieldVels[i], field + i, fieldVels + i, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
		}


		memcpy(lastField, field, FIELD_SIZE * sizeof(float));																			// Copy the current frame data into the slot for the last frame.
		memcpy(lastFieldVels, fieldVels, FIELD_SIZE * sizeof(float));

		for (int i = 0; i < FIELD_SIZE; i++) {
			int frameIndex = i * 4;
			if (field[i] > 0) {
				frame[frameIndex + 1] = clamp(field[i], 100) / 100 * 255;
				frame[frameIndex + 2] = 0;
			}
			else if (field[i] == 0) {
				frame[frameIndex + 1] = 0;
				frame[frameIndex + 2] = 0;
			}
			else {
				frame[frameIndex + 1] = 0;
				frame[frameIndex + 2] = clamp((-field[i]), 100) / 100 * 255;
			}
		}

		SetBitmapBits(bmp, FRAME_SIZE, frame);																				// Copy the current frame data into the bitmap, so that it is displayed on the window.
		BitBlt(finalG, 0, 0, WINDOW_STARTING_WIDTH, WINDOW_STARTING_HEIGHT, g, 0, 0, SRCCOPY);
	}
}