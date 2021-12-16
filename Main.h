#pragma once

typedef struct ConsoleForm
{
	char* LocaleName;
	HANDLE OutputHandler;
	CHAR_INFO* Buffer;
	COORD BufferSize;
} ConsoleForm;

typedef enum MOVE_STATE
{
	STILL_ALIVE,
	BAD_EATEN,
} MOVE_STATE;

typedef enum CELL_TYPE
{
	EMPTY,
	SNAKE_BODY,
	FOOD,
	WALL,
	END,
} CELL_TYPE;

typedef struct MAPCELL {
	CELL_TYPE CellType;
} MAPCELL;

typedef struct MAPCOORD {
	int32_t X;
	int32_t Y;
} MAPCOORD;

BOOL MapCoordEqual(MAPCOORD Coord1, MAPCOORD Coord2);

const MAPCOORD UP = { 0, -1 };
const MAPCOORD DOWN = { 0, 1 };
const MAPCOORD LEFT = { -1, 0 };
const MAPCOORD RIGHT = { 1, 0 };

typedef struct SNAKE {
	MAPCOORD Coords;
	struct SNAKE* Prev;
} SNAKE;

typedef struct SNAKE_MAP {
	SNAKE* SnakeHead;
	SNAKE* SnakeTail;
	MAPCELL* Cells;
	int32_t Height;
	int32_t Width;
	int32_t SnakeLen;
} SNAKE_MAP;

ConsoleForm* NewConsoleForm(COORD Size, const char* LocaleName, const TCHAR* Title);
BOOL SetConsoleSize(ConsoleForm* Console, COORD NewBufferSize);
BOOL SetConsoleTitleAndCursor(ConsoleForm* Console, TCHAR const* Title);
void RefreshConsole(ConsoleForm* Console, SNAKE_MAP SnakeMap);
void SpawnFood(SNAKE_MAP* map);
void SpawnWalls(SNAKE_MAP* Map);
int32_t GetCellByCoord(MAPCOORD Coords);
MOVE_STATE MoveSnakeBody(SNAKE_MAP* Map, MAPCOORD MoveDirection);
void GameMain();
void InitMap(SNAKE_MAP* map);