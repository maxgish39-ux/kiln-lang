#include "raylib.h"
#include <stdio.h>
#include <string.h>
static int frame=0;
void InitWindow(int w,int h,const char*t){printf("OPEN:%d:%d:%s\n",w,h,t);}
void CloseWindow(void){puts("CLOSE");}
int WindowShouldClose(void){return frame++>=1;}
void BeginDrawing(void){}
void EndDrawing(void){}
void ClearBackground(Color c){printf("CLEAR:%u,%u,%u\n",c.r,c.g,c.b);}
void DrawText(const char*t,int x,int y,int s,Color c){printf("TEXT:%s:%d:%d:%d:%u,%u,%u\n",t,x,y,s,c.r,c.g,c.b);}
void DrawRectangle(int x,int y,int w,int h,Color c){printf("RECT:%d,%d,%d,%d:%u,%u,%u\n",x,y,w,h,c.r,c.g,c.b);}
void DrawCircle(int x,int y,float r,Color c){printf("CIRCLE:%d,%d:%.0f:%u,%u,%u\n",x,y,r,c.r,c.g,c.b);}
void DrawLine(int x1,int y1,int x2,int y2,Color c){printf("LINE:%d,%d:%d,%d:%u,%u,%u\n",x1,y1,x2,y2,c.r,c.g,c.b);}
void DrawRectangleLines(int x,int y,int w,int h,Color c){printf("OUTLINE:%d,%d,%d,%d:%u,%u,%u\n",x,y,w,h,c.r,c.g,c.b);}
int MeasureText(const char*t,int size){return (int)(strlen(t)*(size/2.0));}
int GetScreenWidth(void){return 800;}
int GetScreenHeight(void){return 600;}
float GetMouseWheelMove(void){return 1.25f;}
int IsMouseButtonDown(int b){return b==MOUSE_BUTTON_LEFT;}
void SetWindowTitle(const char*t){printf("TITLE:%s\n",t);}
void ToggleFullscreen(void){puts("FULLSCREEN");}
int IsKeyDown(int k){return k==KEY_ESCAPE;}
int IsKeyPressed(int k){return k==KEY_SPACE;}
int GetMouseX(void){return 123;}
int GetMouseY(void){return 234;}
int IsMouseButtonPressed(int b){return b==MOUSE_BUTTON_LEFT;}
void SetTargetFPS(int fps){printf("FPS:%d\n",fps);}
