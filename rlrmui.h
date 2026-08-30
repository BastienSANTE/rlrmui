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



// Forward declaration of structs
typedef struct Widget Widget;
typedef struct Window Window;
typedef struct Frame Frame;
typedef struct Button Button;
typedef struct ValueBox ValueBox;
typedef struct TextBox TextBox;
typedef struct Renderer Renderer;

// Macros
#define TOWIDGET(t) (Widget*)t


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
  DrawText(b->text,
	    w->bounds.x + b->padding,
	    w->bounds.y + b->padding, 20, BLUE);

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
/* Text Line : used inside of multiline text field*/

//-------------------------------------------------------------------------//
/* Multiline Text Field : Enables editing a string or multiple inside
   a rectangle.*/
typedef struct TextBox {
  Widget widget;
  char* text;
  int cursorX; int cursorY;
  int offsetX; int offsetY;
  // Visual
  // Only add lines that are visible on the screen,
  // lest tiny textboxes occupy all of memory.
  // 100 lines to test, making it dynamic after
  TextLine* lines;
  int _lineCount;
};

TextBox* CreateTextBox (char* text, int x, int y, int w, int h){
  TextBox* textbox = (TextBox*)malloc(sizeof(TextBox));
  SetWidgetBounds((Widget*)textbox, x, y, w, h);
  textbox->text = text;
  printf("Copied text to textbox\n");

  int initialLineCount = MeasureText(text, 12) / w;
  textbox->lines = malloc(initialLineCount * sizeof(TextLine));
  return textbox;  
}

void DrawTextBox(TextBox* textbox) {
  printf("Entering draw\n");
  Widget* w = &textbox->widget;

  printf("Casted\n");
  
  // Get the width of all text at once
  //int totalTextWidth = MeasureText(textbox->text, 12);

  for(int i = 0; i < textbox->_lineCount; i++){
    printf("Line %d has string %s\n", i, textbox->lines[i].text);
    DrawText(textbox->lines[i].text, 100, i * 12, 12, RED);
  }
}

void TextBox_SetText(TextBox *textBox) {
  
}

void TextBox_Resize(TextBox* textbox, int w, int h){
  textbox->widget.bounds.w = w;
  textbox->widget.bounds.h = h;
  textbox->_lineCount = 0;

  //Estimate text length
  int textLength = TextLength(textbox->text); //Raylib function
  int totalTextWidth = MeasureText(textbox->text, 12);
  int estimatedLineCount = (int)(totalTextWidth / w) + 1;

  //Add 1 line to the estimation for each newline
  for (int i = 0; i < textLength; i++) {
    if (textbox->text[i] == '\n') { estimatedLineCount++; }
  }
  
  printf("Estimating %d lines for resize\n", estimatedLineCount);

  //free(textbox->lines);
  textbox->lines = realloc(textbox->lines, estimatedLineCount * sizeof(TextLine));
  
  Font font = GetFontDefault(); //Will be replaced after

  int currentLine = 0;
  int lineStart = 0;
  int lineEnd = 0;

  float currentGlyphWidth = 0;
  float totalLineWidth = 0;

  // Almost copied from raylib example
  for (int i = 0; i < textLength; i++){
    //printf("Current byte %d\n", i);
    int codepointByteCount = 0;

    // Gets UTF8 codepoints instead of simply bytes.
    int codepoint = GetCodepoint(&textbox->text[i], &codepointByteCount);
    //printf("Got codepoint %d, is %c\n", codepoint, codepoint);
    int glyphIndex = GetGlyphIndex(font, codepoint);
    //printf("Got index %d\n", index);

    // We are advancing more than 1 byte at a time if we get UTF-8 text.
    // Since the default font is limited, replace invalid codepoints with
    // "?" and keep advancing 1 byte at a time.
    if (codepoint == 0x3f) codepointByteCount = 1;
    i += (codepointByteCount - 1); // i will advance by itself in next iter, dont accumulate offsets.

    currentGlyphWidth = GetGlyphAtlasRec(GetFontDefault(), codepoint).width;
    //printf("Glyph width is %f\n", currentGlyphWidth);
    totalLineWidth += currentGlyphWidth;

    //printf("Total line length is %f\n", totalLineWidth);

    // Follow line
    lineEnd = i;

    if (totalLineWidth >= textbox->widget.bounds.w || codepoint == '\n' || codepoint == 0) {
      printf("line is %f pixels wide\n", totalLineWidth);
      textbox->lines[currentLine].text = calloc((lineEnd - lineStart),  sizeof(char));
      textbox->lines[currentLine].text = strncpy(textbox->lines[currentLine].text, textbox->text + lineStart, (lineEnd - lineStart));

      // Set last char of text to null
      textbox->lines[currentLine].text[lineEnd - lineStart] = '\0';
      currentLine++;
      textbox->_lineCount++;
      lineStart = (codepoint == '\n' ? lineEnd + 1 : lineEnd);

      totalLineWidth = 0;
    } else {
      
    }
  }
}

