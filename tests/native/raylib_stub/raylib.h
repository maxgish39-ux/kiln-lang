#ifndef RAYLIB_H
#define RAYLIB_H
typedef struct Color { unsigned char r,g,b,a; } Color;
#define KEY_ESCAPE 256
#define KEY_SPACE 32
#define KEY_ENTER 257
#define KEY_TAB 258
#define KEY_LEFT 263
#define KEY_RIGHT 262
#define KEY_UP 265
#define KEY_DOWN 264
#define KEY_BACKSPACE 259
#define KEY_F1 290
#define MOUSE_BUTTON_LEFT 0
#define MOUSE_BUTTON_RIGHT 1
#define MOUSE_BUTTON_MIDDLE 2
void InitWindow(int,int,const char*);
void CloseWindow(void);
int WindowShouldClose(void);
void BeginDrawing(void);
void EndDrawing(void);
void ClearBackground(Color);
void DrawText(const char*,int,int,int,Color);
void DrawRectangle(int,int,int,int,Color);
void DrawCircle(int,int,float,Color);
void DrawLine(int,int,int,int,Color);
void DrawRectangleLines(int,int,int,int,Color);
int MeasureText(const char*,int);
int GetScreenWidth(void);
int GetScreenHeight(void);
float GetMouseWheelMove(void);
int IsMouseButtonDown(int);
void SetWindowTitle(const char*);
void ToggleFullscreen(void);
int IsKeyDown(int);
int IsKeyPressed(int);
int GetMouseX(void);
int GetMouseY(void);
int IsMouseButtonPressed(int);
void SetTargetFPS(int);
#endif
