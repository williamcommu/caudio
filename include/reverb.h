#ifndef REVERB_H
#define REVERB_H

#include "audio_core.h"
#include "delay_effects.h"
#include "audio_filters.h"

// Schroeder reverb structure
typedef struct {
    DelayLine comb_delays[4];
    float comb_gains[4];
    DelayLine allpass_delays[2];
    float allpass_gains[2];
    float wet_level;
    float dry_level;
    float room_size;
    float damping;
    OnePoleFilter damping_filters[4];
} SchroederReverb;

// Simple plate reverb structure
typedef struct {
    DelayLine delays[8];
    float gains[8];
    BiquadFilter input_filter;
    BiquadFilter output_filter;
    float decay_time;
    float wet_level;
    float dry_level;
    float pre_delay;
} PlateReverb;

// Freeverb-style reverb structure
typedef struct {
    DelayLine comb_delays[8];
    float comb_feedbacks[8];
    OnePoleFilter comb_filters[8];
    DelayLine allpass_delays[4];
    float room_size;
    float damping;
    float wet_level;
    float dry_level;
    float width; // Stereo width
} Freeverb;

// Schroeder reverb functions
SchroederReverb* schroeder_reverb_create(float sample_rate);
void schroeder_reverb_destroy(SchroederReverb* reverb);
void schroeder_reverb_set_params(SchroederReverb* reverb, float room_size, float damping, float wet_level);
sample_t schroeder_reverb_process(SchroederReverb* reverb, sample_t input);
void schroeder_reverb_process_buffer(SchroederReverb* reverb, AudioBuffer* buffer);

// Plate reverb functions
PlateReverb* plate_reverb_create(float sample_rate);
void plate_reverb_destroy(PlateReverb* reverb);
void plate_reverb_set_params(PlateReverb* reverb, float decay_time, float wet_level, float pre_delay, float sample_rate);
sample_t plate_reverb_process(PlateReverb* reverb, sample_t input);
void plate_reverb_process_buffer(PlateReverb* reverb, AudioBuffer* buffer);

// Freeverb functions
Freeverb* freeverb_create(float sample_rate);
void freeverb_destroy(Freeverb* reverb);
void freeverb_set_params(Freeverb* reverb, float room_size, float damping, float wet_level, float width);
sample_t freeverb_process(Freeverb* reverb, sample_t input);
void freeverb_process_buffer(Freeverb* reverb, AudioBuffer* buffer);

#endif // REVERB_H