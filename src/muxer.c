#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include "muxer.h"

/* Muxer context structure */
struct MuxerContext {
    /* Output */
    AVFormatContext *format_ctx;
    const char *output_filename;

    /* Video encoding */
    AVStream *video_stream;
    AVCodecContext *video_codec_ctx;
    AVFrame *video_frame;
    AVPacket *video_packet;
    struct SwsContext *sws_ctx;
    int64_t video_pts;

    /* Audio encoding */
    AVStream *audio_stream;
    AVCodecContext *audio_codec_ctx;
    AVFrame *audio_frame;
    AVPacket *audio_packet;
    int64_t audio_pts;

    /* Audio buffering */
    float *audio_buffer;
    int audio_buffer_size;
    int audio_buffer_index;

    /* Configuration */
    int width;
    int height;
    int fps;
    int sample_rate;
    int channels;
};

MuxerContext *muxer_init(const char *output_file,
                         int width, int height, int fps,
                         int sample_rate, int channels) {
    MuxerContext *ctx = calloc(1, sizeof(MuxerContext));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate muxer context\n");
        return NULL;
    }

    ctx->output_filename = output_file;
    ctx->width = width;
    ctx->height = height;
    ctx->fps = fps;
    ctx->sample_rate = sample_rate;
    ctx->channels = channels;
    ctx->video_pts = 0;
    ctx->audio_pts = 0;

    /* Allocate output format context */
    avformat_alloc_output_context2(&ctx->format_ctx, NULL, NULL, output_file);
    if (!ctx->format_ctx) {
        fprintf(stderr, "Could not create output context\n");
        free(ctx);
        return NULL;
    }

    /* Initialize video encoder */
    const AVCodec *video_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!video_codec) {
        fprintf(stderr, "H.264 codec not found\n");
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    ctx->video_stream = avformat_new_stream(ctx->format_ctx, NULL);
    if (!ctx->video_stream) {
        fprintf(stderr, "Failed to create video stream\n");
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    ctx->video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!ctx->video_codec_ctx) {
        fprintf(stderr, "Failed to allocate video codec context\n");
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    /* Set video codec parameters */
    ctx->video_codec_ctx->width = width;
    ctx->video_codec_ctx->height = height;
    ctx->video_codec_ctx->time_base = (AVRational){1, fps};
    ctx->video_codec_ctx->framerate = (AVRational){fps, 1};
    ctx->video_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    ctx->video_codec_ctx->gop_size = 10;
    ctx->video_codec_ctx->max_b_frames = 1;

    if (avcodec_open2(ctx->video_codec_ctx, video_codec, NULL) < 0) {
        fprintf(stderr, "Failed to open video codec\n");
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    avcodec_parameters_from_context(ctx->video_stream->codecpar, ctx->video_codec_ctx);
    ctx->video_stream->time_base = ctx->video_codec_ctx->time_base;

    /* Allocate video frame */
    ctx->video_frame = av_frame_alloc();
    ctx->video_frame->format = ctx->video_codec_ctx->pix_fmt;
    ctx->video_frame->width = width;
    ctx->video_frame->height = height;
    av_frame_get_buffer(ctx->video_frame, 0);

    ctx->video_packet = av_packet_alloc();

    /* Initialize audio encoder */
    const AVCodec *audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audio_codec) {
        fprintf(stderr, "AAC codec not found\n");
        /* Cleanup video resources */
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    ctx->audio_stream = avformat_new_stream(ctx->format_ctx, NULL);
    if (!ctx->audio_stream) {
        fprintf(stderr, "Failed to create audio stream\n");
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    ctx->audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (!ctx->audio_codec_ctx) {
        fprintf(stderr, "Failed to allocate audio codec context\n");
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    /* Set audio codec parameters */
    ctx->audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    ctx->audio_codec_ctx->sample_rate = sample_rate;
    av_channel_layout_default(&ctx->audio_codec_ctx->ch_layout, channels);
    ctx->audio_codec_ctx->bit_rate = 128000;
    ctx->audio_codec_ctx->time_base = (AVRational){1, sample_rate};

    if (avcodec_open2(ctx->audio_codec_ctx, audio_codec, NULL) < 0) {
        fprintf(stderr, "Failed to open audio codec\n");
        avcodec_free_context(&ctx->audio_codec_ctx);
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    avcodec_parameters_from_context(ctx->audio_stream->codecpar, ctx->audio_codec_ctx);
    ctx->audio_stream->time_base = ctx->audio_codec_ctx->time_base;

    /* Allocate audio frame */
    ctx->audio_frame = av_frame_alloc();
    ctx->audio_frame->format = ctx->audio_codec_ctx->sample_fmt;
    ctx->audio_frame->nb_samples = ctx->audio_codec_ctx->frame_size;
    av_channel_layout_copy(&ctx->audio_frame->ch_layout, &ctx->audio_codec_ctx->ch_layout);
    av_frame_get_buffer(ctx->audio_frame, 0);

    ctx->audio_packet = av_packet_alloc();

    /* Initialize audio buffer - ADD THIS */
    ctx->audio_buffer_size = ctx->audio_codec_ctx->frame_size * 4; /* Buffer 4 AAC frames */
    ctx->audio_buffer = calloc(ctx->audio_buffer_size * ctx->channels, sizeof(float));
    ctx->audio_buffer_index = 0;

    /* Initialize RGB to YUV converter */
    ctx->sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_RGB24,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL
    );

    /* Open output file */
    if (avio_open(&ctx->format_ctx->pb, output_file, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "Failed to open output file: %s\n", output_file);
        sws_freeContext(ctx->sws_ctx);
        av_packet_free(&ctx->audio_packet);
        av_frame_free(&ctx->audio_frame);
        avcodec_free_context(&ctx->audio_codec_ctx);
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    /* Write file header */
    if (avformat_write_header(ctx->format_ctx, NULL) < 0) {
        fprintf(stderr, "Failed to write header\n");
        avio_closep(&ctx->format_ctx->pb);
        sws_freeContext(ctx->sws_ctx);
        av_packet_free(&ctx->audio_packet);
        av_frame_free(&ctx->audio_frame);
        avcodec_free_context(&ctx->audio_codec_ctx);
        av_packet_free(&ctx->video_packet);
        av_frame_free(&ctx->video_frame);
        avcodec_free_context(&ctx->video_codec_ctx);
        avformat_free_context(ctx->format_ctx);
        free(ctx);
        return NULL;
    }

    printf("Muxer initialized: %dx%d @ %dfps, audio %dHz %dch\n",
           width, height, fps, sample_rate, channels);

    return ctx;
}

int muxer_write_video_frame(MuxerContext *ctx, uint8_t *rgb_buffer) {
    if (!ctx) return -1;
    
    /* Convert RGB to YUV */
    const uint8_t *src_data[1] = {rgb_buffer};
    int src_linesize[1] = {ctx->width * 3};

    sws_scale(ctx->sws_ctx, src_data, src_linesize, 0, ctx->height,
              ctx->video_frame->data, ctx->video_frame->linesize);

    ctx->video_frame->pts = ctx->video_pts++;

    /* Encode frame */
    int ret = avcodec_send_frame(ctx->video_codec_ctx, ctx->video_frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending video frame\n");
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx->video_codec_ctx, ctx->video_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            fprintf(stderr, "Error encoding video frame\n");
            return -1;
        }

        ctx->video_packet->stream_index = ctx->video_stream->index;
        av_packet_rescale_ts(ctx->video_packet,
                            ctx->video_codec_ctx->time_base,
                            ctx->video_stream->time_base);

        ret = av_interleaved_write_frame(ctx->format_ctx, ctx->video_packet);
        if (ret < 0) {
            fprintf(stderr, "Error writing video frame\n");
            return -1;
        }

        av_packet_unref(ctx->video_packet);
    }

    return 0;
}

/* Helper: Encode buffered audio */
static int encode_audio_frame(MuxerContext *ctx, int num_samples) {
    int frame_size = ctx->audio_codec_ctx->frame_size;

    /* Copy samples to frame buffer (convert to planar) */
    for (int ch = 0; ch < ctx->channels; ch++) {
        float *frame_data = (float *)ctx->audio_frame->data[ch];
        for (int i = 0; i < num_samples; i++) {
            frame_data[i] = ctx->audio_buffer[i * ctx->channels + ch];
        }
        /* Pad with silence if needed */
        for (int i = num_samples; i < frame_size; i++) {
            frame_data[i] = 0.0f;
        }
    }

    ctx->audio_frame->pts = ctx->audio_pts;
    ctx->audio_pts += frame_size;

    /* Encode */
    int ret = avcodec_send_frame(ctx->audio_codec_ctx, ctx->audio_frame);
    if (ret < 0) return ret;

    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx->audio_codec_ctx, ctx->audio_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            return ret;
        }

        ctx->audio_packet->stream_index = ctx->audio_stream->index;
        av_packet_rescale_ts(ctx->audio_packet,
                            ctx->audio_codec_ctx->time_base,
                            ctx->audio_stream->time_base);

        ret = av_interleaved_write_frame(ctx->format_ctx, ctx->audio_packet);
        if (ret < 0) return ret;

        av_packet_unref(ctx->audio_packet);
    }

    return 0;
}

int muxer_write_audio_samples(MuxerContext *ctx, float *samples, int num_samples) {
    if (!ctx || !samples || num_samples == 0) return 0;

    int frame_size = ctx->audio_codec_ctx->frame_size;
    int samples_written = 0;

    while (samples_written < num_samples) {
        int samples_to_copy = num_samples - samples_written;
        int buffer_space = frame_size - ctx->audio_buffer_index;

        if (samples_to_copy > buffer_space) {
            samples_to_copy = buffer_space;
        }

        /* Copy samples to buffer */
        memcpy(ctx->audio_buffer + ctx->audio_buffer_index * ctx->channels,
               samples + samples_written * ctx->channels,
               samples_to_copy * ctx->channels * sizeof(float));

        ctx->audio_buffer_index += samples_to_copy;
        samples_written += samples_to_copy;

        /* If buffer is full, encode it */
        if (ctx->audio_buffer_index >= frame_size) {
            if (encode_audio_frame(ctx, frame_size) < 0) {
                return -1;
            }

            /* Shift remaining samples to start of buffer */
            int remaining = ctx->audio_buffer_index - frame_size;
            if (remaining > 0) {
                memmove(ctx->audio_buffer,
                       ctx->audio_buffer + frame_size * ctx->channels,
                       remaining * ctx->channels * sizeof(float));
            }
            ctx->audio_buffer_index = remaining;
        }
    }

    return 0;
}

int muxer_finalize(MuxerContext *ctx) {
    if (!ctx) return -1;

    /* Flush any remaining audio in buffer */
    if (ctx->audio_buffer_index > 0) {
        encode_audio_frame(ctx, ctx->audio_buffer_index);
        ctx->audio_buffer_index = 0;
    }

    /* Flush video encoder */
    avcodec_send_frame(ctx->video_codec_ctx, NULL);
    while (1) {
        int ret = avcodec_receive_packet(ctx->video_codec_ctx, ctx->video_packet);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
        if (ret >= 0) {
            ctx->video_packet->stream_index = ctx->video_stream->index;
            av_packet_rescale_ts(ctx->video_packet,
                                ctx->video_codec_ctx->time_base,
                                ctx->video_stream->time_base);
            av_interleaved_write_frame(ctx->format_ctx, ctx->video_packet);
            av_packet_unref(ctx->video_packet);
        }
    }

    /* Flush audio encoder */
    avcodec_send_frame(ctx->audio_codec_ctx, NULL);
    while (1) {
        int ret = avcodec_receive_packet(ctx->audio_codec_ctx, ctx->audio_packet);
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) break;
        if (ret >= 0) {
            ctx->audio_packet->stream_index = ctx->audio_stream->index;
            av_packet_rescale_ts(ctx->audio_packet,
                                ctx->audio_codec_ctx->time_base,
                                ctx->audio_stream->time_base);
            av_interleaved_write_frame(ctx->format_ctx, ctx->audio_packet);
            av_packet_unref(ctx->audio_packet);
        }
    }

    /* Write trailer */
    av_write_trailer(ctx->format_ctx);

    printf("Muxer finalized: %s\n", ctx->output_filename);
    return 0;
}

void muxer_free(MuxerContext *ctx) {
    if (!ctx) return;

    if (ctx->audio_buffer) free(ctx->audio_buffer);
    if (ctx->sws_ctx) sws_freeContext(ctx->sws_ctx);
    if (ctx->audio_packet) av_packet_free(&ctx->audio_packet);
    if (ctx->audio_frame) av_frame_free(&ctx->audio_frame);
    if (ctx->audio_codec_ctx) avcodec_free_context(&ctx->audio_codec_ctx);
    if (ctx->video_packet) av_packet_free(&ctx->video_packet);
    if (ctx->video_frame) av_frame_free(&ctx->video_frame);
    if (ctx->video_codec_ctx) avcodec_free_context(&ctx->video_codec_ctx);
    if (ctx->format_ctx) {
        avio_closep(&ctx->format_ctx->pb);
        avformat_free_context(ctx->format_ctx);
    }

    free(ctx);
}

double muxer_get_video_time(MuxerContext *ctx) {
    if (!ctx) return 0.0;
    return (double)ctx->video_pts / ctx->fps;
}

double muxer_get_audio_time(MuxerContext *ctx) {
    if (!ctx) return 0.0;
    return (double)ctx->audio_pts / ctx->sample_rate;
}
