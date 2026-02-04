#ifndef VIDEO_H
#define VIDEO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include "colors.h"

/* Video configuration structure */
typedef struct{
  int width;
  int height;
  int fps;
  const char *output_filename;
} VideoConfig;

/* Initialize video encoder */
int video_init(VideoConfig *config);

/* Write a single frame with solid color */
int video_write_frame(uint8_t r, uint8_t g, uint8_t b);

/* Write a frame from RGB buffer */
int video_write_frame_rgb(uint8_t *rgb_buffer);

/* FIll RGB buffer with color */
void video_fill_rgb_color(uint8_t *rgb_buffer, int width, int height, Color color);

/* Fill RGB buffer with solid color */
void video_fill_rgb(uint8_t *rgb_buffer, int width, int height,
                    uint8_t r, uint8_t g, uint8_t b);

/* Get total frames written */
int video_get_frame_count(void);

/* Draw filled rectangle on RGB buffer */
void video_draw_rect(uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                     int x, int y, int width, int height,
                     uint8_t r, uint8_t g, uint8_t b);

/* Draw timer bar (progress indicator) */
void video_draw_timer_bar(uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                          float progress);

/*Close video encoder and write file*/
void video_close(void);
#endif // VIDEO_H
