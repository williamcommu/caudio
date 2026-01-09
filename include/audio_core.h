#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Audio format constants
#define MAX_CHANNELS 2
#define DEFAULT_SAMPLE_RATE 44100
#define MAX_BUFFER_SIZE 8192

// Math constants
#define PI 3.14159265358979323846
#define TWO_PI (2.0 * PI)

// Audio data types
typedef float sample_t;
typedef struct {
    sample_t* data;
    size_t length;
    size_t channels;
    size_t sample_rate;
    size_t capacity;
} AudioBuffer;

typedef struct {
    uint16_t format;        // Audio format (1 = PCM)
    uint16_t channels;      // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Bytes per second
    uint16_t block_align;   // Bytes per sample frame
    uint16_t bits_per_sample; // Bits per sample
} AudioFormat;

// Core audio buffer functions
AudioBuffer* audio_buffer_create(size_t length, size_t channels, size_t sample_rate);
void audio_buffer_destroy(AudioBuffer* buffer);
void audio_buffer_clear(AudioBuffer* buffer);
void audio_buffer_copy(AudioBuffer* dest, AudioBuffer* src);
void audio_buffer_mix(AudioBuffer* dest, AudioBuffer* src, float gain);

// Utility functions
float db_to_linear(float db);
float linear_to_db(float linear);
float clamp(float value, float min, float max);
float lerp(float a, float b, float t);

// Sample conversion functions
int16_t float_to_int16(float sample);
float int16_to_float(int16_t sample);

#endif // AUDIO_CORE_H