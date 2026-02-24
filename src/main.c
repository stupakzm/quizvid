#include <stdio.h>
#include "video.h"
#include "text.h"
#include "quiz.h"
#include "colors.h"
#include "config.h"
#include "muxer.h"

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

    /* Generate audio for all questions */
    if (quiz_generate_audio(&quiz, config.audio.enabled,
                           &config.timing,
                           &config.animation) < 0) {
        fprintf(stderr, "Failed to generate audio\n");
        quiz_free(&quiz);
        config_free(&config);
        audio_cleanup();
        return 1;
    }

/* Initialize muxer with video + audio */
    MuxerContext *muxer = muxer_init(
        config.output_file,
        config.video.width,
        config.video.height,
        config.video.fps,
        config.audio.sample_rate,
        1  /* mono audio */
    );

    if (!muxer) {
        fprintf(stderr, "Failed to initialize muxer\n");
        quiz_free(&quiz);
        audio_cleanup();
        config_free(&config);
        return 1;
    }

    /* Allocate RGB buffer */
    size_t buffer_size = config.video.width * config.video.height * 3;
    uint8_t *rgb_buffer = malloc(buffer_size);
    if (!rgb_buffer) {
        fprintf(stderr, "Failed to allocate RGB buffer\n");
        muxer_free(muxer);
        quiz_free(&quiz);
        audio_cleanup();
        config_free(&config);
        return 1;
    }

    /* Calculate total video info */
    int total_frames = 0;
    float total_duration = 0.0f;
    for (int q = 0; q < quiz.num_questions; q++) {
        total_frames += (int)(quiz.questions[q].total_duration * config.video.fps);
        total_duration += quiz.questions[q].total_duration;
    }

    printf("Generating %d questions (%.1fs, %d frames total)...\n\n",
           quiz.num_questions, total_duration, total_frames);

    /* Generate video with audio for each question */
    int global_frame = 0;

      for (int q = 0; q < quiz.num_questions; q++) {
        QuizQuestion *question = &quiz.questions[q];
        int frames_for_question = (int)(question->total_duration * config.video.fps);

        printf("Question %d/%d: %s\n", q + 1, quiz.num_questions, question->question);
        printf("  Audio: %.2fs, Total: %.2fs (%d frames)\n",
               question->question_duration, question->total_duration, frames_for_question);

        /* Calculate when audio should start (after animations complete) */
        float animation_complete_time = config.animation.question_delay +
                                       config.animation.question_fade_duration +
                                       (question->num_answers - 1) * config.animation.answer_delay_between +
                                       config.animation.answer_fade_duration;
        float audio_start_time = config.animation.question_delay +
                                       config.animation.question_fade_duration;

        /* Write silence until animations complete */
        int silence_before_audio = (int)(audio_start_time * config.audio.sample_rate);
        if (silence_before_audio > 0) {
            float *silence_start = calloc(silence_before_audio, sizeof(float));
            if (silence_start) {
                muxer_write_audio_samples(muxer, silence_start, silence_before_audio);
                free(silence_start);
            }
        }

        /* Write actual audio */
        if (question->audio) {
            muxer_write_audio_samples(muxer, question->audio->samples,
                                     question->audio->num_samples);
        }

        /* Calculate silence for remainder of question phase + reveal phase */
        float audio_end_time = audio_start_time + (question->audio ? question->audio->duration : 0.0f);
        float silence_duration = question->total_duration - audio_end_time;

        if (silence_duration > 0) {
            int silence_samples_end = (int)(silence_duration * config.audio.sample_rate);
            float *silence_end = calloc(silence_samples_end, sizeof(float));
            if (silence_end) {
                muxer_write_audio_samples(muxer, silence_end, silence_samples_end);
                free(silence_end);
            }
        }

        /* Generate frames for this question */
        for (int f = 0; f < frames_for_question; f++) {
            float time = (float)f / config.video.fps;

            /* Just render and write video - audio already written */
            if (quiz_render_frame(&quiz, q, time, rgb_buffer,
                                 config.video.width, config.video.height,
                                 &config.layout, &config.animation) < 0) {
                fprintf(stderr, "Failed to render frame\n");
                break;
            }

            if (muxer_write_video_frame(muxer, rgb_buffer) < 0) {
                fprintf(stderr, "Failed to write frame %d\n", global_frame);
                break;
            }

            global_frame++;

            if ((global_frame % config.video.fps) == 0) {
                printf("  Progress: %d/%d frames (%.1fs)\n",
                       global_frame, total_frames,
                       (float)global_frame / config.video.fps);
            }
        }

        printf("\n");
    }

    /* Finalize muxer */
    printf("Finalizing video...\n");
    if (muxer_finalize(muxer) < 0) {
        fprintf(stderr, "Failed to finalize muxer\n");
    }

    /* Cleanup */
    free(rgb_buffer);
    muxer_free(muxer);
    quiz_free(&quiz);
    audio_cleanup();
    config_free(&config);

    printf("\nQuiz video with audio created successfully!\n");
    printf("Output: %s\n", config.output_file);
    printf("Total duration: %.1f seconds\n", total_duration);

    return 0;
}
