#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include "rlrmui.h"


int main(void) {

  /*int screenWidth = 500;
  int screenHeight = 400;

  SetConfigFlags(FLAG_VSYNC_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  
  InitWindow(screenWidth, screenHeight, "layout test");

  defaultFont = LoadFontEx("UnisonTerm.ttf", 12, 0, 1000);
  
  SetTargetFPS(1);
  
  // UI Definition
  Window *mainWindow = CreateWindow(screenWidth, screenHeight);

  Frame* f = CreateFrame(0, 0, screenWidth, screenHeight, true);
  SetRootFrame(mainWindow, f);
  
  Button* btn = CreateButton("Options & Properties");
  AddWidget(f, (Widget*)btn);

  TextBox* tb = CreateTextBox("Lorem ipsum\n dolor\n sit amet, consectetur", 40, 40, 200, 200);

  int textboxW = 50; int textboxH = 300;
  
  while(!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawButton(btn);
    //TextBox_Resize(tb, textboxW++, textboxH++);
    //DrawTextBox(tb);
    DEBUG_DrawFocus(mainWindow);
    EndDrawing();
    EventLoop(mainWindow);
    }*/

  PieceChain* pc = CreatePieceChain("Lorem ipsum dolor sit amet");
  PieceChain_Insert(pc, "Addition");
  
  //printf("Character at position 0 : %c\n", CharacterAtPosition(pc, 0));

  printf("%s\n", pc->addBuffer);
  
  //PrintPieceChain(pc);
  
}
