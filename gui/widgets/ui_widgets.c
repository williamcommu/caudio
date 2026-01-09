// UI Widgets implementation - buttons, sliders, text rendering
#include "ui_widgets.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

SDL_Color COLOR_BG = {30, 30, 35, 255};
SDL_Color COLOR_PANEL = {50, 50, 55, 255};
SDL_Color COLOR_BUTTON = {70, 130, 180, 255};
SDL_Color COLOR_BUTTON_HOVER = {100, 150, 200, 255};
SDL_Color COLOR_BUTTON_ACTIVE = {50, 100, 150, 255};
SDL_Color COLOR_SLIDER = {100, 100, 100, 255};
SDL_Color COLOR_SLIDER_ACTIVE = {150, 150, 150, 255};
SDL_Color COLOR_TEXT = {220, 220, 220, 255};
SDL_Color COLOR_ACCENT = {255, 165, 0, 255};

// Simple bitmap font data for basic ASCII characters (8x8 pixels)
static const unsigned char font_data[][8] = {
    // Space (32)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ! (33)
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
    // " (34) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // # (35) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // $ (36) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // % (37) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // & (38) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ' (39) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ( (40) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // ) (41) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // * (42) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // + (43) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // , (44) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    // - (45)
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00},
    // . (46)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18},
    // / (47)
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x00},
    // 0-9 (48-57)
    {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00}, // 0
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // 1
    {0x3C, 0x66, 0x06, 0x1C, 0x30, 0x60, 0x7E, 0x00}, // 2
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, // 3
    {0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00}, // 4
    {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, // 5
    {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, // 6
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}, // 7
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, // 8
    {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00}, // 9
    // : (58)
    {0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00},
};

// Extended font data for uppercase letters A-Z (65-90)
static const unsigned char font_letters[][8] = {
    {0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // A
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, // B
    {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, // C
    {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, // D
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}, // E
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}, // F
    {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3E, 0x00}, // G
    {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, // H
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, // I
    {0x3E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00}, // J
    {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, // K
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, // L
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, // M
    {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}, // N
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // O
    {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, // P
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x0E, 0x00}, // Q
    {0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00}, // R
    {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, // S
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, // U
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // W
    {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, // X
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, // Y
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}, // Z
};

// Draw a single character at given position
void draw_char(SDL_Renderer* renderer, int x, int y, char c, int color_r, int color_g, int color_b) {
    const unsigned char* char_data = NULL;
    
    if (c >= 32 && c <= 58) { // Space through :
        char_data = font_data[c - 32];
    } else if (c >= 65 && c <= 90) { // A-Z
        char_data = font_letters[c - 65];
    } else if (c >= 97 && c <= 122) { // a-z (convert to uppercase)
        char_data = font_letters[c - 97];
    }
    
    if (!char_data) return;
    
    SDL_SetRenderDrawColor(renderer, color_r, color_g, color_b, 255);
    
    // Draw 8x8 bitmap
    for (int row = 0; row < 8; row++) {
        unsigned char line = char_data[row];
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                SDL_Rect pixel = {x + col, y + row, 1, 1};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

// Draw text string
void draw_text(SDL_Renderer* renderer, int x, int y, const char* text) {
    draw_text_colored(renderer, x, y, text, 220, 220, 220);
}

// Draw colored text string
void draw_text_colored(SDL_Renderer* renderer, int x, int y, const char* text, int r, int g, int b) {
    int char_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(renderer, char_x, y, text[i], r, g, b);
        char_x += 9; // 8 pixel width + 1 pixel spacing
    }
}

// Simple button rendering
int draw_button(SDL_Renderer* renderer, Button* btn, int mouse_x, int mouse_y, int mouse_pressed) {
    static Button* last_pressed_button = NULL;
    
    btn->hover = (mouse_x >= btn->x && mouse_x < btn->x + btn->width &&
                  mouse_y >= btn->y && mouse_y < btn->y + btn->height);
    
    int clicked = 0;
    
    // Only register click on button press (not hold)
    if (btn->hover && mouse_pressed && !btn->pressed && last_pressed_button != btn) {
        clicked = 1;
        last_pressed_button = btn;
    }
    
    // Reset when mouse is released
    if (!mouse_pressed) {
        last_pressed_button = NULL;
    }
    
    btn->pressed = btn->hover && mouse_pressed;
    
    // Choose color
    SDL_Color color;
    if (btn->pressed) {
        color = COLOR_BUTTON_ACTIVE;
    } else if (btn->hover) {
        color = COLOR_BUTTON_HOVER;
    } else {
        color = COLOR_BUTTON;
    }
    
    // Draw button
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {btn->x, btn->y, btn->width, btn->height};
    SDL_RenderFillRect(renderer, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(renderer, &rect);
    
    // Draw centered text
    int text_width = strlen(btn->text) * 9; // 8 + 1 spacing
    int text_height = 8;
    int text_x = btn->x + (btn->width - text_width) / 2;
    int text_y = btn->y + (btn->height - text_height) / 2;
    
    // Choose text color based on button state
    if (btn->pressed) {
        draw_text_colored(renderer, text_x, text_y, btn->text, 255, 255, 255); // White
    } else {
        draw_text_colored(renderer, text_x, text_y, btn->text, 220, 220, 220); // Light gray
    }
    
    return clicked;
}

// Simple slider rendering
int draw_slider(SDL_Renderer* renderer, Slider* slider, int mouse_x, int mouse_y, int mouse_pressed) {
    static Slider* active_slider = NULL;
    
    int in_slider = (mouse_x >= slider->x && mouse_x < slider->x + slider->width &&
                     mouse_y >= slider->y && mouse_y < slider->y + slider->height);
    
    if (mouse_pressed && in_slider && !active_slider) {
        active_slider = slider;
        slider->dragging = 1;
    }
    
    if (!mouse_pressed) {
        active_slider = NULL;
        slider->dragging = 0;
    }
    
    if (slider->dragging && active_slider == slider) {
        float normalized = (float)(mouse_x - slider->x) / slider->width;
        normalized = fmax(0.0f, fmin(1.0f, normalized));
        slider->value = slider->min + normalized * (slider->max - slider->min);
    }
    
    // Draw slider track
    SDL_Color track_color = COLOR_SLIDER;
    SDL_SetRenderDrawColor(renderer, track_color.r, track_color.g, track_color.b, track_color.a);
    SDL_Rect track = {slider->x, slider->y + slider->height/3, slider->width, slider->height/3};
    SDL_RenderFillRect(renderer, &track);
    
    // Draw slider handle
    float normalized = (slider->value - slider->min) / (slider->max - slider->min);
    int handle_x = slider->x + (int)(normalized * slider->width) - 5;
    
    SDL_Color handle_color = slider->dragging ? COLOR_SLIDER_ACTIVE : COLOR_ACCENT;
    SDL_SetRenderDrawColor(renderer, handle_color.r, handle_color.g, handle_color.b, handle_color.a);
    SDL_Rect handle = {handle_x, slider->y, 10, slider->height};
    SDL_RenderFillRect(renderer, &handle);
    
    // Draw value text above slider (but not for position slider, which shows time separately)
    if (strcmp(slider->label, "Position") != 0) {
        char value_text[32];
        snprintf(value_text, sizeof(value_text), "%.2f", slider->value);
        int text_width = strlen(value_text) * 9;
        int value_x = slider->x + (slider->width - text_width) / 2;
        int value_y = slider->y - 12; // Above the slider
        draw_text_colored(renderer, value_x, value_y, value_text, 180, 180, 180);
    }
    
    return slider->dragging;
}
