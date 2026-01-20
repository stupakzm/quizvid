#include <stdio.h>
#include "video.h"

int main(int argc, char *argv[]){
  (void) argc;
  (void) argv;

  printf("QuizVid - Text to Video Generator\n");
  printf("FFmpeg version: %s\n\n",av_version_info());

  /* Configure video for vertical format */
  VideoConfig config = {
    .width = 1080,
    .height = 1920,
    .fps = 30,
    .output_filename = "test_video.mp4"
  };

  /* Initialize encoder */
  if (video_init(&config) < 0){
    fprintf(stderr, "Failed to initialize video encoder\n");
    return -1;
  }

  /* Generate 3 sec video */
  int total_frames = 3 * config.fps;

  printf("Generating %d frames...\n", total_frames);

  for (int i = 0; i < total_frames; i++){
    if(video_write_frame(255,0,0) < 0){
      fprintf(stderr, "Failed to write frame %d\n", i);
      video_close();
      return -1;
    }

    if((i+1) % 30 == 0){
      printf("Progress: %d/%d frames\n", i+1, total_frames);
    }
  }

  video_close();

  printf("\nVideo created successfully: %s \n", config.output_filename);

  return 0;
}
