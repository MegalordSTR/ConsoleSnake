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

#define MAP_WIDTH 85
#define MAP_HEIGHT 40

//#define STAT_WIDTH 20
//#define STAT_HEIGHT 40
//
//#define WINDOW_HEIGHT MAP_WIDTH + STAT_WIDTH
//#define WINDOW_WIDTH MAP_HEIGHT

#define MAX_FOOD 1

const COORD g_MapSize = { MAP_WIDTH, MAP_HEIGHT };

int main()
{
	time_t CurrentTime = 0;
	srand((unsigned int)time(&CurrentTime));

	GameMain();
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

	GameGUI GUI = {
		.Console = Console,
	};
	GUI.MapFrame = MakeFrame(GUI.Console->BufferSize, 
		(SMALL_RECT) {
			.Left = 0,
			.Top = 0,
			.Right = GUI.Console->BufferSize.X - 25,
			.Bottom = GUI.Console->BufferSize.Y,
	});
	GUI.StatsFrame = MakeFrame(GUI.Console->BufferSize,
		(SMALL_RECT) {
			.Left = GUI.MapFrame->FramePosition.Right + 1,
			.Top = 1,
			.Right = GUI.Console->BufferSize.X - 1,
			.Bottom = GUI.Console->BufferSize.Y - 1,
	});

	GameStats Stats = {
		.FoodEaten = 0,
	};


	MapCell* CellBuffer = (MapCell*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MapCell) * GUI.MapFrame->FrameSize.X * GUI.MapFrame->FrameSize.Y);
	if (CellBuffer == NULL)
	{
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	SnakeMap Map = {
		.SnakeHead = NULL,
		.SnakeTail = NULL,
		.SnakeLen = 0,
		.Cells = CellBuffer,
		.Height = GUI.MapFrame->FrameSize.Y,
		.Width = GUI.MapFrame->FrameSize.X,
		.CurrentFoodCount = 0,
	};

	InitMap(&Map);
	UpdateMap(GUI, Map);
	DrawGUI(GUI);

	BOOL Running = TRUE;
	MapCOORD CurrentMoveDirection = UP;

	time_t LastTime = 0;
	time_t CurrentTime = timeGetTime();
	int32_t AddedSleepDuration = 2 * Map.SnakeLen > 100 ? 0 : 100 - 2 * Map.SnakeLen;
	Stats.CurrentSpeed = 40 + AddedSleepDuration;
	while (Running)
	{
		LastTime = CurrentTime;
		CurrentTime = timeGetTime();
		while (CurrentTime - LastTime < Stats.CurrentSpeed)
		{
			int32_t toSleep = (int32_t)Stats.CurrentSpeed - (int32_t)(CurrentTime - LastTime);
			Sleep(toSleep);
			CurrentTime = timeGetTime();
		}

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
					#pragma warning(suppress : 6386)
					Map.Cells[CellNum].CellType = END;
					CellNum++;
				}
			}
		}

		if (State == FOOD_EATEN)
		{
			Map.CurrentFoodCount--;
			Stats.FoodEaten++;
			if (Map.CurrentFoodCount < MAX_FOOD)
			{
				SpawnFood(&Map);
			}
		}

		Stats.CurrentTurn++;

		AddedSleepDuration = 2 * Map.SnakeLen > 100 ? 0 : 100 - 2 * Map.SnakeLen;
		Stats.CurrentSpeed = 40 + AddedSleepDuration;

		UpdateMap(GUI, Map);
		UpdateStats(GUI, Stats);
		DrawGUI(GUI);
	}

	return;
}

void UpdateMap(GameGUI GUI, SnakeMap SnakeMap)
{
	CellType PrevCellType = EMPTY;
	WORD CurrentAttribute = 0;

	for (int i = 0; i < SnakeMap.Height * SnakeMap.Width; i++)
	{
		CellType CurrentCellType = SnakeMap.Cells[i].CellType;
		
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

		GUI.MapFrame->FrameBuffer[i] = (CHAR_INFO)
		{
			.Attributes = CurrentAttribute,
			.Char.UnicodeChar = TEXT(' ')
		};
	}
}

void UpdateStats(GameGUI GUI, GameStats Stats)
{
	memset(GUI.StatsFrame->FrameBuffer, 0, sizeof(CHAR_INFO) * GUI.StatsFrame->FrameSize.X * GUI.StatsFrame->FrameSize.Y);

	wchar_t Buffer[100] = {0};

	// Current Turn
	_stprintf_s(Buffer, 100, TEXT("Turn       = %d"), Stats.CurrentTurn);
	PrintStringToFrame(GUI.StatsFrame, Buffer, GUI.StatsFrame->FrameSize.Y / 2);

	// Food Eaten
	_stprintf_s(Buffer, 100, TEXT("Food Eaten = %d"), Stats.FoodEaten);
	PrintStringToFrame(GUI.StatsFrame, Buffer, GUI.StatsFrame->FrameSize.Y / 2 + 1);

	// Speed
	_stprintf_s(Buffer, 100, TEXT("Speed = %dms"), Stats.CurrentSpeed);
	PrintStringToFrame(GUI.StatsFrame, Buffer, GUI.StatsFrame->FrameSize.Y / 2 + 5);
}

void PrintStringToFrame(Frame* Frame, const TCHAR* str, int32_t YLevel)
{
	int32_t startPos = GetCellByCoordOnFrame(
		(MapCOORD) {
		.X = 0,
		.Y = YLevel,
	}, Frame->FrameSize);

	for (int32_t i = 0; str[i] != '\0' && i < Frame->FrameSize.X; i++)
	{
		Frame->FrameBuffer[startPos + i] = (CHAR_INFO){
			.Attributes = FOREGROUND_INTENSITY,
			.Char.UnicodeChar = str[i]
		};
	}
}

/// <summary>
/// Принимает Размер родительского буффера, координаты прямоугольника фрейма в SMALL_RECT, возвращает структуру фрейма
/// </summary>
/// <returns>SMALL_RECT реальной позицией фрейма</returns>
Frame* MakeFrame(COORD ParentBufferSize, SMALL_RECT FrameRectPosition)
{
	Frame* NewFrame = (Frame*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Frame));
	if (NewFrame == NULL) {
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	FrameRectPosition.Right = FrameRectPosition.Right < FrameRectPosition.Left ? FrameRectPosition.Left : FrameRectPosition.Right;
	FrameRectPosition.Bottom = FrameRectPosition.Bottom < FrameRectPosition.Top ? FrameRectPosition.Top : FrameRectPosition.Bottom;

	FrameRectPosition.Left = FrameRectPosition.Left < 0 ? 0 : FrameRectPosition.Left;
	FrameRectPosition.Right = FrameRectPosition.Right > ParentBufferSize.X ? ParentBufferSize.X : FrameRectPosition.Right;
	FrameRectPosition.Top = FrameRectPosition.Top < 0 ? 0 : FrameRectPosition.Top;
	FrameRectPosition.Bottom = FrameRectPosition.Bottom > ParentBufferSize.Y ? ParentBufferSize.Y : FrameRectPosition.Bottom;

	NewFrame->FramePosition = FrameRectPosition;
	NewFrame->FrameSize = (COORD) {
		.X = NewFrame->FramePosition.Right - NewFrame->FramePosition.Left,
		.Y = NewFrame->FramePosition.Bottom - NewFrame->FramePosition.Top,
	};

	NewFrame->FrameBuffer = (PCHAR_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHAR_INFO) * NewFrame->FrameSize.X * NewFrame->FrameSize.Y);
	if (NewFrame->FrameBuffer == NULL)
	{
		exit(ERROR_NOT_ENOUGH_MEMORY);
	}

	return NewFrame;
}

/// <summary>
/// Отрисовывает содержимое фреймов на консоль
/// </summary>
/// <param name="GUI"></param>
void DrawGUI(GameGUI GUI)
{
	CopyFrameToConsoleBuffer(GUI.MapFrame, GUI.Console);
	MakeFrameBorder(GUI.MapFrame, GUI.Console);
	CopyFrameToConsoleBuffer(GUI.StatsFrame, GUI.Console);
	MakeFrameBorder(GUI.StatsFrame, GUI.Console);

	SMALL_RECT ConsoleWriteRect = {
			0,
			0,
			GUI.Console->BufferSize.X - 1,
			GUI.Console->BufferSize.Y - 1
	};


	if (!WriteConsoleOutput(GUI.Console->OutputHandler, GUI.Console->Buffer, GUI.Console->BufferSize, (COORD) { 0, 0 }, &ConsoleWriteRect)) {
		_tprintf(TEXT("Ошибка %d записи в консоль\n"), GetLastError());
		return;
	}
}

/// <summary>
/// Копирует буфер фрейма в буфер консоли
/// </summary>
/// <param name="Frame"></param>
/// <param name="Console"></param>
void CopyFrameToConsoleBuffer(Frame* Frame, ConsoleForm* Console)
{
	for (int32_t y = 0; y < Frame->FrameSize.Y; y++)
	{
		int32_t yShift = (Frame->FramePosition.Top * Console->BufferSize.X) + Console->BufferSize.X * y;
		for (int32_t x = 0; x < Frame->FrameSize.X; x++)
		{
			int32_t xShift = Frame->FramePosition.Left + x;
			Console->Buffer[yShift + xShift] = Frame->FrameBuffer[Frame->FrameSize.X * y + x];
		}
	}
}

/// <summary>
/// Создает рамку вокруг фрейма(со всех сторон, если возможно) в буфере консоли
/// </summary>
/// <param name="Frame"></param>
/// <param name="Console"></param>
void MakeFrameBorder(Frame* Frame, ConsoleForm* Console)
{
	_Bool TBorder = FALSE, BBorder = FALSE, LBorder = FALSE, RBorder = FALSE;

	// UpperBorder
	int32_t yShift = (Frame->FramePosition.Top - 1) * Console->BufferSize.X;
	if (yShift/Console->BufferSize.X >= 0)
	{
		TBorder = TRUE;
		for (int i = 0; i < Frame->FrameSize.X; i++)
		{
			int32_t xShift = (Frame->FramePosition.Left + i);
			#pragma warning(suppress : 6386)
			Console->Buffer[yShift + xShift] = (CHAR_INFO)
			{
				.Attributes = FOREGROUND_INTENSITY,
				.Char.UnicodeChar = TEXT('#')
			};
		}
	}
	

	// BottomBorder
	yShift = (Frame->FramePosition.Bottom) * Console->BufferSize.X;
	if (yShift / Console->BufferSize.X < Console->BufferSize.Y)
	{
		BBorder = TRUE;
		for (int i = 0; i < Frame->FrameSize.X; i++)
		{
			int32_t xShift = (Frame->FramePosition.Left + i);
			#pragma warning(suppress : 6386)
			Console->Buffer[yShift + xShift] = (CHAR_INFO)
			{
				.Attributes = FOREGROUND_INTENSITY,
				.Char.UnicodeChar = TEXT('#')
			};
		}
	}

	// LeftBorder
	int32_t xShift = (Frame->FramePosition.Left - 1);
	if (xShift >= 0)
	{
		LBorder = TRUE;
		for (int i = 0; i < Frame->FrameSize.Y; i++)
		{
			yShift = (Frame->FramePosition.Top + i) * Console->BufferSize.X;
			#pragma warning(suppress : 6386)
			Console->Buffer[yShift + xShift] = (CHAR_INFO)
			{
				.Attributes = FOREGROUND_INTENSITY,
				.Char.UnicodeChar = TEXT('#')
			};
		}
	}
	
	 
	// RightBorder
	xShift = (Frame->FramePosition.Right);
	if (xShift < Console->BufferSize.X)
	{
		RBorder = TRUE;
		for (int i = 0; i < Frame->FrameSize.Y; i++)
		{
			yShift = (Frame->FramePosition.Top + i) * Console->BufferSize.X;
			#pragma warning(suppress : 6386)
			Console->Buffer[yShift + xShift] = (CHAR_INFO)
			{
				.Attributes = FOREGROUND_INTENSITY,
				.Char.UnicodeChar = TEXT('#')
			};
		}
	}

	// Upper-left corner
	if (LBorder && TBorder)
	{
		yShift = (Frame->FramePosition.Top - 1) * Console->BufferSize.X;
		xShift = Frame->FramePosition.Left - 1;
		Console->Buffer[yShift + xShift] = (CHAR_INFO)
		{
			.Attributes = FOREGROUND_INTENSITY,
			.Char.UnicodeChar = TEXT('#')
		};
	}
	// Upper-right corner
	if (TBorder && RBorder)
	{
		yShift = (Frame->FramePosition.Top - 1) * Console->BufferSize.X;
		xShift = Frame->FramePosition.Right;
		Console->Buffer[yShift + xShift] = (CHAR_INFO)
		{
			.Attributes = FOREGROUND_INTENSITY,
			.Char.UnicodeChar = TEXT('#')
		};
	}
	// Bottom-right corner
	if (RBorder && BBorder)
	{
		yShift = (Frame->FramePosition.Bottom) * Console->BufferSize.X;
		xShift = Frame->FramePosition.Right;
		Console->Buffer[yShift + xShift] = (CHAR_INFO)
		{
			.Attributes = FOREGROUND_INTENSITY,
			.Char.UnicodeChar = TEXT('#')
		};
	}
	// Bottom-left corner
	if (BBorder && LBorder)
	{
		yShift = (Frame->FramePosition.Bottom) * Console->BufferSize.X;
		xShift = Frame->FramePosition.Left - 1;
		Console->Buffer[yShift + xShift] = (CHAR_INFO)
		{
			.Attributes = FOREGROUND_INTENSITY,
			.Char.UnicodeChar = TEXT('#')
		};
	}
}

BOOL MapCoordEqual(MapCOORD coord1, MapCOORD coord2)
{
	return coord1.X == coord2.X && coord1.Y == coord2.Y;
}

MoveState MoveSnakeBody(SnakeMap* Map, MapCOORD MoveDirection)
{
	MoveState Result = NORMAL;

	MapCOORD newCoord = {
		(Map->SnakeHead->Coords.X + Map->Width + MoveDirection.X) % Map->Width,
		(Map->SnakeHead->Coords.Y + Map->Height + MoveDirection.Y) % Map->Height,
	};

	switch (Map->Cells[GetCellByCoordOnFrame((newCoord), (COORD) { .X = (SHORT)Map->Width, .Y = (SHORT)Map->Height })].CellType)
	{
	case FOOD:
	{
		// Если съели еду, то она становиться головой
		Snake* BodyBlock = (Snake*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Snake));
		if (BodyBlock == NULL)
		{
			exit(ERROR_NOT_ENOUGH_MEMORY);
		}
		BodyBlock->Coords = newCoord;
		Map->SnakeHead->Prev = BodyBlock;
		Map->SnakeHead = BodyBlock;
		Map->SnakeLen++;

		Result = FOOD_EATEN;
		break;
	}
	case SNAKE_BODY:
	case WALL:
	{
		// Съели себя или стену
		Result = BAD_EATEN;
		break;
	}
	default:
	{
		// Просто движемся
		Map->Cells[GetCellByCoordOnFrame((Map->SnakeTail->Coords), (COORD) { .X = (SHORT)Map->Width, .Y = (SHORT)Map->Height })].CellType = EMPTY;

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

	Map->Cells[GetCellByCoordOnFrame((Map->SnakeHead->Coords), (COORD) { .X = (SHORT)Map->Width, .Y = (SHORT)Map->Height })].CellType = SNAKE_BODY;

	return Result;
}

int32_t GetCellByCoordOnFrame(MapCOORD Coords, COORD FrameSize)
{
	return Coords.Y * FrameSize.X + Coords.X;
}

void SpawnFood(SnakeMap* Map)
{
	int СellNum = 0;
	do {
		СellNum = rand() % (Map->Width * Map->Height);
	} while (Map->Cells[СellNum].CellType != EMPTY);

	Map->Cells[СellNum].CellType = FOOD;
	Map->CurrentFoodCount++;
}

void InitMap(SnakeMap* Map)
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

	Snake* Head = (Snake*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Snake));
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
	Map->SnakeLen = 1;

	SpawnWalls(Map);

	for (int i = 0; i < MAX_FOOD; i++)
	{
		SpawnFood(Map);
	}

	return;
}

void SpawnWalls(SnakeMap* Map)
{
	for (int i = 0; i < Map->Width; i++)
	{
		Map->Cells[0 + i].CellType = WALL;
	}
	for (int i = 0; i < Map->Width; i++)
	{
		Map->Cells[Map->Width * (Map->Height - 1) + i].CellType = WALL;
	}

	for (int i = 1; i < Map->Height - 1; i++)
	{
		Map->Cells[Map->Width * i + 0].CellType = WALL;
	}
	for (int i = 1; i < Map->Height - 1; i++)
	{
		Map->Cells[Map->Width * i + Map->Width - 1].CellType = WALL;
	}

	return;
}

ConsoleForm* NewConsoleForm(COORD Size, const char* LocaleName, const TCHAR* Title)
{
	LocaleName = LocaleName == NULL ? "Russian" : LocaleName;
	Title = Title == NULL ? TEXT("") : Title;

	setlocale(LC_ALL, LocaleName);

	HANDLE OutputHandler = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!OutputHandler) {
		_tprintf(TEXT("Ошибка %d\n"), GetLastError());
		return NULL;
	}

	ConsoleForm* Console = (ConsoleForm*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ConsoleForm));
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

	PCHAR_INFO newBuffer = (PCHAR_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CHAR_INFO) * NewBufferSize.X * NewBufferSize.Y);
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