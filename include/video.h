#ifndef VIDEO_H
#define VIDEO_H

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

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

/* Get total frames written */
int video_get_frame_count(void);

/*Close video encoder and write file*/
void video_close(void);
#endif // VIDEO_H
