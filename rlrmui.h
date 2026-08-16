#include "raylib.h"

//*** RLRMUI **// Raylib retained-mode UI
// This is a for-fun project. it is not production-quality code (far from there)

/*-------------------------------------------------------o
|          FORWARD DECLARATIONS AND LIB FUNCTIONS        |$
o--------------------------------------------------------o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

/* Rect : Integer rectangle, as opposed to raylib's float Rectangle*/
typedef struct Rect {
  int x; int y; int w; int h;
}

typedef enum Alignment {
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
} WidgetState ;

enum WidgetType {
  BUTTON = 0,
  LABEL,
  VALUEBOX,
  
}


// Forward declaration of structs
typedef struct Window Window;
typedef struct Widget Widget;
typedef struct Frame Frame;
typedef struct Button Button;
typedef struct ValueBox ValueBox;

// Function prototypes
void SetRootFrame(Frame* frame);
void DrawButton (Button* b);
void DrawValueBox (ValueBox* b);
void SetValue(ValueBox* b, char* text);

/*-------------------------------------------------------o
|                     IMPLEMENTATION                     |$
o--------------------------------------------------------o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

#ifndef RLRMUI_IMPLEMENTATION
#define RLRMUI_IMPLEMENTATION

/* Window : Base container for all underlying UI
   Contains the root layout struct, which keeps the
   rest of the child elements. Its draw command redraws the entire window.
   Used when resizing, or when significantly modifying the layout. */
typedef struct Window {
  Frame* rootFrame;

  void (*setRootFrame)(Frame* f);
};

/* Widget : Base structure of all widgets. */
typedef struct Widget {
  Frame* parent;
  Rect bounds;
  bool active;
  void *
  void (*draw)(Widget *w);
}

/* Layout : Contains Widgets, or other Layouts. Stretches based on
   its alignment (not yet implemented). Layout uses the float rectangle to
be able to hae precise measurements*/
typedef struct Frame {
  Rectangle bounds;
  bool root = false;
  Frame* parent = NULL;
  Frame* children = NULL;

  // Private members, should not be set directly
  int _pixelW;   // Actual width & height in px
  int _pixelH;
};

//-------------------------------------------------------------------------//
// Label - Simple text, non-interactable except by code
typedef struct Label {
  char* text;
  int fontSize;
}

//-------------------------------------------------------------------------//
/* Button : can be hovered, pressed and held */ 
typedef struct Button {
  Widget w;
  char* text;
  int padding;

  // Functions
  void(*draw)(Button* b) = &DrawButton;
};

void DrawButton (Button* b) {
  
   b->w.w = MeasureText(b->text, 10); // Calculate width of text at 10px
   b->w.h = 12;                    // Arbitrary for the moment
   DrawText(b->text,
	    b->w.x + b->padding,
	    b->w.y + b->padding, 10, BLUE);

   DrawRectangleLines(b->w.x, b->w.y,
		      b->w.x + b->w.w + b->padding, b->w.y + b->padding,
		      BLUE);
   return;
};

//-------------------------------------------------------------------------//
// Value Box - cannot be interacted with except by code yet
typedef struct ValueBox {
  Widget w;
  char* text;
  int padding;
  int fontSize;

  // Draw function
  void(*draw)(ValueBox* b) = &DrawValueBox;
  void(*setValue)(ValueBox* b, char* text) = &SetValue;
};

void DrawValueBox(ValueBox* b) {
  Vector2 textSize = MeasureTextEx(GetFontDefault(), b->text, b->fontSize, 1);
  b->w = textSize.x;
  b->h = textSize.y;                   // Arbitrary for the moment
  DrawText(b->text,
	   b->x + b->padding,
	   b->y + b->padding, b->fontSize, BLUE);

  DrawRectangleLines(b->x, b->y,
		     b->x + b->w + b->padding, b->y + b->padding,
		     BLUE);
}

void SetValue(ValueBox* b, char* text) {
  b->text = text;
  b->draw(b);
}

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


/*--------------*
| Window events |
*---------------*/

void ProcessWindowEvents(){
  if (IsWindowResized()) {
    printf("Window Resize\n");
  }
  if (IsWindowFocused()) {
    printf("Window focus\n");
  }
}
