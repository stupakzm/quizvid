#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "video.h"
#include "colors.h"

// Global state for video encoding
static AVFormatContext *format_ctx = NULL;
static AVCodecContext *codec_ctx = NULL;
static AVStream *video_stream = NULL;
static AVFrame *frame = NULL;
static AVPacket *packet = NULL;
static int frame_count = 0;

static void cleanup_on_error(void);
static int rgb_to_yuv_frame(uint8_t *rgb_buffer, int width, int height);
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

static int rgb_to_yuv_frame(uint8_t *rgb_buffer, int width, int height){
  int ret = av_frame_make_writable(frame);
  if (ret < 0){
    fprintf(stderr, "Frame not writtable\n");
    return -1;
  }

  /* Convert RGB to YUV */
  for (int y = 0; y < height; y++){
    for(int x = 0; x < width; x ++){
      int rgb_index = (y * width + x) * 3;
      uint8_t r = rgb_buffer[rgb_index + 0];
      uint8_t g = rgb_buffer[rgb_index + 1];
      uint8_t b = rgb_buffer[rgb_index + 2];

      int yval = (int)(0.299 * r + 0.587 * g + 0.114 * b);
      int uval = (int)(-0.169 * r - 0.331 * g + 0.500 * b + 128);
      int vval = (int)(0.500 * r - 0.419 * g - 0.081 * b + 128);

      yval = yval < 0 ? 0 : (yval > 255 ? 255 : yval);
      uval = uval < 0 ? 0 : (uval > 255 ? 255 : uval);
      vval = vval < 0 ? 0 : (vval > 255 ? 255 : vval);

      frame->data[0][y * frame->linesize[0] + x] = yval;

      if(y % 2 == 0 && x % 2 == 0){
        int uv_x = x / 2;
        int uv_y = y / 2;
        frame->data[1][uv_y * frame->linesize[1] + uv_x] = uval;
        frame->data[2][uv_y * frame->linesize[2] + uv_x] = vval;
      }
    }
  }
  return 0;
}

int video_write_frame_rgb(uint8_t *rgb_buffer){
  int ret;

  ret = rgb_to_yuv_frame(rgb_buffer, codec_ctx->width, codec_ctx->height);
  if(ret < 0){
    fprintf(stderr, "Failed convert RGB to YUV\n");
    return -1;
  }

  frame->pts = frame_count;

  ret = avcodec_send_frame(codec_ctx, frame);
  if(ret < 0){
    fprintf(stderr, "Error sending frame\n");
    return -1;
  }

  while (ret >= 0){
    ret = avcodec_receive_packet(codec_ctx, packet);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
      break;
    } else if (ret < 0){
      fprintf(stderr, "Error encoding frame\n");
      return -1;
    }

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

/* Fill RGB buffer with solid color */
void video_fill_rgb_color(uint8_t *rgb_buffer, int width, int height, Color color) {
    int total_pixels = width * height;
    for (int i = 0; i < total_pixels; i++) {
        rgb_buffer[i * 3 + 0] = color.r;
        rgb_buffer[i * 3 + 1] = color.g;
        rgb_buffer[i * 3 + 2] = color.b;
    }
}

/* Keep old function for backward compatibility */
void video_fill_rgb(uint8_t *rgb_buffer, int width, int height,
                    uint8_t r, uint8_t g, uint8_t b) {
    Color c = {r, g, b};
    video_fill_rgb_color(rgb_buffer, width, height, c);
}

void video_draw_rect(uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                     int x, int y, int width, int height,
                     uint8_t r, uint8_t g, uint8_t b) {
    for (int row = y; row < y + height && row < buffer_height; row++) {
        for (int col = x; col < x + width && col < buffer_width; col++) {
            if (row >= 0 && col >= 0) {
                int index = (row * buffer_width + col) * 3;
                rgb_buffer[index + 0] = r;
                rgb_buffer[index + 1] = g;
                rgb_buffer[index + 2] = b;
            }
        }
    }
}

void video_draw_timer_bar(uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                          float progress, int bar_height) {
    const int bar_y = 0;

    /* Clamp progress to 0.0-1.0 */
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    int fill_width = (int)(buffer_width * progress);

    /* Draw background using color scheme */
    Color bg = active_colors.timer_background;
    video_draw_rect(rgb_buffer, buffer_width, buffer_height,
                    0, bar_y, buffer_width, bar_height,
                    bg.r, bg.g, bg.b);

    /* Draw filled portion using color scheme */
    if (fill_width > 0) {
        Color fill = active_colors.timer_fill;
        video_draw_rect(rgb_buffer, buffer_width, buffer_height,
                        0, bar_y, fill_width, bar_height,
                        fill.r, fill.g, fill.b);
    }
}

/* Helper: Blend color with alpha onto buffer */
static inline void blend_pixel(uint8_t *buffer, int index, Color color, float alpha) {
    if (alpha <= 0.0f) return;
    if (alpha >= 1.0f) {
        buffer[index + 0] = color.r;
        buffer[index + 1] = color.g;
        buffer[index + 2] = color.b;
        return;
    }

    buffer[index + 0] = (uint8_t)(color.r * alpha + buffer[index + 0] * (1.0f - alpha));
    buffer[index + 1] = (uint8_t)(color.g * alpha + buffer[index + 1] * (1.0f - alpha));
    buffer[index + 2] = (uint8_t)(color.b * alpha + buffer[index + 2] * (1.0f - alpha));
}

void video_draw_rounded_rect_alpha(uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                                   int x, int y, int width, int height, int radius,
                                   Color color, float alpha) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    /* Draw all pixels in one pass */
    for (int row = y; row < y + height && row < buffer_height; row++) {
        for (int col = x; col < x + width && col < buffer_width; col++) {
            if (row < 0 || col < 0) continue;

            /* Check if pixel is inside rounded rectangle */
            int in_rect = 0;

            /* Check corners */
            if ((row < y + radius && col < x + radius) ||
                (row < y + radius && col >= x + width - radius) ||
                (row >= y + height - radius && col < x + radius) ||
                (row >= y + height - radius && col >= x + width - radius)) {
                /* In corner region - check circle distance */
                int cx, cy;
                if (row < y + radius && col < x + radius) {
                    cx = x + radius; cy = y + radius;
                } else if (row < y + radius && col >= x + width - radius) {
                    cx = x + width - radius; cy = y + radius;
                } else if (row >= y + height - radius && col < x + radius) {
                    cx = x + radius; cy = y + height - radius;
                } else {
                    cx = x + width - radius; cy = y + height - radius;
                }

                int dx = col - cx;
                int dy = row - cy;
                if (dx * dx + dy * dy <= radius * radius) {
                    in_rect = 1;
                }
            } else {
                /* Not in corner region - always inside */
                in_rect = 1;
            }

            if (in_rect) {
                int index = (row * buffer_width + col) * 3;
                blend_pixel(rgb_buffer, index, color, alpha);
            }
        }
    }
}
