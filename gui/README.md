# Audio Effects Mixer GUI

A real-time graphical interface for the audio effects library, allowing users to:
- Load audio files (WAV format)
- Add multiple effects in a chain
- Adjust effect parameters with sliders
- Real-time preview and processing
- Save processed audio

## Features

### Supported Effects
1. **Filters**: Lowpass, Highpass, EQ
2. **Delays**: Echo, Reverb
3. **Distortion**: Overdrive, Tube, Fuzz
4. **Modulation**: Chorus, Flanger, Phaser, Tremolo

### GUI Controls
- **File Operations**: Load/Save audio files, Process effects
- **Master Controls**: Master volume slider
- **Effect Buttons**: Click to add effects to the chain
- **Effect Slots**: Each effect has enable/disable, remove button, and parameter sliders
- **Real-time Processing**: Optional auto-processing for instant feedback

## Dependencies

The GUI version requires SDL2:

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Fedora/RHEL
sudo dnf install SDL2-devel

# macOS (with Homebrew)
brew install sdl2
```

## Building

```bash
# Build GUI application
make gui

# Build both console and GUI versions
make both

# Run GUI
make run-gui

# Run GUI with test audio file
make run-gui-demo
```

## Usage

### Basic Usage
```bash
./build/audio_mixer_gui [audio_file.wav]
```

### Controls
- **ESC**: Exit application
- **Left Click**: Interact with buttons and sliders
- **Drag**: Adjust slider values

### Workflow
1. Click "Load Audio" or provide file via command line
2. Add effects using the effect buttons
3. Adjust parameters with the sliders in each effect slot
4. Click "Process" to apply effects
5. Click "Save Audio" to save the result

## Effect Parameters

Each effect has up to 3 adjustable parameters:

### Filters
- **Lowpass/Highpass**: Cutoff frequency (Hz)
- **EQ**: Frequency (Hz), Q factor, Gain (dB)

### Delays
- **Echo**: Delay time (s), Feedback, Mix
- **Reverb**: Room size, Damping, Mix

### Distortion
- **Overdrive/Tube/Fuzz**: Drive, Tone, Output level

### Modulation
- **Chorus/Flanger**: Rate (Hz), Depth, Feedback
- **Phaser**: Rate (Hz), Depth, Feedback
- **Tremolo**: Rate (Hz), Depth

## Technical Details

- **Sample Rate**: 44.1 kHz (CD quality)
- **Bit Depth**: 16-bit input/output, 32-bit float processing
- **Max Effects**: 8 effects in chain
- **Real-time**: Optional auto-processing for parameter changes
- **Memory Management**: Automatic buffer allocation/deallocation

## Architecture

```
gui/
├── audio_mixer_gui.h      # GUI interface definitions
├── audio_mixer.c          # Core mixing logic
├── simple_gui.c           # SDL2 GUI implementation
├── audio_mixer_main.c     # Main application
└── README.md             # This file
```

## Performance

The GUI is designed to be lightweight and responsive:
- 60 FPS rendering
- Efficient event handling
- Minimal CPU usage when idle
- Real-time parameter updates

## Limitations

- Simple text rendering (rectangles for text placeholders)
- Basic UI styling
- WAV files only
- Single audio file at a time
- No undo/redo functionality

## Future Enhancements

Possible improvements for a full production GUI:
- True text rendering (FreeType integration)
- Advanced UI styling and themes
- Multiple audio track support
- Waveform visualization
- Spectrum analyzer
- Plugin architecture
- MIDI control support
- Audio recording
- Export to multiple formats

