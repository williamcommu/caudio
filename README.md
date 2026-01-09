# Audio Effects Library

i like audio, i like coding, i like coding audio

## Project Structure

```
audio/
├── src/                     # Source files (.c)
├── include/                 # Header files (.h) 
├── examples/                # Demo applications
├── build/                   # Build artifacts (auto-generated)
├── audio_samples/           # Generated WAV files (auto-generated)
├── docs/                    # Documentation
├── Makefile                 # Build system
└── README.md               # This file
```

## Quick Start

```bash
# Build the library and demo
make

# Run interactive demo
make run

# Run all demos automatically  
make demo

# Clean everything
make clean
```

## Available Effects

### Filter Effects
- Biquad filters (lowpass, highpass, bandpass, notch)
- 4-band parametric EQ
- One-pole filters

### Delay Effects  
- Echo with feedback
- Multi-tap delay
- Ping-pong stereo delay

### Reverb Effects
- Schroeder reverb
- Plate reverb  
- Freeverb algorithm

### Distortion Effects
- Overdrive
- Tube distortion
- Fuzz distortion

### Modulation Effects
- Chorus
- Flanger  
- Phaser
- Tremolo
- Vibrato
- Auto-wah

## Documentation

- [Detailed Documentation](docs/detailed_documentation.md) - Complete API reference and usage examples
- [Examples](examples/) - Demo applications showing how to use each effect
- [Audio Samples](audio_samples/) - Generated test files (created after running demos)

## Build Targets

```bash
make              # Build demo application
make library      # Build static library
make release      # Optimized build
make demo         # Run all effect demos
make clean        # Clean build files
make help         # Show all available targets
```

## Key Features

- **From Scratch**: No external audio libraries required
- **Minimal Dependencies**: Only standard C library + math library  
- **Real-time Safe**: No memory allocation during processing
- **Educational**: Well-documented DSP implementations
- **Modular**: Each effect is self-contained
- **Portable**: Works on Linux, macOS, Windows

Built for audio enthusiasts and DSP learners