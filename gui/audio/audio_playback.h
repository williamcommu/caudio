// Audio playback module
#ifndef AUDIO_PLAYBACK_H
#define AUDIO_PLAYBACK_H

#include "../audio_mixer_gui.h"
#include <SDL2/SDL.h>

// Audio level meters
extern float left_channel_peak;
extern float right_channel_peak;
extern float overall_peak;
extern float left_channel_rms;
extern float right_channel_rms;

// Audio playback state
extern int is_playing;
extern int is_paused;
extern size_t playback_position;
extern float playback_volume;

// Initialize audio subsystem
int init_audio_playback(AudioMixer* mixer);

// Cleanup audio subsystem
void cleanup_audio_playback(void);

// Playback control functions
void start_playback(void);
void stop_playback(void);
void toggle_pause(void);
void seek_playback(float position_normalized);

// Get SDL audio device for locking
SDL_AudioDeviceID get_audio_device(void);

// Draw stereo volume meters
void draw_stereo_meters(SDL_Renderer* renderer, int x, int y);

// Draw dB level scale
void draw_db_scale(SDL_Renderer* renderer, int x, int y, int height);

#endif // AUDIO_PLAYBACK_H
