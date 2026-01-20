#include <stdio.h>
#include "video.h"

int main(int argc, char *argv[]){
  (void)argc;
  (void)argv;
  printf("QuizVid - TExt to Video Generator\n");
  printf("FFmpeg version: %s\n", av_version_info());
  return 0;
}
