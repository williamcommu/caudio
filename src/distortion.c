#include "distortion.h"

// Waveshaping functions

// Hard clipping
sample_t hard_clip(sample_t input, float threshold) {
    if (input > threshold) return threshold;
    if (input < -threshold) return -threshold;
    return input;
}

// Soft clipping (tanh-based)
sample_t soft_clip(sample_t input, float amount) {
    return tanhf(input * amount) / amount;
}

// Tube-style saturation with bias
sample_t tube_saturation(sample_t input, float drive, float bias) {
    sample_t biased = input + bias;
    sample_t driven = biased * drive;
    
    // Asymmetric clipping characteristic
    if (driven > 0.0f) {
        return tanhf(driven * 2.0f) * 0.7f;
    } else {
        return tanhf(driven * 1.5f) * 0.8f;
    }
}

// Cubic distortion
sample_t cubic_distortion(sample_t input, float amount) {
    sample_t x = input * amount;
    if (fabsf(x) < 1.0f) {
        return x - (x * x * x) / 3.0f;
    } else {
        return (x > 0.0f) ? 2.0f/3.0f : -2.0f/3.0f;
    }
}

// Sigmoid distortion
sample_t sigmoid_distortion(sample_t input, float drive) {
    sample_t x = input * drive;
    return (2.0f / (1.0f + expf(-x))) - 1.0f;
}

// Basic distortion functions

// Create basic distortion effect
Distortion* distortion_create(DistortionType type, float sample_rate) {
    Distortion* dist = malloc(sizeof(Distortion));
    if (!dist) return NULL;
    
    dist->type = type;
    dist->drive = 5.0f;
    dist->output_gain = 0.5f;
    dist->mix = 1.0f;
    dist->sample_rate = sample_rate;
    
    // Setup pre and post filters
    biquad_highpass(&dist->pre_filter, 80.0f, 0.7f, sample_rate);
    biquad_lowpass(&dist->post_filter, 8000.0f, 0.7f, sample_rate);
    
    return dist;
}

// Destroy basic distortion
void distortion_destroy(Distortion* dist) {
    if (dist) {
        free(dist);
    }
}

// Set distortion parameters
void distortion_set_params(Distortion* dist, float drive, float output_gain, float mix) {
    if (!dist) return;
    
    dist->drive = clamp(drive, 1.0f, 20.0f);
    dist->output_gain = clamp(output_gain, 0.1f, 2.0f);
    dist->mix = clamp(mix, 0.0f, 1.0f);
}

// Process one sample through basic distortion
sample_t distortion_process(Distortion* dist, sample_t input) {
    if (!dist) return input;
    
    // Pre-filtering
    sample_t filtered = biquad_process(&dist->pre_filter, input);
    
    sample_t distorted;
    
    switch (dist->type) {
        case DISTORTION_HARD_CLIP:
            distorted = hard_clip(filtered * dist->drive, 0.8f);
            break;
        case DISTORTION_SOFT_CLIP:
            distorted = soft_clip(filtered, dist->drive);
            break;
        case DISTORTION_TUBE:
            distorted = tube_saturation(filtered, dist->drive, 0.1f);
            break;
        case DISTORTION_FUZZ:
            distorted = sigmoid_distortion(filtered, dist->drive);
            break;
        case DISTORTION_OVERDRIVE:
            distorted = cubic_distortion(filtered, dist->drive);
            break;
        default:
            distorted = soft_clip(filtered, dist->drive);
            break;
    }
    
    // Post-filtering and output gain
    distorted = biquad_process(&dist->post_filter, distorted) * dist->output_gain;
    
    // Mix wet and dry
    return lerp(input, distorted, dist->mix);
}

// Process buffer through basic distortion
void distortion_process_buffer(Distortion* dist, AudioBuffer* buffer) {
    if (!dist || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = distortion_process(dist, buffer->data[i]);
    }
}

// Tube distortion functions

// Create tube distortion
TubeDistortion* tube_distortion_create(float sample_rate) {
    TubeDistortion* tube = malloc(sizeof(TubeDistortion));
    if (!tube) return NULL;
    
    tube->drive = 3.0f;
    tube->bias = 0.1f;
    tube->output_gain = 0.5f;
    tube->mix = 1.0f;
    
    // Setup filters for tube character
    biquad_highpass(&tube->input_filter, 100.0f, 0.7f, sample_rate);
    biquad_lowpass(&tube->output_filter, 5000.0f, 1.5f, sample_rate);
    onepole_highpass(&tube->dc_blocker, 20.0f, sample_rate);
    
    return tube;
}

// Destroy tube distortion
void tube_distortion_destroy(TubeDistortion* tube) {
    if (tube) {
        free(tube);
    }
}

// Set tube distortion parameters
void tube_distortion_set_params(TubeDistortion* tube, float drive, float bias, float output_gain, float mix) {
    if (!tube) return;
    
    tube->drive = clamp(drive, 1.0f, 10.0f);
    tube->bias = clamp(bias, -0.5f, 0.5f);
    tube->output_gain = clamp(output_gain, 0.1f, 2.0f);
    tube->mix = clamp(mix, 0.0f, 1.0f);
}

// Process one sample through tube distortion
sample_t tube_distortion_process(TubeDistortion* tube, sample_t input) {
    if (!tube) return input;
    
    // Input filtering
    sample_t filtered = biquad_process(&tube->input_filter, input);
    
    // Tube saturation
    sample_t distorted = tube_saturation(filtered, tube->drive, tube->bias);
    
    // DC blocking and output filtering
    distorted = onepole_process(&tube->dc_blocker, distorted, 1);
    distorted = biquad_process(&tube->output_filter, distorted);
    
    // Apply output gain
    distorted *= tube->output_gain;
    
    // Mix
    return lerp(input, distorted, tube->mix);
}

// Process buffer through tube distortion
void tube_distortion_process_buffer(TubeDistortion* tube, AudioBuffer* buffer) {
    if (!tube || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = tube_distortion_process(tube, buffer->data[i]);
    }
}

// Fuzz distortion functions

// Create fuzz distortion
FuzzDistortion* fuzz_distortion_create(float sample_rate) {
    FuzzDistortion* fuzz = malloc(sizeof(FuzzDistortion));
    if (!fuzz) return NULL;
    
    fuzz->fuzz_amount = 8.0f;
    fuzz->gate_threshold = 0.01f;
    fuzz->output_gain = 0.3f;
    fuzz->mix = 1.0f;
    
    // Setup filters for fuzz character
    biquad_highpass(&fuzz->pre_emphasis, 1000.0f, 2.0f, sample_rate);
    biquad_lowpass(&fuzz->de_emphasis, 4000.0f, 0.7f, sample_rate);
    onepole_lowpass(&fuzz->gate_filter, 10.0f, sample_rate);
    
    return fuzz;
}

// Destroy fuzz distortion
void fuzz_distortion_destroy(FuzzDistortion* fuzz) {
    if (fuzz) {
        free(fuzz);
    }
}

// Set fuzz distortion parameters
void fuzz_distortion_set_params(FuzzDistortion* fuzz, float fuzz_amount, float gate_threshold, float output_gain, float mix) {
    if (!fuzz) return;
    
    fuzz->fuzz_amount = clamp(fuzz_amount, 1.0f, 20.0f);
    fuzz->gate_threshold = clamp(gate_threshold, 0.001f, 0.1f);
    fuzz->output_gain = clamp(output_gain, 0.1f, 1.0f);
    fuzz->mix = clamp(mix, 0.0f, 1.0f);
}

// Process one sample through fuzz distortion
sample_t fuzz_distortion_process(FuzzDistortion* fuzz, sample_t input) {
    if (!fuzz) return input;
    
    // Pre-emphasis
    sample_t emphasized = biquad_process(&fuzz->pre_emphasis, input);
    
    // Gate (noise gate for fuzz character)
    float gate_signal = onepole_process(&fuzz->gate_filter, fabsf(emphasized), 0);
    float gate_amount = (gate_signal > fuzz->gate_threshold) ? 1.0f : 0.0f;
    
    // Extreme clipping for fuzz
    sample_t fuzzed = emphasized * fuzz->fuzz_amount;
    fuzzed = hard_clip(fuzzed, 1.0f);
    
    // Add some chaos (bit crushing effect)
    fuzzed = floorf(fuzzed * 32.0f) / 32.0f;
    
    // Apply gate
    fuzzed *= gate_amount;
    
    // De-emphasis and output gain
    fuzzed = biquad_process(&fuzz->de_emphasis, fuzzed);
    fuzzed *= fuzz->output_gain;
    
    // Mix
    return lerp(input, fuzzed, fuzz->mix);
}

// Process buffer through fuzz distortion
void fuzz_distortion_process_buffer(FuzzDistortion* fuzz, AudioBuffer* buffer) {
    if (!fuzz || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = fuzz_distortion_process(fuzz, buffer->data[i]);
    }
}

// Overdrive functions

// Create overdrive effect
Overdrive* overdrive_create(float sample_rate) {
    Overdrive* overdrive = malloc(sizeof(Overdrive));
    if (!overdrive) return NULL;
    
    overdrive->drive = 4.0f;
    overdrive->tone = 0.5f;
    overdrive->output_gain = 0.6f;
    overdrive->mix = 1.0f;
    
    // Setup filters
    biquad_highpass(&overdrive->input_filter, 80.0f, 0.7f, sample_rate);
    biquad_bandpass(&overdrive->tone_filter, 2000.0f, 1.0f, sample_rate);
    biquad_lowpass(&overdrive->output_filter, 6000.0f, 0.7f, sample_rate);
    
    // Multi-stage gains
    overdrive->stage_gains[0] = 2.0f;
    overdrive->stage_gains[1] = 1.5f;
    overdrive->stage_gains[2] = 1.2f;
    
    return overdrive;
}

// Destroy overdrive
void overdrive_destroy(Overdrive* overdrive) {
    if (overdrive) {
        free(overdrive);
    }
}

// Set overdrive parameters
void overdrive_set_params(Overdrive* overdrive, float drive, float tone, float output_gain, float mix) {
    if (!overdrive) return;
    
    overdrive->drive = clamp(drive, 1.0f, 10.0f);
    overdrive->tone = clamp(tone, 0.0f, 1.0f);
    overdrive->output_gain = clamp(output_gain, 0.1f, 2.0f);
    overdrive->mix = clamp(mix, 0.0f, 1.0f);
}

// Process one sample through overdrive
sample_t overdrive_process(Overdrive* overdrive, sample_t input) {
    if (!overdrive) return input;
    
    // Input filtering
    sample_t signal = biquad_process(&overdrive->input_filter, input);
    
    // Multi-stage clipping
    for (int stage = 0; stage < 3; stage++) {
        signal = soft_clip(signal, overdrive->drive * overdrive->stage_gains[stage]);
        signal *= 0.7f; // Reduce level between stages
    }
    
    // Tone control (mix between full signal and mid-boosted signal)
    sample_t toned = biquad_process(&overdrive->tone_filter, signal);
    signal = lerp(signal, toned, overdrive->tone);
    
    // Output filtering and gain
    signal = biquad_process(&overdrive->output_filter, signal);
    signal *= overdrive->output_gain;
    
    // Mix
    return lerp(input, signal, overdrive->mix);
}

// Process buffer through overdrive
void overdrive_process_buffer(Overdrive* overdrive, AudioBuffer* buffer) {
    if (!overdrive || !buffer || !buffer->data) return;
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        buffer->data[i] = overdrive_process(overdrive, buffer->data[i]);
    }
}