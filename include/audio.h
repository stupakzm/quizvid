#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/* Audio source types */
typedef enum {
    AUDIO_SOURCE_NONE,
    AUDIO_SOURCE_FILE,      /* Load from WAV/MP3 file */
    AUDIO_SOURCE_TTS_PIPER, /* Generate with Piper TTS */
    AUDIO_SOURCE_TTS_ESPEAK /* Generate with eSpeak (future) */
} AudioSourceType;

/* Audio configuration */
typedef struct {
    AudioSourceType type;
    const char *voice_model;  /* Path to voice model (for TTS) */
    float speed;              /* 0.5 - 2.0 */
    int sample_rate;          /* e.g., 22050, 44100 */
} AudioConfig;

/* Audio data structure */
typedef struct {
    float *samples;      /* PCM audio samples (interleaved if stereo) */
    int num_samples;     /* Total samples (multiply by channels) */
    int sample_rate;     /* Samples per second */
    int channels;        /* 1 = mono, 2 = stereo */
    float duration;      /* Duration in seconds */
} AudioSource;

/* Initialize audio system with configuration */
int audio_init(const AudioConfig *config);

/* Generate audio from text using configured TTS */
AudioSource *audio_generate_tts(const char *text);

/* Load audio from WAV file */
AudioSource *audio_load_wav(const char *filepath);

/* Get duration of audio (without loading full data) */
float audio_get_duration(const char *text_or_file);

/* Free audio source */
void audio_free(AudioSource *audio);

/* Cleanup audio system */
void audio_cleanup(void);

#endif /* AUDIO_H */
