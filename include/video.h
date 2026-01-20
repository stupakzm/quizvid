#ifndef VIDEO_H_
#define VIDEO_H_

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

/*Close video encoder and write file*/
void video_close(void);

#endif // VIDEO_H_
