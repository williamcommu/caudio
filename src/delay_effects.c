#include "delay_effects.h"

// Create a delay line
DelayLine* delay_line_create(size_t max_delay_samples) {
    DelayLine* delay = malloc(sizeof(DelayLine));
    if (!delay) return NULL;
    
    delay->size = max_delay_samples + 1; // +1 for interpolation safety
    delay->buffer = calloc(delay->size, sizeof(sample_t));
    if (!delay->buffer) {
        free(delay);
        return NULL;
    }
    
    delay->write_pos = 0;
    delay->read_pos = 0;
    
    return delay;
}

// Destroy delay line
void delay_line_destroy(DelayLine* delay) {
    if (delay) {
        if (delay->buffer) {
            free(delay->buffer);
        }
        free(delay);
    }
}

// Write sample to delay line
void delay_line_write(DelayLine* delay, sample_t sample) {
    if (!delay || !delay->buffer) return;
    
    delay->buffer[delay->write_pos] = sample;
    delay->write_pos = (delay->write_pos + 1) % delay->size;
}

// Read sample from delay line
sample_t delay_line_read(DelayLine* delay, size_t delay_samples) {
    if (!delay || !delay->buffer || delay_samples >= delay->size) return 0.0f;
    
    size_t read_pos = (delay->write_pos - delay_samples + delay->size) % delay->size;
    return delay->buffer[read_pos];
}

// Read sample with linear interpolation for fractional delays
sample_t delay_line_read_interpolated(DelayLine* delay, float delay_samples) {
    if (!delay || !delay->buffer) return 0.0f;
    
    size_t delay_int = (size_t)delay_samples;
    float delay_frac = delay_samples - delay_int;
    
    if (delay_int >= delay->size - 1) return 0.0f;
    
    size_t pos1 = (delay->write_pos - delay_int + delay->size) % delay->size;
    size_t pos2 = (pos1 - 1 + delay->size) % delay->size;
    
    return lerp(delay->buffer[pos1], delay->buffer[pos2], delay_frac);
}

// Clear delay line
void delay_line_clear(DelayLine* delay) {
    if (delay && delay->buffer) {
        memset(delay->buffer, 0, delay->size * sizeof(sample_t));
        delay->write_pos = 0;
        delay->read_pos = 0;
    }
}

// Create echo effect
Echo* echo_create(float max_delay_seconds, float sample_rate) {
    Echo* echo = malloc(sizeof(Echo));
    if (!echo) return NULL;
    
    size_t max_delay_samples = (size_t)(max_delay_seconds * sample_rate);
    echo->delay.buffer = NULL;
    
    DelayLine* delay = delay_line_create(max_delay_samples);
    if (!delay) {
        free(echo);
        return NULL;
    }
    
    echo->delay = *delay;
    free(delay); // We copied the contents, don't need the wrapper
    
    echo->feedback = 0.3f;
    echo->wet_level = 0.3f;
    echo->dry_level = 0.7f;
    
    onepole_lowpass(&echo->feedback_filter, 8000.0f, sample_rate);
    
    return echo;
}

// Destroy echo effect
void echo_destroy(Echo* echo) {
    if (echo) {
        if (echo->delay.buffer) {
            free(echo->delay.buffer);
        }
        free(echo);
    }
}

// Set echo parameters
void echo_set_params(Echo* echo, float delay_seconds, float feedback, float wet_level, float sample_rate) {
    if (!echo) return;
    
    echo->feedback = clamp(feedback, 0.0f, 0.95f);
    echo->wet_level = clamp(wet_level, 0.0f, 1.0f);
    echo->dry_level = 1.0f - echo->wet_level;
    
    // Update feedback filter
    onepole_lowpass(&echo->feedback_filter, 8000.0f, sample_rate);
}

// Process one sample through echo effect
sample_t echo_process(Echo* echo, sample_t input) {
    if (!echo) return input;
    
    // Calculate delay time in samples (you'd typically make this adjustable)
    size_t delay_samples = echo->delay.size / 4; // Use 1/4 of max delay as default
    
    sample_t delayed = delay_line_read(&echo->delay, delay_samples);
    sample_t filtered_delayed = onepole_process(&echo->feedback_filter, delayed, 0);
    
    sample_t feedback_sample = input + filtered_delayed * echo->feedback;
    delay_line_write(&echo->delay, feedback_sample);
    
    return input * echo->dry_level + delayed * echo->wet_level;
}

// Process buffer through echo effect
void echo_process_buffer(Echo* echo, AudioBuffer* buffer) {
    if (!echo || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = echo_process(echo, buffer->data[i]);
    }
}

// Create multi-tap delay
MultiTapDelay* multitap_create(float max_delay_seconds, float sample_rate) {
    MultiTapDelay* multitap = malloc(sizeof(MultiTapDelay));
    if (!multitap) return NULL;
    
    size_t max_delay_samples = (size_t)(max_delay_seconds * sample_rate);
    DelayLine* delay = delay_line_create(max_delay_samples);
    if (!delay) {
        free(multitap);
        return NULL;
    }
    
    multitap->delay = *delay;
    free(delay);
    
    multitap->num_taps = 0;
    multitap->feedback = 0.2f;
    multitap->wet_level = 0.3f;
    multitap->dry_level = 0.7f;
    
    // Clear tap arrays
    memset(multitap->tap_gains, 0, sizeof(multitap->tap_gains));
    memset(multitap->tap_delays, 0, sizeof(multitap->tap_delays));
    
    return multitap;
}

// Destroy multi-tap delay
void multitap_destroy(MultiTapDelay* multitap) {
    if (multitap) {
        if (multitap->delay.buffer) {
            free(multitap->delay.buffer);
        }
        free(multitap);
    }
}

// Set tap parameters
void multitap_set_tap(MultiTapDelay* multitap, int tap_index, float delay_seconds, float gain, float sample_rate) {
    if (!multitap || tap_index < 0 || tap_index >= 8) return;
    
    multitap->tap_delays[tap_index] = (size_t)(delay_seconds * sample_rate);
    multitap->tap_gains[tap_index] = gain;
    
    if (tap_index >= multitap->num_taps) {
        multitap->num_taps = tap_index + 1;
    }
}

// Set multi-tap feedback and wet level
void multitap_set_feedback(MultiTapDelay* multitap, float feedback, float wet_level) {
    if (!multitap) return;
    
    multitap->feedback = clamp(feedback, 0.0f, 0.9f);
    multitap->wet_level = clamp(wet_level, 0.0f, 1.0f);
    multitap->dry_level = 1.0f - multitap->wet_level;
}

// Process one sample through multi-tap delay
sample_t multitap_process(MultiTapDelay* multitap, sample_t input) {
    if (!multitap) return input;
    
    sample_t output = input * multitap->dry_level;
    sample_t tap_sum = 0.0f;
    
    // Sum all taps
    for (int i = 0; i < multitap->num_taps; i++) {
        sample_t tap_output = delay_line_read(&multitap->delay, multitap->tap_delays[i]);
        tap_sum += tap_output * multitap->tap_gains[i];
    }
    
    // Add feedback
    sample_t feedback_sample = input + tap_sum * multitap->feedback;
    delay_line_write(&multitap->delay, feedback_sample);
    
    return output + tap_sum * multitap->wet_level;
}

// Process buffer through multi-tap delay
void multitap_process_buffer(MultiTapDelay* multitap, AudioBuffer* buffer) {
    if (!multitap || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = multitap_process(multitap, buffer->data[i]);
    }
}

// Create ping-pong delay
PingPongDelay* pingpong_create(float max_delay_seconds, float sample_rate) {
    PingPongDelay* pingpong = malloc(sizeof(PingPongDelay));
    if (!pingpong) return NULL;
    
    size_t max_delay_samples = (size_t)(max_delay_seconds * sample_rate);
    
    DelayLine* left_delay = delay_line_create(max_delay_samples);
    DelayLine* right_delay = delay_line_create(max_delay_samples);
    
    if (!left_delay || !right_delay) {
        if (left_delay) delay_line_destroy(left_delay);
        if (right_delay) delay_line_destroy(right_delay);
        free(pingpong);
        return NULL;
    }
    
    pingpong->left_delay = *left_delay;
    pingpong->right_delay = *right_delay;
    free(left_delay);
    free(right_delay);
    
    pingpong->feedback = 0.3f;
    pingpong->cross_feedback = 0.2f;
    pingpong->wet_level = 0.3f;
    pingpong->dry_level = 0.7f;
    
    onepole_lowpass(&pingpong->left_filter, 6000.0f, sample_rate);
    onepole_lowpass(&pingpong->right_filter, 6000.0f, sample_rate);
    
    return pingpong;
}

// Destroy ping-pong delay
void pingpong_destroy(PingPongDelay* pingpong) {
    if (pingpong) {
        if (pingpong->left_delay.buffer) {
            free(pingpong->left_delay.buffer);
        }
        if (pingpong->right_delay.buffer) {
            free(pingpong->right_delay.buffer);
        }
        free(pingpong);
    }
}

// Set ping-pong delay parameters
void pingpong_set_params(PingPongDelay* pingpong, float delay_seconds, float feedback, 
                        float cross_feedback, float wet_level, float sample_rate) {
    if (!pingpong) return;
    
    pingpong->feedback = clamp(feedback, 0.0f, 0.9f);
    pingpong->cross_feedback = clamp(cross_feedback, 0.0f, 0.9f);
    pingpong->wet_level = clamp(wet_level, 0.0f, 1.0f);
    pingpong->dry_level = 1.0f - pingpong->wet_level;
    
    onepole_lowpass(&pingpong->left_filter, 6000.0f, sample_rate);
    onepole_lowpass(&pingpong->right_filter, 6000.0f, sample_rate);
}

// Process stereo samples through ping-pong delay
void pingpong_process_stereo(PingPongDelay* pingpong, sample_t* left_in, sample_t* right_in,
                           sample_t* left_out, sample_t* right_out) {
    if (!pingpong) {
        *left_out = *left_in;
        *right_out = *right_in;
        return;
    }
    
    size_t delay_samples = pingpong->left_delay.size / 4; // Default delay time
    
    sample_t left_delayed = delay_line_read(&pingpong->left_delay, delay_samples);
    sample_t right_delayed = delay_line_read(&pingpong->right_delay, delay_samples);
    
    // Apply filters to delayed signals
    left_delayed = onepole_process(&pingpong->left_filter, left_delayed, 0);
    right_delayed = onepole_process(&pingpong->right_filter, right_delayed, 0);
    
    // Calculate feedback with cross-feedback (ping-pong effect)
    sample_t left_feedback = *left_in + left_delayed * pingpong->feedback + right_delayed * pingpong->cross_feedback;
    sample_t right_feedback = *right_in + right_delayed * pingpong->feedback + left_delayed * pingpong->cross_feedback;
    
    delay_line_write(&pingpong->left_delay, left_feedback);
    delay_line_write(&pingpong->right_delay, right_feedback);
    
    *left_out = *left_in * pingpong->dry_level + left_delayed * pingpong->wet_level;
    *right_out = *right_in * pingpong->dry_level + right_delayed * pingpong->wet_level;
}