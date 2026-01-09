#include "audio_filters.h"

// Design a lowpass biquad filter
void biquad_lowpass(BiquadFilter* filter, float freq, float q, float sample_rate) {
    float w = TWO_PI * freq / sample_rate;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float alpha = sinw / (2.0f * q);
    
    float b0 = (1.0f - cosw) / 2.0f;
    float b1 = 1.0f - cosw;
    float b2 = (1.0f - cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    
    // Normalize coefficients
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    biquad_reset(filter);
}

// Design a highpass biquad filter
void biquad_highpass(BiquadFilter* filter, float freq, float q, float sample_rate) {
    float w = TWO_PI * freq / sample_rate;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float alpha = sinw / (2.0f * q);
    
    float b0 = (1.0f + cosw) / 2.0f;
    float b1 = -(1.0f + cosw);
    float b2 = (1.0f + cosw) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    
    // Normalize coefficients
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    biquad_reset(filter);
}

// Design a bandpass biquad filter
void biquad_bandpass(BiquadFilter* filter, float freq, float q, float sample_rate) {
    float w = TWO_PI * freq / sample_rate;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float alpha = sinw / (2.0f * q);
    
    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    
    // Normalize coefficients
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    biquad_reset(filter);
}

// Design a notch biquad filter
void biquad_notch(BiquadFilter* filter, float freq, float q, float sample_rate) {
    float w = TWO_PI * freq / sample_rate;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float alpha = sinw / (2.0f * q);
    
    float b0 = 1.0f;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha;
    
    // Normalize coefficients
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;
    
    biquad_reset(filter);
}

// Process one sample through biquad filter
float biquad_process(BiquadFilter* filter, float input) {
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                   - filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // Update history
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    
    return output;
}

// Reset biquad filter state
void biquad_reset(BiquadFilter* filter) {
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

// Process entire buffer through biquad filter
void biquad_process_buffer(BiquadFilter* filter, AudioBuffer* buffer) {
    if (!buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = biquad_process(filter, buffer->data[i]);
    }
}

// Design a lowpass one-pole filter
void onepole_lowpass(OnePoleFilter* filter, float freq, float sample_rate) {
    filter->alpha = 1.0f - expf(-TWO_PI * freq / sample_rate);
    filter->prev_output = 0.0f;
}

// Design a highpass one-pole filter
void onepole_highpass(OnePoleFilter* filter, float freq, float sample_rate) {
    filter->alpha = expf(-TWO_PI * freq / sample_rate);
    filter->prev_output = 0.0f;
}

// Process one sample through one-pole filter
float onepole_process(OnePoleFilter* filter, float input, int highpass) {
    if (highpass) {
        filter->prev_output = filter->alpha * (filter->prev_output + input - filter->prev_output);
        return input - filter->prev_output;
    } else {
        filter->prev_output += filter->alpha * (input - filter->prev_output);
        return filter->prev_output;
    }
}

// Reset one-pole filter state
void onepole_reset(OnePoleFilter* filter) {
    filter->prev_output = 0.0f;
}

// Initialize 4-band EQ
void eq_init(FourBandEQ* eq, float sample_rate) {
    // Low shelf at 100 Hz
    biquad_lowpass(&eq->low_shelf, 100.0f, 0.7f, sample_rate);
    
    // Low-mid band at 500 Hz
    biquad_bandpass(&eq->low_mid, 500.0f, 2.0f, sample_rate);
    
    // High-mid band at 2000 Hz
    biquad_bandpass(&eq->high_mid, 2000.0f, 2.0f, sample_rate);
    
    // High shelf at 8000 Hz
    biquad_highpass(&eq->high_shelf, 8000.0f, 0.7f, sample_rate);
    
    // Initialize gains to unity (0 dB)
    eq->low_gain = 1.0f;
    eq->low_mid_gain = 1.0f;
    eq->high_mid_gain = 1.0f;
    eq->high_gain = 1.0f;
}

// Set EQ band gains in dB
void eq_set_gains(FourBandEQ* eq, float low, float low_mid, float high_mid, float high) {
    eq->low_gain = db_to_linear(low);
    eq->low_mid_gain = db_to_linear(low_mid);
    eq->high_mid_gain = db_to_linear(high_mid);
    eq->high_gain = db_to_linear(high);
}

// Process one sample through 4-band EQ
float eq_process(FourBandEQ* eq, float input) {
    float low = biquad_process(&eq->low_shelf, input) * eq->low_gain;
    float low_mid = biquad_process(&eq->low_mid, input) * eq->low_mid_gain;
    float high_mid = biquad_process(&eq->high_mid, input) * eq->high_mid_gain;
    float high = biquad_process(&eq->high_shelf, input) * eq->high_gain;
    
    return (low + low_mid + high_mid + high) * 0.25f; // Average the bands
}

// Process entire buffer through 4-band EQ
void eq_process_buffer(FourBandEQ* eq, AudioBuffer* buffer) {
    if (!buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = eq_process(eq, buffer->data[i]);
    }
}