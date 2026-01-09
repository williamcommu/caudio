#ifndef MODULATION_EFFECTS_H
#define MODULATION_EFFECTS_H

#include "audio_core.h"
#include "delay_effects.h"
#include "audio_filters.h"

// LFO (Low Frequency Oscillator) structure
typedef struct {
    float frequency;
    float phase;
    float sample_rate;
    float amplitude;
    float offset;
} LFO;

// Chorus effect structure
typedef struct {
    DelayLine delay;
    LFO lfo;
    float depth;
    float rate;
    float feedback;
    float wet_level;
    float dry_level;
    OnePoleFilter feedback_filter;
} Chorus;

// Flanger effect structure
typedef struct {
    DelayLine delay;
    LFO lfo;
    float depth;
    float rate;
    float feedback;
    float wet_level;
    float dry_level;
    float manual; // Manual delay offset
    OnePoleFilter feedback_filter;
} Flanger;

// Phaser effect structure
typedef struct {
    BiquadFilter allpass_stages[6];
    LFO lfo;
    float depth;
    float rate;
    float feedback;
    float wet_level;
    float dry_level;
    int num_stages;
} Phaser;

// Tremolo effect structure
typedef struct {
    LFO lfo;
    float depth;
    float rate;
    int stereo_phase; // Phase offset for stereo tremolo
} Tremolo;

// Vibrato effect structure
typedef struct {
    DelayLine delay;
    LFO lfo;
    float depth;
    float rate;
    float wet_level;
} Vibrato;

// Auto-wah effect structure
typedef struct {
    BiquadFilter filter;
    LFO lfo;
    float sensitivity;
    float frequency_min;
    float frequency_max;
    float resonance;
    float rate;
    OnePoleFilter envelope_follower;
    float sample_rate;
} AutoWah;

// LFO functions
void lfo_init(LFO* lfo, float frequency, float sample_rate);
void lfo_set_params(LFO* lfo, float frequency, float amplitude, float offset);
float lfo_process(LFO* lfo); // Returns sine wave
float lfo_triangle(LFO* lfo); // Returns triangle wave
float lfo_sawtooth(LFO* lfo); // Returns sawtooth wave
float lfo_square(LFO* lfo);   // Returns square wave

// Chorus functions
Chorus* chorus_create(float max_delay_ms, float sample_rate);
void chorus_destroy(Chorus* chorus);
void chorus_set_params(Chorus* chorus, float rate, float depth, float feedback, float wet_level);
sample_t chorus_process(Chorus* chorus, sample_t input);
void chorus_process_buffer(Chorus* chorus, AudioBuffer* buffer);

// Flanger functions
Flanger* flanger_create(float max_delay_ms, float sample_rate);
void flanger_destroy(Flanger* flanger);
void flanger_set_params(Flanger* flanger, float rate, float depth, float feedback, float manual, float wet_level);
sample_t flanger_process(Flanger* flanger, sample_t input);
void flanger_process_buffer(Flanger* flanger, AudioBuffer* buffer);

// Phaser functions
Phaser* phaser_create(int num_stages, float sample_rate);
void phaser_destroy(Phaser* phaser);
void phaser_set_params(Phaser* phaser, float rate, float depth, float feedback, float wet_level);
sample_t phaser_process(Phaser* phaser, sample_t input);
void phaser_process_buffer(Phaser* phaser, AudioBuffer* buffer);

// Tremolo functions
Tremolo* tremolo_create(float sample_rate);
void tremolo_destroy(Tremolo* tremolo);
void tremolo_set_params(Tremolo* tremolo, float rate, float depth, int stereo_phase);
sample_t tremolo_process(Tremolo* tremolo, sample_t input);
void tremolo_process_stereo(Tremolo* tremolo, sample_t* left, sample_t* right);

// Vibrato functions
Vibrato* vibrato_create(float max_delay_ms, float sample_rate);
void vibrato_destroy(Vibrato* vibrato);
void vibrato_set_params(Vibrato* vibrato, float rate, float depth, float wet_level);
sample_t vibrato_process(Vibrato* vibrato, sample_t input);
void vibrato_process_buffer(Vibrato* vibrato, AudioBuffer* buffer);

// Auto-wah functions
AutoWah* autowah_create(float sample_rate);
void autowah_destroy(AutoWah* autowah);
void autowah_set_params(AutoWah* autowah, float sensitivity, float freq_min, float freq_max, float resonance, float rate);
sample_t autowah_process(AutoWah* autowah, sample_t input);
void autowah_process_buffer(AutoWah* autowah, AudioBuffer* buffer);

#endif // MODULATION_EFFECTS_H