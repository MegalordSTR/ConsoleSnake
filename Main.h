#pragma once

typedef struct ConsoleForm
{
	char* LocaleName;
	HANDLE OutputHandler;
	CHAR_INFO* Buffer;
	COORD BufferSize;
} ConsoleForm;

typedef struct GameStats;


typedef enum MoveState
{
	NORMAL,
	FOOD_EATEN,
	BAD_EATEN,
} MoveState;

typedef enum CellType
{
	EMPTY,
	SNAKE_BODY,
	FOOD,
	WALL,
	END,
} CellType;

typedef struct MapCell {
	CellType CellType;
} MapCell;

typedef struct MapCOORD {
	int32_t X;
	int32_t Y;
} MapCOORD;

const MapCOORD UP = { 0, -1 };
const MapCOORD DOWN = { 0, 1 };
const MapCOORD LEFT = { -1, 0 };
const MapCOORD RIGHT = { 1, 0 };

typedef struct Snake {
	MapCOORD Coords;
	struct Snake* Prev;
} Snake;

typedef struct SnakeMap {
	Snake* SnakeHead;
	Snake* SnakeTail;
	MapCell* Cells;
	int32_t Height;
	int32_t Width;
	int32_t SnakeLen;
	int32_t CurrentFoodCount;
} SnakeMap;

typedef struct Frame {
	PCHAR_INFO FrameBuffer;
	SMALL_RECT FramePosition;
	COORD FrameSize;
} Frame;

typedef struct GameGUI {
	ConsoleForm* Console;
	Frame* MapFrame;
	Frame* StatsFrame;
} GameGUI;

void GameMain();
void UpdateMap(GameGUI GUI, SnakeMap SnakeMap);
BOOL MapCoordEqual(MapCOORD Coord1, MapCOORD Coord2);
MoveState MoveSnakeBody(SnakeMap* Map, MapCOORD MoveDirection);
int32_t GetCellByCoordOnFrame(MapCOORD Coords, COORD FrameSize);
void SpawnFood(SnakeMap* map);
void InitMap(SnakeMap* map);
void SpawnWalls(SnakeMap* Map);

ConsoleForm* NewConsoleForm(COORD Size, const char* LocaleName, const TCHAR* Title);
BOOL SetConsoleSize(ConsoleForm* Console, COORD NewBufferSize);
BOOL SetConsoleTitleAndCursor(ConsoleForm* Console, TCHAR const* Title);

Frame* MakeFrame(COORD ParentBufferSize, SMALL_RECT FrameRectPosition);
void DrawGUI(GameGUI GUI);
void CopyFrameToConsoleBuffer(Frame* Frame, ConsoleForm* Console);
void MakeFrameBorder(Frame* Frame, ConsoleForm* Console);