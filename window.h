#include "rlrmui.h"

/*typedef struct Window Window;
void RedrawWindow(Window *w);

typedef struct Window {
  Frame* rootFrame;
  }*/

typedef struct Theme {
  Color BG = {16, 16, 16, 255};
};

Theme theme;

/*-----------------*
| Window functions |
-------------------*/
void DrawBG() {
  ClearBackground(theme.BG);
}

void UpdateWindow() {
  
}
