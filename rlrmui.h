#include "include/raylib.h"
#include <string.h>
#include "theme.h"

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

/* Float Rect : rectangle of 4*/
typedef struct FloatRect {
  float x; float y; float w; float h;
} FloatRect;

typedef struct TextLine {
  char* text;
  Rect bounds;
} TextLine;

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

// Load better font than the default one
Font defaultFont;


// Forward declaration of structs
typedef struct Widget Widget;
typedef struct Window Window;
typedef struct Frame Frame;
typedef struct Button Button;
typedef struct ValueBox ValueBox;
typedef struct TextBox TextBox;
typedef struct Renderer Renderer;

typedef struct TextPiece TextPiece;
typedef struct PieceChain PieceChain;

// Macros
#define TOWIDGET(t) (Widget*)t

// Print errors;
#define ERROR(...) fprintf(stderr, __VA_ARGS__);


// Function prototypes

// General purpose functions
bool RectContainsPoint(Rect r, int x, int y);


// Widget functions
void AddWidget(Frame* frame, Widget* widget);

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
  WidgetState lastState;
  Alignment alignment;

  //Event handling
  int (*eventHandler)(Widget* w);
  int (*onClick)(Widget* w);
  int (*onHover)(Widget* w);
  int (*onUnhover)(Widget*);
  int (*onKeyboard)(Widget* w);
};

void CreateWidget(Widget* widget) {
  Widget* w = (Widget*)malloc(sizeof(Widget));
  printf("Widget allocated, size %d\n", sizeof(Widget));
  widget->bounds = (Rect){0, 0, 0, 0};
  widget->parent = NULL;
  widget->children = NULL;
  return;
}

void SetWidgetBounds(Widget* widget, int x, int y, int w, int h){
  widget->bounds = (Rect){x, y, w, h};
}

void SetEventHandler(Widget* widget, int (*handler)(Widget* w)){
  widget->eventHandler = handler;
}

void SetOnClick(Widget* widget, int(*callback)(Widget* w)){
  widget->onClick = callback;
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
  Widget widget;
  bool root;
  Alignment alignment;
  // Private members, should not be set directly
  int _pixelW;   // Actual width & height in px
  int _pixelH;
};

Frame* CreateFrame(int x, int y, int w, int h, bool root) {
  Frame* f = (Frame*)malloc(sizeof(Frame));
  CreateWidget(&f->widget);
  SetWidgetBounds(&f->widget, x, y, w, h);
  f->root = root;
  SetEventHandler(&f->widget, FrameHandleEvents);
  //f->widget->eventHandler = &FrameHandleEvents;
  return f;
}

void AddWidget(Frame* frame, Widget* widget) {
  frame->widget.children = widget;
  widget->parent = (Widget*)frame;
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
  Frame* rootFrame;
  Rect bounds;
  UIState state;
  int mouseX; int mouseY;
  Widget* focusedWidget;       // Current target of mouse
  Widget* lastFocusedWidget;   // For unfocus events

  RenderTexture2D texture;
};

Window* CreateWindow(int w, int h) {
  Window* window = (Window*)malloc(sizeof(Window));
  window->bounds = (Rect){0, 0, w, h};
  window->state = NONE;
  window->focusedWidget = NULL;
  window->lastFocusedWidget = NULL;

  window->texture = LoadRenderTexture(w, h);
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
    window->focusedWidget->onClick(window->focusedWidget);
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

  Widget* w = TOWIDGET(window->rootFrame);
  
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
  Widget widget;
  char* text;
  int padding;
};

Button* CreateButton(char* text) {
  Button* b = (Button*)malloc(sizeof(Button));
  CreateWidget(&b->widget);
  b->text = text;
  SetWidgetBounds(&b->widget, 0, 0, MeasureText(b->text, 20), 20 );
  //SetEventHandler(&b->widget, ButtonHandleEvents);
  SetOnClick(&b->widget, ButtonHandleEvents);
  printf("Button at %x, %d, %d, %d, %d\n", b, b->widget.bounds.x, b->widget.bounds.y, b->widget.bounds.w, b->widget.bounds.h);

  return b;
}

void DrawButton (Button* b) {
  Widget* w = &b->widget;

  b->padding = 5;
  
  BeginScissorMode(w->parent->bounds.x,
		   w->parent->bounds.y,
		   w->parent->bounds.w,
		   w->parent->bounds.h);
  DrawTextEx(defaultFont, b->text,
	     (Vector2){w->bounds.x + b->padding,
      w->bounds.y + b->padding}, 12, 1, BLUE);

  DrawRectangleLines(w->bounds.x, w->bounds.y,
		      w->bounds.x + w->bounds.w + b->padding * 2,
		      w->bounds.y + w->bounds.h + b->padding * 2,
		      BLUE);

  //printf("Button at %x, %d, %d, %d, %d\n", &w, w->bounds.x, w->bounds.y, w->bounds.w, w->bounds.h);

   EndScissorMode();
  
  return;
};

int ButtonHandleEvents(Widget* w){
  Button* b = (Button*)w;  // Will it work ?
  
  switch (b->widget.state) {
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
/* Text Piece : used inside of piece tables for multiline text editing. The
   piece does not contain text, but points to a part of a text buffer
   bounded by start and end.*/
typedef struct TextPiece {
  char* source;
  int start;
  int length;
  TextPiece* prev;
  TextPiece* next;
};

typedef struct PieceChain {
  TextPiece* head;
  TextPiece* tail;
  char* baseBuffer;
  char* addBuffer;
  int pieceCount;
  int lastAddLocation;
};

TextPiece* PieceFromPosition(PieceChain* pc, int pos) {
  int totalLength = 0; // Total length of traversed string
  
  for (TextPiece* tp = pc->head; tp->next != NULL; tp = tp->next){ // Traverse piece chain
    
    if (totalLength + tp->length >= pos) {
      return tp; // Get character in right buffer
    }
  }

  ERROR("Invalid TextPiece position\n");
  return NULL;
}

char CharacterAtPosition(PieceChain* pc, int pos) {
  int totalLength = 0; // Total length of traversed string
  char c = 0;           // Character we will return
  
  for (TextPiece* tp = pc->head; tp->next != NULL; tp = tp->next){ // Traverse piece chain
    
    if (totalLength + tp->length >= pos) {
      c = *(tp->source + pos - tp->start);
      return c; // Get character in right buffer
    } else {
      totalLength += tp->length;
    }
  }

  ERROR("Invalid character position\n");
  return 0;
}

int OffsetInPiece(PieceChain* pc, int pos){
  TextPiece* tp = pc->head;
  int p = pos;
  while ( p -= tp->length >= 0 && tp->next != pc->tail){
    tp = tp->next;
  }

  return (pos - p); // Return relative offset in piece
}


/*Deletes a span from the "final text". This function calculates
 the necessary edits to the piece chain*/
void PieceChain_Delete(PieceChain* pc, int start, int end) {
  
  
}

void PieceChain_Insert(PieceChain* pc, char* text, int pos) {

  // First, create new text piece;
  TextPiece* tp = malloc(sizeof(TextPiece));
  printf("Allocated text piece at %x\n" ,&tp);
  tp->source = pc->addBuffer;
  tp->start = pc->lastAddLocation;
  tp->next = NULL;
  
  // Copy new text into correct buffer
  int len = strlen(text);
  printf("string is %d long\n", len);
  if(pc->lastAddLocation + len > (sizeof(pc->addBuffer))) {
    pc->addBuffer = realloc(pc->addBuffer, sizeof(pc->addBuffer) + len);
  }
  
  strncpy(tp->source + pc->lastAddLocation, text, len);
  printf("copied string to buffer at position %d, length %d\n", pc->lastAddLocation, len);
  pc->lastAddLocation += len;
  
  pc->pieceCount++;
  
  // In which piece does insert begin ?
  TextPiece* startPiece = PieceFromPosition(pc, pos);
  int insOffset = OffsetInPiece(pc, pos);
  
  // Calculate character offset from beginning of piece
  printf("Split at char %d of text piece, %s\n", insOffset);


  
  // Split last text piece to insert new text
  startPiece->length -= (startPiece->length - insOffset);
  printf("Start piece shrunk to %d\n", startPiece->length);

  // Did the 2 parts originate in the same buffer ?
  // If so, make new TextPiece with remained of original
  // piece after insertion point
  /*if(startPiece->next !=  NULL) {
    
    }*/

  pc->head = realloc(pc->head, pc->pieceCount * sizeof(TextPiece*));
  pc->head[pc->pieceCount - 1] = *tp;
	 //pc->head[pc->pieceCount - 2].next = tp;

}

void PrintPieceChain(PieceChain* pc) {
  for (TextPiece* tp = pc->head; tp->next != pc->tail; tp = tp->next) {
    memcpy(buf, tp->source + tp->start, tp->length);
    printf("%x | %d, %d", &tp->source, tp->start, tp->length);
    printf("\n");
  }
}

PieceChain* CreatePieceChain(char* text) {
  PieceChain* pc = malloc(sizeof(PieceChain));
  pc->baseBuffer = calloc(1, strlen(text));
  pc->addBuffer = calloc(1, 512);
  pc->pieceCount = 0;

  // Fill read-only base buffer
  strncpy(pc->baseBuffer, text, strlen(text));

  // Create head and tail pieces to delimit array, then set their
  // members to impossible values
  pc->head = malloc(sizeof(TextPiece));
  pc->tail = malloc(sizeof(TextPiece));
  pc->head->prev = NULL; pc->tail->next = NULL;
  pc->head->source = NULL; pc->tail->source = NULL;
  pc->head->start = 0; pc->tail->start = 0;
  pc->head->length = 0; pc->tail->length = 0;

  TextPiece* fp = malloc(sizeof(TextPiece));
  fp->source = pc->baseBuffer;
  fp->start = 0; fp->length = strlen(text);
  fp->prev = pc->head;
  fp->next = pc->tail;
  
  printf("%s\n", pc->baseBuffer);
  

  return pc;
}

//-------------------------------------------------------------------------//
/* Text Box : Enables editing a string or multiple inside
   a rectangle.*/

/*
typedef struct TextBox {
  Widget widget;
  PieceChain pc;
  int cursorX; int cursorY;
  int offsetX; int offsetY;
  PieceChain* pt;
  int _lineCount;

  bool readOnly;
  bool wrapLines;
};

TextBox* CreateTextBox (char* text, int x, int y, int w, int h, bool ro, bool wl){
  TextBox* textbox = (TextBox*)malloc(sizeof(TextBox));
  SetWidgetBounds((Widget*)textbox, x, y, w, h);

  //Copy original text to buffer
  textbox->pc.baseBuffer = strdup(text);

  textbox->readOnly = ro;
  textbox->wrapLines = wl;
  return textbox;  
}

void TextBox_SetText(TextBox *textBox) {
  
}

void TextBox_Draw(TextBox* textBox) {
  
}
*/
