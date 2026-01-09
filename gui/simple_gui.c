// Simple GUI implementation using SDL2 and basic rendering
// This is a minimal GUI - for full ImGui, you'd need to add ImGui libraries

#define _GNU_SOURCE
#include "audio_mixer_gui.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// GUI state
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static int window_width = 1200;
static int window_height = 800;
static int running = 1;

// File dialog function
char* open_file_dialog(void) {
    static char selected_file[512] = {0};
    selected_file[0] = '\0';
    
    // Try different file dialog methods based on available tools
    FILE* fp = NULL;
    
    // Method 1: Try zenity (GNOME)
    fp = popen("zenity --file-selection --title=\"Select Audio File\" "
               "--file-filter=\"Audio files | *.wav *.mp3 *.flac *.ogg\" "
               "--file-filter=\"WAV files | *.wav\" "
               "--file-filter=\"All files | *\" 2>/dev/null", "r");
    
    if (fp && fgets(selected_file, sizeof(selected_file), fp)) {
        // Remove newline
        size_t len = strlen(selected_file);
        if (len > 0 && selected_file[len-1] == '\n') {
            selected_file[len-1] = '\0';
        }
        pclose(fp);
        if (strlen(selected_file) > 0) {
            printf("Selected file via zenity: %s\n", selected_file);
            return selected_file;
        }
    }
    if (fp) pclose(fp);
    
    // Method 2: Try kdialog (KDE)
    fp = popen("kdialog --getopenfilename . \"*.wav *.mp3 *.flac *.ogg | Audio files\" 2>/dev/null", "r");
    if (fp && fgets(selected_file, sizeof(selected_file), fp)) {
        size_t len = strlen(selected_file);
        if (len > 0 && selected_file[len-1] == '\n') {
            selected_file[len-1] = '\0';
        }
        pclose(fp);
        if (strlen(selected_file) > 0) {
            printf("Selected file via kdialog: %s\n", selected_file);
            return selected_file;
        }
    }
    if (fp) pclose(fp);
    
    // Method 3: Try yad (alternative)
    fp = popen("yad --file-selection --title=\"Select Audio File\" "
               "--file-filter=\"Audio files | *.wav *.mp3 *.flac *.ogg\" 2>/dev/null", "r");
    if (fp && fgets(selected_file, sizeof(selected_file), fp)) {
        size_t len = strlen(selected_file);
        if (len > 0 && selected_file[len-1] == '\n') {
            selected_file[len-1] = '\0';
        }
        pclose(fp);
        if (strlen(selected_file) > 0) {
            printf("Selected file via yad: %s\n", selected_file);
            return selected_file;
        }
    }
    if (fp) pclose(fp);
    
    // Fallback: Try to find audio files in current directory
    printf("No file dialog available. Looking for audio files in current directory...\n");
    const char* fallback_files[] = {
        "audio_samples/chain_original.wav",
        "audio_samples/lowpass_filtered.wav", 
        "audio_samples/original_sweep.wav",
        "sine_440_5s.wav",
        "test.wav",
        "*.wav"
    };
    
    for (int i = 0; i < 6; i++) {
        if (access(fallback_files[i], F_OK) == 0) {
            strcpy(selected_file, fallback_files[i]);
            printf("Found audio file: %s\n", selected_file);
            return selected_file;
        }
    }
    
    printf("No audio files found. Please ensure audio files are available.\n");
    return NULL;
}

// Colors as structs
static SDL_Color COLOR_BG = {30, 30, 35, 255};
static SDL_Color COLOR_PANEL = {50, 50, 55, 255};
static SDL_Color COLOR_BUTTON = {70, 130, 180, 255};
static SDL_Color COLOR_BUTTON_HOVER = {100, 150, 200, 255};
static SDL_Color COLOR_BUTTON_ACTIVE = {50, 100, 150, 255};
static SDL_Color COLOR_SLIDER = {100, 100, 100, 255};
static SDL_Color COLOR_SLIDER_ACTIVE = {150, 150, 150, 255};
static SDL_Color COLOR_TEXT = {220, 220, 220, 255};
static SDL_Color COLOR_ACCENT = {255, 165, 0, 255};

// Text rendering functions (forward declarations)
void draw_char(int x, int y, char c, int color_r, int color_g, int color_b);
void draw_text(int x, int y, const char* text);
void draw_text_colored(int x, int y, const char* text, int r, int g, int b);

// Simple UI element structures
typedef struct {
    int x, y, width, height;
    int hover, pressed;
    char text[64];
} Button;

typedef struct {
    int x, y, width, height;
    float value, min, max;
    int dragging;
    char label[32];
} Slider;

// Initialize GUI
int gui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }
    
    window = SDL_CreateWindow(
        "Audio Effects Mixer",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
    
    return 1;
}

// Shutdown GUI
void gui_shutdown(void) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    SDL_Quit();
}

// Simple button rendering
int draw_button(Button* btn, int mouse_x, int mouse_y, int mouse_pressed) {
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
        draw_text_colored(text_x, text_y, btn->text, 255, 255, 255); // White
    } else {
        draw_text_colored(text_x, text_y, btn->text, 220, 220, 220); // Light gray
    }
    
    return clicked;
}

// Simple slider rendering
int draw_slider(Slider* slider, int mouse_x, int mouse_y, int mouse_pressed) {
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
    
    // Draw value text above slider
    char value_text[32];
    snprintf(value_text, sizeof(value_text), "%.2f", slider->value);
    int text_width = strlen(value_text) * 9;
    int value_x = slider->x + (slider->width - text_width) / 2;
    int value_y = slider->y - 12; // Above the slider
    draw_text_colored(value_x, value_y, value_text, 180, 180, 180);
    
    return slider->dragging;
}

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
    // / (47) - skipping for now
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
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
void draw_char(int x, int y, char c, int color_r, int color_g, int color_b) {
    const unsigned char* char_data = NULL;
    
    if (c >= 32 && c <= 57) { // Space through 9
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
void draw_text(int x, int y, const char* text) {
    draw_text_colored(x, y, text, 220, 220, 220); // Default white text
}

// Draw colored text string
void draw_text_colored(int x, int y, const char* text, int r, int g, int b) {
    int char_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        draw_char(char_x, y, text[i], r, g, b);
        char_x += 9; // 8 pixel width + 1 pixel spacing
    }
}

// Main GUI rendering function
int gui_render_frame(AudioMixer* mixer) {
    SDL_Event event;
    int mouse_x, mouse_y;
    int mouse_pressed = SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_LEFT);
    
    // Handle events
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return 0;
                }
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                }
                break;
        }
    }
    
    // Clear screen
    SDL_Color bg = COLOR_BG;
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderClear(renderer);
    
    // Draw main panels
    SDL_Color panel_color = COLOR_PANEL;
    SDL_SetRenderDrawColor(renderer, panel_color.r, panel_color.g, panel_color.b, panel_color.a);
    
    // File operations panel
    SDL_Rect file_panel = {10, 10, window_width - 20, 80};
    SDL_RenderFillRect(renderer, &file_panel);
    draw_text(20, 20, "File Operations");
    
    // Load button
    static int load_button_pressed = 0;
    Button load_btn = {30, 40, 100, 30, 0, 0, "Load Audio"};
    if (draw_button(&load_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!load_button_pressed) {
            load_button_pressed = 1;
            printf("Opening file dialog...\n");
            
            char* selected_file = open_file_dialog();
            if (selected_file && strlen(selected_file) > 0) {
                if (mixer_load_audio(mixer, selected_file)) {
                    printf("Successfully loaded: %s\n", selected_file);
                } else {
                    printf("Failed to load: %s\n", selected_file);
                }
            } else {
                printf("No file selected or dialog cancelled\n");
            }
        }
    } else {
        load_button_pressed = 0;
    }
    
    // Save button
    static int save_button_pressed = 0;
    Button save_btn = {140, 40, 100, 30, 0, 0, "Save Audio"};
    if (draw_button(&save_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!save_button_pressed) {
            save_button_pressed = 1;
            // Generate output filename based on input
            const char* output_file = mixer->output_filename[0] ? mixer->output_filename : "processed_audio.wav";
            if (mixer_save_audio(mixer, output_file)) {
                printf("Audio saved to: %s\n", output_file);
            } else {
                printf("Failed to save audio to: %s\n", output_file);
                // Try alternative location
                if (mixer_save_audio(mixer, "output.wav")) {
                    printf("Audio saved to: output.wav\n");
                } else {
                    printf("Failed to save audio\n");
                }
            }
        }
    } else {
        save_button_pressed = 0;
    }
    
    // Process button
    static int process_button_pressed = 0;
    Button process_btn = {250, 40, 100, 30, 0, 0, "Process"};
    if (draw_button(&process_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!process_button_pressed) {
            process_button_pressed = 1;
            printf("Processing effects...\n");
            mixer_process_effects(mixer);
            printf("Effects processing complete\n");
        }
    } else {
        process_button_pressed = 0;
    }
    
    // Master controls panel
    SDL_Rect master_panel = {10, 100, window_width - 20, 80};
    SDL_RenderFillRect(renderer, &master_panel);
    draw_text(20, 110, "Master Controls");
    
    // Master volume slider
    Slider volume_slider = {30, 130, 200, 30, mixer->master_volume, 0.0f, 2.0f, 0, "Volume"};
    if (draw_slider(&volume_slider, mouse_x, mouse_y, mouse_pressed)) {
        mixer->master_volume = volume_slider.value;
        if (mixer->auto_process) {
            mixer_process_effects(mixer);
        }
    }
    draw_text(30, 165, "Master Volume");
    
    // Effects chain panel
    int effects_y = 190;
    SDL_Rect effects_panel = {10, effects_y, window_width - 20, window_height - effects_y - 10};
    SDL_RenderFillRect(renderer, &effects_panel);
    draw_text(20, effects_y + 10, "Effects Chain");
    
    // Add effect buttons
    int btn_x = 30;
    int btn_y = effects_y + 30;
    int btn_spacing = 110;
    
    const char* effect_names[] = {
        "Lowpass", "Highpass", "EQ", "Echo", 
        "Reverb", "Overdrive", "Tube", "Fuzz",
        "Chorus", "Flanger", "Phaser", "Tremolo"
    };
    
    EffectType effect_types[] = {
        EFFECT_LOWPASS, EFFECT_HIGHPASS, EFFECT_EQ, EFFECT_ECHO,
        EFFECT_REVERB, EFFECT_OVERDRIVE, EFFECT_TUBE, EFFECT_FUZZ,
        EFFECT_CHORUS, EFFECT_FLANGER, EFFECT_PHASER, EFFECT_TREMOLO
    };
    
    static int effect_buttons_pressed[12] = {0};
    
    for (int i = 0; i < 12; i++) {
        if (btn_x + 100 > window_width - 20) {
            btn_x = 30;
            btn_y += 40;
        }
        
        Button effect_btn = {btn_x, btn_y, 100, 30, 0, 0, ""};
        strcpy(effect_btn.text, effect_names[i]);
        
        if (draw_button(&effect_btn, mouse_x, mouse_y, mouse_pressed)) {
            if (!effect_buttons_pressed[i]) {
                effect_buttons_pressed[i] = 1;
                mixer_add_effect(mixer, effect_types[i]);
                printf("Added effect: %s\n", effect_names[i]);
            }
        } else {
            effect_buttons_pressed[i] = 0;
        }
        
        btn_x += btn_spacing;
    }
    
    // Draw current effects
    int effect_slot_y = btn_y + 60;
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (mixer->effects[i].type != EFFECT_NONE) {
            int slot_x = 30 + (i % 4) * 290;
            int slot_y = effect_slot_y + (i / 4) * 120;
            
            if (slot_y + 100 > window_height - 20) break; // Don't draw outside window
            
            // Effect slot panel
            SDL_Rect slot_rect = {slot_x, slot_y, 280, 100};
            SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
            SDL_RenderFillRect(renderer, &slot_rect);
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &slot_rect);
            
            draw_text(slot_x + 5, slot_y + 5, mixer->effects[i].name);
            
            // Remove button
            static int remove_buttons_pressed[MAX_EFFECTS] = {0};
            Button remove_btn = {slot_x + 240, slot_y + 5, 30, 20, 0, 0, "X"};
            if (draw_button(&remove_btn, mouse_x, mouse_y, mouse_pressed)) {
                if (!remove_buttons_pressed[i]) {
                    remove_buttons_pressed[i] = 1;
                    printf("Removing effect: %s\n", mixer->effects[i].name);
                    mixer_remove_effect(mixer, i);
                }
            } else {
                remove_buttons_pressed[i] = 0;
            }
            
            // Enable/disable checkbox (simplified as button)
            static int enable_buttons_pressed[MAX_EFFECTS] = {0};
            Button enable_btn = {slot_x + 5, slot_y + 25, 60, 20, 0, 0, ""};
            strcpy(enable_btn.text, mixer->effects[i].params.enabled ? "Enabled" : "Disabled");
            if (draw_button(&enable_btn, mouse_x, mouse_y, mouse_pressed)) {
                if (!enable_buttons_pressed[i]) {
                    enable_buttons_pressed[i] = 1;
                    mixer->effects[i].params.enabled = !mixer->effects[i].params.enabled;
                    printf("Effect %s: %s\n", mixer->effects[i].name, 
                           mixer->effects[i].params.enabled ? "Enabled" : "Disabled");
                    if (mixer->auto_process) {
                        mixer_process_effects(mixer);
                    }
                }
            } else {
                enable_buttons_pressed[i] = 0;
            }
            
            // Parameter sliders
            for (int p = 0; p < 3; p++) {
                const char* param_name = get_param_name(mixer->effects[i].type, p);
                if (strlen(param_name) > 0) {
                    float min_val, max_val;
                    get_param_range(mixer->effects[i].type, p, &min_val, &max_val);
                    
                    float* param_value = &mixer->effects[i].params.param1;
                    if (p == 1) param_value = &mixer->effects[i].params.param2;
                    if (p == 2) param_value = &mixer->effects[i].params.param3;
                    
                    Slider param_slider = {
                        slot_x + 70 + p * 65, slot_y + 50, 60, 20,
                        *param_value, min_val, max_val, 0, ""
                    };
                    
                    if (draw_slider(&param_slider, mouse_x, mouse_y, mouse_pressed)) {
                        *param_value = param_slider.value;
                        if (mixer->auto_process) {
                            mixer_process_effects(mixer);
                        }
                    }
                    
                    // Draw parameter label below slider
                    int label_width = strlen(param_name) * 9;
                    int label_x = slot_x + 70 + p * 65 + (60 - label_width) / 2;
                    int label_y = slot_y + 75;
                    draw_text_colored(label_x, label_y, param_name, 160, 160, 160);
                }
            }
        }
    }
    
    // Status information
    if (mixer->audio_buffer) {
        char status[256];
        snprintf(status, sizeof(status), "Audio: %zu samples, %zu channels, %.0f Hz | Effects: %d active",
                mixer->audio_buffer->length, mixer->audio_buffer->channels, 
                mixer->sample_rate, mixer->num_effects);
        draw_text(20, window_height - 30, status);
    }
    
    // Present the rendered frame
    SDL_RenderPresent(renderer);
    
    return 1;
}

// Handle events (called from main loop)
void gui_handle_events(void) {
    // Events are handled in gui_render_frame for this simple implementation
}