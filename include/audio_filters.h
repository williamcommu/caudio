#ifndef AUDIO_FILTERS_H
#define AUDIO_FILTERS_H

#include "audio_core.h"

// Filter types
typedef enum {
    FILTER_LOWPASS,
    FILTER_HIGHPASS,
    FILTER_BANDPASS,
    FILTER_NOTCH,
    FILTER_ALLPASS
} FilterType;

// Biquad filter structure (IIR filter)
typedef struct {
    float b0, b1, b2;  // Feedforward coefficients
    float a1, a2;      // Feedback coefficients
    float x1, x2;      // Input history
    float y1, y2;      // Output history
} BiquadFilter;

// Simple one-pole filter (faster, less precise)
typedef struct {
    float alpha;       // Filter coefficient
    float prev_output; // Previous output
} OnePoleFilter;

// Filter design functions
void biquad_lowpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_highpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_bandpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_notch(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_peaking(BiquadFilter* filter, float freq, float q, float gain_db, float sample_rate);

// Filter processing functions
float biquad_process(BiquadFilter* filter, float input);
void biquad_reset(BiquadFilter* filter);
void biquad_process_buffer(BiquadFilter* filter, AudioBuffer* buffer);

// One-pole filter functions
void onepole_lowpass(OnePoleFilter* filter, float freq, float sample_rate);
void onepole_highpass(OnePoleFilter* filter, float freq, float sample_rate);
float onepole_process(OnePoleFilter* filter, float input, int highpass);
void onepole_reset(OnePoleFilter* filter);

// EQ bands structure
typedef struct {
    BiquadFilter low_shelf;
    BiquadFilter low_mid;
    BiquadFilter high_mid;
    BiquadFilter high_shelf;
    float low_gain;
    float low_mid_gain;
    float high_mid_gain;
    float high_gain;
    float sample_rate;  // Store sample rate for parameter updates
} FourBandEQ;

// Parametric EQ band structure (for advanced EQ window)
#define MAX_EQ_BANDS 8
typedef struct {
    float frequency;  // Center frequency in Hz (20-20000)
    float q;          // Q factor/bandwidth (0.1-10.0)
    float gain_db;    // Gain in dB (-24 to +24)
    int enabled;      // Whether this band is active
} ParametricBand;

// Advanced parametric EQ structure
typedef struct {
    ParametricBand bands[MAX_EQ_BANDS];
    BiquadFilter filters[MAX_EQ_BANDS];
    int num_active_bands;
    float sample_rate;
} ParametricEQ;

// EQ functions (3-band EQ: Low, Mid, High)
void eq_init(FourBandEQ* eq, float sample_rate);
void eq_set_gains(FourBandEQ* eq, float low, float mid, float high);
float eq_process(FourBandEQ* eq, float input);
void eq_process_buffer(FourBandEQ* eq, AudioBuffer* buffer);

// Parametric EQ functions
void parametric_eq_init(ParametricEQ* eq, float sample_rate);
void parametric_eq_set_band(ParametricEQ* eq, int band_index, float freq, float q, float gain_db, int enabled);
float parametric_eq_process(ParametricEQ* eq, float input);
void parametric_eq_process_buffer(ParametricEQ* eq, AudioBuffer* buffer);
void parametric_eq_update_filters(ParametricEQ* eq);

#endif // AUDIO_FILTERS_H