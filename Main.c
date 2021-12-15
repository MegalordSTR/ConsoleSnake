#include <stdio.h>
#include <stdint.h>

#pragma warning(push, 3)
#include <windows.h>
#pragma warning(pop)

#include <tchar.h>
#include <locale.h>

#include "Main.h"

#define MAP_WIDTH 80
#define MAP_HEIGHT 30
#define START_FOOD 10

const COORD g_MapSize = { MAP_WIDTH, MAP_HEIGHT };

int main()
{
	ConsoleForm* f = NewConsoleForm(g_MapSize, NULL, TEXT("SNAKE"));
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

	ConsoleForm * Console = (ConsoleForm*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ConsoleForm));
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

	PCHAR_INFO newBuffer = (PCHAR_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHAR_INFO)*NewBufferSize.X * NewBufferSize.Y);
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

void RefreshConsole(ConsoleForm* Console, SNAKE_MAP SnakeMap)
{
	CELL_TYPE PrevCellType = EMPTY;
	WORD CurrentAttribute = 0;

	for (int i = 0; i < SnakeMap.height * SnakeMap.width; i++)
	{
		CELL_TYPE CurrentCellType = SnakeMap.cells[i].cellType;

		if (CurrentCellType != PrevCellType)
		{
			PrevCellType = CurrentCellType;
			if (CurrentCellType == EMPTY)
			{
				CurrentAttribute = 0;
			}
			else if (CurrentCellType == WALL)
			{
				CurrentAttribute = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
			}
			else if (CurrentCellType == SNAKE_BODY)
			{
				CurrentAttribute = BACKGROUND_GREEN;
			}
			else if (CurrentCellType == FOOD)
			{
				CurrentAttribute = BACKGROUND_RED | BACKGROUND_GREEN;
			}
			else if (CurrentCellType == END)
			{
				CurrentAttribute = BACKGROUND_RED | BACKGROUND_INTENSITY;
			}
		}

		Console->Buffer[i] = (CHAR_INFO)
		{
			.Attributes = CurrentAttribute,
			.Char.UnicodeChar = TEXT(' ')
		};
	}

	SMALL_RECT srctWriteRect = {
			0,
			0,
			Console->BufferSize.X - 1,
			Console->BufferSize.Y - 1
	};

	if (!WriteConsoleOutput(Console->OutputHandler, Console->Buffer, Console->BufferSize, (COORD){ 0,0 }, &srctWriteRect)) {
		_tprintf(TEXT("Ошибка %d записи в консоль\n"), GetLastError());
		return;
	}
}

BOOL MapCoordEqual(MAPCOORD coord1, MAPCOORD coord2)
{
	return coord1.x == coord2.x && coord1.y == coord2.y;
}