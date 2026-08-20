#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include "rlrmui.h"


int main(void) {

  int screenWidth = 500;
  int screenHeight = 400;


  // UI Definition
  Window *mainWindow = CreateWindow();

  Frame* f = CreateFrame(0, 0, screenWidth, screenHeight, true);
  SetRootFrame(mainWindow, f);
  
  Button* btn = CreateButton("Lorem ipsum");
  AddWidget(f, (Widget*)btn);
 
  SetConfigFlags(FLAG_VSYNC_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(screenWidth, screenHeight, "layout test");
  
  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawButton(btn);
    EndDrawing();
    EventLoop(mainWindow);
  }
  
  }


  
