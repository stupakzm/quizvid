#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "video.h"

// Global state for video encoding
static AVFormatContext *format_ctx = NULL;
static AVCodecContext *codec_ctx = NULL;
static AVStream *video_stream = NULL;
static AVFrame *frame = NULL;
static AVPacket *packet = NULL;
static int frame_count = 0;

int video_init(VideoConfig *config){
  int ret;
  const AVCodec *codec;

  // Allocate output format context
  avformat_alloc_output_context2(&format_ctx, NULL, NULL, config->output_filename);
  if(!format_ctx){
    fprintf(stderr, "Could not create output context\n");
    cleanup_on_error();
    return -1;
  }

  // Find H.264 encoder
  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec){
    fprintf(stderr, "H.264 codec not found\n");
    cleanup_on_error();
    return -1;
  }

  // Create video stream
  video_stream = avformat_new_stream(format_ctx, NULL);
  if (!video_stream){
    fprintf(stderr, "Could not create video stream\n");
    cleanup_on_error();
    return -1;
  }

  // Allocate codec context
  codec_ctx = avcodec_alloc_context3(codec);
  if(!codec_ctx){
    fprintf(stderr, "Could not allocate codec context\n");
    cleanup_on_error();
    return -1;
  }

  // Set codec parameters
  codec_ctx->width = config->width;
  codec_ctx->height = config->height;
  codec_ctx->time_base = (AVRational){1,config->fps};
  codec_ctx->framerate = (AVRational){config->fps,1};
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx->gop_size = 10;
  codec_ctx->max_b_frames = 1;

  // Open codec
  ret = avcodec_open2(codec_ctx, codec, NULL);
  if(ret < 0){
    fprintf(stderr, "Could not open codec\n");
    cleanup_on_error();
    return -1;
  }

  // Copy codec parameters to stream
  ret = avcodec_parameters_from_context(video_stream->codecpar, codec_ctx);
  if(ret<0){
    fprintf(stderr, "Could not copy codec parameters\n");
    cleanup_on_error();
    return -1;
  }

  // Allocate frame
  frame = av_frame_alloc();
  if(!frame){
    fprintf(stderr, "Could not allocate frame\n");
    cleanup_on_error();
    return -1;
  }
  frame->format = codec_ctx->pix_fmt;
  frame->width = codec_ctx->width;
  frame->height = codec_ctx->height;

  ret = av_frame_get_buffer(frame, 0);
  if(ret < 0){
    fprintf(stderr, "Could not allocate frame buffer\n");
    cleanup_on_error();
    return -1;
  }

  // Allocate packet
  packet = av_packet_alloc();
  if (!packet){
    fprintf(stderr, "Could not allocate packet\n");
    cleanup_on_error();
    return -1;
  }

  // Open output file
  ret = avio_open(&format_ctx->pb, config->output_filename, AVIO_FLAG_WRITE);
  if(ret < 0){
    fprintf(stderr, "Could not open output file\n");
    cleanup_on_error();
    return -1;
  }

  // Write file header
  ret = avformat_write_header(format_ctx, NULL);
  if(ret < 0){
    fprintf(stderr, "Could not write header\n");
    cleanup_on_error();
    return -1;
  }

  frame_count = 0;
  printf("Video encoder initialized: %dx%d @ %d fps\n",
         config->width, config->height, config->fps);
  return 0;
}

void video_close(void){
  // Write file trailer
  if(format_ctx){
    av_write_trailer(format_ctx);
  }

  // Free resources
  if(packet) av_packet_free(&packet);
  if(frame) av_frame_free(&frame);
  if(codec_ctx) avcodec_free_context(&codec_ctx);
  if(format_ctx) {
    avio_closep(&format_ctx->pb);
    avformat_free_context(format_ctx);
  }

  printf("Video encoder closed. Total frames: %d\n", frame_count);
}

static void cleanup_on_error(void){
  if(packet) av_packet_free(&packet);
  if(frame) av_frame_free(&frame);
  if(codec_ctx) avcodec_free_context(&codec_ctx);
  if(format_ctx) {
    avio_closep(&format_ctx->pb);
    avformat_free_context(format_ctx);
  }
}

int video_write_frame(uint8_t r, uint8_t g, uint8_t b){
  int ret;

  // Make frame writtable
  ret = av_frame_make_writable(frame);
  if(ret<0){
    fprintf(stderr, "Frame not writtable\n");
    return -1;
  }

  // Fill frame with solid color (YUV format)
  // Convert RGB to YUV - simplified formula
  int y = (int)(0.299 * r + 0.587 * g + 0.114 * b);
  int u = (int)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
  int v = (int)(0.500 * r - 0.419*g - 0.081 * b + 128);

  y = y < 0 ? 0 : (y > 255 ? 255 : y);
  u = u < 0 ? 0 : (u > 255 ? 255 : u);
  v = v < 0 ? 0 : (v > 255 ? 255 : v);

  // Fill Y plane
  for (int row = 0; row < codec_ctx->height; row++){
    memset(frame->data[0] + row * frame->linesize[0], y, codec_ctx->width);
  }

  // Fill U plane
  for (int row = 0; row < codec_ctx->height/2; row++){
    memset(frame->data[1] + row * frame->linesize[1], u, codec_ctx->width/2);
  }

  // Fill V plane
  for (int row = 0; row < codec_ctx->height/2; row++){
    memset(frame->data[2] + row * frame->linesize[2], v, codec_ctx->width/2);
  }

  // Set frame timestamp
  frame->pts = frame_count;

  // Send frame to encoder
  ret = avcodec_send_frame(codec_ctx, frame);
  if(ret < 0){
    fprintf(stderr, "Error sending frame\n");
    return -1;
  }

  // Receive encoded packets
  while(ret >= 0){
    ret = avcodec_receive_packet(codec_ctx, packet);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
      break;
    } else if (ret < 0){
      fprintf(stderr, "Error encoding frame\n");
      return -1;
    }

    // Write packet to file
    packet->stream_index = video_stream->index;
    av_packet_rescale_ts(packet, codec_ctx->time_base, video_stream->time_base);

    ret = av_interleaved_write_frame(format_ctx, packet);
    if(ret < 0){
      fprintf(stderr, "Error writing frame\n");
      return -1;
    }

    av_packet_unref(packet);
  }

  frame_count++;
  return 0;
}

int video_get_frame_count(void){
  return frame_count;
}
