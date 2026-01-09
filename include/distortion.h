#ifndef DISTORTION_H
#define DISTORTION_H

#include "audio_core.h"
#include "audio_filters.h"

// Distortion types
typedef enum {
    DISTORTION_HARD_CLIP,
    DISTORTION_SOFT_CLIP,
    DISTORTION_TUBE,
    DISTORTION_FUZZ,
    DISTORTION_OVERDRIVE
} DistortionType;

// Basic distortion structure
typedef struct {
    DistortionType type;
    float drive;        // Input gain (distortion amount)
    float output_gain;  // Output level compensation
    float mix;          // Wet/dry mix
    BiquadFilter pre_filter;
    BiquadFilter post_filter;
    float sample_rate;
} Distortion;

// Tube distortion structure with asymmetric clipping
typedef struct {
    float drive;
    float bias;         // DC bias for asymmetric distortion
    float output_gain;
    float mix;
    BiquadFilter input_filter;
    BiquadFilter output_filter;
    OnePoleFilter dc_blocker;
} TubeDistortion;

// Fuzz distortion structure
typedef struct {
    float fuzz_amount;
    float gate_threshold;
    float output_gain;
    float mix;
    BiquadFilter pre_emphasis;
    BiquadFilter de_emphasis;
    OnePoleFilter gate_filter;
} FuzzDistortion;

// Overdrive structure with multi-stage clipping
typedef struct {
    float drive;
    float tone;
    float output_gain;
    float mix;
    BiquadFilter input_filter;
    BiquadFilter tone_filter;
    BiquadFilter output_filter;
    float stage_gains[3];
} Overdrive;

// Basic distortion functions
Distortion* distortion_create(DistortionType type, float sample_rate);
void distortion_destroy(Distortion* dist);
void distortion_set_params(Distortion* dist, float drive, float output_gain, float mix);
sample_t distortion_process(Distortion* dist, sample_t input);
void distortion_process_buffer(Distortion* dist, AudioBuffer* buffer);

// Tube distortion functions
TubeDistortion* tube_distortion_create(float sample_rate);
void tube_distortion_destroy(TubeDistortion* tube);
void tube_distortion_set_params(TubeDistortion* tube, float drive, float bias, float output_gain, float mix);
sample_t tube_distortion_process(TubeDistortion* tube, sample_t input);
void tube_distortion_process_buffer(TubeDistortion* tube, AudioBuffer* buffer);

// Fuzz distortion functions
FuzzDistortion* fuzz_distortion_create(float sample_rate);
void fuzz_distortion_destroy(FuzzDistortion* fuzz);
void fuzz_distortion_set_params(FuzzDistortion* fuzz, float fuzz_amount, float gate_threshold, float output_gain, float mix);
sample_t fuzz_distortion_process(FuzzDistortion* fuzz, sample_t input);
void fuzz_distortion_process_buffer(FuzzDistortion* fuzz, AudioBuffer* buffer);

// Overdrive functions
Overdrive* overdrive_create(float sample_rate);
void overdrive_destroy(Overdrive* overdrive);
void overdrive_set_params(Overdrive* overdrive, float drive, float tone, float output_gain, float mix);
sample_t overdrive_process(Overdrive* overdrive, sample_t input);
void overdrive_process_buffer(Overdrive* overdrive, AudioBuffer* buffer);

// Waveshaping functions
sample_t hard_clip(sample_t input, float threshold);
sample_t soft_clip(sample_t input, float amount);
sample_t tube_saturation(sample_t input, float drive, float bias);
sample_t cubic_distortion(sample_t input, float amount);
sample_t sigmoid_distortion(sample_t input, float drive);

#endif // DISTORTION_H