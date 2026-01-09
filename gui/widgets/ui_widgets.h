#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <SDL2/SDL.h>

// UI element structures
typedef struct {
    int x, y, width, height;
    int hover, pressed;
    char text[64];
} Button;

typedef struct {
    int x, y, width, height;
    float value;
    float min, max;
    int dragging;
    char label[64];
} Slider;

// Color definitions
extern SDL_Color COLOR_BG;
extern SDL_Color COLOR_PANEL;
extern SDL_Color COLOR_BUTTON;
extern SDL_Color COLOR_BUTTON_HOVER;
extern SDL_Color COLOR_BUTTON_ACTIVE;
extern SDL_Color COLOR_SLIDER;
extern SDL_Color COLOR_SLIDER_ACTIVE;
extern SDL_Color COLOR_TEXT;
extern SDL_Color COLOR_ACCENT;

// Text rendering functions
void draw_char(SDL_Renderer* renderer, int x, int y, char c, int color_r, int color_g, int color_b);
void draw_text(SDL_Renderer* renderer, int x, int y, const char* text);
void draw_text_colored(SDL_Renderer* renderer, int x, int y, const char* text, int r, int g, int b);

// Widget drawing functions
int draw_button(SDL_Renderer* renderer, Button* btn, int mouse_x, int mouse_y, int mouse_pressed);
int draw_slider(SDL_Renderer* renderer, Slider* slider, int mouse_x, int mouse_y, int mouse_pressed);

#endif // UI_WIDGETS_H
