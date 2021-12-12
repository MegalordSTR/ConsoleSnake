#pragma once

typedef struct ConsoleForm
{
	char* LocaleName;
	HANDLE OutputHandler;
	CHAR_INFO* Buffer;
	COORD BufferSize;
} ConsoleForm;

ConsoleForm* NewConsoleForm(COORD Size, const char* LocaleName, const TCHAR* Title);