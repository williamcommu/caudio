#include "modulation_effects.h"

// LFO functions

// Initialize LFO
void lfo_init(LFO* lfo, float frequency, float sample_rate) {
    if (!lfo) return;
    
    lfo->frequency = frequency;
    lfo->sample_rate = sample_rate;
    lfo->phase = 0.0f;
    lfo->amplitude = 1.0f;
    lfo->offset = 0.0f;
}

// Set LFO parameters
void lfo_set_params(LFO* lfo, float frequency, float amplitude, float offset) {
    if (!lfo) return;
    
    lfo->frequency = clamp(frequency, 0.01f, 20.0f);
    lfo->amplitude = clamp(amplitude, 0.0f, 2.0f);
    lfo->offset = clamp(offset, -1.0f, 1.0f);
}

// Process LFO (sine wave)
float lfo_process(LFO* lfo) {
    if (!lfo) return 0.0f;
    
    float output = lfo->amplitude * sinf(lfo->phase) + lfo->offset;
    
    lfo->phase += TWO_PI * lfo->frequency / lfo->sample_rate;
    if (lfo->phase >= TWO_PI) {
        lfo->phase -= TWO_PI;
    }
    
    return output;
}

// Triangle wave LFO
float lfo_triangle(LFO* lfo) {
    if (!lfo) return 0.0f;
    
    float phase_norm = lfo->phase / TWO_PI;
    float triangle;
    
    if (phase_norm < 0.5f) {
        triangle = 4.0f * phase_norm - 1.0f;
    } else {
        triangle = 3.0f - 4.0f * phase_norm;
    }
    
    float output = lfo->amplitude * triangle + lfo->offset;
    
    lfo->phase += TWO_PI * lfo->frequency / lfo->sample_rate;
    if (lfo->phase >= TWO_PI) {
        lfo->phase -= TWO_PI;
    }
    
    return output;
}

// Sawtooth wave LFO
float lfo_sawtooth(LFO* lfo) {
    if (!lfo) return 0.0f;
    
    float phase_norm = lfo->phase / TWO_PI;
    float sawtooth = 2.0f * phase_norm - 1.0f;
    float output = lfo->amplitude * sawtooth + lfo->offset;
    
    lfo->phase += TWO_PI * lfo->frequency / lfo->sample_rate;
    if (lfo->phase >= TWO_PI) {
        lfo->phase -= TWO_PI;
    }
    
    return output;
}

// Square wave LFO
float lfo_square(LFO* lfo) {
    if (!lfo) return 0.0f;
    
    float phase_norm = lfo->phase / TWO_PI;
    float square = (phase_norm < 0.5f) ? 1.0f : -1.0f;
    float output = lfo->amplitude * square + lfo->offset;
    
    lfo->phase += TWO_PI * lfo->frequency / lfo->sample_rate;
    if (lfo->phase >= TWO_PI) {
        lfo->phase -= TWO_PI;
    }
    
    return output;
}

// Chorus functions

// Create chorus effect
Chorus* chorus_create(float max_delay_ms, float sample_rate) {
    Chorus* chorus = malloc(sizeof(Chorus));
    if (!chorus) return NULL;
    
    size_t max_delay_samples = (size_t)((max_delay_ms / 1000.0f) * sample_rate);
    DelayLine* delay = delay_line_create(max_delay_samples);
    if (!delay) {
        free(chorus);
        return NULL;
    }
    
    chorus->delay = *delay;
    free(delay);
    
    lfo_init(&chorus->lfo, 1.0f, sample_rate);
    chorus->depth = 0.5f;
    chorus->rate = 1.0f;
    chorus->feedback = 0.1f;
    chorus->wet_level = 0.5f;
    chorus->dry_level = 0.5f;
    
    onepole_lowpass(&chorus->feedback_filter, 5000.0f, sample_rate);
    
    return chorus;
}

// Destroy chorus
void chorus_destroy(Chorus* chorus) {
    if (chorus) {
        if (chorus->delay.buffer) {
            free(chorus->delay.buffer);
        }
        free(chorus);
    }
}

// Set chorus parameters
void chorus_set_params(Chorus* chorus, float rate, float depth, float feedback, float wet_level) {
    if (!chorus) return;
    
    chorus->rate = clamp(rate, 0.1f, 10.0f);
    chorus->depth = clamp(depth, 0.0f, 1.0f);
    chorus->feedback = clamp(feedback, 0.0f, 0.5f);
    chorus->wet_level = clamp(wet_level, 0.0f, 1.0f);
    chorus->dry_level = 1.0f - chorus->wet_level;
    
    lfo_set_params(&chorus->lfo, chorus->rate, chorus->depth, 0.5f);
}

// Process one sample through chorus
sample_t chorus_process(Chorus* chorus, sample_t input) {
    if (!chorus) return input;
    
    // Get LFO value to modulate delay time
    float lfo_value = lfo_process(&chorus->lfo);
    float delay_samples = lfo_value * (chorus->delay.size / 4.0f); // Use portion of max delay
    
    // Read delayed signal with interpolation
    sample_t delayed = delay_line_read_interpolated(&chorus->delay, delay_samples);
    
    // Apply feedback filtering
    sample_t filtered_delayed = onepole_process(&chorus->feedback_filter, delayed, 0);
    
    // Write to delay line with feedback
    sample_t feedback_sample = input + filtered_delayed * chorus->feedback;
    delay_line_write(&chorus->delay, feedback_sample);
    
    // Mix dry and wet signals
    return input * chorus->dry_level + delayed * chorus->wet_level;
}

// Process buffer through chorus
void chorus_process_buffer(Chorus* chorus, AudioBuffer* buffer) {
    if (!chorus || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = chorus_process(chorus, buffer->data[i]);
    }
}

// Flanger functions

// Create flanger effect
Flanger* flanger_create(float max_delay_ms, float sample_rate) {
    Flanger* flanger = malloc(sizeof(Flanger));
    if (!flanger) return NULL;
    
    size_t max_delay_samples = (size_t)((max_delay_ms / 1000.0f) * sample_rate);
    DelayLine* delay = delay_line_create(max_delay_samples);
    if (!delay) {
        free(flanger);
        return NULL;
    }
    
    flanger->delay = *delay;
    free(delay);
    
    lfo_init(&flanger->lfo, 0.5f, sample_rate);
    flanger->depth = 0.8f;
    flanger->rate = 0.5f;
    flanger->feedback = 0.3f;
    flanger->wet_level = 0.5f;
    flanger->dry_level = 0.5f;
    flanger->manual = 0.5f;
    
    onepole_lowpass(&flanger->feedback_filter, 8000.0f, sample_rate);
    
    return flanger;
}

// Destroy flanger
void flanger_destroy(Flanger* flanger) {
    if (flanger) {
        if (flanger->delay.buffer) {
            free(flanger->delay.buffer);
        }
        free(flanger);
    }
}

// Set flanger parameters
void flanger_set_params(Flanger* flanger, float rate, float depth, float feedback, float manual, float wet_level) {
    if (!flanger) return;
    
    flanger->rate = clamp(rate, 0.01f, 5.0f);
    flanger->depth = clamp(depth, 0.0f, 1.0f);
    flanger->feedback = clamp(feedback, 0.0f, 0.9f);
    flanger->manual = clamp(manual, 0.0f, 1.0f);
    flanger->wet_level = clamp(wet_level, 0.0f, 1.0f);
    flanger->dry_level = 1.0f - flanger->wet_level;
    
    lfo_set_params(&flanger->lfo, flanger->rate, flanger->depth, flanger->manual);
}

// Process one sample through flanger
sample_t flanger_process(Flanger* flanger, sample_t input) {
    if (!flanger) return input;
    
    // Get LFO value for delay modulation
    float lfo_value = lfo_triangle(&flanger->lfo); // Triangle wave for flanger
    float delay_samples = lfo_value * (flanger->delay.size / 8.0f); // Shorter delay than chorus
    
    // Ensure minimum delay to avoid zero-delay issues
    delay_samples = clamp(delay_samples, 1.0f, (float)(flanger->delay.size - 1));
    
    // Read delayed signal
    sample_t delayed = delay_line_read_interpolated(&flanger->delay, delay_samples);
    
    // Apply feedback filtering
    sample_t filtered_delayed = onepole_process(&flanger->feedback_filter, delayed, 0);
    
    // Write to delay line with feedback
    sample_t feedback_sample = input + filtered_delayed * flanger->feedback;
    delay_line_write(&flanger->delay, feedback_sample);
    
    // Mix with inverted phase for flanger effect
    return input * flanger->dry_level - delayed * flanger->wet_level;
}

// Process buffer through flanger
void flanger_process_buffer(Flanger* flanger, AudioBuffer* buffer) {
    if (!flanger || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = flanger_process(flanger, buffer->data[i]);
    }
}

// Phaser functions

// Create phaser effect
Phaser* phaser_create(int num_stages, float sample_rate) {
    Phaser* phaser = malloc(sizeof(Phaser));
    if (!phaser) return NULL;
    
    phaser->num_stages = clamp(num_stages, 2, 6);
    
    // Initialize allpass filters
    for (int i = 0; i < phaser->num_stages; i++) {
        float freq = 1000.0f + i * 500.0f; // Spread frequencies
        biquad_bandpass(&phaser->allpass_stages[i], freq, 2.0f, sample_rate);
    }
    
    lfo_init(&phaser->lfo, 0.5f, sample_rate);
    phaser->depth = 0.7f;
    phaser->rate = 0.5f;
    phaser->feedback = 0.2f;
    phaser->wet_level = 0.5f;
    phaser->dry_level = 0.5f;
    
    return phaser;
}

// Destroy phaser
void phaser_destroy(Phaser* phaser) {
    if (phaser) {
        free(phaser);
    }
}

// Set phaser parameters
void phaser_set_params(Phaser* phaser, float rate, float depth, float feedback, float wet_level) {
    if (!phaser) return;
    
    phaser->rate = clamp(rate, 0.01f, 5.0f);
    phaser->depth = clamp(depth, 0.0f, 1.0f);
    phaser->feedback = clamp(feedback, 0.0f, 0.7f);
    phaser->wet_level = clamp(wet_level, 0.0f, 1.0f);
    phaser->dry_level = 1.0f - phaser->wet_level;
    
    lfo_set_params(&phaser->lfo, phaser->rate, phaser->depth, 0.0f);
}

// Process one sample through phaser
sample_t phaser_process(Phaser* phaser, sample_t input) {
    if (!phaser) return input;
    
    float lfo_value = lfo_process(&phaser->lfo);
    
    sample_t processed = input;
    
    // Process through allpass stages with LFO modulation
    for (int i = 0; i < phaser->num_stages; i++) {
        // Modulate the filter coefficients based on LFO
        // This is a simplified approach - real phasers modulate the allpass frequency
        processed = biquad_process(&phaser->allpass_stages[i], processed);
        processed *= (1.0f + lfo_value * phaser->depth * 0.1f);
    }
    
    // Add feedback
    processed += processed * phaser->feedback * 0.5f;
    
    // Mix with original
    return input * phaser->dry_level + processed * phaser->wet_level;
}

// Process buffer through phaser
void phaser_process_buffer(Phaser* phaser, AudioBuffer* buffer) {
    if (!phaser || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = phaser_process(phaser, buffer->data[i]);
    }
}

// Tremolo functions

// Create tremolo effect
Tremolo* tremolo_create(float sample_rate) {
    Tremolo* tremolo = malloc(sizeof(Tremolo));
    if (!tremolo) return NULL;
    
    lfo_init(&tremolo->lfo, 4.0f, sample_rate);
    tremolo->depth = 0.5f;
    tremolo->rate = 4.0f;
    tremolo->stereo_phase = 0;
    
    return tremolo;
}

// Destroy tremolo
void tremolo_destroy(Tremolo* tremolo) {
    if (tremolo) {
        free(tremolo);
    }
}

// Set tremolo parameters
void tremolo_set_params(Tremolo* tremolo, float rate, float depth, int stereo_phase) {
    if (!tremolo) return;
    
    tremolo->rate = clamp(rate, 0.1f, 20.0f);
    tremolo->depth = clamp(depth, 0.0f, 1.0f);
    tremolo->stereo_phase = stereo_phase;
    
    lfo_set_params(&tremolo->lfo, tremolo->rate, tremolo->depth, 1.0f - tremolo->depth);
}

// Process one sample through tremolo
sample_t tremolo_process(Tremolo* tremolo, sample_t input) {
    if (!tremolo) return input;
    
    float lfo_value = lfo_process(&tremolo->lfo);
    return input * lfo_value;
}

// Process stereo samples through tremolo
void tremolo_process_stereo(Tremolo* tremolo, sample_t* left, sample_t* right) {
    if (!tremolo) return;
    
    float lfo_left = lfo_process(&tremolo->lfo);
    
    float lfo_right;
    if (tremolo->stereo_phase) {
        // For stereo tremolo, right channel is 180 degrees out of phase
        lfo_right = 2.0f * (1.0f - tremolo->depth) - lfo_left;
    } else {
        lfo_right = lfo_left;
    }
    
    *left *= lfo_left;
    *right *= lfo_right;
}

// Vibrato functions

// Create vibrato effect
Vibrato* vibrato_create(float max_delay_ms, float sample_rate) {
    Vibrato* vibrato = malloc(sizeof(Vibrato));
    if (!vibrato) return NULL;
    
    size_t max_delay_samples = (size_t)((max_delay_ms / 1000.0f) * sample_rate);
    DelayLine* delay = delay_line_create(max_delay_samples);
    if (!delay) {
        free(vibrato);
        return NULL;
    }
    
    vibrato->delay = *delay;
    free(delay);
    
    lfo_init(&vibrato->lfo, 5.0f, sample_rate);
    vibrato->depth = 0.3f;
    vibrato->rate = 5.0f;
    vibrato->wet_level = 1.0f;
    
    return vibrato;
}

// Destroy vibrato
void vibrato_destroy(Vibrato* vibrato) {
    if (vibrato) {
        if (vibrato->delay.buffer) {
            free(vibrato->delay.buffer);
        }
        free(vibrato);
    }
}

// Set vibrato parameters
void vibrato_set_params(Vibrato* vibrato, float rate, float depth, float wet_level) {
    if (!vibrato) return;
    
    vibrato->rate = clamp(rate, 0.1f, 20.0f);
    vibrato->depth = clamp(depth, 0.0f, 1.0f);
    vibrato->wet_level = clamp(wet_level, 0.0f, 1.0f);
    
    lfo_set_params(&vibrato->lfo, vibrato->rate, vibrato->depth, 0.5f);
}

// Process one sample through vibrato
sample_t vibrato_process(Vibrato* vibrato, sample_t input) {
    if (!vibrato) return input;
    
    // Write input to delay line
    delay_line_write(&vibrato->delay, input);
    
    // Get LFO value to modulate delay time
    float lfo_value = lfo_process(&vibrato->lfo);
    float delay_samples = lfo_value * (vibrato->delay.size / 6.0f);
    
    // Ensure minimum delay
    delay_samples = clamp(delay_samples, 1.0f, (float)(vibrato->delay.size - 1));
    
    // Read modulated signal
    sample_t delayed = delay_line_read_interpolated(&vibrato->delay, delay_samples);
    
    return delayed * vibrato->wet_level + input * (1.0f - vibrato->wet_level);
}

// Process buffer through vibrato
void vibrato_process_buffer(Vibrato* vibrato, AudioBuffer* buffer) {
    if (!vibrato || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = vibrato_process(vibrato, buffer->data[i]);
    }
}

// Auto-wah functions

// Create auto-wah effect
AutoWah* autowah_create(float sample_rate) {
    AutoWah* autowah = malloc(sizeof(AutoWah));
    if (!autowah) return NULL;
    
    autowah->sensitivity = 0.5f;
    autowah->frequency_min = 200.0f;
    autowah->frequency_max = 2000.0f;
    autowah->resonance = 2.0f;
    autowah->rate = 0.0f; // Manual control initially
    autowah->sample_rate = sample_rate;
    
    biquad_bandpass(&autowah->filter, 1000.0f, 2.0f, sample_rate);
    lfo_init(&autowah->lfo, 0.5f, sample_rate);
    onepole_lowpass(&autowah->envelope_follower, 10.0f, sample_rate);
    
    return autowah;
}

// Destroy auto-wah
void autowah_destroy(AutoWah* autowah) {
    if (autowah) {
        free(autowah);
    }
}

// Set auto-wah parameters
void autowah_set_params(AutoWah* autowah, float sensitivity, float freq_min, float freq_max, float resonance, float rate) {
    if (!autowah) return;
    
    autowah->sensitivity = clamp(sensitivity, 0.0f, 1.0f);
    autowah->frequency_min = clamp(freq_min, 50.0f, 500.0f);
    autowah->frequency_max = clamp(freq_max, 500.0f, 5000.0f);
    autowah->resonance = clamp(resonance, 0.5f, 10.0f);
    autowah->rate = clamp(rate, 0.0f, 5.0f);
    
    lfo_set_params(&autowah->lfo, autowah->rate, 1.0f, 0.0f);
}

// Process one sample through auto-wah
sample_t autowah_process(AutoWah* autowah, sample_t input) {
    if (!autowah) return input;
    
    // Envelope following
    float envelope = onepole_process(&autowah->envelope_follower, fabsf(input), 0);
    
    // Calculate filter frequency
    float freq;
    if (autowah->rate > 0.0f) {
        // LFO mode
        float lfo_value = (lfo_process(&autowah->lfo) + 1.0f) * 0.5f; // Normalize to 0-1
        freq = autowah->frequency_min + lfo_value * (autowah->frequency_max - autowah->frequency_min);
    } else {
        // Envelope-following mode
        freq = autowah->frequency_min + envelope * autowah->sensitivity * (autowah->frequency_max - autowah->frequency_min);
    }
    
    // Update filter frequency
    biquad_bandpass(&autowah->filter, freq, autowah->resonance, autowah->sample_rate);
    
    // Process through filter
    return biquad_process(&autowah->filter, input);
}

// Process buffer through auto-wah
void autowah_process_buffer(AutoWah* autowah, AudioBuffer* buffer) {
    if (!autowah || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = autowah_process(autowah, buffer->data[i]);
    }
}