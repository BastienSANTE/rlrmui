#include "raylib.h"

typedef enum {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  CENTER
} Alignment;

enum WidgetState {
  INACTIVE,
  ACTIVE,
  HOVERED,
  CLICKED,
  RELEASED
};

typedef struct Frame {
  int id;
  float x;    // Starting position x and y
  float y;
  float w;    // Fraction of parent filled
  float h;
  bool root = false;
  Frame* parent = NULL;
  Frame* children = NULL;

  // Private members, should not be set directly
  int _pixelW;
  int _pixelH;
};

/*
struct Button {
  char* text;
  int x;    // Starting position x and y
  int y;
  int w;    // Fraction of parent filled
  int h;
  int padding;
  
  void Draw() {
    w = MeasureText(text, 10); // Calculate width of text at 10px
    h = 12;                    // Arbitrary for the moment
    DrawText(text, x + padding, y + padding, 10, BLUE);
    DrawRectangleLines(x, y, x + w + padding, y + padding, BLUE);
  }
  };*/

typedef struct Button {
  char* text;
  int x;    // Starting position x and y
  int y;
  int w;    // Fraction of parent filled
  int h;
  int padding;

  // Draw function
  void(*draw)(Button* b);
};

void DrawButton (Button* b) {
   b->w = MeasureText(b->text, 10); // Calculate width of text at 10px
   b->h = 12;                    // Arbitrary for the moment
   DrawText(b->text,
	    b->x + b->padding,
	    b->y + b->padding, 10, BLUE);

   DrawRectangleLines(b->x, b->y, b->x + b->w + b->padding, b->y + b->padding, BLUE);
   return;
};



void SetParent(Frame* f, Frame* p){
  if (f->root) return;
  f->parent = p;
  return;
}

// Add child frame
void AddChild(Frame* f, Frame* c){
  f->children = c;

  SetParent(c, f);
  return;
}
