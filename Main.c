#include <stdio.h>
#include <stdint.h>

#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)

#include <tchar.h>
#include <locale.h>

#include "Main.h"

int main()
{
	HANDLE outputHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD c = { 40, 40 };
	ConsoleForm* f = NewConsoleForm(c, NULL, TEXT("SNAKE"));
	sleep(10000);
	return 0;
}

ConsoleForm* NewConsoleForm(COORD Size, const char * LocaleName, const TCHAR * Title)
{
	LocaleName = LocaleName == NULL ? "Russian" : LocaleName;
	Title = Title == NULL ? TEXT("") : Title;

	setlocale(LC_ALL, LocaleName);

	HANDLE OutputHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!OutputHandler) {
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return NULL;
	}

	ConsoleForm * Console = (ConsoleForm*)HeapAlloc(NULL, 0, sizeof(ConsoleForm));
	if (Console == NULL) {
		_tprintf(TEXT("Ошибка выделения памяти под консоль\n"));
		return NULL;
	}

	Console->LocaleName = _strdup(LocaleName);
	Console->OutputHandler = OutputHandler;

	if (!SetConsoleSize(Console, Size))
	{
		free(Console->LocaleName);
		return NULL;
	}

	if (!SetConsoleTitleAndCursor(Console, Title))
	{
		free(Console->LocaleName);
		return NULL;
	}

	return Console;
}

BOOL SetConsoleSize(ConsoleForm* Console, COORD NewBufferSize)
{
	SMALL_RECT WindowInfo = { 0 };
	// Зануление размера консоли
	if (!SetConsoleWindowInfo(Console->OutputHandler, TRUE, &WindowInfo))
	{
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return FALSE;
	}

	if (!SetConsoleScreenBufferSize(Console->OutputHandler, NewBufferSize))
	{
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return FALSE;
	}

	WindowInfo.Right = NewBufferSize.X - 1;
	WindowInfo.Bottom = NewBufferSize.Y - 1;
	if (!SetConsoleWindowInfo(Console->OutputHandler, TRUE, &WindowInfo))
	{
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return FALSE;
	}

	PCHAR_INFO newBuffer = (PCHAR_INFO)calloc(sizeof (CHAR_INFO), NewBufferSize.X * NewBufferSize.Y);
	if (!newBuffer)
	{
		_tprintf(TEXT("Ошибка выделения памяти под буффер %d\n"), GetLastError());
		return FALSE;
	}

	Console->Buffer = newBuffer;
	Console->BufferSize = NewBufferSize;

	return TRUE;
}

BOOL SetConsoleTitleAndCursor(ConsoleForm* Console, TCHAR const* Title)
{
	if (!SetConsoleTitle(Title))
	{
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return FALSE;
	}

	CONSOLE_CURSOR_INFO ci = { 1, FALSE };

	if (!SetConsoleCursorInfo(Console->OutputHandler, &ci))
	{
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return FALSE;
	}

	return TRUE;
}