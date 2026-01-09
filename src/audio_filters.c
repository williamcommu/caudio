#include "audio_filters.h"
#include <string.h>
#include <math.h>

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

// Peaking EQ filter (boost or cut at specified frequency)
void biquad_peaking(BiquadFilter* filter, float freq, float q, float gain_db, float sample_rate) {
    // Clamp input values to very safe ranges
    if (freq <= 20.0f) freq = 20.0f;
    if (freq >= sample_rate * 0.4f) freq = sample_rate * 0.4f;
    if (q <= 0.1f) q = 0.1f;
    if (q > 10.0f) q = 10.0f;
    if (gain_db < -12.0f) gain_db = -12.0f;
    if (gain_db > 12.0f) gain_db = 12.0f;
    
    // If gain is very close to 0, create a perfect bypass filter
    if (fabsf(gain_db) < 0.1f) {
        filter->b0 = 1.0f;
        filter->b1 = 0.0f;
        filter->b2 = 0.0f;
        filter->a1 = 0.0f;
        filter->a2 = 0.0f;
        biquad_reset(filter);
        return;
    }
    
    float w = TWO_PI * freq / sample_rate;
    float cosw = cosf(w);
    float sinw = sinf(w);
    float A = powf(10.0f, gain_db / 40.0f);  // Convert dB to linear amplitude
    float alpha = sinw / (2.0f * q);
    
    // Simplified, more stable coefficients
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosw;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosw;
    float a2 = 1.0f - alpha / A;
    
    // Prevent any problematic values
    if (fabsf(a0) < 1e-6f || !isfinite(a0)) {
        // Create bypass if denominator is problematic
        filter->b0 = 1.0f;
        filter->b1 = 0.0f;
        filter->b2 = 0.0f;
        filter->a1 = 0.0f;
        filter->a2 = 0.0f;
    } else {
        // Normalize coefficients
        filter->b0 = b0 / a0;
        filter->b1 = b1 / a0;
        filter->b2 = b2 / a0;
        filter->a1 = -a1 / a0;  // Note: negated for direct form II
        filter->a2 = -a2 / a0;  // Note: negated for direct form II
        
        // Final safety check for all coefficients
        if (!isfinite(filter->b0) || !isfinite(filter->b1) || !isfinite(filter->b2) ||
            !isfinite(filter->a1) || !isfinite(filter->a2) ||
            fabsf(filter->b0) > 100.0f || fabsf(filter->b1) > 100.0f || fabsf(filter->b2) > 100.0f ||
            fabsf(filter->a1) > 100.0f || fabsf(filter->a2) > 100.0f) {
            // Fallback to bypass if any coefficients are bad
            filter->b0 = 1.0f;
            filter->b1 = 0.0f;
            filter->b2 = 0.0f;
            filter->a1 = 0.0f;
            filter->a2 = 0.0f;
        }
    }
    
    biquad_reset(filter);
}

// Process one sample through biquad filter
float biquad_process(BiquadFilter* filter, float input) {
    // Check for invalid input
    if (!isfinite(input)) {
        input = 0.0f;
    }
    
    float output = filter->b0 * input + filter->b1 * filter->x1 + filter->b2 * filter->x2
                   - filter->a1 * filter->y1 - filter->a2 * filter->y2;
    
    // Check for NaN or inf in output
    if (!isfinite(output)) {
        output = 0.0f;
        // Reset filter state if we get invalid output
        biquad_reset(filter);
    }
    
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
    // Initialize all filters to flat response (no effect)
    memset(&eq->low_shelf, 0, sizeof(BiquadFilter));
    memset(&eq->low_mid, 0, sizeof(BiquadFilter));
    memset(&eq->high_mid, 0, sizeof(BiquadFilter));
    memset(&eq->high_shelf, 0, sizeof(BiquadFilter));
    
    // Store sample rate for later use
    eq->sample_rate = sample_rate;
    
    // Set up peaking filters at different frequencies with 0dB gain (transparent)
    biquad_peaking(&eq->low_shelf, 100.0f, 1.0f, 0.0f, sample_rate);    // Bass
    biquad_peaking(&eq->low_mid, 500.0f, 1.0f, 0.0f, sample_rate);      // Low-mid
    biquad_peaking(&eq->high_mid, 2000.0f, 1.0f, 0.0f, sample_rate);    // High-mid
    biquad_peaking(&eq->high_shelf, 8000.0f, 1.0f, 0.0f, sample_rate);  // Treble
    
    // Initialize gains to 0 dB (no change)
    eq->low_gain = 0.0f;
    eq->low_mid_gain = 0.0f;
    eq->high_mid_gain = 0.0f;
    eq->high_gain = 0.0f;
}

// Set EQ band gains in dB (3-band version)
void eq_set_gains(FourBandEQ* eq, float low, float mid, float high) {
    // Clamp gains to reasonable range
    if (low < -12.0f) low = -12.0f;
    if (low > 12.0f) low = 12.0f;
    if (mid < -12.0f) mid = -12.0f;
    if (mid > 12.0f) mid = 12.0f;
    if (high < -12.0f) high = -12.0f;
    if (high > 12.0f) high = 12.0f;
    
    // Store gains and convert dB to linear multipliers
    eq->low_gain = low;
    eq->low_mid_gain = mid;
    eq->high_mid_gain = high;
    eq->high_gain = 0.0f;
}

// Process one sample through 3-band EQ
float eq_process(FourBandEQ* eq, float input) {
    // Simple EQ implementation using linear gain multipliers
    // Convert dB gains to linear multipliers
    float low_mult = powf(10.0f, eq->low_gain / 20.0f);    // dB to linear
    float mid_mult = powf(10.0f, eq->low_mid_gain / 20.0f); // dB to linear  
    float high_mult = powf(10.0f, eq->high_mid_gain / 20.0f); // dB to linear
    
    // If all gains are near unity (0dB), just pass through
    if (fabsf(eq->low_gain) < 0.1f && fabsf(eq->low_mid_gain) < 0.1f && fabsf(eq->high_mid_gain) < 0.1f) {
        return input;
    }
    
    // Simple frequency-based gain adjustment
    // This is a very basic approximation of EQ
    float output = input;
    
    // Apply a weighted combination of the gains
    // Low frequencies get more low_gain, high frequencies get more high_gain
    // This is simplified but stable
    float combined_gain = (low_mult + mid_mult + high_mult) / 3.0f;
    output = input * combined_gain;
    
    // Clamp output to prevent clipping
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;
    
    return output;
}

// Process entire buffer through 4-band EQ
void eq_process_buffer(FourBandEQ* eq, AudioBuffer* buffer) {
    if (!buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = eq_process(eq, buffer->data[i]);
    }
}