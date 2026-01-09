// Spectrum analyzer implementation
#include "spectrum_analyzer.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../widgets/ui_widgets.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TWO_PI (2.0f * M_PI)

// Spectrum analyzer data
static float fft_input[FFT_SIZE];
static float fft_magnitude[FFT_SIZE / 2];
static float spectrum_bars[SPECTRUM_BARS];
static int spectrum_bar_freqs[SPECTRUM_BARS];
static Uint32 last_spectrum_update = 0;

// Simple FFT implementation (Cooley-Tukey algorithm)
static void fft(float* real, float* imag, int N) {
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        
        if (i < j) {
            float temp = real[i];
            real[i] = real[j];
            real[j] = temp;
            temp = imag[i];
            imag[i] = imag[j];
            imag[j] = temp;
        }
    }
    
    // Cooley-Tukey FFT
    for (int len = 2; len <= N; len <<= 1) {
        float wlen_r = cosf(TWO_PI / len);
        float wlen_i = -sinf(TWO_PI / len);
        
        for (int i = 0; i < N; i += len) {
            float w_r = 1.0f;
            float w_i = 0.0f;
            
            for (int j = 0; j < len / 2; j++) {
                float u_r = real[i + j];
                float u_i = imag[i + j];
                float v_r = real[i + j + len / 2] * w_r - imag[i + j + len / 2] * w_i;
                float v_i = real[i + j + len / 2] * w_i + imag[i + j + len / 2] * w_r;
                
                real[i + j] = u_r + v_r;
                imag[i + j] = u_i + v_i;
                real[i + j + len / 2] = u_r - v_r;
                imag[i + j + len / 2] = u_i - v_i;
                
                float temp_r = w_r * wlen_r - w_i * wlen_i;
                w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = temp_r;
            }
        }
    }
}

// Initialize spectrum analyzer frequency bins
void init_spectrum_analyzer(void) {
    int bar_index = 0;
    
    // 20-100Hz: 10Hz per bar (8 bars)
    for (int freq = 30; freq <= 100 && bar_index < SPECTRUM_BARS; freq += 10) {
        spectrum_bar_freqs[bar_index++] = freq;
    }
    
    // 100Hz-1kHz: 100Hz per bar (9 bars)
    for (int freq = 200; freq <= 1000 && bar_index < SPECTRUM_BARS; freq += 100) {
        spectrum_bar_freqs[bar_index++] = freq;
    }
    
    // 1kHz-4kHz: 250Hz per bar (12 bars)
    for (int freq = 1250; freq <= 4000 && bar_index < SPECTRUM_BARS; freq += 250) {
        spectrum_bar_freqs[bar_index++] = freq;
    }
    
    // 4kHz-8kHz: 400Hz per bar (10 bars)
    for (int freq = 4400; freq <= 8000 && bar_index < SPECTRUM_BARS; freq += 400) {
        spectrum_bar_freqs[bar_index++] = freq;
    }
    
    // 8kHz-20kHz: 1kHz per bar (12 bars)
    for (int freq = 9000; freq <= 20000 && bar_index < SPECTRUM_BARS; freq += 1000) {
        spectrum_bar_freqs[bar_index++] = freq;
    }
}

// Store audio sample for FFT processing
void spectrum_add_sample(float sample) {
    static int fft_index = 0;
    
    if (fft_index < FFT_SIZE) {
        fft_input[fft_index] = sample;
        fft_index++;
        if (fft_index >= FFT_SIZE) {
            fft_index = 0;  // Reset for next batch
        }
    }
}

// Update spectrum analysis
void update_spectrum_analysis(float sample_rate, int is_playing, int is_paused) {
    // Only update when audio is playing
    if (!is_playing || is_paused) {
        // Clear spectrum bars when not playing
        for (int bar = 0; bar < SPECTRUM_BARS; bar++) {
            spectrum_bars[bar] *= 0.95f; // Gradual fade
        }
        return;
    }
    
    // Only update spectrum 15 times per second to reduce CPU usage
    Uint32 current_time = SDL_GetTicks();
    if (current_time - last_spectrum_update < 66) {  // ~15 FPS
        return;
    }
    last_spectrum_update = current_time;
    
    static float fft_real[FFT_SIZE];
    static float fft_imag[FFT_SIZE];
    
    // Copy input to FFT arrays
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_real[i] = fft_input[i];
        fft_imag[i] = 0.0f;
    }
    
    // Apply window function (Hamming window)
    for (int i = 0; i < FFT_SIZE; i++) {
        float window = 0.54f - 0.46f * cosf(TWO_PI * i / (FFT_SIZE - 1));
        fft_real[i] *= window;
    }
    
    // Perform FFT
    fft(fft_real, fft_imag, FFT_SIZE);
    
    // Calculate magnitudes - straight FFT amplitude values with visualization gain
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        float magnitude = sqrtf(fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i]);
        fft_magnitude[i] = (magnitude * 2.0f * 50.0f) / FFT_SIZE;
    }
    
    // Map FFT bins to spectrum bars
    float freq_per_bin = sample_rate / FFT_SIZE;
    
    for (int bar = 0; bar < SPECTRUM_BARS; bar++) {
        // Calculate frequency range for this bar
        float freq_start = (bar == 0) ? 20.0f : spectrum_bar_freqs[bar - 1];
        float freq_end = spectrum_bar_freqs[bar];
        
        int bin_start = (int)(freq_start / freq_per_bin);
        int bin_end = (int)(freq_end / freq_per_bin);
        
        // Make sure we have at least one bin
        if (bin_end <= bin_start) bin_end = bin_start + 1;
        if (bin_end > FFT_SIZE / 2) bin_end = FFT_SIZE / 2;
        
        // Find the peak magnitude in this frequency range
        float peak_magnitude = 0.0f;
        for (int bin = bin_start; bin < bin_end; bin++) {
            if (fft_magnitude[bin] > peak_magnitude) {
                peak_magnitude = fft_magnitude[bin];
            }
        }
        
        // Apply frequency-dependent boost for better visibility
        float center_freq = (freq_start + freq_end) / 2.0f;
        float boost_factor = 1.0f;
        if (center_freq < 200.0f) {
            boost_factor = 4.0f;  // Deep bass boost: 12dB
        } else if (center_freq < 1000.0f) {
            boost_factor = 2.0f;  // Low-mid boost: 6dB
        } else if (center_freq < 2000.0f) {
            boost_factor = 3.0f;  // Mid boost: 9.5dB
        } else if (center_freq < 4000.0f) {
            boost_factor = 5.0f;  // Upper-mid boost: 14dB
        } else if (center_freq < 8000.0f) {
            boost_factor = 8.0f;  // High frequency boost: 18dB
        } else {
            boost_factor = 12.0f + (center_freq - 8000.0f) / 12000.0f * 8.0f;
        }
        peak_magnitude *= boost_factor;
        
        // Convert to dBFS: map from -30dBFS to +10dBFS to 0-40 for display
        float dbfs = 20.0f * log10f(peak_magnitude + 1e-10f);
        float normalized_db = dbfs + 30.0f;
        normalized_db = fmaxf(0.0f, fminf(40.0f, normalized_db));
        
        // Smooth the bars
        spectrum_bars[bar] = spectrum_bars[bar] * 0.85f + normalized_db * 0.15f;
    }
}

// Draw spectrum visualizer
void draw_spectrum_visualizer(SDL_Renderer* renderer, int window_width, int window_height) {
    // Graph dimensions and positioning
    int graph_x = 60;
    int graph_y = window_height - 220;
    int graph_height = 150;
    int graph_width = window_width - 120;
    int bar_width = graph_width / SPECTRUM_BARS;
    
    // Draw graph background with grid
    SDL_SetRenderDrawColor(renderer, 15, 15, 25, 255);
    SDL_Rect graph_bg = {graph_x, graph_y, graph_width, graph_height};
    SDL_RenderFillRect(renderer, &graph_bg);
    
    // Draw graph border
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &graph_bg);
    
    // Draw horizontal grid lines with dB labels (-30 dB to +10 dB)
    const float db_marks[] = {10.0f, 0.0f, -10.0f, -20.0f, -30.0f};
    int num_db_marks = sizeof(db_marks) / sizeof(db_marks[0]);
    for (int i = 0; i < num_db_marks; i++) {
        float normalized = (db_marks[i] + 30.0f) / 40.0f;
        int y = graph_y + graph_height - (int)(normalized * graph_height);
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
        SDL_RenderDrawLine(renderer, graph_x, y, graph_x + graph_width, y);
        char label[8];
        snprintf(label, sizeof(label), "%+gdB", db_marks[i]);
        draw_text_colored(renderer, graph_x - 50, y - 5, label, 160, 160, 160);
    }
    draw_text_colored(renderer, graph_x - 40, graph_y - 15, "dB", 200, 200, 100);
    
    // Draw vertical grid lines (frequency divisions)
    const float freq_labels[] = {20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f};
    int num_freq_labels = sizeof(freq_labels) / sizeof(freq_labels[0]);
    for (int i = 0; i < num_freq_labels; i++) {
        int bar_index = 0;
        while (bar_index < SPECTRUM_BARS && spectrum_bar_freqs[bar_index] < freq_labels[i]) {
            bar_index++;
        }
        if (bar_index >= SPECTRUM_BARS) bar_index = SPECTRUM_BARS - 1;
        int x = graph_x + bar_index * bar_width;
        SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
        SDL_RenderDrawLine(renderer, x, graph_y, x, graph_y + graph_height);

        char label[12];
        if (freq_labels[i] >= 1000.0f) {
            snprintf(label, sizeof(label), "%.0fk", freq_labels[i] / 1000.0f);
        } else {
            snprintf(label, sizeof(label), "%.0f", freq_labels[i]);
        }
        draw_text_colored(renderer, x - 10, graph_y + graph_height + 5, label, 160, 160, 160);
    }
    draw_text_colored(renderer, graph_x + graph_width / 2 - 10, graph_y + graph_height + 25, "Hz", 200, 200, 100);
    
    // Draw spectrum bars
    for (int i = 0; i < SPECTRUM_BARS; i++) {
        int bar_x = graph_x + i * bar_width;
        float normalized_value = spectrum_bars[i];
        float normalized_db = normalized_value / 40.0f; // 0-1 range
        normalized_db = fmaxf(0.0f, fminf(1.0f, normalized_db));
        int bar_height = (int)(normalized_db * graph_height);
        
        // Enhanced color coding for frequency response
        Uint8 r, g, b;
        float freq_ratio = (float)i / SPECTRUM_BARS;
        if (freq_ratio < 0.2f) {
            // Deep bass: warm red/orange
            r = 255; g = 120; b = 40;
        } else if (freq_ratio < 0.4f) {
            // Low-mid: golden yellow
            r = 255; g = 200; b = 60;
        } else if (freq_ratio < 0.6f) {
            // Midrange: green
            r = 120; g = 220; b = 80;
        } else if (freq_ratio < 0.8f) {
            // Upper-mid: teal
            r = 60; g = 200; b = 200;
        } else {
            // High frequencies: cool blue/purple
            r = 80; g = 120; b = 255;
        }
        
        // Intensity based on amplitude
        float intensity = normalized_db;
        r = (Uint8)(r * intensity);
        g = (Uint8)(g * intensity);
        b = (Uint8)(b * intensity);
        
        // Draw bar
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect bar = {bar_x + 1, graph_y + graph_height - bar_height, bar_width - 2, bar_height};
        SDL_RenderFillRect(renderer, &bar);
    }
}
