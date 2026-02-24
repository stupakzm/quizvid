#ifndef MUXER_H
#define MUXER_H

#include <stdint.h>
#include "config.h"
#include "audio.h"

/* Muxer context (opaque - implementation details hidden) */
typedef struct MuxerContext MuxerContext;

/* Initialize muxer with video and audio configuration */
MuxerContext *muxer_init(const char *output_file,
                         int width, int height, int fps,
                         int sample_rate, int channels);

/* Write video frame (RGB buffer) */
int muxer_write_video_frame(MuxerContext *ctx, uint8_t *rgb_buffer);

/* Write audio samples (float PCM) */
int muxer_write_audio_samples(MuxerContext *ctx, float *samples, int num_samples);

/* Finalize and close output file */
int muxer_finalize(MuxerContext *ctx);

/* Free muxer resources */
void muxer_free(MuxerContext *ctx);

/* Get current video timestamp (for sync debugging) */
double muxer_get_video_time(MuxerContext *ctx);

/* Get current audio timestamp (for sync debugging) */
double muxer_get_audio_time(MuxerContext *ctx);

#endif /* MUXER_H */
