# Audio Effects Library

A comprehensive, from-scratch implementation of audio effects in C with minimal dependencies. Built for maximum efficiency and educational value.

## Features

### Core Audio Processing
- **Audio Buffer Management**: Efficient memory management for audio data
- **WAV File I/O**: Read and write 16-bit WAV files
- **Sample Rate Conversion**: Support for various sample rates
- **Real-time Processing**: Optimized for low-latency audio processing

### Filter Effects
- **Biquad Filters**: Lowpass, highpass, bandpass, notch filters
- **One-pole Filters**: Simple and efficient filtering
- **4-Band EQ**: Professional-grade parametric equalizer
- **Custom Filter Design**: Easy-to-use filter coefficient calculation

### Delay Effects
- **Echo/Delay**: Classic delay with feedback and filtering
- **Multi-tap Delay**: Complex rhythmic delays with multiple taps
- **Ping-pong Delay**: Stereo delay with cross-feedback
- **Interpolated Delay Lines**: Smooth fractional delay times

### Reverb Effects
- **Schroeder Reverb**: Classic algorithmic reverb with comb and allpass filters
- **Plate Reverb**: Emulation of mechanical plate reverb
- **Freeverb**: Popular open-source reverb algorithm
- **Adjustable Parameters**: Room size, damping, decay time

### Distortion Effects
- **Overdrive**: Smooth tube-style overdrive with multi-stage clipping
- **Tube Distortion**: Asymmetric tube saturation with bias control
- **Fuzz Distortion**: Extreme distortion with gating and bit-crushing
- **Waveshaping Functions**: Various mathematical distortion curves

### Modulation Effects
- **Chorus**: Rich chorus effect with adjustable depth and rate
- **Flanger**: Classic flanging with feedback and manual control
- **Phaser**: Multi-stage phaser with LFO modulation
- **Tremolo**: Amplitude modulation with stereo capabilities
- **Vibrato**: Pitch modulation using delay-based techniques
- **Auto-wah**: Envelope-following and LFO-driven filter sweep

## Quick Start

### Prerequisites
- GCC compiler (or any C99-compatible compiler)
- Make utility
- Linux, macOS, or Windows with MinGW

### Building

```bash
# Clone and enter the directory
cd audio

# Build the demo application
make

# Or build optimized release version
make release

# Build static library for your projects
make library
```

### Running the Demo

```bash
# Interactive demo with menu
make run

# Run all demos automatically
make demo

# Test specific effect categories
make test-filters
make test-delays
make test-reverbs
make test-distortion
make test-modulation
make test-chain
```

## Project Structure

```
audio/
├── audio_core.h/c           # Core audio structures and utilities
├── wav_io.h/c               # WAV file input/output
├── audio_filters.h/c        # Filter implementations
├── delay_effects.h/c        # Delay and echo effects
├── reverb.h/c               # Reverb algorithms
├── distortion.h/c           # Distortion effects
├── modulation_effects.h/c   # Modulation effects
├── audio_effects_demo.c     # Demo application
├── Makefile                 # Build system
└── README.md               # This file
```

## Usage Examples

### Basic Audio Processing

```c
#include "audio_core.h"
#include "wav_io.h"

// Load audio file
AudioBuffer* buffer = wav_load("input.wav");

// Process audio...

// Save processed audio
wav_save("output.wav", buffer);

// Cleanup
audio_buffer_destroy(buffer);
```

### Applying Effects

```c
#include "distortion.h"
#include "reverb.h"

// Create effects
Overdrive* overdrive = overdrive_create(44100.0f);
SchroederReverb* reverb = schroeder_reverb_create(44100.0f);

// Set parameters
overdrive_set_params(overdrive, 6.0f, 0.8f, 0.7f, 1.0f);
schroeder_reverb_set_params(reverb, 0.7f, 0.5f, 0.3f);

// Process audio buffer
overdrive_process_buffer(overdrive, buffer);
schroeder_reverb_process_buffer(reverb, buffer);

// Cleanup
overdrive_destroy(overdrive);
schroeder_reverb_destroy(reverb);
```

### Real-time Processing

```c
// Process single samples in real-time
for (size_t i = 0; i < buffer_size; i++) {
    float sample = input_buffer[i];
    
    sample = overdrive_process(overdrive, sample);
    sample = chorus_process(chorus, sample);
    sample = reverb_process(reverb, sample);
    
    output_buffer[i] = sample;
}
```

## Effect Parameters

### Distortion Effects
- **Drive**: Input gain/distortion amount (1.0 - 20.0)
- **Output Gain**: Level compensation (0.1 - 2.0)
- **Mix**: Wet/dry balance (0.0 - 1.0)

### Delay Effects
- **Delay Time**: Echo delay in seconds
- **Feedback**: Amount of signal fed back (0.0 - 0.95)
- **Wet Level**: Effect level (0.0 - 1.0)

### Reverb Effects
- **Room Size**: Virtual room size (0.0 - 1.0)
- **Damping**: High-frequency damping (0.0 - 1.0)
- **Decay Time**: Reverb tail length in seconds

### Modulation Effects
- **Rate**: LFO frequency in Hz (0.01 - 20.0)
- **Depth**: Modulation intensity (0.0 - 1.0)
- **Feedback**: For flangers/chorus (0.0 - 0.9)

## Architecture

### Design Principles
1. **Minimal Dependencies**: Only standard C library and math library
2. **Real-time Safe**: No memory allocation in processing functions
3. **Modular Design**: Each effect is self-contained
4. **Efficient Processing**: Optimized algorithms for low CPU usage
5. **Educational Code**: Clear, well-documented implementations

### Memory Management
- All effect structures use pre-allocated buffers
- No dynamic allocation during audio processing
- Clean initialization and cleanup functions
- Safe buffer bounds checking

### Audio Flow
```
Input → Filters → Distortion → Modulation → Delay → Reverb → Output
```

## Testing

The demo application generates various test signals and processes them through each effect category:

- **Sine waves** for filter testing
- **Frequency sweeps** for frequency response analysis
- **Impulse responses** for reverb characteristics
- **Chord progressions** for modulation effects
- **Guitar-like signals** for distortion testing

## Audio Quality

All effects operate at:
- **Sample rates**: 22.05kHz - 192kHz (optimized for 44.1kHz)
- **Bit depth**: Internal 32-bit float processing
- **Channels**: Mono and stereo support
- **Latency**: Minimal buffering for real-time use

## Learning Resources

This library serves as an excellent learning resource for:
- Digital Signal Processing (DSP) fundamentals
- Audio effect algorithms and implementation
- Real-time audio programming techniques
- C programming for audio applications

Each effect includes detailed comments explaining the mathematics and theory behind the implementation.

## Contributing

This is an educational project demonstrating audio effects from scratch. Feel free to:
- Study the implementations
- Modify parameters and algorithms
- Add new effects using the existing patterns
- Optimize for specific use cases

## License

This project is created for educational purposes. The implementations are based on well-known DSP algorithms and techniques available in academic literature.

## Acknowledgments

The algorithms implemented here are based on research and techniques developed by:
- M.R. Schroeder (Schroeder Reverb)
- Jezar at Dreampoint (Freeverb)
- Various DSP researchers and practitioners in the audio community

---

**Built for audio enthusiasts and DSP learners**