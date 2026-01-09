# Audio Effects Library - API Reference

## Core Audio Functions

### AudioBuffer Management
```c
AudioBuffer* audio_buffer_create(size_t length, size_t channels, size_t sample_rate);
void audio_buffer_destroy(AudioBuffer* buffer);
void audio_buffer_clear(AudioBuffer* buffer);
void audio_buffer_copy(AudioBuffer* dest, AudioBuffer* src);
void audio_buffer_mix(AudioBuffer* dest, AudioBuffer* src, float gain);
```

### WAV File I/O
```c
AudioBuffer* wav_load(const char* filename);
int wav_save(const char* filename, AudioBuffer* buffer);
void print_wav_info(const char* filename);
```

## Filter Effects

### Biquad Filters
```c
void biquad_lowpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_highpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_bandpass(BiquadFilter* filter, float freq, float q, float sample_rate);
void biquad_notch(BiquadFilter* filter, float freq, float q, float sample_rate);
float biquad_process(BiquadFilter* filter, float input);
void biquad_process_buffer(BiquadFilter* filter, AudioBuffer* buffer);
```

### 4-Band EQ
```c
void eq_init(FourBandEQ* eq, float sample_rate);
void eq_set_gains(FourBandEQ* eq, float low, float low_mid, float high_mid, float high);
float eq_process(FourBandEQ* eq, float input);
void eq_process_buffer(FourBandEQ* eq, AudioBuffer* buffer);
```

## Delay Effects

### Echo
```c
Echo* echo_create(float max_delay_seconds, float sample_rate);
void echo_destroy(Echo* echo);
void echo_set_params(Echo* echo, float delay_seconds, float feedback, float wet_level, float sample_rate);
sample_t echo_process(Echo* echo, sample_t input);
void echo_process_buffer(Echo* echo, AudioBuffer* buffer);
```

### Multi-tap Delay
```c
MultiTapDelay* multitap_create(float max_delay_seconds, float sample_rate);
void multitap_destroy(MultiTapDelay* multitap);
void multitap_set_tap(MultiTapDelay* multitap, int tap_index, float delay_seconds, float gain, float sample_rate);
void multitap_set_feedback(MultiTapDelay* multitap, float feedback, float wet_level);
sample_t multitap_process(MultiTapDelay* multitap, sample_t input);
```

## Reverb Effects

### Schroeder Reverb
```c
SchroederReverb* schroeder_reverb_create(float sample_rate);
void schroeder_reverb_destroy(SchroederReverb* reverb);
void schroeder_reverb_set_params(SchroederReverb* reverb, float room_size, float damping, float wet_level);
sample_t schroeder_reverb_process(SchroederReverb* reverb, sample_t input);
```

### Plate Reverb
```c
PlateReverb* plate_reverb_create(float sample_rate);
void plate_reverb_destroy(PlateReverb* reverb);
void plate_reverb_set_params(PlateReverb* reverb, float decay_time, float wet_level, float pre_delay, float sample_rate);
sample_t plate_reverb_process(PlateReverb* reverb, sample_t input);
```

## Distortion Effects

### Overdrive
```c
Overdrive* overdrive_create(float sample_rate);
void overdrive_destroy(Overdrive* overdrive);
void overdrive_set_params(Overdrive* overdrive, float drive, float tone, float output_gain, float mix);
sample_t overdrive_process(Overdrive* overdrive, sample_t input);
```

### Tube Distortion
```c
TubeDistortion* tube_distortion_create(float sample_rate);
void tube_distortion_destroy(TubeDistortion* tube);
void tube_distortion_set_params(TubeDistortion* tube, float drive, float bias, float output_gain, float mix);
sample_t tube_distortion_process(TubeDistortion* tube, sample_t input);
```

### Fuzz Distortion
```c
FuzzDistortion* fuzz_distortion_create(float sample_rate);
void fuzz_distortion_destroy(FuzzDistortion* fuzz);
void fuzz_distortion_set_params(FuzzDistortion* fuzz, float fuzz_amount, float gate_threshold, float output_gain, float mix);
sample_t fuzz_distortion_process(FuzzDistortion* fuzz, sample_t input);
```

## Modulation Effects

### Chorus
```c
Chorus* chorus_create(float max_delay_ms, float sample_rate);
void chorus_destroy(Chorus* chorus);
void chorus_set_params(Chorus* chorus, float rate, float depth, float feedback, float wet_level);
sample_t chorus_process(Chorus* chorus, sample_t input);
```

### Flanger
```c
Flanger* flanger_create(float max_delay_ms, float sample_rate);
void flanger_destroy(Flanger* flanger);
void flanger_set_params(Flanger* flanger, float rate, float depth, float feedback, float manual, float wet_level);
sample_t flanger_process(Flanger* flanger, sample_t input);
```

### Phaser
```c
Phaser* phaser_create(int num_stages, float sample_rate);
void phaser_destroy(Phaser* phaser);
void phaser_set_params(Phaser* phaser, float rate, float depth, float feedback, float wet_level);
sample_t phaser_process(Phaser* phaser, sample_t input);
```

### Tremolo
```c
Tremolo* tremolo_create(float sample_rate);
void tremolo_destroy(Tremolo* tremolo);
void tremolo_set_params(Tremolo* tremolo, float rate, float depth, int stereo_phase);
sample_t tremolo_process(Tremolo* tremolo, sample_t input);
void tremolo_process_stereo(Tremolo* tremolo, sample_t* left, sample_t* right);
```

## Utility Functions

### Sample Conversion
```c
float db_to_linear(float db);
float linear_to_db(float linear);
float clamp(float value, float min, float max);
float lerp(float a, float b, float t);
int16_t float_to_int16(float sample);
float int16_to_float(int16_t sample);
```

### Waveshaping
```c
sample_t hard_clip(sample_t input, float threshold);
sample_t soft_clip(sample_t input, float amount);
sample_t tube_saturation(sample_t input, float drive, float bias);
sample_t cubic_distortion(sample_t input, float amount);
sample_t sigmoid_distortion(sample_t input, float drive);
```

## Data Structures

### AudioBuffer
```c
typedef struct {
    sample_t* data;      // Audio samples
    size_t length;       // Number of sample frames
    size_t channels;     // Number of audio channels  
    size_t sample_rate;  // Sample rate in Hz
    size_t capacity;     // Total allocated samples
} AudioBuffer;
```

### BiquadFilter
```c
typedef struct {
    float b0, b1, b2;    // Feedforward coefficients
    float a1, a2;        // Feedback coefficients
    float x1, x2;        // Input history
    float y1, y2;        // Output history
} BiquadFilter;
```