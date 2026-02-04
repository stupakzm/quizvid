#include <stdio.h>
#include "video.h"
#include "text.h"
#include "quiz.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("QuizVid - Generating Quiz Video\n\n");

    /* Load quiz data */
    QuizData quiz = {0};
    if (quiz_load(&quiz, "examples/sample_quiz.json") < 0) {
        fprintf(stderr, "Failed to load quiz\n");
        return 1;
    }

    /* Configure video */
    VideoConfig config = {
        .width = 1080,
        .height = 1920,
        .fps = 30,
        .output_filename = "quiz_video.mp4"
    };

    /* Initialize video encoder */
    if (video_init(&config) < 0) {
        fprintf(stderr, "Failed to initialize video encoder\n");
        quiz_free(&quiz);
        return 1;
    }

    /* Allocate RGB buffer */
    size_t buffer_size = config.width * config.height * 3;
    uint8_t *rgb_buffer = malloc(buffer_size);
    if (!rgb_buffer) {
        fprintf(stderr, "Failed to allocate RGB buffer\n");
        video_close();
        quiz_free(&quiz);
        return 1;
    }

    /* Generate video for each question */
    int total_duration = quiz.question_duration + quiz.reveal_duration;
    int frames_per_question = total_duration * config.fps;
    int total_frames = frames_per_question * quiz.num_questions;

    printf("Generating %d questions (%d frames total)...\n",
           quiz.num_questions, total_frames);

    int frame = 0;
    for (int q = 0; q < quiz.num_questions; q++) {
        printf("Question %d/%d: %s\n", q + 1, quiz.num_questions,
               quiz.questions[q].question);

        for (int f = 0; f < frames_per_question; f++) {
            float time = (float)f / config.fps;

            /* Render quiz frame */
            if (quiz_render_frame(&quiz, q, time, rgb_buffer,
                                 config.width, config.height) < 0) {
                fprintf(stderr, "Failed to render frame\n");
                break;
            }

            /* Write frame to video */
            if (video_write_frame_rgb(rgb_buffer) < 0) {
                fprintf(stderr, "Failed to write frame %d\n", frame);
                break;
            }

            frame++;

            /* Progress every second */
            if ((frame % config.fps) == 0) {
                printf("  Progress: %d/%d frames (%.1f seconds)\n",
                       frame, total_frames, (float)frame / config.fps);
            }
        }
    }

    /* Cleanup */
    free(rgb_buffer);
    video_close();
    quiz_free(&quiz);

    printf("\nQuiz video created successfully: %s\n", config.output_filename);
    printf("Total duration: %d seconds\n", total_frames / config.fps);

    return 0;
}
