// Advanced Parametric EQ Window implementation
#include "eq_window.h"
#include "../widgets/ui_widgets.h"
#include "../audio/audio_playback.h"
#include "../audio/spectrum_analyzer.h"
#include "../../include/audio_filters.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// External access to spectrum data from spectrum_analyzer.c
extern float fft_magnitude[FFT_SIZE / 2];
extern float spectrum_bars[SPECTRUM_BARS];
extern int spectrum_bar_freqs[SPECTRUM_BARS];

// EQ window state
static SDL_Window* eq_window = NULL;
static SDL_Renderer* eq_renderer = NULL;
static SDL_Renderer* main_renderer_ref = NULL;
static int eq_window_open = 0;
static int eq_window_width = 900;
static int eq_window_height = 600;
static int eq_effect_index = -1;
static ParametricEQ* current_parametric_eq = NULL;
static int eq_selected_band = 0;
static int eq_dragging_band = -1;
static AudioMixer* eq_mixer_ref = NULL;
static Uint32 eq_last_process_tick = 0;
static Uint32 eq_last_change_tick = 0;
static int eq_reprocess_pending = 0;

// Distinct palette for each EQ band to keep overlapping regions legible
static SDL_Color get_band_color(int band_index) {
    static const SDL_Color palette[MAX_EQ_BANDS] = {
        {255, 120, 80, 255},   // 1 - Warm red
        {255, 180, 60, 255},   // 2 - Orange
        {230, 220, 60, 255},   // 3 - Yellow
        {120, 210, 80, 255},   // 4 - Green
        {60, 200, 170, 255},   // 5 - Teal
        {80, 160, 255, 255},   // 6 - Blue
        {150, 100, 255, 255},  // 7 - Purple
        {255, 120, 200, 255}   // 8 - Magenta
    };
    if (band_index < 0 || band_index >= MAX_EQ_BANDS) {
        return (SDL_Color){100, 200, 255, 255};
    }
    return palette[band_index];
}

// Helper functions for frequency/gain conversions
static float freq_to_x(float freq, int graph_x, int graph_width) {
    float min_log = logf(20.0f);
    float max_log = logf(20000.0f);
    float freq_log = logf(fmaxf(20.0f, fminf(20000.0f, freq)));
    float normalized = (freq_log - min_log) / (max_log - min_log);
    return graph_x + normalized * graph_width;
}

static float x_to_freq(float x, int graph_x, int graph_width) {
    float normalized = (x - graph_x) / graph_width;
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    float min_log = logf(20.0f);
    float max_log = logf(20000.0f);
    float freq_log = min_log + normalized * (max_log - min_log);
    return expf(freq_log);
}

static float gain_to_y(float gain_db, int graph_y, int graph_height) {
    float normalized = (24.0f - gain_db) / 48.0f;
    return graph_y + normalized * graph_height;
}

static float y_to_gain(float y, int graph_y, int graph_height) {
    float normalized = (y - graph_y) / graph_height;
    normalized = fmaxf(0.0f, fminf(1.0f, normalized));
    return 24.0f - normalized * 48.0f;
}

static void request_eq_reprocess(int immediate) {
    if (!eq_mixer_ref || !eq_mixer_ref->auto_process) return;
    if (!eq_mixer_ref->audio_buffer || !eq_mixer_ref->processed_buffer) return;
    
    if (immediate) {
        Uint32 now = SDL_GetTicks();
        if (now - eq_last_process_tick < 30) {
            eq_reprocess_pending = 1;
            eq_last_change_tick = now;
            return;
        }
        mixer_process_effects(eq_mixer_ref);
        eq_last_process_tick = now;
        eq_last_change_tick = now;
        eq_reprocess_pending = 0;
        return;
    }
    
    eq_reprocess_pending = 1;
    eq_last_change_tick = SDL_GetTicks();
}

// Calculate frequency response magnitude for a single parametric EQ band
static float calculate_band_response(float freq, float center_freq, float q, float gain_db) {
    if (!center_freq || center_freq <= 0) return 0.0f;
    
    float omega = 2.0f * M_PI * freq;
    float omega0 = 2.0f * M_PI * center_freq;
    float A = powf(10.0f, gain_db / 40.0f); // Linear gain
    float S = 1.0f; // Shelf slope parameter
    float alpha = sinf(omega0) / (2.0f * q);
    
    // Calculate biquad coefficients for peaking EQ
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosf(omega0);
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosf(omega0);
    float a2 = 1.0f - alpha / A;
    
    // Normalize coefficients
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0;
    
    // Calculate frequency response H(jw) = (b0 + b1*e^(-jw) + b2*e^(-j2w)) / (1 + a1*e^(-jw) + a2*e^(-j2w))
    float cos_w = cosf(omega);
    float cos_2w = cosf(2.0f * omega);
    float sin_w = sinf(omega);
    float sin_2w = sinf(2.0f * omega);
    
    // Numerator: b0 + b1*cos(w) + b2*cos(2w) + j*(-b1*sin(w) - b2*sin(2w))
    float num_real = b0 + b1 * cos_w + b2 * cos_2w;
    float num_imag = -b1 * sin_w - b2 * sin_2w;
    
    // Denominator: 1 + a1*cos(w) + a2*cos(2w) + j*(-a1*sin(w) - a2*sin(2w))
    float den_real = 1.0f + a1 * cos_w + a2 * cos_2w;
    float den_imag = -a1 * sin_w - a2 * sin_2w;
    
    // Magnitude of H(jw) = |numerator| / |denominator|
    float num_mag = sqrtf(num_real * num_real + num_imag * num_imag);
    float den_mag = sqrtf(den_real * den_real + den_imag * den_imag);
    
    if (den_mag < 1e-10f) return 0.0f;
    
    float magnitude = num_mag / den_mag;
    return 20.0f * log10f(magnitude + 1e-10f); // Convert to dB
}

// Calculate combined frequency response for all enabled EQ bands
static float calculate_total_response(float freq) {
    if (!current_parametric_eq) return 0.0f;
    
    float total_db = 0.0f;
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (current_parametric_eq->bands[i].enabled && 
            current_parametric_eq->bands[i].frequency > 0) {
            float band_response = calculate_band_response(
                freq,
                current_parametric_eq->bands[i].frequency,
                current_parametric_eq->bands[i].q,
                current_parametric_eq->bands[i].gain_db
            );
            total_db += band_response;
        }
    }
    return total_db;
}

// Apply perceptual frequency weighting (similar to A-weighting for better high-freq representation)
static float apply_perceptual_weighting(float freq, float magnitude_db) {
    // A-weighting-inspired curve to boost high frequencies for better visual representation
    float f2 = freq * freq;
    float f4 = f2 * f2;
    
    // Simplified A-weighting formula adapted for visualization
    float numerator = 12194.0f * 12194.0f * f4;
    float denominator = (f2 + 20.6f * 20.6f) * sqrtf((f2 + 107.7f * 107.7f) * (f2 + 737.9f * 737.9f)) * (f2 + 12194.0f * 12194.0f);
    
    float a_weight_db = 0.0f;
    if (denominator > 0.0f) {
        a_weight_db = 20.0f * log10f(numerator / denominator) + 2.0f; // +2dB reference
    }
    
    // Additional perceptual boost for visualization (not true A-weighting)
    float vis_boost = 0.0f;
    if (freq > 1000.0f) {
        vis_boost = 3.0f * log10f(freq / 1000.0f); // Extra boost above 1kHz for visualization
    }
    
    return magnitude_db + a_weight_db + vis_boost;
}

// Get spectrum magnitude for a specific frequency from FFT data
static float get_spectrum_magnitude_at_freq(float freq, float sample_rate) {
    if (!eq_mixer_ref || sample_rate <= 0) return -60.0f; // Very low if no audio
    
    float freq_per_bin = sample_rate / FFT_SIZE;
    int bin_index = (int)(freq / freq_per_bin);
    
    if (bin_index < 0 || bin_index >= FFT_SIZE / 2) return -60.0f;
    
    // For higher frequencies, average multiple bins to get better representation
    float magnitude = 0.0f;
    int bins_to_average = 1;
    
    if (freq > 2000.0f) {
        bins_to_average = (int)(freq / 2000.0f) + 1; // More averaging for higher frequencies
        bins_to_average = fminf(bins_to_average, 5); // Cap at 5 bins
    }
    
    // Average magnitude across multiple bins for smoother high-frequency response
    for (int i = 0; i < bins_to_average; i++) {
        int bin = bin_index + i;
        if (bin < FFT_SIZE / 2) {
            magnitude += fft_magnitude[bin];
        }
    }
    magnitude /= bins_to_average;
    
    if (magnitude <= 0.0f) return -60.0f;
    
    float db = 20.0f * log10f(magnitude + 1e-10f);
    
    // Apply perceptual weighting to better represent what you hear
    db = apply_perceptual_weighting(freq, db);
    
    return fmaxf(-60.0f, fminf(20.0f, db)); // Increased max to 20dB
}

// Draw real-time audio spectrum wave with EQ response applied
static void draw_frequency_response_wave(int graph_x, int graph_y, int graph_width, int graph_height) {
    if (!current_parametric_eq || !eq_mixer_ref) return;
    
    const int num_points = graph_width / 2; // Calculate every 2 pixels for smooth curve
    SDL_Point* spectrum_points = malloc(num_points * sizeof(SDL_Point));
    SDL_Point* eq_points = malloc(num_points * sizeof(SDL_Point));
    if (!spectrum_points || !eq_points) {
        free(spectrum_points);
        free(eq_points);
        return;
    }
    
    float sample_rate = eq_mixer_ref->sample_rate;
    
    // Calculate both original spectrum and EQ-modified spectrum
    for (int i = 0; i < num_points; i++) {
        float x_pos = (float)i * 2.0f; // Every 2 pixels
        float freq = x_to_freq(graph_x + x_pos, graph_x, graph_width);
        
        // Get original spectrum magnitude from live audio
        float original_db = get_spectrum_magnitude_at_freq(freq, sample_rate);
        
        // Get EQ response at this frequency
        float eq_response_db = calculate_total_response(freq);
        
        // Apply EQ to the spectrum (add in dB domain)
        float modified_db = original_db + eq_response_db;
        
        // Improved mapping for better high-frequency visibility
        // Map wider dB range to display range with better scaling
        float display_original = (original_db + 50.0f) / 70.0f * 48.0f - 24.0f; // Map -50 to +20 dB -> -24 to +24 dB display
        float display_modified = (modified_db + 50.0f) / 70.0f * 48.0f - 24.0f;
        
        // Clamp to display range
        display_original = fmaxf(-24.0f, fminf(24.0f, display_original));
        display_modified = fmaxf(-24.0f, fminf(24.0f, display_modified));
        
        spectrum_points[i].x = graph_x + (int)x_pos;
        spectrum_points[i].y = gain_to_y(display_original, graph_y, graph_height);
        
        eq_points[i].x = graph_x + (int)x_pos;
        eq_points[i].y = gain_to_y(display_modified, graph_y, graph_height);
    }
    
    // Draw original spectrum (dim, behind)
    SDL_SetRenderDrawColor(eq_renderer, 80, 80, 80, 60);
    for (int i = 0; i < num_points - 1; i++) {
        SDL_RenderDrawLine(eq_renderer, 
                         spectrum_points[i].x, spectrum_points[i].y,
                         spectrum_points[i+1].x, spectrum_points[i+1].y);
    }
    
    // Fill area under original spectrum (very dim)
    int zero_y = gain_to_y(0.0f, graph_y, graph_height);
    SDL_SetRenderDrawColor(eq_renderer, 60, 60, 80, 20);
    for (int i = 0; i < num_points - 1; i++) {
        int y_start = (spectrum_points[i].y > zero_y) ? zero_y : spectrum_points[i].y;
        int y_end = (spectrum_points[i].y < zero_y) ? zero_y : spectrum_points[i].y;
        if (y_end > y_start) {
            SDL_RenderDrawLine(eq_renderer, spectrum_points[i].x, y_start, spectrum_points[i].x, y_end);
        }
    }
    
    // Draw EQ-modified spectrum (bright, in front)
    for (int pass = 0; pass < 3; pass++) {
        int alpha = (pass == 0) ? 120 : (pass == 1) ? 60 : 30;
        int thickness = (pass == 0) ? 1 : (pass == 1) ? 2 : 3;
        
        SDL_SetRenderDrawColor(eq_renderer, 100, 255, 150, alpha);
        
        for (int i = 0; i < num_points - 1; i++) {
            for (int t = -thickness; t <= thickness; t++) {
                SDL_RenderDrawLine(eq_renderer, 
                                 eq_points[i].x, eq_points[i].y + t,
                                 eq_points[i+1].x, eq_points[i+1].y + t);
            }
        }
    }
    
    // Fill area under EQ-modified spectrum with dynamic colors
    for (int i = 0; i < num_points - 1; i++) {
        float freq = x_to_freq(eq_points[i].x, graph_x, graph_width);
        float eq_response_db = calculate_total_response(freq);
        
        // Color based on EQ modification
        if (eq_response_db > 0.5f) {
            SDL_SetRenderDrawColor(eq_renderer, 120, 255, 120, 40); // Green for boost
        } else if (eq_response_db < -0.5f) {
            SDL_SetRenderDrawColor(eq_renderer, 255, 120, 120, 40); // Red for cut
        } else {
            SDL_SetRenderDrawColor(eq_renderer, 100, 200, 255, 30); // Blue for neutral
        }
        
        int y_start = (eq_points[i].y > zero_y) ? zero_y : eq_points[i].y;
        int y_end = (eq_points[i].y < zero_y) ? zero_y : eq_points[i].y;
        if (y_end > y_start) {
            SDL_RenderDrawLine(eq_renderer, eq_points[i].x, y_start, eq_points[i].x, y_end);
        }
    }
    
    free(spectrum_points);
    free(eq_points);
}

// Open advanced EQ window for a specific effect
void open_advanced_eq_window(AudioMixer* mixer, int effect_index, SDL_Renderer* main_renderer, SDL_AudioDeviceID device, int is_playing_flag) {
    (void)device;
    (void)is_playing_flag;
    if (eq_window_open || effect_index < 0 || effect_index >= MAX_EFFECTS) return;
    if (mixer->effects[effect_index].type != EFFECT_EQ) return;
    
    eq_effect_index = effect_index;
    main_renderer_ref = main_renderer;
    eq_mixer_ref = mixer;
    
    // Check if we need to convert from FourBandEQ to ParametricEQ
    if (mixer->effects[effect_index].params.param4 != 1.0f) {
        // Currently using FourBandEQ, convert to ParametricEQ
        float low_gain = mixer->effects[effect_index].params.param1;
        float mid_gain = mixer->effects[effect_index].params.param2;
        float high_gain = mixer->effects[effect_index].params.param3;
        
        printf("[EQ Window] Converting FourBandEQ to ParametricEQ: low=%.1f, mid=%.1f, high=%.1f\n",
               low_gain, mid_gain, high_gain);
        
        // Free the old FourBandEQ
        if (mixer->effects[effect_index].effect_instance) {
            free(mixer->effects[effect_index].effect_instance);
            mixer->effects[effect_index].effect_instance = NULL;
        }
        
        // Create new parametric EQ instance
        current_parametric_eq = (ParametricEQ*)malloc(sizeof(ParametricEQ));
        parametric_eq_init(current_parametric_eq, mixer->sample_rate);
        
        // Copy old gain values
        current_parametric_eq->bands[0].gain_db = low_gain;
        current_parametric_eq->bands[1].gain_db = mid_gain;
        current_parametric_eq->bands[2].gain_db = high_gain;
        
        // Clamp to reasonable range
        for (int i = 0; i < 3; i++) {
            if (current_parametric_eq->bands[i].gain_db < -24.0f) current_parametric_eq->bands[i].gain_db = 0.0f;
            if (current_parametric_eq->bands[i].gain_db > 24.0f) current_parametric_eq->bands[i].gain_db = 0.0f;
        }
        
        parametric_eq_update_filters(current_parametric_eq);
        
        // Update mixer slot before triggering any reprocessing so processing code
        // never sees a freed FourBandEQ pointer.
        mixer->effects[effect_index].effect_instance = current_parametric_eq;
        mixer->effects[effect_index].params.param4 = 1.0f;
        
        request_eq_reprocess(1);
    } else {
        // Already using ParametricEQ
        current_parametric_eq = (ParametricEQ*)mixer->effects[effect_index].effect_instance;
    }
    
    // Create SDL window for advanced EQ
    eq_window = SDL_CreateWindow(
        "Advanced Parametric EQ",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        eq_window_width,
        eq_window_height,
        SDL_WINDOW_SHOWN
    );
    
    if (!eq_window) {
        printf("Failed to create EQ window: %s\n", SDL_GetError());
        return;
    }
    
    eq_renderer = SDL_CreateRenderer(eq_window, -1, SDL_RENDERER_ACCELERATED);
    if (!eq_renderer) {
        printf("Failed to create EQ renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(eq_window);
        eq_window = NULL;
        return;
    }
    if (SDL_SetRenderDrawBlendMode(eq_renderer, SDL_BLENDMODE_BLEND) != 0) {
        printf("Warning: Could not enable blending for EQ renderer: %s\n", SDL_GetError());
    }
    
    eq_window_open = 1;
    eq_selected_band = 0;
    eq_dragging_band = -1;
}

// Close advanced EQ window
void close_advanced_eq_window(void) {
    if (!eq_window_open) return;
    
    if (eq_renderer) {
        SDL_DestroyRenderer(eq_renderer);
        eq_renderer = NULL;
    }
    
    if (eq_window) {
        SDL_DestroyWindow(eq_window);
        eq_window = NULL;
    }
    
    eq_window_open = 0;
    eq_effect_index = -1;
    current_parametric_eq = NULL;
    eq_dragging_band = -1;
    eq_mixer_ref = NULL;
    eq_reprocess_pending = 0;
    eq_last_change_tick = 0;
    eq_last_process_tick = 0;
}

// Check if EQ window is open
int is_eq_window_open(void) {
    return eq_window_open;
}

// Render advanced parametric EQ window
void render_advanced_eq_window(AudioMixer* mixer) {
    (void)mixer;
    if (!eq_window_open || !current_parametric_eq || eq_effect_index < 0) return;
    
    SDL_Event event;
    int mouse_x = 0, mouse_y = 0;
    static int prev_mouse_pressed = 0;
    Uint32 eq_window_id = SDL_GetWindowID(eq_window);
    
    // Handle events for EQ window without stealing global events
    while (SDL_PollEvent(&event)) {
        int handled = 0;
        switch (event.type) {
            case SDL_WINDOWEVENT:
                if (event.window.windowID == eq_window_id) {
                    handled = 1;
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        close_advanced_eq_window();
                        return;
                    }
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.windowID == eq_window_id && event.key.keysym.sym == SDLK_ESCAPE) {
                    close_advanced_eq_window();
                    return;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (event.button.windowID == eq_window_id) {
                    handled = 1;
                }
                break;
            case SDL_MOUSEMOTION:
                if (event.motion.windowID == eq_window_id) {
                    handled = 1;
                }
                break;
            default:
                break;
        }
        if (!handled) {
            SDL_PushEvent(&event);
            break;
        }
    }
    
    // Get mouse state - use global coordinates then convert to window-local
    int global_mouse_x, global_mouse_y;
    Uint32 mouse_state = SDL_GetGlobalMouseState(&global_mouse_x, &global_mouse_y);
    int mouse_pressed = (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    
    // Get EQ window position
    int eq_window_x, eq_window_y;
    SDL_GetWindowPosition(eq_window, &eq_window_x, &eq_window_y);
    
    // Convert global mouse to window-local coordinates
    mouse_x = global_mouse_x - eq_window_x;
    mouse_y = global_mouse_y - eq_window_y;
    
    // Check if mouse is within window bounds
    int eq_has_focus = (mouse_x >= 0 && mouse_x < eq_window_width &&
                        mouse_y >= 0 && mouse_y < eq_window_height);
    
    // Clear EQ window
    SDL_SetRenderTarget(eq_renderer, NULL);
    SDL_SetRenderDrawColor(eq_renderer, 30, 30, 35, 255);
    SDL_RenderClear(eq_renderer);
    
    // Draw title
    draw_text_colored(eq_renderer, 20, 10, "Advanced Parametric EQ - Drag bands to adjust frequency", 220, 220, 100);
    draw_text_colored(eq_renderer, 20, 30, "Click band to select, use sliders below to fine-tune", 160, 160, 160);
    
    // EQ Graph area
    int graph_x = 50;
    int graph_y = 70;
    int graph_width = eq_window_width - 100;
    int graph_height = 250;
    
    // Draw graph background
    SDL_SetRenderDrawColor(eq_renderer, 40, 40, 45, 255);
    SDL_Rect graph_rect = {graph_x, graph_y, graph_width, graph_height};
    SDL_RenderFillRect(eq_renderer, &graph_rect);
    
    // Draw grid lines - horizontal (gain levels)
    SDL_SetRenderDrawColor(eq_renderer, 60, 60, 65, 255);
    for (int db = -24; db <= 24; db += 6) {
        int y = gain_to_y(db, graph_y, graph_height);
        SDL_RenderDrawLine(eq_renderer, graph_x, y, graph_x + graph_width, y);
        
        char label[8];
        snprintf(label, sizeof(label), "%+ddB", db);
        draw_text_colored(eq_renderer, graph_x - 45, y - 5, label, 140, 140, 140);
    }
    
    // 0dB line (center) - brighter
    SDL_SetRenderDrawColor(eq_renderer, 80, 80, 85, 255);
    int zero_y = gain_to_y(0.0f, graph_y, graph_height);
    SDL_RenderDrawLine(eq_renderer, graph_x, zero_y, graph_x + graph_width, zero_y);
    
    // Vertical lines (frequency markers)
    float freq_markers[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    SDL_SetRenderDrawColor(eq_renderer, 60, 60, 65, 255);
    for (int i = 0; i < 10; i++) {
        int x = freq_to_x(freq_markers[i], graph_x, graph_width);
        SDL_RenderDrawLine(eq_renderer, x, graph_y, x, graph_y + graph_height);
        
        char label[16];
        if (freq_markers[i] >= 1000) {
            snprintf(label, sizeof(label), "%.0fk", freq_markers[i] / 1000.0f);
        } else {
            snprintf(label, sizeof(label), "%.0f", freq_markers[i]);
        }
        draw_text_colored(eq_renderer, x - 10, graph_y + graph_height + 5, label, 140, 140, 140);
    }
    
    // Draw frequency response wave behind the bands
    draw_frequency_response_wave(graph_x, graph_y, graph_width, graph_height);
    
    // Draw EQ curve (band positions and influence)
    SDL_SetRenderDrawColor(eq_renderer, 100, 180, 255, 255);
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (!current_parametric_eq->bands[i].enabled) continue;
        
        float freq = current_parametric_eq->bands[i].frequency;
        float gain_db = current_parametric_eq->bands[i].gain_db;
        float q = current_parametric_eq->bands[i].q;
        SDL_Color base_color = get_band_color(i);
        
        int center_x = freq_to_x(freq, graph_x, graph_width);
        int center_y = gain_to_y(gain_db, graph_y, graph_height);
        
        // Draw band influence (bell curve approximation)
        int bandwidth_pixels = (int)(graph_width / (q * 10));
        SDL_SetRenderDrawColor(eq_renderer, base_color.r, base_color.g, base_color.b, 40);
        SDL_Rect band_rect = {center_x - bandwidth_pixels/2, graph_y, bandwidth_pixels, graph_height};
        SDL_RenderFillRect(eq_renderer, &band_rect);
        
        // Draw band point
        int point_size = (i == eq_selected_band) ? 12 : 8;
        SDL_Color point_color = base_color;
        if (i == eq_selected_band) {
            point_color.r = (Uint8)fminf(255.0f, point_color.r + 40.0f);
            point_color.g = (Uint8)fminf(255.0f, point_color.g + 40.0f);
            point_color.b = (Uint8)fminf(255.0f, point_color.b + 40.0f);
        }
        SDL_SetRenderDrawColor(eq_renderer, point_color.r, point_color.g, point_color.b, 255);
        SDL_Rect point_rect = {center_x - point_size/2, center_y - point_size/2, point_size, point_size};
        SDL_RenderFillRect(eq_renderer, &point_rect);
        
        // Draw outline
        SDL_SetRenderDrawColor(eq_renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(eq_renderer, &point_rect);
        
        // Draw band label
        char band_label[32];
        snprintf(band_label, sizeof(band_label), "%d", i + 1);
        draw_text_colored(eq_renderer, center_x - 4, center_y - 20, band_label, 255, 255, 255);
    }
    
    // Handle band dragging and selection
    if (eq_has_focus && mouse_pressed && !prev_mouse_pressed) {
        for (int i = 0; i < MAX_EQ_BANDS; i++) {
            if (!current_parametric_eq->bands[i].enabled) continue;
            
            float freq = current_parametric_eq->bands[i].frequency;
            float gain_db = current_parametric_eq->bands[i].gain_db;
            
            int center_x = freq_to_x(freq, graph_x, graph_width);
            int center_y = gain_to_y(gain_db, graph_y, graph_height);
            
            int dx = mouse_x - center_x;
            int dy = mouse_y - center_y;
            if (dx*dx + dy*dy < 144) {
                eq_selected_band = i;
                eq_dragging_band = i;
                break;
            }
        }
    }
    
    // Dragging band
    if (mouse_pressed && eq_dragging_band >= 0 && eq_has_focus) {
        if (mouse_x >= graph_x && mouse_x <= graph_x + graph_width &&
            mouse_y >= graph_y && mouse_y <= graph_y + graph_height) {
            
            float new_freq = x_to_freq(mouse_x, graph_x, graph_width);
            float new_gain = y_to_gain(mouse_y, graph_y, graph_height);
            
            new_freq = fmaxf(30.0f, fminf(18000.0f, new_freq));
            new_gain = fmaxf(-18.0f, fminf(18.0f, new_gain));
            
            current_parametric_eq->bands[eq_dragging_band].frequency = new_freq;
            current_parametric_eq->bands[eq_dragging_band].gain_db = new_gain;
            
            parametric_eq_update_filters(current_parametric_eq);
            request_eq_reprocess(0);
        }
    }
    
    if (!mouse_pressed) {
        eq_dragging_band = -1;
    }
    
    // Control panel at bottom
    int control_y = graph_y + graph_height + 40;
    draw_text_colored(eq_renderer, 20, control_y, "Band Controls:", 200, 200, 100);
    
    // Band selection buttons
    int btn_x = 20;
    int btn_y = control_y + 25;
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        SDL_Color btn_color = current_parametric_eq->bands[i].enabled ?
            (i == eq_selected_band ? (SDL_Color){255, 200, 50, 255} : (SDL_Color){70, 130, 180, 255}) :
            (SDL_Color){50, 50, 50, 255};
        
        SDL_SetRenderDrawColor(eq_renderer, btn_color.r, btn_color.g, btn_color.b, btn_color.a);
        SDL_Rect btn_rect = {btn_x + i * 45, btn_y, 40, 25};
        SDL_RenderFillRect(eq_renderer, &btn_rect);
        SDL_SetRenderDrawColor(eq_renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(eq_renderer, &btn_rect);
        
        char btn_text[4];
        snprintf(btn_text, sizeof(btn_text), "%d", i + 1);
        draw_text_colored(eq_renderer, btn_x + i * 45 + 14, btn_y + 6, btn_text, 255, 255, 255);
        
        int btn_hover = (mouse_x >= btn_rect.x && mouse_x <= btn_rect.x + btn_rect.w &&
                         mouse_y >= btn_rect.y && mouse_y <= btn_rect.y + btn_rect.h);
        
        if (btn_hover && mouse_pressed && !prev_mouse_pressed) {
            eq_selected_band = i;
        }
    }
    
    // Selected band parameters
    if (eq_selected_band >= 0 && eq_selected_band < MAX_EQ_BANDS) {
        ParametricBand* band = &current_parametric_eq->bands[eq_selected_band];
        
        int param_y = btn_y + 40;
        draw_text_colored(eq_renderer, 20, param_y, "Selected Band Parameters:", 180, 180, 180);
        param_y += 25;
        
        // Frequency display
        char freq_text[64];
        snprintf(freq_text, sizeof(freq_text), "Frequency: %.1f Hz", band->frequency);
        draw_text_colored(eq_renderer, 30, param_y, freq_text, 200, 200, 200);
        
        // Q factor slider
        param_y += 25;
        draw_text_colored(eq_renderer, 30, param_y, "Q Factor (Bandwidth):", 180, 180, 180);
        Slider q_slider = {200, param_y, 250, 20, band->q, 0.1f, 10.0f, 0, ""};
        if (draw_slider(eq_renderer, &q_slider, mouse_x, mouse_y, mouse_pressed)) {
            band->q = q_slider.value;
            parametric_eq_update_filters(current_parametric_eq);
            request_eq_reprocess(0);
        }
        char q_text[16];
        snprintf(q_text, sizeof(q_text), "%.2f", band->q);
        draw_text_colored(eq_renderer, 460, param_y, q_text, 200, 200, 100);
        
        // Gain display
        param_y += 25;
        char gain_text[64];
        snprintf(gain_text, sizeof(gain_text), "Gain: %+.1f dB", band->gain_db);
        draw_text_colored(eq_renderer, 30, param_y, gain_text, 200, 200, 200);
        
        // Enable/Disable button
        param_y += 30;
        SDL_Color enable_color = band->enabled ? 
            (SDL_Color){50, 180, 50, 255} : (SDL_Color){180, 50, 50, 255};
        SDL_SetRenderDrawColor(eq_renderer, enable_color.r, enable_color.g, enable_color.b, enable_color.a);
        SDL_Rect enable_rect = {30, param_y, 100, 30};
        SDL_RenderFillRect(eq_renderer, &enable_rect);
        SDL_SetRenderDrawColor(eq_renderer, 200, 200, 200, 255);
        SDL_RenderDrawRect(eq_renderer, &enable_rect);
        draw_text_colored(eq_renderer, 40, param_y + 8, band->enabled ? "Enabled" : "Disabled", 255, 255, 255);
        
        int enable_hover = (mouse_x >= enable_rect.x && mouse_x <= enable_rect.x + enable_rect.w &&
                            mouse_y >= enable_rect.y && mouse_y <= enable_rect.y + enable_rect.h);
        
        if (enable_hover && mouse_pressed && !prev_mouse_pressed) {
            band->enabled = !band->enabled;
            parametric_eq_update_filters(current_parametric_eq);
            request_eq_reprocess(0);
        }
    }
    
    // Close button
    SDL_SetRenderDrawColor(eq_renderer, 180, 50, 50, 255);
    SDL_Rect close_rect = {eq_window_width - 100, 10, 80, 30};
    SDL_RenderFillRect(eq_renderer, &close_rect);
    SDL_SetRenderDrawColor(eq_renderer, 200, 200, 200, 255);
    SDL_RenderDrawRect(eq_renderer, &close_rect);
    draw_text_colored(eq_renderer, eq_window_width - 80, 18, "Close", 255, 255, 255);
    
    int close_hover = (mouse_x >= close_rect.x && mouse_x <= close_rect.x + close_rect.w &&
                       mouse_y >= close_rect.y && mouse_y <= close_rect.y + close_rect.h);
    
    if (close_hover && mouse_pressed && !prev_mouse_pressed) {
        close_advanced_eq_window();
        return;
    }
    
    // Update prev_mouse_pressed for next frame
    prev_mouse_pressed = mouse_pressed;
    
    // Perform deferred reprocessing with rate limiting
    if (eq_reprocess_pending && eq_mixer_ref && eq_mixer_ref->auto_process) {
        Uint32 now = SDL_GetTicks();
        Uint32 since_process = now - eq_last_process_tick;
        Uint32 since_change = now - eq_last_change_tick;
        int ready = 0;
        if (since_process > 70 && since_change > 25) {
            ready = 1;
        } else if (!mouse_pressed && since_process > 35 && since_change > 30) {
            ready = 1;
        }
        if (ready) {
            mixer_process_effects(eq_mixer_ref);
            eq_last_process_tick = now;
            if (!mouse_pressed) {
                eq_reprocess_pending = 0;
            } else {
                eq_last_change_tick = now;
            }
        }
    }
    
    // Present EQ window
    SDL_RenderPresent(eq_renderer);
}
