#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <thread>

#define FIELD_PULL 0.01f																											// The strength with which points on the field are forced to return to their origin.

#define WINDOW_STARTING_WIDTH 300
#define WINDOW_STARTING_HEIGHT 300

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


void calculate(float currentValue, float currentVel, float* nextCurrentValue, float* nextCurrentVel, float topValue, float leftValue, float bottomValue, float rightValue, float* top, float* left, float* bottom, float* right) {			// Calculates the actual wave propagation.

	if (currentValue == 0) {
		*nextCurrentVel = currentVel;
	}
	else if (currentValue > 0) {
		*nextCurrentVel = currentVel - FIELD_PULL;
	}
	else if (currentValue < 0) {
		*nextCurrentVel = currentVel + FIELD_PULL;
	}
	*nextCurrentValue = currentValue;

	if (top) {
			float dist = abs(currentValue - topValue);
	float thing = dist * 1;

	if (currentValue > topValue) {
		*nextCurrentVel -= thing;
		*top += thing;
	}
	else {
		*nextCurrentVel += thing;
		*top -= thing;
	}
	}

	if (left) {
		float dist = abs(currentValue - leftValue);
	float thing = dist * 1;

	if (currentValue > leftValue) {
		*nextCurrentVel -= thing;
		*left += thing;
	}
	else {
		*nextCurrentVel += thing;
		*left -= thing;
	}
	}

	if (bottom) {
		float dist = abs(currentValue - bottomValue);
	float thing = dist * 1;

	if (currentValue > bottomValue) {
		*nextCurrentVel -= thing;
		*bottom += thing;
	}
	else {
		*nextCurrentVel += thing;
		*bottom -= thing;
	}
	}

	if (right) {
		float dist = abs(currentValue - rightValue);
	float thing = dist * 1;

	if (currentValue > rightValue) {
		*nextCurrentVel -= thing;
		*right += thing;
	}
	else {
		*nextCurrentVel += thing;
		*right -= thing;
	}
	}

	/*if (up == 0) {

		if (velUp == 0) {
			if (velDown == 0) {
				if (down == 0) { return; }
				if (FIELD_PULL > down) {
					*current = FIELD_PULL - down;
					*(current + 1) = 0;
					*(current + 2) = FIELD_PULL - *current;
				}
			}
		}


		if (FIELD_PULL > *currentUnderValue) {
			*current = FIELD_PULL - *currentUnderValue;
			*(current + 1) = 0;
		}
		*current = *currentUnderValue - FIELD_PULL;
		*(current + 2) =
	}*/
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

	float* lastField = new float[FIELD_SIZE];
	float* lastFieldVels = new float[FIELD_SIZE];
	ZeroMemory(lastField, FIELD_SIZE * sizeof(float));
	ZeroMemory(lastFieldVels, FIELD_SIZE * sizeof(float));

	for (int i = 0; i < 300; i++) {
		lastField[i] = 100;
	}

#define FRAME_SIZE (FIELD_SIZE * 4)
	char* frame = new char[FRAME_SIZE];
	for (int i = 0; i < FRAME_SIZE; i += 4) {
		frame[i] =  0;
		frame[i + 3] = 255;
	}

	while (isAlive) {																											// Start the actual graphics loop.


		for (int i = 0; i < FIELD_SIZE; i++) {

			int left = -1;
			if (i % 300 > 0) { left = i - 1; }
			int top = -1;
			if (i > 300) { top = i - 300; }
			int right = -1;
			if (i % 300 < 299) { right = i + 1; }
			int bottom = -1;
			if (i < FIELD_SIZE - 300) { bottom = i + 300; }

			float leftValue = lastField[left];
			float topValue = lastField[top];
			float rightValue = lastField[right];
			float bottomValue = lastField[bottom];

			float* leftVel = fieldVels + left;
			if (left == -1) { leftVel = nullptr; }
			float* topVel = fieldVels + top;
			if (top == -1) { topVel = nullptr; }
			float* rightVel = fieldVels + right;
			if (right == -1) { rightVel = nullptr; }
			float* bottomVel = fieldVels + bottom;
			if (bottom == -1) { bottomVel = nullptr; }

			calculate(lastField[i], lastFieldVels[i], field + i, fieldVels + i, topValue, leftValue, bottomValue, rightValue, topVel, leftVel, bottomVel, rightVel);
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
