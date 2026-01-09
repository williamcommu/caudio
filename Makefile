# Audio Effects Library Makefile
# Built from scratch in C with minimal dependencies

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99 -ffast-math -Iinclude
LDFLAGS = -lm
GUI_LDFLAGS = -lm -lSDL2
DEBUG_FLAGS = -g -DDEBUG -O0
RELEASE_FLAGS = -O3 -DNDEBUG -march=native

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
EXAMPLES_DIR = examples
AUDIO_SAMPLES_DIR = audio_samples
GUI_DIR = gui

# Project names
PROJECT = audio_effects_demo
GUI_PROJECT = audio_mixer_gui
LIBRARY = libaudiofx.a

# Source files
SOURCES = audio_core.c wav_io.c audio_filters.c delay_effects.c reverb.c distortion.c modulation_effects.c
MAIN_SOURCE = audio_effects_demo.c
GUI_SOURCES = audio_mixer.c simple_gui.c
GUI_MAIN_SOURCE = audio_mixer_main.c

SRC_OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCES:.c=.o))
MAIN_OBJECT = $(BUILD_DIR)/$(MAIN_SOURCE:.c=.o)
GUI_OBJECTS = $(addprefix $(BUILD_DIR)/, $(GUI_SOURCES:.c=.o))
GUI_MAIN_OBJECT = $(BUILD_DIR)/$(GUI_MAIN_SOURCE:.c=.o)

# Header files
HEADERS = $(addprefix $(INCLUDE_DIR)/, audio_core.h wav_io.h audio_filters.h delay_effects.h reverb.h distortion.h modulation_effects.h)
GUI_HEADERS = $(GUI_DIR)/audio_mixer_gui.h

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(AUDIO_SAMPLES_DIR):
	mkdir -p $(AUDIO_SAMPLES_DIR)

# Default target
all: $(BUILD_DIR) $(PROJECT)

# Build GUI application
gui: $(BUILD_DIR) $(GUI_PROJECT)

# Build both console and GUI applications
both: all gui

# Build the main executable
$(PROJECT): $(SRC_OBJECTS) $(MAIN_OBJECT)
	@echo "Linking $(PROJECT)..."
	$(CC) $(SRC_OBJECTS) $(MAIN_OBJECT) -o $(BUILD_DIR)/$(PROJECT) $(LDFLAGS)
	@echo "Build complete! Run with: ./$(BUILD_DIR)/$(PROJECT)"

# Build GUI application
$(GUI_PROJECT): $(SRC_OBJECTS) $(GUI_OBJECTS) $(GUI_MAIN_OBJECT)
	@echo "Linking $(GUI_PROJECT)..."
	$(CC) $(SRC_OBJECTS) $(GUI_OBJECTS) $(GUI_MAIN_OBJECT) -o $(BUILD_DIR)/$(GUI_PROJECT) $(GUI_LDFLAGS)
	@echo "GUI build complete! Run with: ./$(BUILD_DIR)/$(GUI_PROJECT) [audio_file.wav]"

# Build static library
library: $(BUILD_DIR) $(LIBRARY)

$(LIBRARY): $(SRC_OBJECTS)
	@echo "Creating static library $(LIBRARY)..."
	ar rcs $(BUILD_DIR)/$(LIBRARY) $(SRC_OBJECTS)
	@echo "Library created: $(BUILD_DIR)/$(LIBRARY)"

# Compile source files from src directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile demo from examples directory  
$(BUILD_DIR)/$(MAIN_SOURCE:.c=.o): $(EXAMPLES_DIR)/$(MAIN_SOURCE) $(HEADERS) | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GUI files from gui directory
$(BUILD_DIR)/%.o: $(GUI_DIR)/%.c $(HEADERS) $(GUI_HEADERS) | $(BUILD_DIR)
	@echo "Compiling GUI $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(PROJECT)
	@echo "Debug build complete!"

# Release build with optimizations
release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(PROJECT)
	@echo "Release build complete!"

# Run the demo
run: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	cd $(AUDIO_SAMPLES_DIR) && ../$(BUILD_DIR)/$(PROJECT)

# Run GUI application
run-gui: $(AUDIO_SAMPLES_DIR) gui
	cd $(AUDIO_SAMPLES_DIR) && ../$(BUILD_DIR)/$(GUI_PROJECT)

# Run GUI with test audio file
run-gui-demo: $(AUDIO_SAMPLES_DIR) gui
	cd $(AUDIO_SAMPLES_DIR) && ../$(BUILD_DIR)/$(GUI_PROJECT) sine_440_5s.wav

# Run all demos automatically
demo: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	cd $(AUDIO_SAMPLES_DIR) && ../$(BUILD_DIR)/$(PROJECT) --all

# Test individual components
test-filters: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing filter effects..."
	cd $(AUDIO_SAMPLES_DIR) && echo "1" | ../$(BUILD_DIR)/$(PROJECT)

test-delays: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing delay effects..."
	cd $(AUDIO_SAMPLES_DIR) && echo "2" | ../$(BUILD_DIR)/$(PROJECT)

test-reverbs: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing reverb effects..."
	cd $(AUDIO_SAMPLES_DIR) && echo "3" | ../$(BUILD_DIR)/$(PROJECT)

test-distortion: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing distortion effects..."
	cd $(AUDIO_SAMPLES_DIR) && echo "4" | ../$(BUILD_DIR)/$(PROJECT)

test-modulation: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing modulation effects..."
	cd $(AUDIO_SAMPLES_DIR) && echo "5" | ../$(BUILD_DIR)/$(PROJECT)

test-chain: $(AUDIO_SAMPLES_DIR) $(PROJECT)
	@echo "Testing effect chain..."
	cd $(AUDIO_SAMPLES_DIR) && echo "6" | ../$(BUILD_DIR)/$(PROJECT)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(AUDIO_SAMPLES_DIR)/*.wav  # Clean generated audio files
	@echo "Clean complete!"

# Deep clean - remove all generated content
clean-all: clean
	rm -rf $(AUDIO_SAMPLES_DIR)

# Install (copy to /usr/local/bin)
install: release
	@echo "Installing $(PROJECT) to /usr/local/bin..."
	sudo cp $(BUILD_DIR)/$(PROJECT) /usr/local/bin/
	@echo "Installation complete!"

# Uninstall
uninstall:
	@echo "Removing $(PROJECT) from /usr/local/bin..."
	sudo rm -f /usr/local/bin/$(PROJECT)
	@echo "Uninstall complete!"

# Generate documentation
docs:
	@echo "Generating documentation..."
	@echo "=== AUDIO EFFECTS LIBRARY DOCUMENTATION ===" > README.txt
	@echo "" >> README.txt
	@echo "This library implements various audio effects from scratch in C:" >> README.txt
	@echo "" >> README.txt
	@echo "FILTERS:" >> README.txt
	@echo "- Biquad filters (lowpass, highpass, bandpass, notch)" >> README.txt
	@echo "- One-pole filters" >> README.txt
	@echo "- 4-band EQ" >> README.txt
	@echo "" >> README.txt
	@echo "DELAY EFFECTS:" >> README.txt
	@echo "- Simple echo/delay" >> README.txt
	@echo "- Multi-tap delay" >> README.txt
	@echo "- Ping-pong delay (stereo)" >> README.txt
	@echo "" >> README.txt
	@echo "REVERB EFFECTS:" >> README.txt
	@echo "- Schroeder reverb" >> README.txt
	@echo "- Plate reverb" >> README.txt
	@echo "- Freeverb algorithm" >> README.txt
	@echo "" >> README.txt
	@echo "DISTORTION EFFECTS:" >> README.txt
	@echo "- Overdrive" >> README.txt
	@echo "- Tube distortion" >> README.txt
	@echo "- Fuzz distortion" >> README.txt
	@echo "- Various waveshaping functions" >> README.txt
	@echo "" >> README.txt
	@echo "MODULATION EFFECTS:" >> README.txt
	@echo "- Chorus" >> README.txt
	@echo "- Flanger" >> README.txt
	@echo "- Phaser" >> README.txt
	@echo "- Tremolo" >> README.txt
	@echo "- Vibrato" >> README.txt
	@echo "- Auto-wah" >> README.txt
	@echo "" >> README.txt
	@echo "BUILD INSTRUCTIONS:" >> README.txt
	@echo "  make          - Build debug version" >> README.txt
	@echo "  make release  - Build optimized version" >> README.txt
	@echo "  make library  - Build static library" >> README.txt
	@echo "  make run      - Run interactive demo" >> README.txt
	@echo "  make demo     - Run all demos automatically" >> README.txt
	@echo "  make clean    - Clean build files" >> README.txt
	@echo "" >> README.txt
	@echo "All effects work with 16-bit WAV files and process audio in real-time." >> README.txt
	@echo "Documentation generated!"

# Show help
help:
	@echo "Audio Effects Library - Available targets:"
	@echo ""
	@echo "BUILD TARGETS:"
	@echo "  all       - Build the console demo application (default)"
	@echo "  gui       - Build the GUI application"
	@echo "  both      - Build both console and GUI versions"
	@echo "  library   - Build static library (libaudiofx.a)"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo ""
	@echo "RUN TARGETS:"
	@echo "  run       - Run interactive console demo"
	@echo "  run-gui   - Run GUI application"
	@echo "  run-gui-demo - Run GUI with test audio file"
	@echo "  demo      - Run all console demos automatically"
	@echo ""
	@echo "TEST TARGETS:"
	@echo "  test-filters     - Test filter effects only"
	@echo "  test-delays      - Test delay effects only"
	@echo "  test-reverbs     - Test reverb effects only"
	@echo "  test-distortion  - Test distortion effects only"
	@echo "  test-modulation  - Test modulation effects only"
	@echo "  test-chain       - Test effect chain only"
	@echo ""
	@echo "UTILITY TARGETS:"
	@echo "  clean     - Remove build artifacts and generated WAV files"
	@echo "  clean-all - Deep clean including audio sample directory"
	@echo "  install   - Install console version to /usr/local/bin (requires sudo)"
	@echo "  install-gui - Install GUI version to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (requires sudo)"
	@echo "  docs      - Generate README.txt documentation"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "REQUIREMENTS:"
	@echo "  Console: gcc, make, libm (math library)"
	@echo "  GUI:     gcc, make, libm, SDL2 development libraries"
	@echo ""
	@echo "EXAMPLES:"
	@echo "  make && make run        - Build and run console demo"
	@echo "  make gui && make run-gui - Build and run GUI application"
	@echo "  make both              - Build both versions"
	@echo "  make release demo   - Build optimized version and run all demos"
	@echo "  make library        - Build static library for your projects"

# Phony targets
.PHONY: all clean debug release run demo install uninstall docs help library
.PHONY: test-filters test-delays test-reverbs test-distortion test-modulation test-chain

# Make sure intermediate files are not deleted
.PRECIOUS: %.o