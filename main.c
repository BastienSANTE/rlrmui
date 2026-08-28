#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include "rlrmui.h"


int main(void) {

  int screenWidth = 500;
  int screenHeight = 400;

  SetConfigFlags(FLAG_VSYNC_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  
  InitWindow(screenWidth, screenHeight, "layout test");
  
  // UI Definition
  Window *mainWindow = CreateWindow(screenWidth, screenHeight);

  Frame* f = CreateFrame(0, 0, screenWidth, screenHeight, true);
  SetRootFrame(mainWindow, f);
  
  Button* btn = CreateButton("Options & Properties");
  AddWidget(f, (Widget*)btn);

  TextBox* tb = CreateTextBox("Lorem ipsum dolor sit amet, consectetur adpiscing elit", 0, 40, 200, 200);
 
 
  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawButton(btn);
    
    DEBUG_DrawFocus(mainWindow);
    EndDrawing();
    EventLoop(mainWindow);
  }  
}
