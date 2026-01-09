// Advanced Parametric EQ Window module
#ifndef EQ_WINDOW_H
#define EQ_WINDOW_H

#include "../audio_mixer_gui.h"
#include <SDL2/SDL.h>

// Open advanced EQ window for a specific effect
void open_advanced_eq_window(AudioMixer* mixer, int effect_index, SDL_Renderer* main_renderer, SDL_AudioDeviceID device, int is_playing_flag);

// Close advanced EQ window
void close_advanced_eq_window(void);

// Render advanced parametric EQ window
void render_advanced_eq_window(AudioMixer* mixer);

// Check if EQ window is open
int is_eq_window_open(void);

#endif // EQ_WINDOW_H
