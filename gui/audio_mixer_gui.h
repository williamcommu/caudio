#ifndef AUDIO_MIXER_GUI_H
#define AUDIO_MIXER_GUI_H

#include "audio_core.h"
#include "audio_filters.h"
#include "delay_effects.h"
#include "reverb.h"
#include "distortion.h"
#include "modulation_effects.h"

// Maximum number of effects in the chain
#define MAX_EFFECTS 8
#define MAX_FILENAME 256

// Effect types for the GUI
typedef enum {
    EFFECT_NONE = 0,
    EFFECT_LOWPASS,
    EFFECT_HIGHPASS,
    EFFECT_EQ,
    EFFECT_ECHO,
    EFFECT_REVERB,
    EFFECT_OVERDRIVE,
    EFFECT_TUBE,
    EFFECT_FUZZ,
    EFFECT_CHORUS,
    EFFECT_FLANGER,
    EFFECT_PHASER,
    EFFECT_TREMOLO,
    EFFECT_COUNT
} EffectType;

// Effect parameter structure
typedef struct {
    float param1;
    float param2;
    float param3;
    float param4;
    float mix;
    int enabled;
} EffectParams;

// Effect chain slot
typedef struct {
    EffectType type;
    EffectParams params;
    void* effect_instance;  // Pointer to the actual effect
    char name[64];
    int processing_order;   // User-defined processing order (1-8)
} EffectSlot;

// Audio mixer state
typedef struct {
    // Audio data
    AudioBuffer* audio_buffer;
    AudioBuffer* processed_buffer;
    AudioBuffer* processing_buffer;   // Working buffer for background effect processing
    char input_filename[MAX_FILENAME];
    char output_filename[MAX_FILENAME];
    
    // Effect chain
    EffectSlot effects[MAX_EFFECTS];
    int num_effects;
    
    // Playback state
    int is_playing;
    int is_processed;
    size_t playback_position;
    
    // Master controls
    float master_volume;
    float dry_wet_mix;
    
    // GUI state
    int show_file_dialog;
    int show_about;
    int auto_process;
    
    // Audio settings
    float sample_rate;
    int channels;
} AudioMixer;

// GUI function declarations
int gui_init(void);
void gui_shutdown(void);
int gui_render_frame(AudioMixer* mixer);
void gui_handle_events(void);

// Audio processing functions
void mixer_init(AudioMixer* mixer);
void mixer_cleanup(AudioMixer* mixer);
int mixer_load_audio(AudioMixer* mixer, const char* filename);
int mixer_save_audio(AudioMixer* mixer, const char* filename);
void mixer_process_effects(AudioMixer* mixer);
void mixer_add_effect(AudioMixer* mixer, EffectType type);
void mixer_remove_effect(AudioMixer* mixer, int index);
void mixer_move_effect(AudioMixer* mixer, int from, int to);

// Effect management
void* create_effect_instance(EffectType type, float sample_rate);
void destroy_effect_instance(EffectType type, void* instance);
void process_effect(EffectType type, void* instance, EffectParams* params, AudioBuffer* buffer);
const char* get_effect_name(EffectType type);
const char* get_param_name(EffectType type, int param_index);
void get_param_range(EffectType type, int param_index, float* min, float* max);

#endif // AUDIO_MIXER_GUI_H