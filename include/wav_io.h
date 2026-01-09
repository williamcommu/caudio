#ifndef WAV_IO_H
#define WAV_IO_H

#include "audio_core.h"

// WAV file header structure
#pragma pack(push, 1)
typedef struct {
    char riff_id[4];        // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave_id[4];        // "WAVE"
    char fmt_id[4];         // "fmt "
    uint32_t fmt_size;      // Format chunk size
    uint16_t format;        // Audio format
    uint16_t channels;      // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Bytes per second
    uint16_t block_align;   // Bytes per sample frame
    uint16_t bits_per_sample; // Bits per sample
    char data_id[4];        // "data"
    uint32_t data_size;     // Data chunk size
} WavHeader;
#pragma pack(pop)

// WAV file I/O functions
AudioBuffer* wav_load(const char* filename);
int wav_save(const char* filename, AudioBuffer* buffer);
void print_wav_info(const char* filename);

#endif // WAV_IO_H