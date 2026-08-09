#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
//#include <iostream>
#include "raylib.h"
#include "rlrmui.h"


void DrawLayout(Frame* f){

printf("frame %d has dimensions %d, %d, %f, %f, root=%d\n, parent = %#x, children = %#x\n", f->id, f->x, f->y, f->_pixelW, f->_pixelH, f->root, f->parent, f->children);
  
 if (f->root) {
    f->x = 0; f->y = 0;
    f->w = 1.0; f->h = 1.0;

    // Make root frame fill entire screen (this might be optional later)
    f->_pixelW = f->w * GetScreenWidth();
    f->_pixelH = f->h * GetScreenHeight();

 }

 if (f->parent != NULL) {
    // Make root frame fill defined secition of parent (this might be optional later)
    f->_pixelW = f->w * f->parent->_pixelW;
    f->_pixelH = f->h * f->parent->_pixelH;
    DrawRectangleLines(f->x, f->y, f->_pixelW, f->_pixelH, BLUE);
 }
  
  if (f->children != NULL){
    printf("Descending into cild frame\n");
    DrawRectangleLines(f->x, f->y, f->_pixelW, f->_pixelH, BLUE);
    f = f->children;
    DrawLayout(f);

  }
  
  DrawRectangleLines(f->x, f->y, f->_pixelW, f->_pixelH, BLUE);
}


int main(void) {

  int screenWidth = 500;
  int screenHeight = 400;

  Frame f = {1, 0, 0, screenWidth, screenHeight, true, NULL, NULL};
  Frame cf = {2, 0, 0, 0.5, 0.5, false, NULL, NULL};
  AddChild(&f, &cf);
  
  //SetConfigFlags(FLAG_VSYNC_HINT);
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);

  InitWindow(screenWidth, screenHeight, "layout test");
  SetTargetFPS(0);

  while(!WindowShouldClose()){

    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawLayout(&f);

    //sleep(3);
    system("clear");

    EndDrawing();
  }

}

  
