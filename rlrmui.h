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
} Rect;

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
} WidgetState;

typedef enum WidgetType {
  BUTTON = 0,
  LABEL,
  VALUEBOX
} WidgetType;


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
} UIState;



// Forward declaration of structs
typedef struct Widget Widget;
typedef struct Window Window;
typedef struct Frame Frame;
typedef struct Button Button;
typedef struct ValueBox ValueBox;

// Function prototypes

// General purpose functions
bool RectContainsPoint(Rect r, int x, int y);


// Widget functions
void AddWidget();

// Input events
void EventLoop (Window* w);
void SetRootFrame(Window* w, Frame* frame);
Widget* FindWidgetByMousePosition(Window* w);

// Frame
int FrameHandleEvents(Widget* w);

// Button
void DrawButton (Button* b);
int ButtonHandleEvents(Widget* w);

// Value Box
void DrawValueBox (ValueBox* b);
void SetValue(ValueBox* b, char* text);


/*-------------------------------------------------------o
|                     IMPLEMENTATION                     |$
o--------------------------------------------------------o$
 $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

//-------------------------------------------------------------------------//
/* General purpose functions
   Only slightly related to the UI, but are helpers for cumbersome calculations
   and boilerplate to speed up code writing */
bool RectContainsPoint(Rect r, int x, int y) {
  //Damn perfect fit into 70 chars (why not 80 as default, Emacs ?)
  return (x > r.x) && (x < r.x + r.w) && (y > r.y) && (y < r.y + r.h);
}


//-------------------------------------------------------------------------//
/* Widget : Base structure of all widgets. */
typedef struct Widget {
  Rect bounds;             // Actual bounds
  //Rect clip;               // Clipping rectangle (usually the parent)
  bool active;
  Widget* parent;
  Widget* children;
  WidgetState state;
  Alignment alignment;

  //Event handling
  int (*eventHandler)(Widget* w);
};

Widget* CreateWidget() {
  Widget* w = (Widget*)malloc(sizeof(Widget));
  printf("Widget allocated, size %d\n", sizeof(Widget));
  w->bounds = (Rect){0, 0, 0, 0};
  w->parent = NULL; w->children = NULL;
  return w;
}

void SetBounds(Widget* widget, int x, int y, int w, int h){
  widget->bounds = (Rect){x, y, w, h};
}

Widget* SearchPointInChildren(Widget* w, int x, int y){
  printf("Begin search in widget %x\n", &w);
  if(w->children == NULL) {
    printf("Widget %x has no children\n", &w);

    return w;
  } else {
    Widget* c = w->children;
    if (RectContainsPoint(c->bounds, x, y)){
      printf("Child widget %x contains cursor\n", &w->children);
      w = SearchPointInChildren(w->children, x, y);
    }
    return w;
  }
  return NULL;
}

//-------------------------------------------------------------------------//
/* Frame : Contains Widgets, or other Layouts. Stretches based on
   its alignment (not yet implemented). Layout uses the float rectangle to
be able to hae precise measurements*/
typedef struct Frame {
  Widget* widget;
  bool root;
  // Private members, should not be set directly
  int _pixelW;   // Actual width & height in px
  int _pixelH;
};

Frame* CreateFrame(int x, int y, int w, int h, bool root) {
  Frame* f = (Frame*)malloc(sizeof(Frame));
  f->widget = CreateWidget();
  f->widget->bounds = (Rect){x, y, w, h};
  f->root = root;
  f->widget->eventHandler = &FrameHandleEvents;
  return f;
}

void AddWidget(Frame* frame, Widget* widget) {
  frame->widget->children = widget;
  widget->parent = frame->widget;
}

int FrameHandleEvents(Widget* widget){
  Frame* frame = (Frame*)widget; 
  printf("Frame event!, root = %d\n", frame->root ? 1 : 0);
  return 0;
}

//-------------------------------------------------------------------------//
/* Window : Base container for all underlying UI
   Contains the root layout struct, which keeps the
   rest of the child elements. Its draw command redraws the entire window.
   Used when resizing, or when significantly modifying the layout. */
typedef struct Window {
  Rect bounds;
  Frame* rootFrame;
  UIState state;
  int mouseX; int mouseY;
  Widget* focusedWidget;       // Current target of mouse
  Widget* lastFocusedWidget;   // For unfocus events
};

Window* CreateWindow(int w, int h) {
  Window* window = (Window*)malloc(sizeof(Window));
  window->bounds = (Rect){0, 0, w, h};
  window->state = NONE;
  window->focusedWidget = NULL;
  window->lastFocusedWidget = NULL;
  return window;
}

void ResizeWindow(Window* window) {
  window->bounds = (Rect){0, 0, GetRenderWidth(), GetRenderHeight()};
}

void SetRootFrame(Window* window, Frame* frame){
  window->rootFrame = frame;
}

void EventLoop(Window* window) {
  window->mouseX = GetMouseX(); window->mouseY = GetMouseY();
  if(!RectContainsPoint(window->bounds, window->mouseX, window->mouseY)){
    return;
  }
   
  if (!IsCursorOnScreen()) {
    window->state = NONE;
    return;
  }

  window->focusedWidget = FindWidgetByMousePosition(window);

  if(window->focusedWidget) {
    printf("Window has a focused widget %x\n", &window->focusedWidget);
    window->focusedWidget->state = HOVERED;
  }
  
  if (IsMouseButtonPressed(1)) {
    window->state = CLICK;
    window->focusedWidget->state = CLICKED;
    printf("CLICK\n");
  }

  if (IsMouseButtonDown(1)) {
    window->state = CLICK;
    printf("DRAG\n");
  }

  if (GetMouseWheelMove()) {
    window->state = SCROLL;
    printf("SCROLL\n");
  }

  
  if (IsWindowResized()) {
    window->state = RESIZE;
    ResizeWindow(window);
    printf("RESIZE\n");
  }

  if(window->focusedWidget->eventHandler != NULL) {
    window->focusedWidget->eventHandler(window->focusedWidget);
  }
  printf("%x\n", &window->focusedWidget);
}

// Goes through the UI tree to find the corresponding widget
// TODO : Make Frames into widgets for more compact traversal code
// Maybe luigi was right about this
Widget* FindWidgetByMousePosition(Window* window){
  if (window->rootFrame == NULL) return NULL;

  Widget* w = window->rootFrame->widget;
  
  if(!RectContainsPoint(w->bounds, window->mouseX, window->mouseY)) {
    printf("Mouse at %d, %d outside of frame at %d, %d, %d, %d\n",
	   window->mouseX, window->mouseY,
	   w->bounds.x, w->bounds.y, w->bounds.w, w->bounds.h);
    return NULL;
  } else {  
    return SearchPointInChildren(w, window->mouseX, window->mouseY);
  }
}


void DEBUG_DrawFocus(Window* window) {
  if (window->focusedWidget != NULL) {
    Widget* f = window->focusedWidget;
    DrawRectangleLines(f->bounds.x, f->bounds.y,
		       f->bounds.x + f->bounds.w,
		       f->bounds.y + f->bounds.h,
		       RED);
		      }
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
  Widget* widget;
  char* text;
  int padding;
};

Button* CreateButton(char* text) {
  Button* b = (Button*)malloc(sizeof(Button));
  b->widget = CreateWidget();
  
  b->widget->eventHandler = &ButtonHandleEvents;
  b->text = text;
  b->widget->bounds = (Rect){0, 0, MeasureText(b->text, 20), 20 };
  return b;
}

void DrawButton (Button* b) {
  Widget* w = b->widget;

  b->padding = 5;
  
  BeginScissorMode(w->parent->bounds.x,
		   w->parent->bounds.y,
		   w->parent->bounds.w,
		   w->parent->bounds.h);
  DrawText(b->text,
	    w->bounds.x + b->padding,
	    w->bounds.y + b->padding, 20, BLUE);

  DrawRectangleLines(w->bounds.x, w->bounds.y,
		      w->bounds.x + w->bounds.w + b->padding * 2,
		      w->bounds.y + w->bounds.h + b->padding * 2,
		      BLUE);

   EndScissorMode();
  
  return;
};

int ButtonHandleEvents(Widget* w){
  Button* b = (Button*)w;  // Will it work ?
  
  switch (b->widget->state) {
  case HOVERED:
    b->padding = 3;
    b->text = "A";
    break;

  case CLICKED:
    b->text = "Clicked!";
    break;

  default:
    break;
  }

  return 0;
}

//-------------------------------------------------------------------------//
// Value Box - cannot be interacted with except by code yet
