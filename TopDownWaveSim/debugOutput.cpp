#define _CRT_SECURE_NO_WARNINGS

#include "debugOutput.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <stdlib.h>

DebugOutput& DebugOutput::operator<<(const char* input) {
	OutputDebugStringA(input);
	return *this;
}

DebugOutput& DebugOutput::operator<<(char* input) {
	OutputDebugStringA(input);
	return *this;
}

DebugOutput& DebugOutput::operator<<(int32_t input) {
	char buffer[12];
	_itoa(input, buffer, 10);
	OutputDebugStringA(buffer);
	return *this;
}
