#define _CRT_SECURE_NO_WARNINGS

#include "debugOutput.h"

#define WIN32_LEAN_AND_MEAN									// TODO: Make sure to read the docs for OutputDebugStringA again and make sure that we're doing it right here.
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

DebugOutput& DebugOutput::operator<<(char input) {
	char buffer[] { input, '\0' };								// TODO: Make sure this is just as efficient as initializing them normally.
	OutputDebugStringA(buffer);
	return *this;
}

DebugOutput& DebugOutput::operator<<(int32_t input) {
	char buffer[12];
	_itoa(input, buffer, 10);
	OutputDebugStringA(buffer);
	return *this;
}

DebugOutput& DebugOutput::operator<<(uint32_t input) {
	char buffer[11];
	_itoa(input, buffer, 10);
	OutputDebugStringA(buffer);
	return *this;
}
