#include <stdio.h>
#include <conio.h>
#include <stdint.h>

#pragma warning(push, 3)
#include <windows.h>
#include <Winuser.h>
#pragma warning(pop)

#include <tchar.h>
#include <locale.h>

#include <time.h>

#include "Main.h"

#define MAP_WIDTH 120
#define MAP_HEIGHT 40
#define START_FOOD 20

const COORD g_MapSize = { MAP_WIDTH, MAP_HEIGHT };

int main()
{
	time_t CurrentTime = 0;
	srand((unsigned int)time(&CurrentTime));

	GameMain();
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

	for (int i = 0; i < SnakeMap.Height * SnakeMap.Width; i++)
	{
		CELL_TYPE CurrentCellType = SnakeMap.Cells[i].CellType;

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
	return coord1.X == coord2.X && coord1.Y == coord2.Y;
}

void SpawnFood(SNAKE_MAP* Map)
{
	int СellNum = 0;
	do {
		СellNum = rand() % (MAP_HEIGHT * MAP_WIDTH);
	} while (Map->Cells[СellNum].CellType != EMPTY);

	Map->Cells[СellNum].CellType = FOOD;
}

void SpawnWalls(SNAKE_MAP* Map)
{
	for (int i = 0; i < MAP_WIDTH; i++)
	{
		Map->Cells[0 + i].CellType = WALL;
	}
	for (int i = 0; i < MAP_WIDTH; i++)
	{
		Map->Cells[MAP_WIDTH * (MAP_HEIGHT - 1) + i].CellType = WALL;
	}

	for (int i = 1; i < MAP_HEIGHT - 1; i++)
	{
		Map->Cells[MAP_WIDTH * i + 0].CellType = WALL;
	}
	for (int i = 1; i < MAP_HEIGHT - 1; i++)
	{
		Map->Cells[MAP_WIDTH * i + MAP_WIDTH - 1].CellType = WALL;
	}

	return;
}

MOVE_STATE MoveSnakeBody(SNAKE_MAP* Map, MAPCOORD MoveDirection)
{
	MAPCOORD newCoord = {
		(Map->SnakeHead->Coords.X + Map->Width + MoveDirection.X) % Map->Width,
		(Map->SnakeHead->Coords.Y + Map->Height + MoveDirection.Y) % Map->Height,
	};

	switch (Map->Cells[GetCellByCoord(newCoord)].CellType)
	{
		case FOOD:
		{
			// Если съели еду, то она становиться головой
			SNAKE* BodyBlock = (SNAKE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SNAKE));
			if (BodyBlock == NULL)
			{
				exit(ERROR_NOT_ENOUGH_MEMORY);
			}
			BodyBlock->Coords = newCoord;
			Map->SnakeHead->Prev = BodyBlock;
			Map->SnakeHead = BodyBlock;

			SpawnFood(Map);
			Map->SnakeLen++;
			break;
		}
		case SNAKE_BODY:
		case WALL:
		{
			// Съели себя или стену
			return BAD_EATEN;
		}
		default:
		{
			// Просто движемся
			Map->Cells[GetCellByCoord(Map->SnakeTail->Coords)].CellType = EMPTY;

			if (Map->SnakeTail != Map->SnakeHead) {
				Map->SnakeHead->Prev = Map->SnakeTail;
				Map->SnakeHead = Map->SnakeTail;
				Map->SnakeTail = Map->SnakeHead->Prev;
				Map->SnakeHead->Prev = NULL;
			}

			Map->SnakeHead->Coords = newCoord;
			break;
		}
	}

	Map->Cells[GetCellByCoord(Map->SnakeHead->Coords)].CellType = SNAKE_BODY;

	return STILL_ALIVE;
}

int32_t GetCellByCoord(MAPCOORD Coords)
{
	return Coords.Y * MAP_WIDTH + Coords.X;
}

void GameMain()
{
	ConsoleForm* Console = NewConsoleForm(g_MapSize, "Russian", TEXT("S_N_A_K_E"));
	if (Console == NULL) {
		_tprintf(TEXT("Ошибка создания консоли!\n"));
		return;
	}

	CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;
	if (!GetConsoleScreenBufferInfo(Console->OutputHandler, &ConsoleInfo)) {
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return;
	}

	MAPCELL* CellBuffer = (MAPCELL*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MAPCELL) * MAP_HEIGHT * MAP_WIDTH);
	if (CellBuffer == NULL)
	{
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	SNAKE_MAP Map = {
		NULL,
		NULL,
		CellBuffer,
		MAP_HEIGHT,
		MAP_WIDTH,
		1
	};

	InitMap(&Map);
	RefreshConsole(Console, Map);

	BOOL Running = TRUE;
	MAPCOORD CurrentMoveDirection = UP;

	while (Running)
	{
		int State = 0;
		int KeyboardWasHit = _kbhit();
		if (KeyboardWasHit) {
			switch (_totupper(_gettch()))
			{
			case 0x57: // W
				CurrentMoveDirection = (!MapCoordEqual(CurrentMoveDirection, DOWN)) ? UP : DOWN;
				break;
			case 0x53: // S
				CurrentMoveDirection = (!MapCoordEqual(CurrentMoveDirection, UP)) ? DOWN : UP;
				break;
			case 0x41: // A
				CurrentMoveDirection = (!MapCoordEqual(CurrentMoveDirection, RIGHT)) ? LEFT : RIGHT;
				break;
			case 0x44: // D
				CurrentMoveDirection = (!MapCoordEqual(CurrentMoveDirection, LEFT)) ? RIGHT : LEFT;
				break;
			case 0x45: // E
				Running = FALSE;
				break;
			case 0x46: // F
				SpawnFood(&Map);
				break;
			default:
				break;
			}
		}

		// TODO: добавить проверку на действие движения, чтобы можно было выполнять служебные команды вне хода()
		State = MoveSnakeBody(&Map, CurrentMoveDirection);

		if (State == BAD_EATEN)
		{
			Running = FALSE;

			int CellNum = 0;
			for (int i = 0; i < Map.Height; i++)
			{
				for (int j = 0; j < Map.Width; j++)
				{
					Map.Cells[CellNum].CellType = END;
					CellNum++;
				}
			}
		}


		RefreshConsole(Console, Map);
		int AddedSleepDuration = 2 * Map.SnakeLen > 100 ? 0 : 100 - 2 * Map.SnakeLen;
		Sleep(40 + AddedSleepDuration);
	}

	return;
}

void InitMap(SNAKE_MAP* Map)
{
	int CellNum = 0;
	for (int i = 0; i < Map->Height; i++)
	{
		for (int j = 0; j < Map->Width; j++)
		{
			Map->Cells[CellNum].CellType = EMPTY;
			CellNum++;
		}
	}

	SNAKE* Head = (SNAKE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SNAKE));
	if (Head == NULL)
	{
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	Head->Coords.X = Map->Width / 2;
	Head->Coords.Y = Map->Height / 2;
	Head->Prev = NULL;

	Map->SnakeHead = Head;
	Map->SnakeTail = Head;
	Map->Cells[Map->SnakeHead->Coords.Y * Map->Width + Map->SnakeHead->Coords.X].CellType = SNAKE_BODY;

	SpawnWalls(Map);

	for (int i = 0; i < START_FOOD; i++)
	{
		SpawnFood(Map);
	}

	return;
}