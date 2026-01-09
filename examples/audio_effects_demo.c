#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "audio_core.h"
#include "wav_io.h"
#include "audio_filters.h"
#include "delay_effects.h"
#include "reverb.h"
#include "distortion.h"
#include "modulation_effects.h"

// Demo functions
void generate_test_tone(AudioBuffer* buffer, float frequency, float duration, float sample_rate);
void generate_white_noise(AudioBuffer* buffer, float duration, float sample_rate);
void generate_sweep(AudioBuffer* buffer, float start_freq, float end_freq, float duration, float sample_rate);

void demo_filters(void);
void demo_delay_effects(void);
void demo_reverb_effects(void);
void demo_distortion_effects(void);
void demo_modulation_effects(void);
void demo_effect_chain(void);

void print_menu(void);
void print_separator(void);

int main(int argc, char* argv[]) {
    printf("=== Audio Effects Library Demo ===\n");
    printf("Built from scratch in C with minimal dependencies\n\n");
    
    if (argc > 1) {
        // Process command line arguments for batch processing
        if (strcmp(argv[1], "--all") == 0) {
            printf("Running all demos...\n\n");
            demo_filters();
            demo_delay_effects();
            demo_reverb_effects();
            demo_distortion_effects();
            demo_modulation_effects();
            demo_effect_chain();
            return 0;
        }
    }
    
    int choice;
    do {
        print_menu();
        printf("Enter your choice: ");
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        
        switch (choice) {
            case 1:
                demo_filters();
                break;
            case 2:
                demo_delay_effects();
                break;
            case 3:
                demo_reverb_effects();
                break;
            case 4:
                demo_distortion_effects();
                break;
            case 5:
                demo_modulation_effects();
                break;
            case 6:
                demo_effect_chain();
                break;
            case 0:
                printf("Goodbye!\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
        
        if (choice != 0) {
            printf("\nPress Enter to continue...");
            getchar();
            getchar(); // Clear the newline
        }
        
    } while (choice != 0);
    
    return 0;
}

void print_menu(void) {
    print_separator();
    printf("AUDIO EFFECTS DEMO MENU\n");
    print_separator();
    printf("1. Filter Effects (Lowpass, Highpass, EQ)\n");
    printf("2. Delay Effects (Echo, Multi-tap, Ping-pong)\n");
    printf("3. Reverb Effects (Schroeder, Plate, Freeverb)\n");
    printf("4. Distortion Effects (Overdrive, Fuzz, Tube)\n");
    printf("5. Modulation Effects (Chorus, Flanger, Phaser)\n");
    printf("6. Effect Chain Demo\n");
    printf("0. Exit\n");
    print_separator();
}

void print_separator(void) {
    printf("================================================\n");
}

// Generate a test tone
void generate_test_tone(AudioBuffer* buffer, float frequency, float duration, float sample_rate) {
    if (!buffer || !buffer->data) return;
    
    size_t num_samples = (size_t)(duration * sample_rate);
    if (num_samples > buffer->length) num_samples = buffer->length;
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = (float)i / sample_rate;
        float sample = 0.3f * sinf(TWO_PI * frequency * t);
        
        // Apply envelope to avoid clicks
        float envelope = 1.0f;
        float fade_time = 0.01f; // 10ms fade
        size_t fade_samples = (size_t)(fade_time * sample_rate);
        
        if (i < fade_samples) {
            envelope = (float)i / fade_samples;
        } else if (i > num_samples - fade_samples) {
            envelope = (float)(num_samples - i) / fade_samples;
        }
        
        sample *= envelope;
        
        // For stereo, duplicate to both channels
        for (size_t ch = 0; ch < buffer->channels; ch++) {
            buffer->data[i * buffer->channels + ch] = sample;
        }
    }
}

// Generate white noise
void generate_white_noise(AudioBuffer* buffer, float duration, float sample_rate) {
    if (!buffer || !buffer->data) return;
    
    size_t num_samples = (size_t)(duration * sample_rate);
    if (num_samples > buffer->length) num_samples = buffer->length;
    
    srand((unsigned int)time(NULL));
    
    for (size_t i = 0; i < num_samples; i++) {
        float sample = 0.1f * ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        
        for (size_t ch = 0; ch < buffer->channels; ch++) {
            buffer->data[i * buffer->channels + ch] = sample;
        }
    }
}

// Generate frequency sweep
void generate_sweep(AudioBuffer* buffer, float start_freq, float end_freq, float duration, float sample_rate) {
    if (!buffer || !buffer->data) return;
    
    size_t num_samples = (size_t)(duration * sample_rate);
    if (num_samples > buffer->length) num_samples = buffer->length;
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = (float)i / sample_rate;
        float progress = t / duration;
        float frequency = start_freq + progress * (end_freq - start_freq);
        
        float sample = 0.3f * sinf(TWO_PI * frequency * t);
        
        for (size_t ch = 0; ch < buffer->channels; ch++) {
            buffer->data[i * buffer->channels + ch] = sample;
        }
    }
}

void demo_filters(void) {
    printf("\n=== FILTER EFFECTS DEMO ===\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 3.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate test sweep
    printf("Generating frequency sweep (100Hz - 4000Hz)...\n");
    generate_sweep(buffer, 100.0f, 4000.0f, duration, sample_rate);
    wav_save("original_sweep.wav", buffer);
    
    // Lowpass filter demo
    printf("Applying lowpass filter (1000Hz cutoff)...\n");
    AudioBuffer* filtered = audio_buffer_create(buffer->length, buffer->channels, buffer->sample_rate);
    audio_buffer_copy(filtered, buffer);
    
    BiquadFilter lowpass;
    biquad_lowpass(&lowpass, 1000.0f, 0.7f, sample_rate);
    biquad_process_buffer(&lowpass, filtered);
    wav_save("lowpass_filtered.wav", filtered);
    
    // Highpass filter demo
    printf("Applying highpass filter (1000Hz cutoff)...\n");
    audio_buffer_copy(filtered, buffer);
    
    BiquadFilter highpass;
    biquad_highpass(&highpass, 1000.0f, 0.7f, sample_rate);
    biquad_process_buffer(&highpass, filtered);
    wav_save("highpass_filtered.wav", filtered);
    
    // EQ demo
    printf("Applying 4-band EQ (bass boost, treble cut)...\n");
    audio_buffer_copy(filtered, buffer);
    
    FourBandEQ eq;
    eq_init(&eq, sample_rate);
    eq_set_gains(&eq, 6.0f, 0.0f, -6.0f); // Bass +6dB, mid 0dB, treble -6dB
    eq_process_buffer(&eq, filtered);
    wav_save("eq_filtered.wav", filtered);
    
    printf("Filter demo complete! Generated files:\n");
    printf("  - original_sweep.wav\n");
    printf("  - lowpass_filtered.wav\n");
    printf("  - highpass_filtered.wav\n");
    printf("  - eq_filtered.wav\n");
    
    audio_buffer_destroy(buffer);
    audio_buffer_destroy(filtered);
}

void demo_delay_effects(void) {
    printf("\n=== DELAY EFFECTS DEMO ===\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 4.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate test tone with gaps
    printf("Generating test pattern...\n");
    for (int i = 0; i < 4; i++) {
        AudioBuffer* tone = audio_buffer_create((size_t)(0.5f * sample_rate), 1, (size_t)sample_rate);
        generate_test_tone(tone, 440.0f + i * 110.0f, 0.5f, sample_rate);
        
        // Copy tone to main buffer with gaps
        size_t start_pos = i * (size_t)(sample_rate); // 1-second intervals
        size_t copy_length = tone->capacity < (buffer->capacity - start_pos) ? tone->capacity : (buffer->capacity - start_pos);
        
        for (size_t j = 0; j < copy_length; j++) {
            buffer->data[start_pos + j] = tone->data[j];
        }
        
        audio_buffer_destroy(tone);
    }
    wav_save("delay_original.wav", buffer);
    
    // Echo demo
    printf("Applying echo effect...\n");
    AudioBuffer* processed = audio_buffer_create(buffer->length, buffer->channels, buffer->sample_rate);
    audio_buffer_copy(processed, buffer);
    
    Echo* echo = echo_create(2.0f, sample_rate);
    echo_set_params(echo, 0.3f, 0.4f, 0.5f, sample_rate);
    echo_process_buffer(echo, processed);
    wav_save("echo_processed.wav", processed);
    echo_destroy(echo);
    
    // Multi-tap delay demo
    printf("Applying multi-tap delay...\n");
    audio_buffer_copy(processed, buffer);
    
    MultiTapDelay* multitap = multitap_create(2.0f, sample_rate);
    multitap_set_tap(multitap, 0, 0.1f, 0.6f, sample_rate);
    multitap_set_tap(multitap, 1, 0.25f, 0.4f, sample_rate);
    multitap_set_tap(multitap, 2, 0.4f, 0.3f, sample_rate);
    multitap_set_feedback(multitap, 0.2f, 0.6f);
    multitap_process_buffer(multitap, processed);
    wav_save("multitap_processed.wav", processed);
    multitap_destroy(multitap);
    
    printf("Delay effects demo complete! Generated files:\n");
    printf("  - delay_original.wav\n");
    printf("  - echo_processed.wav\n");
    printf("  - multitap_processed.wav\n");
    
    audio_buffer_destroy(buffer);
    audio_buffer_destroy(processed);
}

void demo_reverb_effects(void) {
    printf("\n=== REVERB EFFECTS DEMO ===\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 3.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate impulse-like sound
    printf("Generating percussive test sound...\n");
    generate_test_tone(buffer, 220.0f, 0.1f, sample_rate);
    // Add some noise burst
    for (size_t i = 0; i < (size_t)(0.05f * sample_rate); i++) {
        float noise = 0.2f * ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        buffer->data[i] += noise * expf(-10.0f * i / sample_rate); // Decaying noise
    }
    wav_save("reverb_original.wav", buffer);
    
    AudioBuffer* processed = audio_buffer_create(buffer->length, buffer->channels, buffer->sample_rate);
    
    // Schroeder reverb demo
    printf("Applying Schroeder reverb...\n");
    audio_buffer_copy(processed, buffer);
    
    SchroederReverb* schroeder = schroeder_reverb_create(sample_rate);
    schroeder_reverb_set_params(schroeder, 0.7f, 0.5f, 0.4f);
    schroeder_reverb_process_buffer(schroeder, processed);
    wav_save("schroeder_reverb.wav", processed);
    schroeder_reverb_destroy(schroeder);
    
    // Plate reverb demo
    printf("Applying plate reverb...\n");
    audio_buffer_copy(processed, buffer);
    
    PlateReverb* plate = plate_reverb_create(sample_rate);
    plate_reverb_set_params(plate, 3.0f, 0.4f, 0.02f, sample_rate);
    plate_reverb_process_buffer(plate, processed);
    wav_save("plate_reverb.wav", processed);
    plate_reverb_destroy(plate);
    
    // Freeverb demo
    printf("Applying Freeverb...\n");
    audio_buffer_copy(processed, buffer);
    
    Freeverb* freeverb = freeverb_create(sample_rate);
    freeverb_set_params(freeverb, 0.8f, 0.4f, 0.3f, 1.0f);
    freeverb_process_buffer(freeverb, processed);
    wav_save("freeverb_processed.wav", processed);
    freeverb_destroy(freeverb);
    
    printf("Reverb effects demo complete! Generated files:\n");
    printf("  - reverb_original.wav\n");
    printf("  - schroeder_reverb.wav\n");
    printf("  - plate_reverb.wav\n");
    printf("  - freeverb_processed.wav\n");
    
    audio_buffer_destroy(buffer);
    audio_buffer_destroy(processed);
}

void demo_distortion_effects(void) {
    printf("\n=== DISTORTION EFFECTS DEMO ===\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 3.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate guitar-like test signal
    printf("Generating guitar-like test signal...\n");
    for (size_t i = 0; i < buffer->capacity; i++) {
        float t = (float)i / sample_rate;
        // Fundamental + harmonics for richer sound
        float sample = 0.5f * sinf(TWO_PI * 220.0f * t);
        sample += 0.3f * sinf(TWO_PI * 440.0f * t);
        sample += 0.2f * sinf(TWO_PI * 660.0f * t);
        
        // Add some amplitude modulation
        sample *= (1.0f + 0.1f * sinf(TWO_PI * 4.0f * t));
        
        // Apply envelope
        float envelope = expf(-t * 0.5f);
        buffer->data[i] = sample * envelope * 0.7f;
    }
    wav_save("distortion_original.wav", buffer);
    
    AudioBuffer* processed = audio_buffer_create(buffer->length, buffer->channels, buffer->sample_rate);
    
    // Overdrive demo
    printf("Applying overdrive...\n");
    audio_buffer_copy(processed, buffer);
    
    Overdrive* overdrive = overdrive_create(sample_rate);
    overdrive_set_params(overdrive, 6.0f, 0.7f, 0.8f, 1.0f);
    overdrive_process_buffer(overdrive, processed);
    wav_save("overdrive_processed.wav", processed);
    overdrive_destroy(overdrive);
    
    // Tube distortion demo
    printf("Applying tube distortion...\n");
    audio_buffer_copy(processed, buffer);
    
    TubeDistortion* tube = tube_distortion_create(sample_rate);
    tube_distortion_set_params(tube, 5.0f, 0.15f, 0.7f, 1.0f);
    tube_distortion_process_buffer(tube, processed);
    wav_save("tube_distortion.wav", processed);
    tube_distortion_destroy(tube);
    
    // Fuzz demo
    printf("Applying fuzz distortion...\n");
    audio_buffer_copy(processed, buffer);
    
    FuzzDistortion* fuzz = fuzz_distortion_create(sample_rate);
    fuzz_distortion_set_params(fuzz, 12.0f, 0.02f, 0.4f, 1.0f);
    fuzz_distortion_process_buffer(fuzz, processed);
    wav_save("fuzz_distortion.wav", processed);
    fuzz_distortion_destroy(fuzz);
    
    printf("Distortion effects demo complete! Generated files:\n");
    printf("  - distortion_original.wav\n");
    printf("  - overdrive_processed.wav\n");
    printf("  - tube_distortion.wav\n");
    printf("  - fuzz_distortion.wav\n");
    
    audio_buffer_destroy(buffer);
    audio_buffer_destroy(processed);
}

void demo_modulation_effects(void) {
    printf("\n=== MODULATION EFFECTS DEMO ===\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 4.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate sustained chord
    printf("Generating sustained chord...\n");
    for (size_t i = 0; i < buffer->capacity; i++) {
        float t = (float)i / sample_rate;
        float sample = 0.3f * (sinf(TWO_PI * 261.63f * t) +    // C
                              sinf(TWO_PI * 329.63f * t) +    // E
                              sinf(TWO_PI * 392.00f * t));    // G
        
        // Gentle envelope
        float envelope = (t < 0.1f) ? t / 0.1f : 1.0f;
        if (t > duration - 0.1f) envelope = (duration - t) / 0.1f;
        
        buffer->data[i] = sample * envelope;
    }
    wav_save("modulation_original.wav", buffer);
    
    AudioBuffer* processed = audio_buffer_create(buffer->length, buffer->channels, buffer->sample_rate);
    
    // Chorus demo
    printf("Applying chorus effect...\n");
    audio_buffer_copy(processed, buffer);
    
    Chorus* chorus = chorus_create(50.0f, sample_rate);
    chorus_set_params(chorus, 1.2f, 0.6f, 0.15f, 0.4f);
    chorus_process_buffer(chorus, processed);
    wav_save("chorus_processed.wav", processed);
    chorus_destroy(chorus);
    
    // Flanger demo
    printf("Applying flanger effect...\n");
    audio_buffer_copy(processed, buffer);
    
    Flanger* flanger = flanger_create(20.0f, sample_rate);
    flanger_set_params(flanger, 0.3f, 0.8f, 0.6f, 0.5f, 0.5f);
    flanger_process_buffer(flanger, processed);
    wav_save("flanger_processed.wav", processed);
    flanger_destroy(flanger);
    
    // Phaser demo
    printf("Applying phaser effect...\n");
    audio_buffer_copy(processed, buffer);
    
    Phaser* phaser = phaser_create(4, sample_rate);
    phaser_set_params(phaser, 0.5f, 0.7f, 0.3f, 0.4f);
    phaser_process_buffer(phaser, processed);
    wav_save("phaser_processed.wav", processed);
    phaser_destroy(phaser);
    
    // Tremolo demo
    printf("Applying tremolo effect...\n");
    audio_buffer_copy(processed, buffer);
    
    Tremolo* tremolo = tremolo_create(sample_rate);
    tremolo_set_params(tremolo, 6.0f, 0.8f, 0);
    
    for (size_t i = 0; i < processed->capacity; i++) {
        processed->data[i] = tremolo_process(tremolo, processed->data[i]);
    }
    wav_save("tremolo_processed.wav", processed);
    tremolo_destroy(tremolo);
    
    printf("Modulation effects demo complete! Generated files:\n");
    printf("  - modulation_original.wav\n");
    printf("  - chorus_processed.wav\n");
    printf("  - flanger_processed.wav\n");
    printf("  - phaser_processed.wav\n");
    printf("  - tremolo_processed.wav\n");
    
    audio_buffer_destroy(buffer);
    audio_buffer_destroy(processed);
}

void demo_effect_chain(void) {
    printf("\n=== EFFECT CHAIN DEMO ===\n");
    printf("Demonstrating multiple effects in series...\n");
    
    const float sample_rate = 44100.0f;
    const float duration = 6.0f;
    
    AudioBuffer* buffer = audio_buffer_create((size_t)(duration * sample_rate), 1, (size_t)sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        return;
    }
    
    // Generate riff-like pattern
    printf("Generating guitar riff pattern...\n");
    float notes[] = {220.0f, 246.94f, 261.63f, 293.66f, 329.63f, 369.99f};
    int num_notes = sizeof(notes) / sizeof(notes[0]);
    float note_duration = duration / num_notes;
    
    for (int note = 0; note < num_notes; note++) {
        size_t start_sample = (size_t)(note * note_duration * sample_rate);
        size_t note_samples = (size_t)(note_duration * sample_rate);
        
        for (size_t i = 0; i < note_samples && (start_sample + i) < buffer->capacity; i++) {
            float t = (float)i / sample_rate;
            float sample = 0.6f * sinf(TWO_PI * notes[note] * t);
            sample += 0.3f * sinf(TWO_PI * notes[note] * 2.0f * t); // Octave
            
            // Note envelope
            float env = expf(-t * 2.0f);
            buffer->data[start_sample + i] = sample * env;
        }
    }
    wav_save("chain_original.wav", buffer);
    
    printf("Applying effect chain: Overdrive -> Chorus -> Delay -> Reverb...\n");
    
    // Step 1: Overdrive
    Overdrive* overdrive = overdrive_create(sample_rate);
    overdrive_set_params(overdrive, 4.0f, 0.6f, 0.9f, 1.0f);
    overdrive_process_buffer(overdrive, buffer);
    wav_save("chain_step1_overdrive.wav", buffer);
    
    // Step 2: Chorus
    Chorus* chorus = chorus_create(30.0f, sample_rate);
    chorus_set_params(chorus, 1.0f, 0.4f, 0.1f, 0.3f);
    chorus_process_buffer(chorus, buffer);
    wav_save("chain_step2_chorus.wav", buffer);
    
    // Step 3: Echo
    Echo* echo = echo_create(1.0f, sample_rate);
    echo_set_params(echo, 0.25f, 0.3f, 0.3f, sample_rate);
    echo_process_buffer(echo, buffer);
    wav_save("chain_step3_echo.wav", buffer);
    
    // Step 4: Reverb
    SchroederReverb* reverb = schroeder_reverb_create(sample_rate);
    schroeder_reverb_set_params(reverb, 0.6f, 0.3f, 0.25f);
    schroeder_reverb_process_buffer(reverb, buffer);
    wav_save("chain_final.wav", buffer);
    
    printf("Effect chain demo complete! Generated files:\n");
    printf("  - chain_original.wav (dry signal)\n");
    printf("  - chain_step1_overdrive.wav\n");
    printf("  - chain_step2_chorus.wav\n");
    printf("  - chain_step3_echo.wav\n");
    printf("  - chain_final.wav (full chain)\n");
    
    // Cleanup
    overdrive_destroy(overdrive);
    chorus_destroy(chorus);
    echo_destroy(echo);
    schroeder_reverb_destroy(reverb);
    audio_buffer_destroy(buffer);
}