#include "wav_io.h"

// Load WAV file into AudioBuffer
AudioBuffer* wav_load(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }
    
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
        printf("Error: Could not read WAV header\n");
        fclose(file);
        return NULL;
    }
    
    // Verify WAV header
    if (strncmp(header.riff_id, "RIFF", 4) != 0 ||
        strncmp(header.wave_id, "WAVE", 4) != 0 ||
        strncmp(header.fmt_id, "fmt ", 4) != 0 ||
        strncmp(header.data_id, "data", 4) != 0) {
        printf("Error: Invalid WAV file format\n");
        fclose(file);
        return NULL;
    }
    
    if (header.format != 1) {
        printf("Error: Only PCM format is supported\n");
        fclose(file);
        return NULL;
    }
    
    if (header.bits_per_sample != 16) {
        printf("Error: Only 16-bit samples are supported\n");
        fclose(file);
        return NULL;
    }
    
    // Calculate number of samples
    size_t num_samples = header.data_size / (header.bits_per_sample / 8);
    size_t samples_per_channel = num_samples / header.channels;
    
    // Create audio buffer
    AudioBuffer* buffer = audio_buffer_create(samples_per_channel, header.channels, header.sample_rate);
    if (!buffer) {
        printf("Error: Could not create audio buffer\n");
        fclose(file);
        return NULL;
    }
    
    // Read sample data
    int16_t* temp_buffer = malloc(num_samples * sizeof(int16_t));
    if (!temp_buffer) {
        printf("Error: Could not allocate temporary buffer\n");
        audio_buffer_destroy(buffer);
        fclose(file);
        return NULL;
    }
    
    if (fread(temp_buffer, sizeof(int16_t), num_samples, file) != num_samples) {
        printf("Error: Could not read sample data\n");
        free(temp_buffer);
        audio_buffer_destroy(buffer);
        fclose(file);
        return NULL;
    }
    
    // Convert to float samples
    for (size_t i = 0; i < num_samples; i++) {
        buffer->data[i] = int16_to_float(temp_buffer[i]);
    }
    
    free(temp_buffer);
    fclose(file);
    
    printf("Loaded %s: %zu samples, %zu channels, %zu Hz\n", 
           filename, samples_per_channel, buffer->channels, buffer->sample_rate);
    
    return buffer;
}

// Save AudioBuffer to WAV file
int wav_save(const char* filename, AudioBuffer* buffer) {
    if (!buffer || !buffer->data) {
        printf("Error: Invalid audio buffer\n");
        return 0;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Could not create file %s\n", filename);
        return 0;
    }
    
    // Prepare WAV header
    WavHeader header;
    memcpy(header.riff_id, "RIFF", 4);
    memcpy(header.wave_id, "WAVE", 4);
    memcpy(header.fmt_id, "fmt ", 4);
    memcpy(header.data_id, "data", 4);
    
    header.fmt_size = 16;
    header.format = 1; // PCM
    header.channels = (uint16_t)buffer->channels;
    header.sample_rate = (uint32_t)buffer->sample_rate;
    header.bits_per_sample = 16;
    header.block_align = (header.channels * header.bits_per_sample) / 8;
    header.byte_rate = header.sample_rate * header.block_align;
    header.data_size = (uint32_t)(buffer->capacity * sizeof(int16_t));
    header.file_size = sizeof(WavHeader) - 8 + header.data_size;
    
    // Write header
    if (fwrite(&header, sizeof(WavHeader), 1, file) != 1) {
        printf("Error: Could not write WAV header\n");
        fclose(file);
        return 0;
    }
    
    // Convert and write sample data
    int16_t* temp_buffer = malloc(buffer->capacity * sizeof(int16_t));
    if (!temp_buffer) {
        printf("Error: Could not allocate temporary buffer\n");
        fclose(file);
        return 0;
    }
    
    for (size_t i = 0; i < buffer->capacity; i++) {
        temp_buffer[i] = float_to_int16(buffer->data[i]);
    }
    
    if (fwrite(temp_buffer, sizeof(int16_t), buffer->capacity, file) != buffer->capacity) {
        printf("Error: Could not write sample data\n");
        free(temp_buffer);
        fclose(file);
        return 0;
    }
    
    free(temp_buffer);
    fclose(file);
    
    printf("Saved %s: %zu samples, %zu channels, %zu Hz\n", 
           filename, buffer->length, buffer->channels, buffer->sample_rate);
    
    return 1;
}

// Print WAV file information
void print_wav_info(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return;
    }
    
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
        printf("Error: Could not read WAV header\n");
        fclose(file);
        return;
    }
    
    printf("WAV File Info for %s:\n", filename);
    printf("  Format: %s\n", (header.format == 1) ? "PCM" : "Unknown");
    printf("  Channels: %u\n", header.channels);
    printf("  Sample Rate: %u Hz\n", header.sample_rate);
    printf("  Bit Depth: %u bits\n", header.bits_per_sample);
    printf("  Duration: %.2f seconds\n", 
           (float)header.data_size / (header.sample_rate * header.channels * (header.bits_per_sample / 8)));
    
    fclose(file);
}