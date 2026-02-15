#include <stdio.h>
#include "video.h"
#include "text.h"
#include "quiz.h"
#include "colors.h"
#include "config.h"

int main(int argc, char *argv[]) {
    const char *config_file = "config.json";

    /* Allow config file as command line argument */
    if (argc > 1) {
        config_file = argv[1];
    }

    printf("QuizVid - Generating Quiz Video\n\n");

    /* Load configuration */
    AppConfig config;
    config_load(&config, config_file);

    /* Apply configuration (sets colors) */
    config_apply(&config);

    /* Load quiz data */
    QuizData quiz = {0};
    if (quiz_load(&quiz, config.quiz_file) < 0) {
        fprintf(stderr, "Failed to load quiz\n");
        config_free(&config);
        return 1;
    }

    /* Configure video using config */
    VideoConfig video_config = {
        .width = config.video.width,
        .height = config.video.height,
        .fps = config.video.fps,
        .output_filename = config.output_file
    };

    /* Initialize video encoder */
    if (video_init(&video_config) < 0) {
        fprintf(stderr, "Failed to initialize video encoder\n");
        quiz_free(&quiz);
        config_free(&config);
        return 1;
    }

    /* Allocate RGB buffer */
    size_t buffer_size = config.video.width * config.video.height * 3;
    uint8_t *rgb_buffer = malloc(buffer_size);
    if (!rgb_buffer) {
        fprintf(stderr, "Failed to allocate RGB buffer\n");
        video_close();
        quiz_free(&quiz);
        config_free(&config);
        return 1;
    }

    /* Generate video for each question */
    int total_duration = quiz.question_duration + quiz.reveal_duration;
    int frames_per_question = total_duration * config.video.fps;
    int total_frames = frames_per_question * quiz.num_questions;

    printf("Generating %d questions (%d frames total)...\n",
           quiz.num_questions, total_frames);

    int frame = 0;
    for (int q = 0; q < quiz.num_questions; q++) {
        printf("Question %d/%d: %s\n", q + 1, quiz.num_questions,
               quiz.questions[q].question);

        for (int f = 0; f < frames_per_question; f++) {
            float time = (float)f / config.video.fps;

            /* Render quiz frame with layout config */
            if (quiz_render_frame(&quiz, q, time, rgb_buffer,
                                 config.video.width, config.video.height,
                                 &config.layout, &config.animation) < 0) {
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
            if ((frame % config.video.fps) == 0) {
                printf("  Progress: %d/%d frames (%.1f seconds)\n",
                       frame, total_frames, (float)frame / config.video.fps);
            }
        }
    }

    /* Cleanup */
    free(rgb_buffer);
    video_close();
    quiz_free(&quiz);
    config_free(&config);

    printf("\nQuiz video created successfully!\n");
    printf("Total duration: %d seconds\n", total_frames / config.video.fps);

    return 0;
}
