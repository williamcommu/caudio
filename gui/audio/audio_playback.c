// Audio playback implementation
#include "audio_playback.h"
#include "spectrum_analyzer.h"
#include "../widgets/ui_widgets.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Audio level meters
float left_channel_peak = 0.0f;
float right_channel_peak = 0.0f;
float overall_peak = 0.0f;
float left_channel_rms = 0.0f;
float right_channel_rms = 0.0f;

// Audio playback state
static SDL_AudioDeviceID audio_device = 0;
static SDL_AudioSpec audio_spec;
static AudioMixer* current_mixer = NULL;
int is_playing = 0;
int is_paused = 0;
size_t playback_position = 0;
float playback_volume = 0.7f;

// Audio callback function for SDL
static void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioMixer* mixer = (AudioMixer*)userdata;
    
    if (!mixer || !mixer->processed_buffer || !mixer->processed_buffer->data || 
        !is_playing || is_paused) {
        // Fill with silence
        SDL_memset(stream, 0, len);
        return;
    }
    
    int samples_requested = len / sizeof(float);
    int channels = mixer->processed_buffer->channels;
    int frames_requested = samples_requested / channels;
    float* output = (float*)stream;
    
    for (int frame = 0; frame < frames_requested; frame++) {
        if (playback_position >= mixer->processed_buffer->length / channels) {
            // End of audio reached - fill remaining with silence
            for (int ch = 0; ch < channels; ch++) {
                output[frame * channels + ch] = 0.0f;
            }
            if (frame == 0) {
                is_playing = 0;
                playback_position = 0;
            }
        } else {
            // Get samples for all channels for this frame
            float sample_sum = 0.0f;
            float original_sample_sum = 0.0f;
            float left_sample = 0.0f, right_sample = 0.0f;
            
            for (int ch = 0; ch < channels; ch++) {
                size_t sample_idx = playback_position * channels + ch;
                if (sample_idx < mixer->processed_buffer->capacity) {
                    float sample = mixer->processed_buffer->data[sample_idx];
                    original_sample_sum += sample;
                    float final_sample = sample * playback_volume;
                    output[frame * channels + ch] = final_sample;
                    sample_sum += final_sample;
                    
                    // Capture channel-specific levels for meters
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
                left_channel_peak *= 0.995f;
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
            
            // RMS calculation
            left_channel_rms = left_channel_rms * 0.99f + left_abs * left_abs * 0.01f;
            right_channel_rms = right_channel_rms * 0.99f + right_abs * right_abs * 0.01f;
            
            // Store sample for spectrum analysis
            spectrum_add_sample(original_sample_sum / channels);
            
            playback_position++;
        }
    }
}

// Initialize audio subsystem
int init_audio_playback(AudioMixer* mixer) {
    current_mixer = mixer;
    
    int channels = 1;
    int sample_rate = 44100;
    
    if (mixer && mixer->audio_buffer) {
        channels = mixer->audio_buffer->channels;
        sample_rate = mixer->audio_buffer->sample_rate;
        printf("Setting up audio for: %d channels, %d Hz\n", channels, sample_rate);
    }
    
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = sample_rate;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = channels;
    desired_spec.samples = 1024;
    desired_spec.callback = audio_callback;
    desired_spec.userdata = mixer;
    
    audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &audio_spec, 0);
    if (audio_device == 0) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        return 0;
    }
    
    printf("Audio device opened: %d Hz, %d channels, buffer size %d\n", 
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
        SDL_PauseAudioDevice(audio_device, 0);
        printf("Playback started\n");
    }
}

void toggle_pause(void) {
    if (audio_device && is_playing) {
        is_paused = !is_paused;
        SDL_PauseAudioDevice(audio_device, is_paused ? 1 : 0);
        printf("Playback %s\n", is_paused ? "paused" : "resumed");
    }
}

void stop_playback(void) {
    if (audio_device) {
        is_playing = 0;
        is_paused = 0;
        playback_position = 0;
        SDL_PauseAudioDevice(audio_device, 1);
        printf("Playback stopped\n");
    }
}

void seek_playback(float position_normalized) {
    if (current_mixer && current_mixer->processed_buffer) {
        int channels = current_mixer->processed_buffer->channels;
        size_t total_frames = current_mixer->processed_buffer->length / channels;
        size_t new_frame = (size_t)(position_normalized * total_frames);
        if (new_frame < total_frames) {
            playback_position = new_frame;
            printf("Seek to position: %.2f%%\n", position_normalized * 100.0f);
        }
    }
}

// Get SDL audio device for locking
SDL_AudioDeviceID get_audio_device(void) {
    return audio_device;
}

// Draw stereo volume meters
void draw_stereo_meters(SDL_Renderer* renderer, int x, int y) {
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
    
    // Labels
    draw_text_colored(renderer, x + 2, y + meter_height - 10, "L", 200, 200, 200);
    draw_text_colored(renderer, x + meter_spacing + 2, y + meter_height - 10, "R", 200, 200, 200);
    
    // Live dB readouts
    char left_label[16], right_label[16];
    snprintf(left_label, sizeof(left_label), "L: %.1fdB", left_db);
    snprintf(right_label, sizeof(right_label), "R: %.1fdB", right_db);
    draw_text_colored(renderer, x + meter_spacing + meter_width + 2, y + 20, left_label, 0, 0, 0);
    draw_text_colored(renderer, x + meter_spacing + meter_width + 2, y + 40, right_label, 0, 0, 0);
}

// Draw dB level scale
void draw_db_scale(SDL_Renderer* renderer, int x, int y, int height) {
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
        draw_text_colored(renderer, x + scale_width - 4, mark_y - 5, db_text, 160, 160, 160);
    }
    
    // Current dB value
    char current_db_text[16];
    snprintf(current_db_text, sizeof(current_db_text), "%.1fdB", overall_db);
    draw_text_colored(renderer, x + 5, y - 20, current_db_text, 200, 200, 100);
}
