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

// FFT for spectrum analysis
#define FFT_SIZE 2048
#define SPECTRUM_BARS 50

// Spectrum analyzer data
static float fft_input[FFT_SIZE];
static float fft_magnitude[FFT_SIZE / 2];
static float spectrum_bars[SPECTRUM_BARS];
static int spectrum_bar_freqs[SPECTRUM_BARS];
static Uint32 last_spectrum_update = 0;

// Audio level meters
static float left_channel_peak = 0.0f;
static float right_channel_peak = 0.0f;
static float overall_peak = 0.0f;
static float left_channel_rms = 0.0f;
static float right_channel_rms = 0.0f;

// GUI state
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static int window_width = 1200;
static int window_height = 800;
static int running = 1;

// Audio playback state
static SDL_AudioDeviceID audio_device = 0;
static SDL_AudioSpec audio_spec;
static AudioMixer* current_mixer = NULL;
static int is_playing = 0;
static int is_paused = 0;
static size_t playback_position = 0;
static float playback_volume = 0.7f;

// Audio callback function for SDL
void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioMixer* mixer = (AudioMixer*)userdata;
    
    if (!mixer || !mixer->processed_buffer || !mixer->processed_buffer->data || 
        !is_playing || is_paused) {
        // Fill with silence
        SDL_memset(stream, 0, len);
        return;
    }
    
    int samples_requested = len / sizeof(float);
    int channels = mixer->processed_buffer->channels;
    int frames_requested = samples_requested / channels;  // Number of sample frames
    float* output = (float*)stream;
    
    // Copy audio data for spectrum analysis
    static int fft_index = 0;
    
    for (int frame = 0; frame < frames_requested; frame++) {
        if (playback_position >= mixer->processed_buffer->length / channels) {
            // End of audio reached - fill remaining with silence
            for (int ch = 0; ch < channels; ch++) {
                output[frame * channels + ch] = 0.0f;
            }
            if (frame == 0) {
                is_playing = 0;  // Stop playback when audio ends
                playback_position = 0;
            }
        } else {
            // Get samples for all channels for this frame
            float sample_sum = 0.0f;
            float original_sample_sum = 0.0f;  // Track original levels for FFT
            float left_sample = 0.0f, right_sample = 0.0f;
            
            for (int ch = 0; ch < channels; ch++) {
                size_t sample_idx = playback_position * channels + ch;
                if (sample_idx < mixer->processed_buffer->capacity) {
                    float sample = mixer->processed_buffer->data[sample_idx];
                    original_sample_sum += sample;  // Store original before volume adjustment
                    float final_sample = sample * playback_volume;
                    output[frame * channels + ch] = final_sample;
                    sample_sum += final_sample;
                    
                    // Capture channel-specific levels for meters (using playback volume)
                    if (ch == 0) left_sample = final_sample;
                    if (ch == 1 || channels == 1) right_sample = final_sample;
                } else {
                    output[frame * channels + ch] = 0.0f;
                }
            }
            
            // Update peak and RMS levels for meters
            float left_abs = fabsf(left_sample);
            float right_abs = fabsf(right_sample);
            float overall_abs = fabsf(sample_sum / channels);
            
            // Peak detection with decay
            if (left_abs > left_channel_peak) {
                left_channel_peak = left_abs;
            } else {
                left_channel_peak *= 0.995f; // Slow decay
            }
            
            if (right_abs > right_channel_peak) {
                right_channel_peak = right_abs;
            } else {
                right_channel_peak *= 0.995f;
            }
            
            if (overall_abs > overall_peak) {
                overall_peak = overall_abs;
            } else {
                overall_peak *= 0.995f;
            }
            
            // RMS calculation (simplified)
            left_channel_rms = left_channel_rms * 0.99f + left_abs * left_abs * 0.01f;
            right_channel_rms = right_channel_rms * 0.99f + right_abs * right_abs * 0.01f;
            
            // Store ORIGINAL (pre-volume) mono sample for accurate FFT analysis
            if (fft_index < FFT_SIZE) {
                fft_input[fft_index] = original_sample_sum / channels;
                fft_index++;
                if (fft_index >= FFT_SIZE) {
                    fft_index = 0;  // Reset for next batch
                }
            }
            
            playback_position++;
        }
    }
}

// Simple FFT implementation (Cooley-Tukey algorithm)
void fft(float* real, float* imag, int N) {
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

// Update spectrum analysis
void update_spectrum_analysis(float sample_rate) {
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
        // FFT magnitude: divide by FFT_SIZE, multiply by 2 for single-sided spectrum
        // Apply 34dB gain (50x multiplier) for much better visualization
        float magnitude = sqrtf(fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i]);
        fft_magnitude[i] = (magnitude * 2.0f * 50.0f) / FFT_SIZE;
    }
    
    // Map FFT bins to spectrum bars
    float freq_per_bin = sample_rate / FFT_SIZE;
    
    // Debug: Check if we have any audio data and some actual spectrum values
    static int debug_counter = 0;
    if (debug_counter++ % 15 == 0) { // Print every 15 updates (~1 second)
        float total_input = 0;
        for (int i = 0; i < FFT_SIZE; i++) {
            total_input += fabsf(fft_input[i]);
        }
        // Also check some bar values
        float total_bars = 0;
        for (int i = 0; i < SPECTRUM_BARS; i++) {
            total_bars += spectrum_bars[i];
        }
        printf("Spectrum debug: total_input=%.3f, total_bars=%.3f, sample_rate=%.0f\n", 
               total_input, total_bars, sample_rate);
    }
    
    // Map FFT bins to spectrum bars - each bar covers a frequency range
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
        
        // Apply frequency-dependent boost for better visibility across all ranges
        float center_freq = (freq_start + freq_end) / 2.0f;
        float boost_factor = 1.0f;
        if (center_freq < 200.0f) {
            // Deep bass boost: 4x (12dB)
            boost_factor = 4.0f;
        } else if (center_freq < 1000.0f) {
            // Low-mid boost: 2x (6dB)
            boost_factor = 2.0f;
        } else if (center_freq < 2000.0f) {
            // Mid boost: 3x (9.5dB)
            boost_factor = 3.0f;
        } else if (center_freq < 4000.0f) {
            // Upper-mid boost: 5x (14dB)
            boost_factor = 5.0f;
        } else if (center_freq < 8000.0f) {
            // High frequency boost: 8x (18dB)
            boost_factor = 8.0f;
        } else {
            // Very high frequency boost: 12x to 20x (21.5-26dB)
            boost_factor = 12.0f + (center_freq - 8000.0f) / 12000.0f * 8.0f;
        }
        peak_magnitude *= boost_factor;
        
        // Convert to dBFS: 0 dBFS = 1.0 amplitude, -inf dBFS = 0.0 amplitude
        float dbfs = 20.0f * log10f(peak_magnitude + 1e-10f);
        // Map dB range: -30dBFS to +10dBFS becomes 0 to 40 for display
        float normalized_db = dbfs + 30.0f;
        normalized_db = fmaxf(0.0f, fminf(40.0f, normalized_db));
        
        // Smooth the bars
        spectrum_bars[bar] = spectrum_bars[bar] * 0.85f + normalized_db * 0.15f;
    }
}

// Initialize audio subsystem
int init_audio_playback(AudioMixer* mixer) {
    current_mixer = mixer;
    
    // Use the loaded audio file's properties
    int channels = 1;  // Default to mono
    int sample_rate = 44100;  // Default sample rate
    
    if (mixer && mixer->audio_buffer) {
        channels = mixer->audio_buffer->channels;
        sample_rate = mixer->audio_buffer->sample_rate;
        printf("Setting up audio for: %d channels, %d Hz\n", channels, sample_rate);
    }
    
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = sample_rate;
    desired_spec.format = AUDIO_F32SYS;  // 32-bit float
    desired_spec.channels = channels;    // Match the loaded audio file
    desired_spec.samples = 1024;         // Buffer size
    desired_spec.callback = audio_callback;
    desired_spec.userdata = mixer;
    
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &audio_spec, 0);
    if (audio_device == 0) {
        printf("Failed to open audio device: %s\\n", SDL_GetError());
        return 0;
    }
    
    printf("Audio device opened: %d Hz, %d channels, buffer size %d\\n", 
           audio_spec.freq, audio_spec.channels, audio_spec.samples);
    
    return 1;
}

// Cleanup audio subsystem
void cleanup_audio_playback(void) {
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    is_playing = 0;
    is_paused = 0;
    playback_position = 0;
}

// Playback control functions
void start_playback(void) {
    if (audio_device && current_mixer && current_mixer->processed_buffer) {
        is_playing = 1;
        is_paused = 0;
        SDL_PauseAudioDevice(audio_device, 0);  // Start playback
        printf("Playback started\\n");
    }
}

void pause_playback(void) {
    if (audio_device && is_playing) {
        is_paused = !is_paused;
        SDL_PauseAudioDevice(audio_device, is_paused ? 1 : 0);
        printf("Playback %s\\n", is_paused ? "paused" : "resumed");
    }
}

void stop_playback(void) {
    if (audio_device) {
        is_playing = 0;
        is_paused = 0;
        playback_position = 0;
        SDL_PauseAudioDevice(audio_device, 1);  // Stop playback
        printf("Playback stopped\\n");
    }
}

void seek_playback(float position) {
    if (current_mixer && current_mixer->processed_buffer) {
        int channels = current_mixer->processed_buffer->channels;
        size_t total_frames = current_mixer->processed_buffer->length / channels;
        size_t new_frame = (size_t)(position * total_frames);
        if (new_frame < total_frames) {
            playback_position = new_frame;
            printf("Seek to position: %.2f%%\\n", position * 100.0f);
        }
    }
}

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

// Audio playback functions (forward declarations)
void audio_callback(void* userdata, Uint8* stream, int len);
int init_audio_playback(AudioMixer* mixer);
void cleanup_audio_playback(void);
void start_playback(void);
void pause_playback(void);
void stop_playback(void);
void seek_playback(float position);

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
    
    // Initialize spectrum analyzer
    init_spectrum_analyzer();
    
    return 1;
}

// Shutdown GUI
void gui_shutdown(void) {
    cleanup_audio_playback();
    
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
    
    // Draw value text above slider (but not for position slider, which shows time separately)
    if (strcmp(slider->label, "Position") != 0) {
        char value_text[32];
        snprintf(value_text, sizeof(value_text), "%.2f", slider->value);
        int text_width = strlen(value_text) * 9;
        int value_x = slider->x + (slider->width - text_width) / 2;
        int value_y = slider->y - 12; // Above the slider
        draw_text_colored(value_x, value_y, value_text, 180, 180, 180);
    }
    
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
void draw_char(int x, int y, char c, int color_r, int color_g, int color_b) {
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

// Draw stereo volume meters (top left)
void draw_stereo_meters(int x, int y) {
    int meter_width = 15;
    int meter_height = 100;
    int meter_spacing = 20;
    
    // Convert to dB for display
    float left_db = left_channel_peak > 0.0f ? 20.0f * log10f(left_channel_peak) : -60.0f;
    float right_db = right_channel_peak > 0.0f ? 20.0f * log10f(right_channel_peak) : -60.0f;
    
    // Normalize to 0-1 range (-60dB to 0dB)
    float left_level = fmaxf(0.0f, (left_db + 60.0f) / 60.0f);
    float right_level = fmaxf(0.0f, (right_db + 60.0f) / 60.0f);
    
    // Left channel meter
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect left_bg = {x, y, meter_width, meter_height};
    SDL_RenderFillRect(renderer, &left_bg);
    
    // Left channel level (green to red gradient)
    int left_fill_height = (int)(left_level * meter_height);
    for (int i = 0; i < left_fill_height; i++) {
        float pos = (float)i / meter_height;
        Uint8 r = (Uint8)(pos * 255);
        Uint8 g = (Uint8)((1.0f - pos) * 255);
        SDL_SetRenderDrawColor(renderer, r, g, 0, 255);
        SDL_Rect line = {x, y + meter_height - i - 1, meter_width, 1};
        SDL_RenderFillRect(renderer, &line);
    }
    
    // Right channel meter
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_Rect right_bg = {x + meter_spacing, y, meter_width, meter_height};
    SDL_RenderFillRect(renderer, &right_bg);
    
    // Right channel level
    int right_fill_height = (int)(right_level * meter_height);
    for (int i = 0; i < right_fill_height; i++) {
        float pos = (float)i / meter_height;
        Uint8 r = (Uint8)(pos * 255);
        Uint8 g = (Uint8)((1.0f - pos) * 255);
        SDL_SetRenderDrawColor(renderer, r, g, 0, 255);
        SDL_Rect line = {x + meter_spacing, y + meter_height - i - 1, meter_width, 1};
        SDL_RenderFillRect(renderer, &line);
    }
    
    // Labels (moved up to stay in box)
    draw_text_colored(x + 2, y + meter_height - 10, "L", 200, 200, 200);
    draw_text_colored(x + meter_spacing + 2, y + meter_height - 10, "R", 200, 200, 200);
    
    // Live dB readouts on the right side
    char left_label[16], right_label[16];
    snprintf(left_label, sizeof(left_label), "L: %.1fdB", left_db);
    snprintf(right_label, sizeof(right_label), "R: %.1fdB", right_db);
    draw_text_colored(x + meter_spacing + meter_width + 2, y + 20, left_label, 0, 0, 0);
    draw_text_colored(x + meter_spacing + meter_width + 2, y + 40, right_label, 0, 0, 0);
}

// Draw dB level scale (right side)
void draw_db_scale(int x, int y, int height) {
    int scale_width = 50;
    
    // Background
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
    SDL_Rect scale_bg = {x, y, scale_width, height};
    SDL_RenderFillRect(renderer, &scale_bg);
    
    // Overall level meter
    float overall_db = overall_peak > 0.0f ? 20.0f * log10f(overall_peak) : -60.0f;
    float overall_level = fmaxf(0.0f, (overall_db + 60.0f) / 60.0f);
    
    int fill_height = (int)(overall_level * height);
    for (int i = 0; i < fill_height; i++) {
        float pos = (float)i / height;
        Uint8 r = (Uint8)(pos * 255);
        Uint8 g = (Uint8)((1.0f - pos) * 255);
        Uint8 b = 50;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect line = {x + 5, y + height - i - 1, scale_width - 25, 1};
        SDL_RenderFillRect(renderer, &line);
    }
    
    // dB scale marks and labels
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    int db_marks[] = {0, -6, -12, -18, -24, -30, -40, -50, -60};
    int num_marks = sizeof(db_marks) / sizeof(db_marks[0]);
    
    for (int i = 0; i < num_marks; i++) {
        int db = db_marks[i];
        float mark_pos = (float)(db + 60) / 60.0f;
        int mark_y = y + height - (int)(mark_pos * height);
        
        // Draw tick mark
        SDL_RenderDrawLine(renderer, x + scale_width - 20, mark_y, x + scale_width - 5, mark_y);
        
        // Draw label
        char db_text[8];
        snprintf(db_text, sizeof(db_text), "%d", db);
        draw_text_colored(x + scale_width - 4, mark_y - 5, db_text, 160, 160, 160);
    }
    
    // Current dB value
    char current_db_text[16];
    snprintf(current_db_text, sizeof(current_db_text), "%.1fdB", overall_db);
    draw_text_colored(x + 5, y - 20, current_db_text, 200, 200, 100);
}

// Draw spectrum visualizer as frequency analyzer graph
void draw_spectrum_visualizer(AudioMixer* mixer) {
    if (!mixer || !mixer->processed_buffer) return;
    
    // Update spectrum analysis (only from live audio data, don't reprocess effects)
    update_spectrum_analysis(mixer->sample_rate);
    
    // Graph dimensions and positioning
    int graph_x = 60;  // Leave space for dB scale on left
    int graph_y = window_height - 220;  // Reserve 220px at bottom
    int graph_height = 150;
    int graph_width = window_width - 120;  // Space for scales on both sides
    int bar_width = graph_width / SPECTRUM_BARS;
    
    // Draw graph background with grid
    SDL_SetRenderDrawColor(renderer, 15, 15, 25, 255);
    SDL_Rect graph_bg = {graph_x, graph_y, graph_width, graph_height};
    SDL_RenderFillRect(renderer, &graph_bg);
    
    // Draw graph border
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderDrawRect(renderer, &graph_bg);
    
    // Draw horizontal grid lines (dB levels)
    SDL_SetRenderDrawColor(renderer, 40, 40, 50, 255);
    for (int i = 1; i < 5; i++) {
        int y = graph_y + (i * graph_height / 4);
        SDL_RenderDrawLine(renderer, graph_x, y, graph_x + graph_width, y);
    }
    
    // Draw vertical grid lines (frequency divisions)
    for (int i = 1; i < SPECTRUM_BARS; i += SPECTRUM_BARS / 10) {
        int x = graph_x + i * bar_width;
        SDL_RenderDrawLine(renderer, x, graph_y, x, graph_y + graph_height);
    }
    
    // Draw spectrum bars
    for (int i = 0; i < SPECTRUM_BARS; i++) {
        int bar_x = graph_x + i * bar_width;
        // spectrum_bars[i] is in 0-40 range (representing -30dBFS to +10dBFS)
        float normalized_value = spectrum_bars[i];
        float normalized_db = normalized_value / 40.0f; // 0-1 range
        normalized_db = fmaxf(0.0f, fminf(1.0f, normalized_db));
        int bar_height = (int)(normalized_db * graph_height);
        
        // Debug: Print some bar values occasionally (convert back to actual dBFS)
        static int debug_bar_counter = 0;
        if (debug_bar_counter++ % 150 == 0 && i < 5) { // Print first few bars every ~10 seconds
            float actual_dbfs = normalized_value - 30.0f; // Convert from 0-40 range to -30 to +10 dBFS
            printf("Bar[%d]: dBFS=%.1f, normalized=%.3f, height=%d\n", i, actual_dbfs, normalized_db, bar_height);
        }
        
        // Enhanced color coding for frequency response
        Uint8 r, g, b;
        float freq_ratio = (float)i / SPECTRUM_BARS;
        if (freq_ratio < 0.1f) {
            // Sub-bass/bass - deep red
            r = 200; g = 0; b = 0;
        } else if (freq_ratio < 0.3f) {
            // Low-mid - orange to yellow
            r = 255; g = (Uint8)(200 * (freq_ratio - 0.1f) / 0.2f); b = 0;
        } else if (freq_ratio < 0.6f) {
            // Mid - yellow to green
            r = (Uint8)(255 * (0.6f - freq_ratio) / 0.3f); g = 255; b = 0;
        } else if (freq_ratio < 0.8f) {
            // High-mid - green to cyan
            r = 0; g = 255; b = (Uint8)(255 * (freq_ratio - 0.6f) / 0.2f);
        } else {
            // High frequencies - cyan to blue
            r = 0; g = (Uint8)(255 * (1.0f - freq_ratio) / 0.2f); b = 255;
        }
        
        // Add intensity based on amplitude
        float intensity = normalized_db;
        r = (Uint8)(r * intensity);
        g = (Uint8)(g * intensity);
        b = (Uint8)(b * intensity);
        
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_Rect bar = {bar_x, graph_y + graph_height - bar_height, bar_width - 1, bar_height};
        SDL_RenderFillRect(renderer, &bar);
    }
    
    // Draw dB scale on the left (range: +10dBFS at top to -30dBFS at bottom)
    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    const char* db_labels[] = {"+10", "0", "-10", "-20", "-30"};
    for (int i = 0; i < 5; i++) {
        int y = graph_y + (i * graph_height / 4);
        draw_text(10, y - 8, db_labels[i]);
    }
    
    // Draw detailed frequency labels below graph
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    const char* freq_labels[] = {"20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k"};
    int freq_positions[] = {0, SPECTRUM_BARS/10, SPECTRUM_BARS/5, 2*SPECTRUM_BARS/5, 
                           SPECTRUM_BARS/2, 3*SPECTRUM_BARS/5, 4*SPECTRUM_BARS/5, 
                           9*SPECTRUM_BARS/10, 19*SPECTRUM_BARS/20, SPECTRUM_BARS-1};
    
    for (int i = 0; i < 10; i++) {
        if (freq_positions[i] < SPECTRUM_BARS) {
            int x = graph_x + freq_positions[i] * bar_width;
            draw_text(x - 10, graph_y + graph_height + 10, freq_labels[i]);
        }
    }
    
    // Draw axis labels
    draw_text(10, graph_y - 20, "dB");
    draw_text(graph_x + graph_width/2 - 10, graph_y + graph_height + 35, "Hz");
    draw_text_colored(graph_x, graph_y - 40, "Frequency Analyzer", 220, 220, 100);
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
                    // Reinitialize audio with the new file's properties
                    cleanup_audio_playback();
                    init_audio_playback(mixer);
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
            
            // Stop playback during processing
            int was_playing = is_playing;
            if (is_playing) {
                stop_playback();
            }
            
            mixer_process_effects(mixer);
            printf("Effects processing complete\\n");
            
            // Restart playback if it was playing before
            if (was_playing && mixer->processed_buffer) {
                if (!audio_device) {
                    init_audio_playback(mixer);
                }
                start_playback();
            }
        }
    } else {
        process_button_pressed = 0;
    }
    
    // Audio controls panel (simplified)
    SDL_Rect audio_panel = {10, 100, window_width - 20, 80};
    SDL_RenderFillRect(renderer, &audio_panel);
    draw_text(20, 110, "Audio Controls");
    
    // Playback volume slider only
    Slider playback_vol_slider = {30, 130, 150, 30, playback_volume, 0.0f, 1.0f, 0, "Volume"};
    if (draw_slider(&playback_vol_slider, mouse_x, mouse_y, mouse_pressed)) {
        playback_volume = playback_vol_slider.value;
    }
    draw_text(30, 165, "Volume");
    
    // Stereo level meters (repositioned to top right)
    draw_stereo_meters(window_width - 150, 110);
    
    // Playback controls
    static int play_button_pressed = 0, pause_button_pressed = 0, stop_button_pressed = 0;
    
    Button play_btn = {200, 130, 60, 30, 0, 0, ""};
    strcpy(play_btn.text, is_playing && !is_paused ? "Playing" : "Play");
    if (draw_button(&play_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!play_button_pressed) {
            play_button_pressed = 1;
            if (!audio_device && mixer->processed_buffer) {
                init_audio_playback(mixer);
            }
            start_playback();
        }
    } else {
        play_button_pressed = 0;
    }
    
    Button pause_btn = {270, 130, 60, 30, 0, 0, ""};
    strcpy(pause_btn.text, is_paused ? "Resume" : "Pause");
    if (draw_button(&pause_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!pause_button_pressed) {
            pause_button_pressed = 1;
            pause_playback();
        }
    } else {
        pause_button_pressed = 0;
    }
    
    Button stop_btn = {340, 130, 60, 30, 0, 0, "Stop"};
    if (draw_button(&stop_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!stop_button_pressed) {
            stop_button_pressed = 1;
            stop_playback();
        }
    } else {
        stop_button_pressed = 0;
    }
    
    // Playback position slider (seek bar) - positioned to the right of Stop button
    if (mixer->processed_buffer && mixer->processed_buffer->length > 0) {
        int channels = mixer->processed_buffer->channels;
        size_t total_frames = mixer->processed_buffer->length / channels;
        float position = (float)playback_position / total_frames;
        
        // Debug output to check values (print once when playback starts)
        static int debug_printed = 0;
        if (is_playing && !debug_printed) {
            printf("=== PLAYBACK DEBUG ===\n");
            printf("total_frames=%zu\n", total_frames);
            printf("buffer sample_rate=%zu\n", mixer->processed_buffer->sample_rate);
            printf("mixer sample_rate=%.0f\n", mixer->sample_rate);
            printf("calculated total_time=%.2fs\n", (float)total_frames / mixer->processed_buffer->sample_rate);
            printf("playback_pos=%zu\n", playback_position);
            printf("======================\n");
            debug_printed = 1;
        }
        if (!is_playing) debug_printed = 0;
        
        static int seeking = 0;  // Track if user is actively seeking
        static float seek_position = 0.0f;
        
        // Seek bar positioned to the right of stop button
        Slider position_slider = {410, 130, 300, 20, seeking ? seek_position : position, 0.0f, 1.0f, 0, "Position"};
        
        // Check if mouse is over the slider and pressed
        int slider_mouse_over = mouse_x >= position_slider.x && mouse_x <= position_slider.x + position_slider.width &&
                                mouse_y >= position_slider.y && mouse_y <= position_slider.y + position_slider.height;
        
        if (mouse_pressed && slider_mouse_over) {
            seeking = 1;
            seek_position = (float)(mouse_x - position_slider.x) / position_slider.width;
            seek_position = fmaxf(0.0f, fminf(1.0f, seek_position));
        } else if (seeking && !mouse_pressed) {
            // User released mouse - apply the seek
            seeking = 0;
            seek_playback(seek_position);
        }
        
        // Draw the slider
        draw_slider(&position_slider, mouse_x, mouse_y, mouse_pressed);
        
        // Show playback time in MM:SS format - draw ABOVE the seek bar for visibility
        float current_time = (float)playback_position / mixer->processed_buffer->sample_rate;
        float total_time = (float)total_frames / mixer->processed_buffer->sample_rate;
        int current_mins = (int)current_time / 60;
        int current_secs = (int)current_time % 60;
        int total_mins = (int)total_time / 60;
        int total_secs = (int)total_time % 60;
        char time_text[64];
        snprintf(time_text, sizeof(time_text), "%d:%02d / %d:%02d", current_mins, current_secs, total_mins, total_secs);
        
        // Draw time display above the seek bar in light gray text
        draw_text_colored(position_slider.x + 10, position_slider.y - 20, time_text, 180, 180, 180);
        
        // Debug: Print time info every 2 seconds
        static int time_debug_counter = 0;
        if (is_playing && time_debug_counter++ % 120 == 0) {
            printf("TIME: pos=%zu/%zu (%.1f%%), time=%d:%02d/%d:%02d\n", 
                   playback_position, total_frames, position * 100.0f,
                   current_mins, current_secs, total_mins, total_secs);
        }
    }
    
    // Effects chain panel (adjusted for smaller audio controls)
    int effects_y = 190;
    int spectrum_height = 250;  // Reserve space for spectrum graph
    SDL_Rect effects_panel = {10, effects_y, window_width - 20, window_height - effects_y - spectrum_height - 30};
    SDL_RenderFillRect(renderer, &effects_panel);
    draw_text(20, effects_y + 10, "Effects Chain (Processing Order)");
    draw_text_colored(20, effects_y + 25, "Numbers show processing order: #1 → #2 → #3 → ... (signal flow)", 160, 160, 160);
    
    // Add effect buttons
    int btn_x = 30;
    int btn_y = effects_y + 45;  // Moved down for extra header text
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
            
            // Draw processing order number (clickable to edit)
            if (mixer->effects[i].type != EFFECT_NONE && mixer->effects[i].params.enabled) {
                // Draw editable order number in top-left corner
                char order_text[8];
                snprintf(order_text, sizeof(order_text), "%d", mixer->effects[i].processing_order);
                
                // Make it clickable
                SDL_Rect order_bg = {slot_x + 5, slot_y + 5, 25, 15};
                static int order_editing[MAX_EFFECTS] = {0};
                static int order_buttons_pressed[MAX_EFFECTS] = {0};
                
                // Check if order number is clicked
                int order_mouse_over = mouse_x >= order_bg.x && mouse_x <= order_bg.x + order_bg.w &&
                                     mouse_y >= order_bg.y && mouse_y <= order_bg.y + order_bg.h;
                
                if (mouse_pressed && order_mouse_over && !order_buttons_pressed[i]) {
                    order_buttons_pressed[i] = 1;
                    // Cycle through processing orders 1-8
                    mixer->effects[i].processing_order++;
                    if (mixer->effects[i].processing_order > 8) {
                        mixer->effects[i].processing_order = 1;
                    }
                    if (mixer->auto_process) {
                        mixer_process_effects(mixer);
                    }
                } else if (!mouse_pressed) {
                    order_buttons_pressed[i] = 0;
                }
                
                // Draw background with hover effect
                if (order_mouse_over) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);  // Brighter yellow when hovering
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);  // Normal yellow
                }
                SDL_RenderFillRect(renderer, &order_bg);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &order_bg);
                
                draw_text_colored(slot_x + 12, slot_y + 7, order_text, 0, 0, 0);  // Black text, centered
                
                // Adjust effect name position to make room for order number
                draw_text(slot_x + 35, slot_y + 5, mixer->effects[i].name);
            } else {
                // No order number for disabled/empty effects
                draw_text(slot_x + 5, slot_y + 5, mixer->effects[i].name);
            }
            
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
                        // Less frequent real-time processing to reduce lag
                        if (mixer->auto_process) {
                            static Uint32 last_process_time = 0;
                            Uint32 current_time = SDL_GetTicks();
                            if (current_time - last_process_time > 50) { // Max 20 FPS for parameter changes
                                mixer_process_effects(mixer);
                                last_process_time = current_time;
                                
                                // If audio is playing, briefly pause and resume to update the audio buffer
                                if (is_playing && audio_device) {
                                    SDL_LockAudioDevice(audio_device);
                                    // Audio processing happens here safely
                                    SDL_UnlockAudioDevice(audio_device);
                                }
                            }
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
    
    // Draw spectrum visualizer
    draw_spectrum_visualizer(mixer);
    
    // Status information
    if (mixer->audio_buffer) {
        char status[256];
        snprintf(status, sizeof(status), "Audio: %zu samples, %zu channels, %.0f Hz | Effects: %d active",
                mixer->audio_buffer->length, mixer->audio_buffer->channels, 
                mixer->sample_rate, mixer->num_effects);
        draw_text(20, window_height - 30, status);
    }
    
    // Draw version number in bottom right corner
    char version_text[32];
    snprintf(version_text, sizeof(version_text), "v1.2.6");
    draw_text_colored(window_width - 80, window_height - 25, version_text, 100, 100, 100);
    
    // Present the rendered frame
    SDL_RenderPresent(renderer);
    
    return 1;
}

// Handle events (called from main loop)
void gui_handle_events(void) {
    // Events are handled in gui_render_frame for this simple implementation
}