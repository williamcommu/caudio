// Simple example: Apply reverb to an audio file
// Compile with: gcc -Iinclude -o simple_reverb simple_reverb.c src/*.c -lm

#include "audio_core.h"
#include "wav_io.h"
#include "reverb.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.wav output.wav\n", argv[0]);
        return 1;
    }
    
    // Load input audio file
    AudioBuffer* buffer = wav_load(argv[1]);
    if (!buffer) {
        printf("Error: Could not load %s\n", argv[1]);
        return 1;
    }
    
    // Create reverb effect
    SchroederReverb* reverb = schroeder_reverb_create((float)buffer->sample_rate);
    if (!reverb) {
        printf("Error: Could not create reverb\n");
        audio_buffer_destroy(buffer);
        return 1;
    }
    
    // Set reverb parameters (room_size, damping, wet_level)
    schroeder_reverb_set_params(reverb, 0.8f, 0.3f, 0.4f);
    
    // Process audio
    printf("Processing audio with reverb...\n");
    schroeder_reverb_process_buffer(reverb, buffer);
    
    // Save output
    if (wav_save(argv[2], buffer)) {
        printf("Reverb applied successfully! Output saved to %s\n", argv[2]);
    } else {
        printf("Error: Could not save output file\n");
    }
    
    // Cleanup
    schroeder_reverb_destroy(reverb);
    audio_buffer_destroy(buffer);
    
    return 0;
}