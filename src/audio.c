#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include "audio.h"

/* Global audio configuration */
static AudioConfig global_audio_config = {0};
static int audio_initialized = 0;

int audio_init(const AudioConfig *config) {
    if (!config) {
        fprintf(stderr, "Audio config is NULL\n");
        return -1;
    }

    global_audio_config = *config;

    /* Validate configuration */
    if (config->type == AUDIO_SOURCE_TTS_PIPER) {
        if (!config->voice_model) {
            fprintf(stderr, "Piper TTS requires voice_model path\n");
            return -1;
        }

        /* Check if voice model exists */
        if (access(config->voice_model, F_OK) != 0) {
            fprintf(stderr, "Voice model not found: %s\n", config->voice_model);
            return -1;
        }

        /* Check if piper binary exists */
        if (system("which piper > /dev/null 2>&1") != 0) {
            fprintf(stderr, "Piper TTS not installed. Install with: sudo apt install piper-tts\n");
            return -1;
        }
    }

    audio_initialized = 1;
    printf("Audio system initialized: %s\n",
           config->type == AUDIO_SOURCE_TTS_PIPER ? "Piper TTS" :
           config->type == AUDIO_SOURCE_FILE ? "File loading" : "Unknown");

    return 0;
}

AudioSource *audio_generate_tts(const char *text) {
    if (!audio_initialized) {
        fprintf(stderr, "Audio system not initialized\n");
        return NULL;
    }

    if (global_audio_config.type != AUDIO_SOURCE_TTS_PIPER) {
        fprintf(stderr, "TTS not configured\n");
        return NULL;
    }

    /* Create temporary file for output */
    char temp_file[] = "/tmp/quizvid_audio_XXXXXX.wav";
    int fd = mkstemps(temp_file, 4);
    if (fd == -1) {
        fprintf(stderr, "Failed to create temporary file\n");
        return NULL;
    }
    close(fd);

    /* Build piper command */
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "echo \"%s\" | piper --model %s --output_file %s --length_scale %.2f > /dev/null 2>&1",
             text, global_audio_config.voice_model, temp_file,
             1.0f / global_audio_config.speed); /* Piper uses length_scale, inverse of speed */

    /* Execute piper */
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "Piper TTS failed\n");
        unlink(temp_file);
        return NULL;
    }

    /* Load generated WAV file */
    AudioSource *audio = audio_load_wav(temp_file);

    /* Clean up temp file */
    unlink(temp_file);

    return audio;
}

AudioSource *audio_load_wav(const char *filepath) {
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    const AVCodec *codec = NULL;
    SwrContext *swr_ctx = NULL;
    AudioSource *audio = NULL;
    int audio_stream_index = -1;

    /* Open file */
    if (avformat_open_input(&fmt_ctx, filepath, NULL, NULL) < 0) {
        fprintf(stderr, "Failed to open audio file: %s\n", filepath);
        return NULL;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Failed to find stream info\n");
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    /* Find audio stream */
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }

    if (audio_stream_index == -1) {
        fprintf(stderr, "No audio stream found\n");
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    AVStream *audio_stream = fmt_ctx->streams[audio_stream_index];

    /* Find decoder */
    codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    /* Allocate codec context */
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    if (avcodec_parameters_to_context(codec_ctx, audio_stream->codecpar) < 0) {
        fprintf(stderr, "Failed to copy codec parameters\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    /* Calculate duration */
    float duration = (float)fmt_ctx->duration / AV_TIME_BASE;
    int sample_rate = codec_ctx->sample_rate;
    int channels = codec_ctx->ch_layout.nb_channels;

    /* Allocate audio source */
    audio = calloc(1, sizeof(AudioSource));
    if (!audio) {
        fprintf(stderr, "Failed to allocate AudioSource\n");
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    audio->sample_rate = sample_rate;
    audio->channels = channels;
    audio->duration = duration;
    audio->num_samples = (int)(duration * sample_rate * channels);
    audio->samples = calloc(audio->num_samples, sizeof(float));

    if (!audio->samples) {
        fprintf(stderr, "Failed to allocate sample buffer\n");
        free(audio);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    /* Setup resampler to convert to float32 */
    swr_ctx = swr_alloc();
    av_opt_set_chlayout(swr_ctx, "in_chlayout", &codec_ctx->ch_layout, 0);
    av_opt_set_chlayout(swr_ctx, "out_chlayout", &codec_ctx->ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", codec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    if (swr_init(swr_ctx) < 0) {
        fprintf(stderr, "Failed to initialize resampler\n");
        free(audio->samples);
        free(audio);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        return NULL;
    }

    /* Read and decode audio */
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int sample_offset = 0;

    while (av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_index) {
            if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                    /* Convert to float32 */
                    uint8_t *output_buffer = (uint8_t *)audio->samples +
                                            (sample_offset * sizeof(float));

                    int out_samples = swr_convert(swr_ctx, &output_buffer,
                                                 frame->nb_samples,
                                                 (const uint8_t **)frame->data,
                                                 frame->nb_samples);

                    sample_offset += out_samples * channels;
                }
            }
        }
        av_packet_unref(packet);
    }

    /* Cleanup */
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr_ctx);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&fmt_ctx);

    printf("Loaded audio: %.2fs, %d Hz, %d channels\n",
           audio->duration, audio->sample_rate, audio->channels);

    return audio;
}

float audio_get_duration(const char *text_or_file) {
    /* Quick estimate: ~150 words per minute, ~5 chars per word */
    /* This is rough - better to generate audio and measure */
    if (!text_or_file) return 0.0f;

    size_t len = strlen(text_or_file);
    float words = len / 5.0f;
    float minutes = words / 150.0f;
    float seconds = minutes * 60.0f;

    /* Adjust for speed */
    if (audio_initialized && global_audio_config.speed > 0.0f) {
        seconds /= global_audio_config.speed;
    }

    return seconds;
}

void audio_free(AudioSource *audio) {
    if (!audio) return;

    if (audio->samples) {
        free(audio->samples);
        audio->samples = NULL;
    }

    free(audio);
}

void audio_cleanup(void) {
    audio_initialized = 0;
}
