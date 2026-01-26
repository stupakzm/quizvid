#include <stdio.h>
#include "video.h"
#include "text.h"

int main(int argc, char *argv[]){
  (void)argc;
  (void)argv;

  printf("QuizVid - Text to Video Generator\n");
  printf("FFmpeg verison: %s\n\n", av_version_info());

  VideoConfig config = {
    .width = 1080,
    .height = 1920,
    .fps = 30,
    .output_filename = "test_text.mp4"
  };

  /* Initialize video encoder */
  if (video_init(&config) < 0) {
    fprintf(stderr, "Failed to initialize video encoder\n");
    return 1;
  }

  TextContext text_ctx;
  if(text_init(&text_ctx, "assets/fonts/Roboto-Bold.ttf", 72) < 0){
    fprintf(stderr, "Failed to initialize text renderer\n");
    video_close();
    return 1;
  }

  size_t buffer_size = config.width * config.height * 3;
  uint8_t *rgb_buffer = malloc(buffer_size);
  if(!rgb_buffer){
    fprintf(stderr, "Failed to allocate RGB buffer\n");
    text_close(&text_ctx);
    video_close();
    return -1;
  }

  int total_frames = 3 * config.fps;
  printf("Generating %d frames with text...\n", total_frames);

  for(int i = 0; i < total_frames; i++){
    printf("Frame %d: Filling background...\n", i);

    video_fill_rgb(rgb_buffer, config.width, config.height, 30, 60, 120);

    printf("Frame %d: Rendering text...\n", i);

    text_render(&text_ctx, rgb_buffer, config.width, config.height,
                "Hello QuizVid ! 12:3.", 300, 960,
                255, 255, 255);

    printf("Frame %d: Writing frame...\n", i);

    if(video_write_frame_rgb(rgb_buffer) < 0){
      fprintf(stderr, "Failed to write frame %d\n", i);
      break;
    }

    if((i + 1) % 30 == 0){
      printf("Progress: %d/%d frames\n", i+1, total_frames);
    }
  }

  free(rgb_buffer);
  text_close(&text_ctx);
  video_close();

  printf("\nVideo created successfully: %s\n", config.output_filename);

  return 0;
}
