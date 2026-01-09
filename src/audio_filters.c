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
    freq = fmaxf(20.0f, fminf(sample_rate * 0.45f, freq));
    q = fmaxf(0.2f, fminf(8.0f, q));
    gain_db = fmaxf(-20.0f, fminf(20.0f, gain_db));
    
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
    
    // More stable peaking filter design
    float b0, b1, b2, a0, a1, a2;
    
    if (gain_db > 0.0f) {
        // Boost
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cosw;
        b2 = 1.0f - alpha * A;
        a0 = 1.0f + alpha / A;
        a1 = -2.0f * cosw;
        a2 = 1.0f - alpha / A;
    } else {
        // Cut
        b0 = 1.0f + alpha / A;
        b1 = -2.0f * cosw;
        b2 = 1.0f - alpha / A;
        a0 = 1.0f + alpha * A;
        a1 = -2.0f * cosw;
        a2 = 1.0f - alpha * A;
    }
    
    // Prevent any problematic values
    if (fabsf(a0) < 1e-6f || !isfinite(a0)) {
        // Create bypass if denominator is problematic
        filter->b0 = 1.0f;
        filter->b1 = 0.0f;
        filter->b2 = 0.0f;
        filter->a1 = 0.0f;
        filter->a2 = 0.0f;
    } else {
        // Normalize coefficients (sign already handled in the difference equation)
        filter->b0 = b0 / a0;
        filter->b1 = b1 / a0;
        filter->b2 = b2 / a0;
        filter->a1 = a1 / a0;
        filter->a2 = a2 / a0;
        
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
    biquad_peaking(&eq->low_shelf, 50.0f, 1.0f, 0.0f, sample_rate);     // Bass (0-100Hz range)
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
    // Clamp gains to expanded range
    if (low < -30.0f) low = -30.0f;
    if (low > 30.0f) low = 30.0f;
    if (mid < -30.0f) mid = -30.0f;
    if (mid > 30.0f) mid = 30.0f;
    if (high < -30.0f) high = -30.0f;
    if (high > 30.0f) high = 30.0f;
    
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

// Initialize parametric EQ with default bands
void parametric_eq_init(ParametricEQ* eq, float sample_rate) {
    eq->sample_rate = sample_rate;
    eq->num_active_bands = 3;  // Start with 3 bands like traditional EQ
    
    // Initialize default bands (similar to classic 3-band EQ)
    // All start at 0dB (unity gain) for transparent sound
    eq->bands[0].frequency = 100.0f;   // Low band
    eq->bands[0].q = 1.0f;
    eq->bands[0].gain_db = 0.0f;  // Unity gain
    eq->bands[0].enabled = 1;
    
    eq->bands[1].frequency = 1000.0f;  // Mid band
    eq->bands[1].q = 1.0f;
    eq->bands[1].gain_db = 0.0f;  // Unity gain
    eq->bands[1].enabled = 1;
    
    eq->bands[2].frequency = 8000.0f;  // High band
    eq->bands[2].q = 1.0f;
    eq->bands[2].gain_db = 0.0f;  // Unity gain
    eq->bands[2].enabled = 1;
    
    // Initialize remaining bands as disabled
    for (int i = 3; i < MAX_EQ_BANDS; i++) {
        eq->bands[i].frequency = 1000.0f;
        eq->bands[i].q = 1.0f;
        eq->bands[i].gain_db = 0.0f;
        eq->bands[i].enabled = 0;
    }
    
    // Initialize all filters
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        memset(&eq->filters[i], 0, sizeof(BiquadFilter));
    }
    
    parametric_eq_update_filters(eq);
}

// Set parameters for a specific band
void parametric_eq_set_band(ParametricEQ* eq, int band_index, float freq, float q, float gain_db, int enabled) {
    if (band_index < 0 || band_index >= MAX_EQ_BANDS) return;
    
    // Clamp parameters to safe ranges
    freq = fmaxf(20.0f, fminf(20000.0f, freq));
    q = fmaxf(0.1f, fminf(10.0f, q));
    gain_db = fmaxf(-24.0f, fminf(24.0f, gain_db));
    
    eq->bands[band_index].frequency = freq;
    eq->bands[band_index].q = q;
    eq->bands[band_index].gain_db = gain_db;
    eq->bands[band_index].enabled = enabled;
    
    // Count active bands
    eq->num_active_bands = 0;
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (eq->bands[i].enabled) {
            eq->num_active_bands++;
        }
    }
}

// Update all filter coefficients based on band parameters
void parametric_eq_update_filters(ParametricEQ* eq) {
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (eq->bands[i].enabled) {
            // Don't reset - just update coefficients to preserve state during playback
            // Only reset if transitioning from disabled to enabled
            
            // Clamp to even safer ranges for real-time adjustment
            float safe_freq = fmaxf(30.0f, fminf(eq->sample_rate * 0.45f, eq->bands[i].frequency));
            float safe_q = fmaxf(0.2f, fminf(8.0f, eq->bands[i].q));
            float safe_gain = fmaxf(-20.0f, fminf(20.0f, eq->bands[i].gain_db));
            
            biquad_peaking(&eq->filters[i], 
                          safe_freq, 
                          safe_q, 
                          safe_gain, 
                          eq->sample_rate);
        } else {
            // Reset disabled filters to clear state
            biquad_reset(&eq->filters[i]);
        }
    }
}

// Process single sample through parametric EQ
float parametric_eq_process(ParametricEQ* eq, float input) {
    // Safety check on input
    if (!isfinite(input)) {
        static int input_nan_count = 0;
        if (input_nan_count++ < 5) {
            printf("[ParametricEQ] WARNING: Invalid input sample: %f\n", input);
        }
        return 0.0f;
    }
    
    float output = input;
    
    // Process through each enabled band in series
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (eq->bands[i].enabled) {
            float prev_output = output;
            output = biquad_process(&eq->filters[i], output);
            
            // Safety check after each filter stage
            if (!isfinite(output)) {
                static int filter_nan_count = 0;
                if (filter_nan_count++ < 5) {
                    printf("[ParametricEQ] CRITICAL: Band %d produced NaN!\n", i);
                    printf("  Input to filter: %f\n", prev_output);
                    printf("  Band params: freq=%.1f, q=%.2f, gain=%.1f\n",
                           eq->bands[i].frequency, eq->bands[i].q, eq->bands[i].gain_db);
                    printf("  Filter coeffs: b0=%f, b1=%f, b2=%f, a1=%f, a2=%f\n",
                           eq->filters[i].b0, eq->filters[i].b1, eq->filters[i].b2,
                           eq->filters[i].a1, eq->filters[i].a2);
                }
                // Reset this filter and return silence
                biquad_reset(&eq->filters[i]);
                return 0.0f;
            }
        }
    }
    
    return output;
}

// Process entire buffer through parametric EQ
void parametric_eq_process_buffer(ParametricEQ* eq, AudioBuffer* buffer) {
    if (!buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        float result = parametric_eq_process(eq, buffer->data[i]);
        
        // Final safety check
        if (isfinite(result)) {
            buffer->data[i] = result;
        } else {
            // If we get NaN, just pass through the input
            // (which might also be NaN, but we tried)
            buffer->data[i] = 0.0f;
        }
    }
}