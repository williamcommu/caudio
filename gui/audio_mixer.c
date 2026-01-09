#include "audio_mixer_gui.h"
#include "../include/wav_io.h"
#include "../include/audio_core.h"
#include "../include/audio_filters.h"
#include "../include/delay_effects.h"
#include "../include/reverb.h"
#include "../include/distortion.h"
#include "../include/modulation_effects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Initialize the mixer
void mixer_init(AudioMixer* mixer) {
    memset(mixer, 0, sizeof(AudioMixer));
    
    mixer->master_volume = 1.0f;
    mixer->dry_wet_mix = 1.0f;
    mixer->sample_rate = 44100.0f;
    mixer->channels = 1;
    mixer->auto_process = 1;
    
    strcpy(mixer->input_filename, "input.wav");
    strcpy(mixer->output_filename, "output.wav");
    
    // Initialize effect slots
    for (int i = 0; i < MAX_EFFECTS; i++) {
        mixer->effects[i].type = EFFECT_NONE;
        mixer->effects[i].params.mix = 1.0f;
        mixer->effects[i].params.enabled = 1;
        mixer->effects[i].effect_instance = NULL;
        snprintf(mixer->effects[i].name, sizeof(mixer->effects[i].name), "Slot %d", i + 1);
    }
}

// Cleanup mixer resources
void mixer_cleanup(AudioMixer* mixer) {
    if (mixer->audio_buffer) {
        audio_buffer_destroy(mixer->audio_buffer);
        mixer->audio_buffer = NULL;
    }
    
    if (mixer->processed_buffer) {
        audio_buffer_destroy(mixer->processed_buffer);
        mixer->processed_buffer = NULL;
    }
    
    // Clean up effect instances
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (mixer->effects[i].effect_instance) {
            destroy_effect_instance(mixer->effects[i].type, mixer->effects[i].effect_instance);
            mixer->effects[i].effect_instance = NULL;
        }
    }
}

// Load audio file
int mixer_load_audio(AudioMixer* mixer, const char* filename) {
    if (!filename || strlen(filename) == 0) {
        printf("Error: No filename provided\n");
        return 0;
    }
    
    printf("Attempting to load: %s\n", filename);
    
    if (mixer->audio_buffer) {
        audio_buffer_destroy(mixer->audio_buffer);
    }
    
    mixer->audio_buffer = wav_load(filename);
    if (!mixer->audio_buffer) {
        printf("Error: Could not load audio file: %s\n", filename);
        printf("Please check that the file exists and is a valid WAV file\n");
        return 0;
    }
    
    mixer->sample_rate = (float)mixer->audio_buffer->sample_rate;
    mixer->channels = mixer->audio_buffer->channels;
    mixer->playback_position = 0;
    mixer->is_processed = 0;
    
    // Create processed buffer
    if (mixer->processed_buffer) {
        audio_buffer_destroy(mixer->processed_buffer);
    }
    mixer->processed_buffer = audio_buffer_create(
        mixer->audio_buffer->length,
        mixer->audio_buffer->channels,
        mixer->audio_buffer->sample_rate
    );
    
    // Auto-process if enabled
    if (mixer->auto_process) {
        mixer_process_effects(mixer);
    }
    
    // Store input filename and generate output filename
    strcpy(mixer->input_filename, filename);
    
    // Generate output filename
    const char* basename = strrchr(filename, '/');
    if (!basename) basename = strrchr(filename, '\\');
    if (!basename) basename = filename; else basename++;
    
    // Remove extension and add _processed
    char name_without_ext[256];
    strncpy(name_without_ext, basename, sizeof(name_without_ext) - 1);
    name_without_ext[sizeof(name_without_ext) - 1] = '\0';
    
    char* dot = strrchr(name_without_ext, '.');
    if (dot) *dot = '\0';
    
    snprintf(mixer->output_filename, sizeof(mixer->output_filename), 
             "%s_processed.wav", name_without_ext);
    
    printf("Loaded %s: %zu samples, %zu channels, %.0f Hz\n", 
           filename, mixer->audio_buffer->length, 
           mixer->audio_buffer->channels, mixer->sample_rate);
    
    return 1;
}

// Save processed audio
int mixer_save_audio(AudioMixer* mixer, const char* filename) {
    if (!mixer->processed_buffer) {
        printf("Error: No audio processed to save. Load audio and process effects first.\n");
        return 0;
    }
    
    if (!filename || strlen(filename) == 0) {
        printf("Error: No output filename provided\n");
        return 0;
    }
    
    printf("Saving audio to: %s\n", filename);
    int result = wav_save(filename, mixer->processed_buffer);
    
    if (!result) {
        printf("Error: Failed to save audio file: %s\n", filename);
        printf("Check that the directory is writable\n");
        return 0;
    }
    
    printf("Successfully saved: %s (%zu samples)\n", filename, mixer->processed_buffer->length);
    return 1;
}

// Process the effect chain
void mixer_process_effects(AudioMixer* mixer) {
    if (!mixer->audio_buffer || !mixer->processed_buffer) {
        return;
    }
    
    // Copy original to processed buffer
    audio_buffer_copy(mixer->processed_buffer, mixer->audio_buffer);
    
    // Apply effects in sequence
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (mixer->effects[i].type != EFFECT_NONE && 
            mixer->effects[i].params.enabled && 
            mixer->effects[i].effect_instance) {
            
            process_effect(
                mixer->effects[i].type,
                mixer->effects[i].effect_instance,
                &mixer->effects[i].params,
                mixer->processed_buffer
            );
        }
    }
    
    // Apply master volume
    if (mixer->master_volume != 1.0f) {
        for (size_t i = 0; i < mixer->processed_buffer->capacity; i++) {
            mixer->processed_buffer->data[i] *= mixer->master_volume;
        }
    }
    
    mixer->is_processed = 1;
}

// Add an effect to the chain
void mixer_add_effect(AudioMixer* mixer, EffectType type) {
    // Find first empty slot
    for (int i = 0; i < MAX_EFFECTS; i++) {
        if (mixer->effects[i].type == EFFECT_NONE) {
            mixer->effects[i].type = type;
            mixer->effects[i].params.enabled = 1;
            mixer->effects[i].params.mix = 1.0f;
            
            // Set default parameters based on effect type
            switch (type) {
                case EFFECT_LOWPASS:
                case EFFECT_HIGHPASS:
                    mixer->effects[i].params.param1 = 1000.0f; // Frequency
                    mixer->effects[i].params.param2 = 0.7f;    // Q
                    break;
                case EFFECT_EQ:
                    mixer->effects[i].params.param1 = 0.0f;    // Low (neutral)
                    mixer->effects[i].params.param2 = 0.0f;    // Mid (neutral)
                    mixer->effects[i].params.param3 = 0.0f;    // High (neutral)
                    break;
                case EFFECT_ECHO:
                    mixer->effects[i].params.param1 = 0.3f;    // Delay
                    mixer->effects[i].params.param2 = 0.1f;    // Feedback
                    break;
                case EFFECT_REVERB:
                    mixer->effects[i].params.param1 = 0.5f;    // Room size
                    mixer->effects[i].params.param2 = 0.3f;    // Damping
                    break;
                case EFFECT_OVERDRIVE:
                case EFFECT_TUBE:
                case EFFECT_FUZZ:
                    mixer->effects[i].params.param1 = 5.0f;    // Drive
                    mixer->effects[i].params.param2 = 1.0f;    // Output
                    break;
                case EFFECT_CHORUS:
                case EFFECT_FLANGER:
                    mixer->effects[i].params.param1 = 1.0f;    // Rate
                    mixer->effects[i].params.param2 = 0.5f;    // Depth
                    mixer->effects[i].params.param3 = 0.2f;    // Feedback
                    break;
                case EFFECT_PHASER:
                    mixer->effects[i].params.param1 = 0.5f;    // Rate
                    mixer->effects[i].params.param2 = 0.7f;    // Depth
                    mixer->effects[i].params.param3 = 0.1f;    // Feedback
                    break;
                case EFFECT_TREMOLO:
                    mixer->effects[i].params.param1 = 5.0f;    // Rate
                    mixer->effects[i].params.param2 = 0.8f;    // Depth
                    break;
                default:
                    break;
            }
            
            // Create effect instance
            if (mixer->effects[i].effect_instance) {
                destroy_effect_instance(mixer->effects[i].type, mixer->effects[i].effect_instance);
            }
            mixer->effects[i].effect_instance = create_effect_instance(type, mixer->sample_rate);
            
            strcpy(mixer->effects[i].name, get_effect_name(type));
            
            if (mixer->auto_process) {
                mixer_process_effects(mixer);
            }
            break;
        }
    }
}

// Remove an effect from the chain
void mixer_remove_effect(AudioMixer* mixer, int index) {
    if (index >= 0 && index < MAX_EFFECTS) {
        if (mixer->effects[index].effect_instance) {
            destroy_effect_instance(mixer->effects[index].type, mixer->effects[index].effect_instance);
            mixer->effects[index].effect_instance = NULL;
        }
        
        mixer->effects[index].type = EFFECT_NONE;
        mixer->effects[index].params.enabled = 0;
        snprintf(mixer->effects[index].name, sizeof(mixer->effects[index].name), "Slot %d", index + 1);
        
        if (mixer->auto_process) {
            mixer_process_effects(mixer);
        }
    }
}

// Move effect in the chain
void mixer_move_effect(AudioMixer* mixer, int from, int to) {
    if (from >= 0 && from < MAX_EFFECTS && to >= 0 && to < MAX_EFFECTS && from != to) {
        EffectSlot temp = mixer->effects[from];
        mixer->effects[from] = mixer->effects[to];
        mixer->effects[to] = temp;
        
        if (mixer->auto_process) {
            mixer_process_effects(mixer);
        }
    }
}

// Create effect instance
void* create_effect_instance(EffectType type, float sample_rate) {
    switch (type) {
        case EFFECT_LOWPASS:
        case EFFECT_HIGHPASS:
            return malloc(sizeof(BiquadFilter));
        case EFFECT_EQ: {
            FourBandEQ* eq = malloc(sizeof(FourBandEQ));
            if (eq) eq_init(eq, sample_rate);
            return eq;
        }
        case EFFECT_ECHO:
            return echo_create(2.0f, sample_rate);
        case EFFECT_REVERB:
            return schroeder_reverb_create(sample_rate);
        case EFFECT_OVERDRIVE:
            return overdrive_create(sample_rate);
        case EFFECT_TUBE:
            return tube_distortion_create(sample_rate);
        case EFFECT_FUZZ:
            return fuzz_distortion_create(sample_rate);
        case EFFECT_CHORUS:
            return chorus_create(50.0f, sample_rate);
        case EFFECT_FLANGER:
            return flanger_create(20.0f, sample_rate);
        case EFFECT_PHASER:
            return phaser_create(4, sample_rate);
        case EFFECT_TREMOLO:
            return tremolo_create(sample_rate);
        default:
            return NULL;
    }
}

// Destroy effect instance
void destroy_effect_instance(EffectType type, void* instance) {
    if (!instance) return;
    
    switch (type) {
        case EFFECT_LOWPASS:
        case EFFECT_HIGHPASS:
        case EFFECT_EQ:
            free(instance);
            break;
        case EFFECT_ECHO:
            echo_destroy((Echo*)instance);
            break;
        case EFFECT_REVERB:
            schroeder_reverb_destroy((SchroederReverb*)instance);
            break;
        case EFFECT_OVERDRIVE:
            overdrive_destroy((Overdrive*)instance);
            break;
        case EFFECT_TUBE:
            tube_distortion_destroy((TubeDistortion*)instance);
            break;
        case EFFECT_FUZZ:
            fuzz_distortion_destroy((FuzzDistortion*)instance);
            break;
        case EFFECT_CHORUS:
            chorus_destroy((Chorus*)instance);
            break;
        case EFFECT_FLANGER:
            flanger_destroy((Flanger*)instance);
            break;
        case EFFECT_PHASER:
            phaser_destroy((Phaser*)instance);
            break;
        case EFFECT_TREMOLO:
            tremolo_destroy((Tremolo*)instance);
            break;
        default:
            break;
    }
}

// Process effect
void process_effect(EffectType type, void* instance, EffectParams* params, AudioBuffer* buffer) {
    if (!instance || !params || !buffer) return;
    
    switch (type) {
        case EFFECT_LOWPASS: {
            BiquadFilter* filter = (BiquadFilter*)instance;
            biquad_lowpass(filter, params->param1, params->param2, buffer->sample_rate);
            biquad_process_buffer(filter, buffer);
            break;
        }
        case EFFECT_HIGHPASS: {
            BiquadFilter* filter = (BiquadFilter*)instance;
            biquad_highpass(filter, params->param1, params->param2, buffer->sample_rate);
            biquad_process_buffer(filter, buffer);
            break;
        }
        case EFFECT_EQ: {
            FourBandEQ* eq = (FourBandEQ*)instance;
            eq_set_gains(eq, params->param1, params->param2, params->param3);
            eq_process_buffer(eq, buffer);
            break;
        }
        case EFFECT_ECHO: {
            Echo* echo = (Echo*)instance;
            echo_set_params(echo, params->param1, params->param2, params->mix, buffer->sample_rate);
            echo_process_buffer(echo, buffer);
            break;
        }
        case EFFECT_REVERB: {
            SchroederReverb* reverb = (SchroederReverb*)instance;
            schroeder_reverb_set_params(reverb, params->param1, params->param2, params->mix);
            schroeder_reverb_process_buffer(reverb, buffer);
            break;
        }
        case EFFECT_OVERDRIVE: {
            Overdrive* overdrive = (Overdrive*)instance;
            overdrive_set_params(overdrive, params->param1, 0.5f, params->param2, params->mix);
            overdrive_process_buffer(overdrive, buffer);
            break;
        }
        case EFFECT_TUBE: {
            TubeDistortion* tube = (TubeDistortion*)instance;
            tube_distortion_set_params(tube, params->param1, 0.1f, params->param2, params->mix);
            tube_distortion_process_buffer(tube, buffer);
            break;
        }
        case EFFECT_FUZZ: {
            FuzzDistortion* fuzz = (FuzzDistortion*)instance;
            fuzz_distortion_set_params(fuzz, params->param1, 0.02f, params->param2, params->mix);
            fuzz_distortion_process_buffer(fuzz, buffer);
            break;
        }
        case EFFECT_CHORUS: {
            Chorus* chorus = (Chorus*)instance;
            chorus_set_params(chorus, params->param1, params->param2, params->param3, params->mix);
            chorus_process_buffer(chorus, buffer);
            break;
        }
        case EFFECT_FLANGER: {
            Flanger* flanger = (Flanger*)instance;
            flanger_set_params(flanger, params->param1, params->param2, params->param3, 0.5f, params->mix);
            flanger_process_buffer(flanger, buffer);
            break;
        }
        case EFFECT_PHASER: {
            Phaser* phaser = (Phaser*)instance;
            phaser_set_params(phaser, params->param1, params->param2, params->param3, params->mix);
            phaser_process_buffer(phaser, buffer);
            break;
        }
        case EFFECT_TREMOLO: {
            Tremolo* tremolo = (Tremolo*)instance;
            tremolo_set_params(tremolo, params->param1, params->param2, 0);
            for (size_t i = 0; i < buffer->capacity; i++) {
                buffer->data[i] = tremolo_process(tremolo, buffer->data[i]);
            }
            break;
        }
        default:
            break;
    }
}

// Get effect name
const char* get_effect_name(EffectType type) {
    switch (type) {
        case EFFECT_NONE: return "None";
        case EFFECT_LOWPASS: return "Lowpass Filter";
        case EFFECT_HIGHPASS: return "Highpass Filter";
        case EFFECT_EQ: return "4-Band EQ";
        case EFFECT_ECHO: return "Echo/Delay";
        case EFFECT_REVERB: return "Reverb";
        case EFFECT_OVERDRIVE: return "Overdrive";
        case EFFECT_TUBE: return "Tube Distortion";
        case EFFECT_FUZZ: return "Fuzz";
        case EFFECT_CHORUS: return "Chorus";
        case EFFECT_FLANGER: return "Flanger";
        case EFFECT_PHASER: return "Phaser";
        case EFFECT_TREMOLO: return "Tremolo";
        default: return "Unknown";
    }
}

// Get parameter name
const char* get_param_name(EffectType type, int param_index) {
    switch (type) {
        case EFFECT_LOWPASS:
        case EFFECT_HIGHPASS:
            return (param_index == 0) ? "Frequency" : "Q Factor";
        case EFFECT_EQ:
            switch (param_index) {
                case 0: return "Low";
                case 1: return "Mid";
                case 2: return "High";
                default: return "";
            }
        case EFFECT_ECHO:
            return (param_index == 0) ? "Delay" : "Feedback";
        case EFFECT_REVERB:
            return (param_index == 0) ? "Room Size" : "Damping";
        case EFFECT_OVERDRIVE:
        case EFFECT_TUBE:
        case EFFECT_FUZZ:
            return (param_index == 0) ? "Drive" : "Output";
        case EFFECT_CHORUS:
        case EFFECT_FLANGER:
            switch (param_index) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Feedback";
                default: return "";
            }
        case EFFECT_PHASER:
            switch (param_index) {
                case 0: return "Rate";
                case 1: return "Depth";
                case 2: return "Feedback";
                default: return "";
            }
        case EFFECT_TREMOLO:
            return (param_index == 0) ? "Rate" : "Depth";
        default:
            return "";
    }
}

// Get parameter range
void get_param_range(EffectType type, int param_index, float* min, float* max) {
    switch (type) {
        case EFFECT_LOWPASS:
        case EFFECT_HIGHPASS:
            if (param_index == 0) {
                *min = 20.0f; *max = 20000.0f; // Frequency
            } else {
                *min = 0.1f; *max = 10.0f; // Q
            }
            break;
        case EFFECT_EQ:
            *min = -30.0f; *max = 30.0f; // dB
            break;
        case EFFECT_ECHO:
            if (param_index == 0) {
                *min = 0.01f; *max = 2.0f; // Delay
            } else {
                *min = 0.0f; *max = 0.9f; // Feedback
            }
            break;
        case EFFECT_REVERB:
            *min = 0.0f; *max = 1.0f;
            break;
        case EFFECT_OVERDRIVE:
        case EFFECT_TUBE:
        case EFFECT_FUZZ:
            if (param_index == 0) {
                *min = 1.0f; *max = 20.0f; // Drive
            } else {
                *min = 0.1f; *max = 2.0f; // Output
            }
            break;
        case EFFECT_CHORUS:
        case EFFECT_FLANGER:
        case EFFECT_PHASER:
            if (param_index == 0) {
                *min = 0.1f; *max = 10.0f; // Rate
            } else {
                *min = 0.0f; *max = 1.0f; // Depth/Feedback
            }
            break;
        case EFFECT_TREMOLO:
            if (param_index == 0) {
                *min = 0.1f; *max = 20.0f; // Rate
            } else {
                *min = 0.0f; *max = 1.0f; // Depth
            }
            break;
        default:
            *min = 0.0f; *max = 1.0f;
            break;
    }
}