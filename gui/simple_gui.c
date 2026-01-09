// Modular GUI implementation using SDL2
// Refactored to use separate modules for UI widgets, spectrum, audio playback, and EQ window

#define _GNU_SOURCE
#include "audio_mixer_gui.h"
#include "widgets/ui_widgets.h"
#include "audio/spectrum_analyzer.h"
#include "audio/audio_playback.h"
#include "windows/eq_window.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// GUI state
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static int window_width = 1200;
static int window_height = 800;

// File dialog function
static char* open_file_dialog(void) {
    static char selected_file[512] = {0};
    selected_file[0] = '\0';
    FILE* fp = NULL;
    
    // Try zenity (GNOME)
    fp = popen("zenity --file-selection --title=\"Select Audio File\" "
               "--file-filter=\"Audio files | *.wav *.mp3 *.flac *.ogg\" 2>/dev/null", "r");
    if (fp && fgets(selected_file, sizeof(selected_file), fp)) {
        size_t len = strlen(selected_file);
        if (len > 0 && selected_file[len-1] == '\n') selected_file[len-1] = '\0';
        pclose(fp);
        if (strlen(selected_file) > 0) return selected_file;
    }
    if (fp) pclose(fp);
    
    // Try kdialog (KDE)
    fp = popen("kdialog --getopenfilename . \"*.wav *.mp3 *.flac *.ogg | Audio files\" 2>/dev/null", "r");
    if (fp && fgets(selected_file, sizeof(selected_file), fp)) {
        size_t len = strlen(selected_file);
        if (len > 0 && selected_file[len-1] == '\n') selected_file[len-1] = '\0';
        pclose(fp);
        if (strlen(selected_file) > 0) return selected_file;
    }
    if (fp) pclose(fp);
    
    // Fallback to sample files
    const char* fallback_files[] = {"audio_samples/chain_original.wav", "test.wav"};
    for (int i = 0; i < 2; i++) {
        if (access(fallback_files[i], F_OK) == 0) {
            strcpy(selected_file, fallback_files[i]);
            return selected_file;
        }
    }
    return NULL;
}

// Initialize GUI
int gui_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 0;
    }
    
    window = SDL_CreateWindow("Audio Effects Mixer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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
    
    init_spectrum_analyzer();
    return 1;
}

// Shutdown GUI
void gui_shutdown(void) {
    cleanup_audio_playback();
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

// Main GUI rendering function
int gui_render_frame(AudioMixer* mixer) {
    SDL_Event event;
    int mouse_x, mouse_y;
    int mouse_pressed = SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_LEFT);
    SDL_Window* focused_window = SDL_GetMouseFocus();
    int main_window_in_focus = (focused_window == window);
    if (is_eq_window_open() && !main_window_in_focus) {
        mouse_pressed = 0;
        mouse_x = -1000;
        mouse_y = -1000;
    }
    
    // Handle events
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return 0;
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) return 0;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            window_width = event.window.data1;
            window_height = event.window.data2;
        }
    }
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);
    SDL_RenderClear(renderer);
    
    // File operations panel
    SDL_SetRenderDrawColor(renderer, COLOR_PANEL.r, COLOR_PANEL.g, COLOR_PANEL.b, COLOR_PANEL.a);
    SDL_Rect file_panel = {10, 10, window_width - 20, 80};
    SDL_RenderFillRect(renderer, &file_panel);
    draw_text(renderer, 20, 20, "File Operations");
    
    // Load button
    static int load_pressed = 0;
    Button load_btn = {30, 40, 100, 30, 0, 0, "Load Audio"};
    if (draw_button(renderer, &load_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!load_pressed) {
            load_pressed = 1;
            char* file = open_file_dialog();
            if (file && mixer_load_audio(mixer, file)) {
                cleanup_audio_playback();
                init_audio_playback(mixer);
                printf("Loaded: %s\n", file);
            }
        }
    } else load_pressed = 0;
    
    // Save button
    static int save_pressed = 0;
    Button save_btn = {140, 40, 100, 30, 0, 0, "Save Audio"};
    if (draw_button(renderer, &save_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!save_pressed) {
            save_pressed = 1;
            const char* output = mixer->output_filename[0] ? mixer->output_filename : "processed_audio.wav";
            if (mixer_save_audio(mixer, output)) printf("Saved: %s\n", output);
        }
    } else save_pressed = 0;
    
    // Process button
    static int process_pressed = 0;
    Button process_btn = {250, 40, 100, 30, 0, 0, "Process"};
    if (draw_button(renderer, &process_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!process_pressed) {
            process_pressed = 1;
            int was_playing = is_playing;
            if (is_playing) stop_playback();
            mixer_process_effects(mixer);
            if (was_playing && mixer->processed_buffer) start_playback();
        }
    } else process_pressed = 0;
    
    // Audio controls panel
    SDL_Rect audio_panel = {10, 100, window_width - 20, 80};
    SDL_RenderFillRect(renderer, &audio_panel);
    draw_text(renderer, 20, 110, "Audio Controls");
    
    // Volume slider
    Slider vol_slider = {30, 130, 150, 30, playback_volume, 0.0f, 1.0f, 0, "Volume"};
    if (draw_slider(renderer, &vol_slider, mouse_x, mouse_y, mouse_pressed)) {
        playback_volume = vol_slider.value;
    }
    draw_text(renderer, 30, 165, "Volume");
    
    // Stereo meters
    draw_stereo_meters(renderer, window_width - 150, 110);
    
    // Playback controls
    static int play_pressed = 0, pause_pressed = 0, stop_pressed = 0;
    Button play_btn = {200, 130, 60, 30, 0, 0, ""};
    strcpy(play_btn.text, is_playing && !is_paused ? "Playing" : "Play");
    Button pause_btn = {270, 130, 60, 30, 0, 0, ""};
    strcpy(pause_btn.text, is_paused ? "Resume" : "Pause");
    Button stop_btn = {340, 130, 60, 30, 0, 0, "Stop"};
    
    if (draw_button(renderer, &play_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!play_pressed) {
            play_pressed = 1;
            if (!get_audio_device() && mixer->processed_buffer) init_audio_playback(mixer);
            start_playback();
        }
    } else play_pressed = 0;
    
    if (draw_button(renderer, &pause_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!pause_pressed) {
            pause_pressed = 1;
            toggle_pause();
        }
    } else pause_pressed = 0;
    
    if (draw_button(renderer, &stop_btn, mouse_x, mouse_y, mouse_pressed)) {
        if (!stop_pressed) {
            stop_pressed = 1;
            stop_playback();
        }
    } else stop_pressed = 0;
    
    // Seek bar
    if (mixer->processed_buffer && mixer->processed_buffer->length > 0) {
        int channels = mixer->processed_buffer->channels;
        size_t total_frames = mixer->processed_buffer->length / channels;
        float position = (float)playback_position / total_frames;
        
        static int seeking = 0;
        static float seek_pos = 0.0f;
        Slider pos_slider = {410, 130, 300, 20, seeking ? seek_pos : position, 0.0f, 1.0f, 0, "Position"};
        
        int slider_hover = mouse_x >= pos_slider.x && mouse_x <= pos_slider.x + pos_slider.width &&
                          mouse_y >= pos_slider.y && mouse_y <= pos_slider.y + pos_slider.height;
        
        if (mouse_pressed && slider_hover) {
            seeking = 1;
            seek_pos = (float)(mouse_x - pos_slider.x) / pos_slider.width;
            seek_pos = fmaxf(0.0f, fminf(1.0f, seek_pos));
        } else if (seeking && !mouse_pressed) {
            seeking = 0;
            seek_playback(seek_pos);
        }
        
        draw_slider(renderer, &pos_slider, mouse_x, mouse_y, mouse_pressed);
        
        // Time display
        float current_time = (float)playback_position / mixer->processed_buffer->sample_rate;
        float total_time = (float)total_frames / mixer->processed_buffer->sample_rate;
        char time_text[64];
        snprintf(time_text, sizeof(time_text), "%d:%02d / %d:%02d",
                (int)current_time / 60, (int)current_time % 60,
                (int)total_time / 60, (int)total_time % 60);
        draw_text_colored(renderer, pos_slider.x + 10, pos_slider.y - 20, time_text, 180, 180, 180);
    }
    
    // Effects chain panel
    int effects_y = 190;
    int spectrum_height = 250;
    SDL_Rect effects_panel = {10, effects_y, window_width - 20, window_height - effects_y - spectrum_height - 30};
    SDL_RenderFillRect(renderer, &effects_panel);
    draw_text(renderer, 20, effects_y + 10, "Effects Chain (Processing Order)");
    draw_text_colored(renderer, 20, effects_y + 25, "Click order number to change, click Advanced for parametric EQ", 160, 160, 160);
    
    // Add effect buttons
    const char* effect_names[] = {"Lowpass", "Highpass", "EQ", "Echo", "Reverb", "Overdrive", "Tube", "Fuzz", "Chorus", "Flanger", "Phaser", "Tremolo"};
    EffectType effect_types[] = {EFFECT_LOWPASS, EFFECT_HIGHPASS, EFFECT_EQ, EFFECT_ECHO, EFFECT_REVERB, EFFECT_OVERDRIVE, EFFECT_TUBE, EFFECT_FUZZ, EFFECT_CHORUS, EFFECT_FLANGER, EFFECT_PHASER, EFFECT_TREMOLO};
    
    int btn_x = 30, btn_y = effects_y + 45;
    static int effect_btns_pressed[12] = {0};
    
    for (int i = 0; i < 12; i++) {
        if (btn_x + 100 > window_width - 20) {
            btn_x = 30;
            btn_y += 40;
        }
        
        Button effect_btn = {btn_x, btn_y, 100, 30, 0, 0, ""};
        strcpy(effect_btn.text, effect_names[i]);
        
        if (draw_button(renderer, &effect_btn, mouse_x, mouse_y, mouse_pressed)) {
            if (!effect_btns_pressed[i]) {
                effect_btns_pressed[i] = 1;
                mixer_add_effect(mixer, effect_types[i]);
            }
        } else effect_btns_pressed[i] = 0;
        
        btn_x += 110;
    }
    
    // Draw current effects
    int effect_slot_y = btn_y + 60;
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (mixer->effects[i].type != EFFECT_NONE) {
            int slot_x = 30 + (i % 4) * 290;
            int slot_y = effect_slot_y + (i / 4) * 120;
            
            if (slot_y + 100 > window_height - 20) break;
            
            // Effect slot
            SDL_Rect slot_rect = {slot_x, slot_y, 280, 100};
            SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
            SDL_RenderFillRect(renderer, &slot_rect);
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &slot_rect);
            
            // Processing order number
            if (mixer->effects[i].params.enabled) {
                char order_text[8];
                snprintf(order_text, sizeof(order_text), "%d", mixer->effects[i].processing_order);
                SDL_Rect order_bg = {slot_x + 5, slot_y + 5, 25, 15};
                static int order_pressed[MAX_EFFECTS] = {0};
                
                int order_hover = mouse_x >= order_bg.x && mouse_x <= order_bg.x + order_bg.w &&
                                 mouse_y >= order_bg.y && mouse_y <= order_bg.y + order_bg.h;
                
                if (mouse_pressed && order_hover && !order_pressed[i]) {
                    order_pressed[i] = 1;
                    mixer->effects[i].processing_order++;
                    if (mixer->effects[i].processing_order > 8) mixer->effects[i].processing_order = 1;
                    if (mixer->auto_process) mixer_process_effects(mixer);
                } else if (!mouse_pressed) order_pressed[i] = 0;
                
                SDL_SetRenderDrawColor(renderer, order_hover ? 255 : 200, order_hover ? 255 : 200, order_hover ? 150 : 100, 255);
                SDL_RenderFillRect(renderer, &order_bg);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawRect(renderer, &order_bg);
                draw_text_colored(renderer, slot_x + 12, slot_y + 7, order_text, 0, 0, 0);
                
                draw_text(renderer, slot_x + 35, slot_y + 5, mixer->effects[i].name);
            } else {
                draw_text(renderer, slot_x + 5, slot_y + 5, mixer->effects[i].name);
            }
            
            // Advanced EQ button
            if (mixer->effects[i].type == EFFECT_EQ) {
                static int adv_eq_pressed[MAX_EFFECTS] = {0};
                Button adv_eq_btn = {slot_x + 180, slot_y + 5, 55, 20, 0, 0, "Advanced"};
                if (draw_button(renderer, &adv_eq_btn, mouse_x, mouse_y, mouse_pressed)) {
                    if (!adv_eq_pressed[i]) {
                        adv_eq_pressed[i] = 1;
                        open_advanced_eq_window(mixer, i, renderer, get_audio_device(), is_playing);
                    }
                } else adv_eq_pressed[i] = 0;
            }
            
            // Remove button
            static int remove_pressed[MAX_EFFECTS] = {0};
            Button remove_btn = {slot_x + 240, slot_y + 5, 30, 20, 0, 0, "X"};
            if (draw_button(renderer, &remove_btn, mouse_x, mouse_y, mouse_pressed)) {
                if (!remove_pressed[i]) {
                    remove_pressed[i] = 1;
                    if (is_eq_window_open()) close_advanced_eq_window();
                    mixer_remove_effect(mixer, i);
                }
            } else remove_pressed[i] = 0;
            
            // Enable/disable button
            static int enable_pressed[MAX_EFFECTS] = {0};
            Button enable_btn = {slot_x + 5, slot_y + 25, 60, 20, 0, 0, ""};
            strcpy(enable_btn.text, mixer->effects[i].params.enabled ? "Enabled" : "Disabled");
            if (draw_button(renderer, &enable_btn, mouse_x, mouse_y, mouse_pressed)) {
                if (!enable_pressed[i]) {
                    enable_pressed[i] = 1;
                    mixer->effects[i].params.enabled = !mixer->effects[i].params.enabled;
                    if (mixer->auto_process) mixer_process_effects(mixer);
                }
            } else enable_pressed[i] = 0;
            
            // Parameter sliders
            for (int p = 0; p < 3; p++) {
                const char* param_name = get_param_name(mixer->effects[i].type, p);
                if (strlen(param_name) > 0) {
                    float min_val, max_val;
                    get_param_range(mixer->effects[i].type, p, &min_val, &max_val);
                    
                    float* param_value = (p == 0) ? &mixer->effects[i].params.param1 :
                                        (p == 1) ? &mixer->effects[i].params.param2 :
                                                  &mixer->effects[i].params.param3;
                    
                    Slider param_slider = {slot_x + 70 + p * 65, slot_y + 50, 60, 20, *param_value, min_val, max_val, 0, ""};
                    
                    if (draw_slider(renderer, &param_slider, mouse_x, mouse_y, mouse_pressed)) {
                        *param_value = param_slider.value;
                        if (mixer->auto_process) {
                            static Uint32 last_process = 0;
                            Uint32 now = SDL_GetTicks();
                            if (now - last_process > 50) {
                                mixer_process_effects(mixer);
                                last_process = now;
                            }
                        }
                    }
                    
                    int label_width = strlen(param_name) * 9;
                    int label_x = slot_x + 70 + p * 65 + (60 - label_width) / 2;
                    draw_text_colored(renderer, label_x, slot_y + 75, param_name, 160, 160, 160);
                }
            }
        }
    }
    
    // Update and draw spectrum
    update_spectrum_analysis(mixer->sample_rate, is_playing, is_paused);
    draw_spectrum_visualizer(renderer, window_width, window_height);
    
    // Status info
    if (mixer->audio_buffer) {
        char status[256];
        snprintf(status, sizeof(status), "Audio: %zu samples, %zu channels, %.0f Hz | Effects: %d active",
                mixer->audio_buffer->length, mixer->audio_buffer->channels, mixer->sample_rate, mixer->num_effects);
        draw_text(renderer, 20, window_height - 30, status);
    }
    
    // Version
    draw_text_colored(renderer, window_width - 80, window_height - 25, "v1.3.1", 100, 100, 100);
    
    // Present
    SDL_RenderPresent(renderer);
    
    // Render EQ window if open
    if (is_eq_window_open()) {
        render_advanced_eq_window(mixer);
    }
    
    return 1;
}

void gui_handle_events(void) {
    // Events handled in gui_render_frame
}
