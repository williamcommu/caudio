#include "audio_mixer_gui.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

// External GUI functions (from simple_gui.c)
extern int gui_init(void);
extern void gui_shutdown(void);
extern int gui_render_frame(AudioMixer* mixer);

int main(int argc, char** argv) {
    printf("Audio Effects Mixer - GUI Application\n");
    printf("=====================================\n\n");
    
    // Initialize audio mixer
    AudioMixer mixer;
    mixer_init(&mixer);
    
    // Enable auto-processing for real-time feedback
    mixer.auto_process = 1;
    
    // If audio file provided via command line, load it
    if (argc > 1) {
        printf("Loading audio file: %s\n", argv[1]);
        if (mixer_load_audio(&mixer, argv[1])) {
            printf("Audio loaded successfully\n");
        } else {
            printf("Failed to load audio file\n");
        }
    } else {
        printf("Usage: %s [audio_file.wav]\n", argv[0]);
        printf("You can also load files using the GUI\n\n");
    }
    
    // Initialize GUI
    if (!gui_init()) {
        fprintf(stderr, "Failed to initialize GUI\n");
        mixer_cleanup(&mixer);
        return 1;
    }
    
    printf("GUI initialized successfully\n");
    printf("Controls:\n");
    printf("- Load Audio: Load a WAV file for processing\n");
    printf("- Effect buttons: Add effects to the chain\n");
    printf("- Sliders: Adjust effect parameters\n");
    printf("- Process: Apply all effects to the audio\n");
    printf("- Save Audio: Save the processed audio\n");
    printf("- ESC: Exit application\n\n");
    
    // Main GUI loop
    while (1) {
        if (!gui_render_frame(&mixer)) {
            break; // Exit requested
        }
        
        // Small delay to prevent high CPU usage
        SDL_Delay(16); // ~60 FPS
    }
    
    printf("Shutting down...\n");
    
    // Cleanup
    gui_shutdown();
    mixer_cleanup(&mixer);
    
    printf("Audio Effects Mixer closed successfully\n");
    return 0;
}