#include "reverb.h"

// Schroeder reverb delay times (in samples at 44.1kHz)
static const int schroeder_comb_delays[] = {1116, 1188, 1277, 1356};
static const int schroeder_allpass_delays[] = {556, 441};
static const float schroeder_comb_gains[] = {0.773f, 0.802f, 0.753f, 0.733f};
static const float schroeder_allpass_gains[] = {0.7f, 0.7f};

// Create Schroeder reverb
SchroederReverb* schroeder_reverb_create(float sample_rate) {
    SchroederReverb* reverb = malloc(sizeof(SchroederReverb));
    if (!reverb) return NULL;
    
    // Scale delay times to sample rate
    float scale = sample_rate / 44100.0f;
    
    // Initialize comb delays
    for (int i = 0; i < 4; i++) {
        size_t delay_samples = (size_t)(schroeder_comb_delays[i] * scale);
        DelayLine* delay = delay_line_create(delay_samples);
        if (!delay) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                delay_line_destroy(&reverb->comb_delays[j]);
            }
            free(reverb);
            return NULL;
        }
        reverb->comb_delays[i] = *delay;
        free(delay);
        
        reverb->comb_gains[i] = schroeder_comb_gains[i];
        onepole_lowpass(&reverb->damping_filters[i], 5000.0f, sample_rate);
    }
    
    // Initialize allpass delays
    for (int i = 0; i < 2; i++) {
        size_t delay_samples = (size_t)(schroeder_allpass_delays[i] * scale);
        DelayLine* delay = delay_line_create(delay_samples);
        if (!delay) {
            // Cleanup on failure
            for (int j = 0; j < 4; j++) {
                delay_line_destroy(&reverb->comb_delays[j]);
            }
            for (int j = 0; j < i; j++) {
                delay_line_destroy(&reverb->allpass_delays[j]);
            }
            free(reverb);
            return NULL;
        }
        reverb->allpass_delays[i] = *delay;
        free(delay);
        
        reverb->allpass_gains[i] = schroeder_allpass_gains[i];
    }
    
    reverb->room_size = 0.5f;
    reverb->damping = 0.5f;
    reverb->wet_level = 0.3f;
    reverb->dry_level = 0.7f;
    
    return reverb;
}

// Destroy Schroeder reverb
void schroeder_reverb_destroy(SchroederReverb* reverb) {
    if (reverb) {
        for (int i = 0; i < 4; i++) {
            if (reverb->comb_delays[i].buffer) {
                free(reverb->comb_delays[i].buffer);
            }
        }
        for (int i = 0; i < 2; i++) {
            if (reverb->allpass_delays[i].buffer) {
                free(reverb->allpass_delays[i].buffer);
            }
        }
        free(reverb);
    }
}

// Set Schroeder reverb parameters
void schroeder_reverb_set_params(SchroederReverb* reverb, float room_size, float damping, float wet_level) {
    if (!reverb) return;
    
    reverb->room_size = clamp(room_size, 0.0f, 1.0f);
    reverb->damping = clamp(damping, 0.0f, 1.0f);
    reverb->wet_level = clamp(wet_level, 0.0f, 1.0f);
    reverb->dry_level = 1.0f - reverb->wet_level;
    
    // Update comb gains based on room size
    for (int i = 0; i < 4; i++) {
        reverb->comb_gains[i] = schroeder_comb_gains[i] * reverb->room_size;
    }
}

// Process one sample through Schroeder reverb
sample_t schroeder_reverb_process(SchroederReverb* reverb, sample_t input) {
    if (!reverb) return input;
    
    sample_t comb_sum = 0.0f;
    
    // Process through comb filters
    for (int i = 0; i < 4; i++) {
        size_t delay_samples = reverb->comb_delays[i].size - 1;
        sample_t delayed = delay_line_read(&reverb->comb_delays[i], delay_samples);
        
        // Apply damping filter
        delayed = onepole_process(&reverb->damping_filters[i], delayed, 0);
        
        sample_t feedback = input + delayed * reverb->comb_gains[i];
        delay_line_write(&reverb->comb_delays[i], feedback);
        
        comb_sum += delayed;
    }
    
    comb_sum *= 0.25f; // Average the comb outputs
    
    // Process through allpass filters
    sample_t allpass_output = comb_sum;
    for (int i = 0; i < 2; i++) {
        size_t delay_samples = reverb->allpass_delays[i].size - 1;
        sample_t delayed = delay_line_read(&reverb->allpass_delays[i], delay_samples);
        
        sample_t feedback = allpass_output + delayed * reverb->allpass_gains[i];
        delay_line_write(&reverb->allpass_delays[i], feedback);
        
        allpass_output = delayed - allpass_output * reverb->allpass_gains[i];
    }
    
    return input * reverb->dry_level + allpass_output * reverb->wet_level;
}

// Process buffer through Schroeder reverb
void schroeder_reverb_process_buffer(SchroederReverb* reverb, AudioBuffer* buffer) {
    if (!reverb || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = schroeder_reverb_process(reverb, buffer->data[i]);
    }
}

// Plate reverb delay times and gains
static const int plate_delays[] = {142, 107, 379, 277, 1011, 1687, 1229, 1597};
static const float plate_gains[] = {0.841f, 0.504f, 0.491f, 0.379f, 0.380f, 0.346f, 0.289f, 0.272f};

// Create plate reverb
PlateReverb* plate_reverb_create(float sample_rate) {
    PlateReverb* reverb = malloc(sizeof(PlateReverb));
    if (!reverb) return NULL;
    
    float scale = sample_rate / 44100.0f;
    
    // Initialize delays
    for (int i = 0; i < 8; i++) {
        size_t delay_samples = (size_t)(plate_delays[i] * scale);
        DelayLine* delay = delay_line_create(delay_samples);
        if (!delay) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                delay_line_destroy(&reverb->delays[j]);
            }
            free(reverb);
            return NULL;
        }
        reverb->delays[i] = *delay;
        free(delay);
        
        reverb->gains[i] = plate_gains[i];
    }
    
    // Initialize filters
    biquad_highpass(&reverb->input_filter, 80.0f, 0.7f, sample_rate);
    biquad_lowpass(&reverb->output_filter, 8000.0f, 0.7f, sample_rate);
    
    reverb->decay_time = 2.0f;
    reverb->wet_level = 0.3f;
    reverb->dry_level = 0.7f;
    reverb->pre_delay = 0.02f; // 20ms pre-delay
    
    return reverb;
}

// Destroy plate reverb
void plate_reverb_destroy(PlateReverb* reverb) {
    if (reverb) {
        for (int i = 0; i < 8; i++) {
            if (reverb->delays[i].buffer) {
                free(reverb->delays[i].buffer);
            }
        }
        free(reverb);
    }
}

// Set plate reverb parameters
void plate_reverb_set_params(PlateReverb* reverb, float decay_time, float wet_level, float pre_delay, float sample_rate) {
    if (!reverb) return;
    
    reverb->decay_time = clamp(decay_time, 0.1f, 10.0f);
    reverb->wet_level = clamp(wet_level, 0.0f, 1.0f);
    reverb->dry_level = 1.0f - reverb->wet_level;
    reverb->pre_delay = clamp(pre_delay, 0.0f, 0.1f);
    
    // Update gains based on decay time
    float decay_factor = powf(0.001f, 1.0f / (reverb->decay_time * sample_rate));
    for (int i = 0; i < 8; i++) {
        reverb->gains[i] = plate_gains[i] * decay_factor;
    }
}

// Process one sample through plate reverb
sample_t plate_reverb_process(PlateReverb* reverb, sample_t input) {
    if (!reverb) return input;
    
    // Apply input filtering
    sample_t filtered_input = biquad_process(&reverb->input_filter, input);
    
    sample_t reverb_sum = 0.0f;
    
    // Process through delay network
    for (int i = 0; i < 8; i++) {
        size_t delay_samples = reverb->delays[i].size - 1;
        sample_t delayed = delay_line_read(&reverb->delays[i], delay_samples);
        
        sample_t feedback = filtered_input + delayed * reverb->gains[i];
        delay_line_write(&reverb->delays[i], feedback);
        
        reverb_sum += delayed;
    }
    
    reverb_sum *= 0.125f; // Average the outputs
    
    // Apply output filtering
    sample_t filtered_output = biquad_process(&reverb->output_filter, reverb_sum);
    
    return input * reverb->dry_level + filtered_output * reverb->wet_level;
}

// Process buffer through plate reverb
void plate_reverb_process_buffer(PlateReverb* reverb, AudioBuffer* buffer) {
    if (!reverb || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = plate_reverb_process(reverb, buffer->data[i]);
    }
}

// Freeverb delay times
static const int freeverb_comb_delays[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
static const int freeverb_allpass_delays[] = {556, 441, 341, 225};

// Create Freeverb
Freeverb* freeverb_create(float sample_rate) {
    Freeverb* reverb = malloc(sizeof(Freeverb));
    if (!reverb) return NULL;
    
    float scale = sample_rate / 44100.0f;
    
    // Initialize comb delays
    for (int i = 0; i < 8; i++) {
        size_t delay_samples = (size_t)(freeverb_comb_delays[i] * scale);
        DelayLine* delay = delay_line_create(delay_samples);
        if (!delay) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                delay_line_destroy(&reverb->comb_delays[j]);
            }
            free(reverb);
            return NULL;
        }
        reverb->comb_delays[i] = *delay;
        free(delay);
        
        reverb->comb_feedbacks[i] = 0.84f;
        onepole_lowpass(&reverb->comb_filters[i], 5000.0f, sample_rate);
    }
    
    // Initialize allpass delays
    for (int i = 0; i < 4; i++) {
        size_t delay_samples = (size_t)(freeverb_allpass_delays[i] * scale);
        DelayLine* delay = delay_line_create(delay_samples);
        if (!delay) {
            // Cleanup on failure
            for (int j = 0; j < 8; j++) {
                delay_line_destroy(&reverb->comb_delays[j]);
            }
            for (int j = 0; j < i; j++) {
                delay_line_destroy(&reverb->allpass_delays[j]);
            }
            free(reverb);
            return NULL;
        }
        reverb->allpass_delays[i] = *delay;
        free(delay);
    }
    
    reverb->room_size = 0.5f;
    reverb->damping = 0.5f;
    reverb->wet_level = 0.3f;
    reverb->dry_level = 0.7f;
    reverb->width = 1.0f;
    
    return reverb;
}

// Destroy Freeverb
void freeverb_destroy(Freeverb* reverb) {
    if (reverb) {
        for (int i = 0; i < 8; i++) {
            if (reverb->comb_delays[i].buffer) {
                free(reverb->comb_delays[i].buffer);
            }
        }
        for (int i = 0; i < 4; i++) {
            if (reverb->allpass_delays[i].buffer) {
                free(reverb->allpass_delays[i].buffer);
            }
        }
        free(reverb);
    }
}

// Set Freeverb parameters
void freeverb_set_params(Freeverb* reverb, float room_size, float damping, float wet_level, float width) {
    if (!reverb) return;
    
    reverb->room_size = clamp(room_size, 0.0f, 1.0f);
    reverb->damping = clamp(damping, 0.0f, 1.0f);
    reverb->wet_level = clamp(wet_level, 0.0f, 1.0f);
    reverb->dry_level = 1.0f - reverb->wet_level;
    reverb->width = clamp(width, 0.0f, 1.0f);
    
    // Update comb feedbacks based on room size
    for (int i = 0; i < 8; i++) {
        reverb->comb_feedbacks[i] = 0.28f + 0.7f * reverb->room_size;
    }
}

// Process one sample through Freeverb
sample_t freeverb_process(Freeverb* reverb, sample_t input) {
    if (!reverb) return input;
    
    sample_t comb_sum = 0.0f;
    
    // Process through comb filters
    for (int i = 0; i < 8; i++) {
        size_t delay_samples = reverb->comb_delays[i].size - 1;
        sample_t delayed = delay_line_read(&reverb->comb_delays[i], delay_samples);
        
        // Apply damping filter
        delayed = onepole_process(&reverb->comb_filters[i], delayed, 0);
        
        sample_t feedback = input + delayed * reverb->comb_feedbacks[i];
        delay_line_write(&reverb->comb_delays[i], feedback);
        
        comb_sum += delayed;
    }
    
    // Process through allpass filters
    sample_t allpass_output = comb_sum;
    for (int i = 0; i < 4; i++) {
        size_t delay_samples = reverb->allpass_delays[i].size - 1;
        sample_t delayed = delay_line_read(&reverb->allpass_delays[i], delay_samples);
        
        sample_t feedback = allpass_output + delayed * 0.5f;
        delay_line_write(&reverb->allpass_delays[i], feedback);
        
        allpass_output = delayed - allpass_output * 0.5f;
    }
    
    return input * reverb->dry_level + allpass_output * reverb->wet_level;
}

// Process buffer through Freeverb
void freeverb_process_buffer(Freeverb* reverb, AudioBuffer* buffer) {
    if (!reverb || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = freeverb_process(reverb, buffer->data[i]);
    }
}