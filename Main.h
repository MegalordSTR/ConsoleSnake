#pragma once

typedef struct ConsoleForm
{
	char* LocaleName;
	HANDLE OutputHandler;
	CHAR_INFO* Buffer;
	COORD BufferSize;
} ConsoleForm;


const int BAD_EATEN = 1;

typedef enum CELL_TYPE
{
	EMPTY,
	SNAKE_BODY,
	FOOD,
	WALL,
	END,
} CELL_TYPE;

typedef struct MAPCELL {
	CELL_TYPE cellType;
} MAPCELL;

typedef struct MAPCOORD {
	int32_t x;
	int32_t y;
} MAPCOORD;

BOOL MapCoordEqual(MAPCOORD coord1, MAPCOORD coord2);

const MAPCOORD UP = { 0, -1 };
const MAPCOORD DOWN = { 0, 1 };
const MAPCOORD LEFT = { -1, 0 };
const MAPCOORD RIGHT = { 1, 0 };

typedef struct SNAKE {
	MAPCOORD coords;
	struct SNAKE* prev;
} SNAKE;

typedef struct SNAKE_MAP {
	SNAKE* snakeHead;
	SNAKE* snakeTail;
	MAPCELL* cells;
	int32_t height;
	int32_t width;
	int32_t snakeLen;
} SNAKE_MAP;

ConsoleForm* NewConsoleForm(COORD Size, const char* LocaleName, const TCHAR* Title);
BOOL SetConsoleSize(ConsoleForm* Console, COORD NewBufferSize);
BOOL SetConsoleTitleAndCursor(ConsoleForm* Console, TCHAR const* Title);
void RefreshConsole(ConsoleForm* Console, SNAKE_MAP SnakeMap);