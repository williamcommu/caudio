#include "wav_io.h"
#include <mpg123.h>
#include <strings.h>

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

// Load MP3 file into AudioBuffer
AudioBuffer* mp3_load(const char* filename) {
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;
    
    // Initialize mpg123
    if (mpg123_init() != MPG123_OK) {
        printf("Error: Could not initialize mpg123\n");
        return NULL;
    }
    
    mh = mpg123_new(NULL, &err);
    if (mh == NULL) {
        printf("Error: Could not create mpg123 handle: %s\n", mpg123_plain_strerror(err));
        mpg123_exit();
        return NULL;
    }
    
    // Open the file
    if (mpg123_open(mh, filename) != MPG123_OK) {
        printf("Error: Could not open MP3 file: %s\n", mpg123_strerror(mh));
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    // Get format info
    long rate;
    int channels, encoding;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
        printf("Error: Could not get MP3 format info: %s\n", mpg123_strerror(mh));
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    // Force the encoding to be 16bit signed integer
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16);
    
    // Scan the file to get accurate length (important for VBR MP3s)
    if (mpg123_scan(mh) != MPG123_OK) {
        printf("Warning: Could not scan MP3 file for accurate length\n");
    }
    
    // Get the length of the file in samples
    off_t length = mpg123_length(mh);
    if (length < 0) {
        printf("Error: Could not determine MP3 length\n");
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    printf("MP3 DEBUG: mpg123_length returned %ld frames (%.2f seconds at %ld Hz, %d channels)\n", 
           length, (float)length / rate, rate, channels);
    
    // Create audio buffer - length should be total samples (frames * channels)
    AudioBuffer* audio_buf = audio_buffer_create(length * channels, channels, rate);
    if (!audio_buf) {
        printf("Error: Could not create audio buffer\n");
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    // Allocate temporary buffer for reading
    buffer_size = mpg123_outblock(mh);
    buffer = malloc(buffer_size);
    if (!buffer) {
        printf("Error: Could not allocate MP3 read buffer\n");
        audio_buffer_destroy(audio_buf);
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    // Read the entire file
    size_t total_samples = 0;
    int16_t* temp_data = malloc(length * channels * sizeof(int16_t));
    if (!temp_data) {
        printf("Error: Could not allocate temporary sample buffer\n");
        free(buffer);
        audio_buffer_destroy(audio_buf);
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
        return NULL;
    }
    
    while (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
        int16_t* samples = (int16_t*)buffer;
        size_t num_samples = done / sizeof(int16_t);
        
        // Check if we have space
        if (total_samples + num_samples > length * channels) {
            num_samples = (length * channels) - total_samples;
        }
        
        memcpy(temp_data + total_samples, samples, num_samples * sizeof(int16_t));
        total_samples += num_samples;
        
        if (total_samples >= length * channels) break;
    }
    
    // Convert to float samples
    for (size_t i = 0; i < total_samples; i++) {
        audio_buf->data[i] = int16_to_float(temp_data[i]);
    }
    
    printf("Loaded MP3 %s: %ld frames, %d channels, %ld Hz (%.2f seconds)\n", 
           filename, length, channels, rate, (float)length / rate);
    
    // Cleanup
    free(temp_data);
    free(buffer);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    
    return audio_buf;
}

// Helper function to detect if file is MP3 based on extension
int is_mp3_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcasecmp(ext, ".mp3") == 0);
}

// Helper function to detect if file is WAV based on extension
int is_wav_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return 0;
    return (strcasecmp(ext, ".wav") == 0);
}

// Generic audio loading function that detects format automatically
AudioBuffer* audio_load(const char* filename) {
    if (is_wav_file(filename)) {
        return wav_load(filename);
    } else if (is_mp3_file(filename)) {
        return mp3_load(filename);
    } else {
        printf("Error: Unsupported audio format. Only WAV and MP3 files are supported.\n");
        return NULL;
    }
}