// Spectrum analyzer module
#ifndef SPECTRUM_ANALYZER_H
#define SPECTRUM_ANALYZER_H

#include <SDL2/SDL.h>

// FFT for spectrum analysis
#define FFT_SIZE 2048
#define SPECTRUM_BARS 50

// Initialize spectrum analyzer frequency bins
void init_spectrum_analyzer(void);

// Store audio sample for FFT processing
void spectrum_add_sample(float sample);

// Update spectrum analysis
void update_spectrum_analysis(float sample_rate, int is_playing, int is_paused);

// Draw spectrum visualizer
void draw_spectrum_visualizer(SDL_Renderer* renderer, int window_width, int window_height);

#endif // SPECTRUM_ANALYZER_H
