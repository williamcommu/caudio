#ifndef DELAY_EFFECTS_H
#define DELAY_EFFECTS_H

#include "audio_core.h"
#include "audio_filters.h"

// Simple delay line structure
typedef struct {
    sample_t* buffer;
    size_t size;
    size_t write_pos;
    size_t read_pos;
} DelayLine;

// Echo effect structure
typedef struct {
    DelayLine delay;
    float feedback;
    float wet_level;
    float dry_level;
    OnePoleFilter feedback_filter;
} Echo;

// Multi-tap delay structure
typedef struct {
    DelayLine delay;
    float tap_gains[8];
    size_t tap_delays[8];
    int num_taps;
    float feedback;
    float wet_level;
    float dry_level;
} MultiTapDelay;

// Ping-pong delay structure (stereo)
typedef struct {
    DelayLine left_delay;
    DelayLine right_delay;
    float feedback;
    float cross_feedback;
    float wet_level;
    float dry_level;
    OnePoleFilter left_filter;
    OnePoleFilter right_filter;
} PingPongDelay;

// Delay line functions
DelayLine* delay_line_create(size_t max_delay_samples);
void delay_line_destroy(DelayLine* delay);
void delay_line_write(DelayLine* delay, sample_t sample);
sample_t delay_line_read(DelayLine* delay, size_t delay_samples);
sample_t delay_line_read_interpolated(DelayLine* delay, float delay_samples);
void delay_line_clear(DelayLine* delay);

// Echo effect functions
Echo* echo_create(float max_delay_seconds, float sample_rate);
void echo_destroy(Echo* echo);
void echo_set_params(Echo* echo, float delay_seconds, float feedback, float wet_level, float sample_rate);
sample_t echo_process(Echo* echo, sample_t input);
void echo_process_buffer(Echo* echo, AudioBuffer* buffer);

// Multi-tap delay functions
MultiTapDelay* multitap_create(float max_delay_seconds, float sample_rate);
void multitap_destroy(MultiTapDelay* multitap);
void multitap_set_tap(MultiTapDelay* multitap, int tap_index, float delay_seconds, float gain, float sample_rate);
void multitap_set_feedback(MultiTapDelay* multitap, float feedback, float wet_level);
sample_t multitap_process(MultiTapDelay* multitap, sample_t input);
void multitap_process_buffer(MultiTapDelay* multitap, AudioBuffer* buffer);

// Ping-pong delay functions (for stereo processing)
PingPongDelay* pingpong_create(float max_delay_seconds, float sample_rate);
void pingpong_destroy(PingPongDelay* pingpong);
void pingpong_set_params(PingPongDelay* pingpong, float delay_seconds, float feedback, 
                        float cross_feedback, float wet_level, float sample_rate);
void pingpong_process_stereo(PingPongDelay* pingpong, sample_t* left_in, sample_t* right_in,
                           sample_t* left_out, sample_t* right_out);

#endif // DELAY_EFFECTS_H