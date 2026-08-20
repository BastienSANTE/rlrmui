#include "include/raylib.h"

//*** RLRMUI ***// Raylib retained-mode UI
// This is a for-fun project. it is not production-quality code
//(far from there)

/*-------------------------------------------------------o
|          FORWARD DECLARATIONS AND LIB FUNCTIONS        |$
o--------------------------------------------------------o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

/* Rect : Integer rectangle, as opposed to raylib's float Rectangle*/
typedef struct Rect {
  int x; int y; int w; int h;
};

// After looking at the code for Luigi(2), I might use bitfields to
// contain widget properties

typedef enum Alignment {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  CENTER
} Alignment;

typedef enum WidgetState {
  INACTIVE,
  ACTIVE,
  HOVERED,
  CLICKED,
  RELEASED
} WidgetState ;

typedef enum WidgetType {
  BUTTON = 0,
  LABEL,
  VALUEBOX
};


// Idea : why not treat the UI as a state machine ?
// The state machine is managed by the window, and transmits events to child widgets
typedef enum UIState {
  NONE = 0,
  HOVER,
  CLICK,
  DRAG,
  SCROLL,
  RESIZE,
  KEYPRESS,
  KEYPRESS_MOD
  // Probably other states coming
};



// Forward declaration of structs
typedef struct Window Window;
typedef struct Widget Widget;
typedef struct Frame Frame;
typedef struct Button Button;
typedef struct ValueBox ValueBox;

// Function prototypes

// General purpose functions
void AddWidget();
void EventLoop (Window* w);
void SetRootFrame(Frame* frame);
void DrawButton (Button* b);
void DrawValueBox (ValueBox* b);
void SetValue(ValueBox* b, char* text);

/*-------------------------------------------------------o
|                     IMPLEMENTATION                     |$
o--------------------------------------------------------o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/


//-------------------------------------------------------------------------//
/* Window : Base container for all underlying UI
   Contains the root layout struct, which keeps the
   rest of the child elements. Its draw command redraws the entire window.
   Used when resizing, or when significantly modifying the layout. */
typedef struct Window {
  Frame* rootFrame = NULL;
  UIState state = NONE;
  int mouseX; int mouseY;
};

Window* CreateWindow() {
  Window* w = (Window*)malloc(sizeof(Window));
  w->state = NONE;
  return w;
}

void SetRootFrame(Window* w, Frame* f){
  w->rootFrame = f;
}

void EventLoop(Window* w) {
  if (!IsWindowFocused()) {
    w->state = NONE;
  }

  if (IsMouseButtonPressed(1)) {
    w->state = CLICK;
    printf("CLICK\n");
  }

  if (IsMouseButtonDown(1)) {
    w->state = CLICK;
    printf("DRAG\n");
  }

  if (GetMouseWheelMove()) {
    w->state = SCROLL;
    printf("SCROLL\n");
  }


  printf("%d, %d\n", GetMouseX(), GetMouseY()); 
   if (IsWindowResized()) {
     w->state = RESIZE;
     printf("RESIZE\n");
  }
}

//-------------------------------------------------------------------------//
/* Widget : Base structure of all widgets. */
typedef struct Widget {
  Frame* parent;
  Rect bounds;             // Actual bounds
  Rect clip;               // Clipping rectangle (usually the parent)
  bool active;
  WidgetState state;
  void (*draw)(Widget *w);
};




//-------------------------------------------------------------------------//
/* Frame : Contains Widgets, or other Layouts. Stretches based on
   its alignment (not yet implemented). Layout uses the float rectangle to
be able to hae precise measurements*/
typedef struct Frame {
  Rectangle bounds;
  bool root = false;
  Frame* parent = NULL;
  Frame* children = NULL;
  Widget* widgets = NULL;

  // Private members, should not be set directly
  int _pixelW;   // Actual width & height in px
  int _pixelH;
};

Frame* CreateFrame(int x, int y, int w, int h, bool root) {
  Frame* f = (Frame*)malloc(sizeof(Frame));
  f->bounds = (Rectangle){x, y, w, h};
  f->root = root;
  return f;
}

void AddWidget(Frame* f, Widget* w) {
  f->widgets = w;
}

//-------------------------------------------------------------------------//
// Label - Simple text, non-interactable except by code
typedef struct Label {
  Widget* w;
  char* text;
  int fontSize;
};

//-------------------------------------------------------------------------//
/* Button : can be hovered, pressed and held */ 
typedef struct Button {
  Widget w;
  char* text;
  int padding = 5;

  // Functions
  void(*draw)(Button* b) = &DrawButton;
};

Button* CreateButton(char* text) {
  Button* b = (Button*)malloc(sizeof(Button));
  b->text = text;
  return b;
}

void DrawButton (Button* b) {
   b->w.bounds.w = MeasureText(b->text, 10); // Calculate width of text at 10px
   b->w.bounds.h = 12;                    // Arbitrary for the moment
   DrawText(b->text,
	    b->w.bounds.x + b->padding,
	    b->w.bounds.y + b->padding, 10, BLUE);

   DrawRectangleLines(b->w.bounds.x, b->w.bounds.y,
		      b->w.bounds.x + b->w.bounds.w + b->padding, b->w.bounds.y + b->padding,
		      BLUE);
   printf("Drew button\n");
   printf("button is at %d, %d, w=%d, h=%d",
	  b->w.bounds.x, b->w.bounds.y, b->w.bounds.w, b->w.bounds.h);
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
  b->w.bounds.w = textSize.x;
  b->w.bounds.h = textSize.y;                   // Arbitrary for the moment
  DrawText(b->text,
	   b->w.bounds.x + b->padding,
	   b->w.bounds.y + b->padding, b->fontSize, BLUE);

  DrawRectangleLines(b->w.bounds.x, b->w.bounds.y,
		     b->w.bounds.x + b->w.bounds.w + b->padding, b->w.bounds.y + b->padding,
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

// Function prototype to find a widget with mouse position

Widget* FindWidgetByMouse(Window *win);
