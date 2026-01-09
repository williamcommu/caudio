#include "audio_core.h"

// Create a new audio buffer
AudioBuffer* audio_buffer_create(size_t length, size_t channels, size_t sample_rate) {
    AudioBuffer* buffer = malloc(sizeof(AudioBuffer));
    if (!buffer) return NULL;
    
    buffer->length = length;
    buffer->channels = channels;
    buffer->sample_rate = sample_rate;
    buffer->capacity = length * channels;
    
    buffer->data = calloc(buffer->capacity, sizeof(sample_t));
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }
    
    return buffer;
}

// Destroy audio buffer
void audio_buffer_destroy(AudioBuffer* buffer) {
    if (buffer) {
        if (buffer->data) {
            free(buffer->data);
        }
        free(buffer);
    }
}

// Clear audio buffer (set all samples to zero)
void audio_buffer_clear(AudioBuffer* buffer) {
    if (buffer && buffer->data) {
        memset(buffer->data, 0, buffer->capacity * sizeof(sample_t));
    }
}

// Copy audio buffer
void audio_buffer_copy(AudioBuffer* dest, AudioBuffer* src) {
    if (!dest || !src || !dest->data || !src->data) return;
    
    size_t samples_to_copy = (dest->capacity < src->capacity) ? dest->capacity : src->capacity;
    memcpy(dest->data, src->data, samples_to_copy * sizeof(sample_t));
}

// Mix two audio buffers
void audio_buffer_mix(AudioBuffer* dest, AudioBuffer* src, float gain) {
    if (!dest || !src || !dest->data || !src->data) return;
    
    size_t samples_to_mix = (dest->capacity < src->capacity) ? dest->capacity : src->capacity;
    
    for (size_t i = 0; i < samples_to_mix; i++) {
        dest->data[i] += src->data[i] * gain;
    }
}

// Convert decibels to linear scale
float db_to_linear(float db) {
    return powf(10.0f, db / 20.0f);
}

// Convert linear scale to decibels
float linear_to_db(float linear) {
    if (linear <= 0.0f) return -100.0f; // Very quiet
    return 20.0f * log10f(linear);
}

// Clamp value between min and max
float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Linear interpolation
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Convert float sample to 16-bit integer
int16_t float_to_int16(float sample) {
    sample = clamp(sample, -1.0f, 1.0f);
    return (int16_t)(sample * 32767.0f);
}

// Convert 16-bit integer to float sample
float int16_to_float(int16_t sample) {
    return (float)sample / 32767.0f;
}