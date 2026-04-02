#include <stdio.h>
#include <ctype.h>
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

    /* Load background audio if enabled */
    AudioSource *background_loop = NULL;
    if (config.audio.background.enabled && config.audio.background.file) {
        printf("Loading background audio: %s\n", config.audio.background.file);
        background_loop = audio_load_wav(config.audio.background.file);
        if (!background_loop) {
            fprintf(stderr, "Warning: Failed to load background audio\n");
        } else {
            printf("  Background loaded: %.2fs\n", background_loop->duration);
        }
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

    /* Render preview frame if configured */
    if (config.preview_category != NULL) {
        /* Fill background */
        video_fill_rgb_color(rgb_buffer, config.video.width, config.video.height, active_colors.background);

        /* Build uppercase category string */
        char upper_category[256];
        int i;
        for (i = 0; config.preview_category[i] && i < (int)(sizeof(upper_category) - 1); i++) {
            upper_category[i] = (char)toupper((unsigned char)config.preview_category[i]);
        }
        upper_category[i] = '\0';

        /* Render category name at font size 130, centered at y=900 */
        TextContext cat_ctx;
        text_init(&cat_ctx, config.font_path, 130);
        text_render_centered_alpha(&cat_ctx, rgb_buffer, config.video.width, config.video.height,
                                   upper_category, 900,
                                   active_colors.question_text.r,
                                   active_colors.question_text.g,
                                   active_colors.question_text.b, 1.0f);
        text_close(&cat_ctx);

        /* Render counter string at font size 80, centered at y=1020 */
        TextContext ctr_ctx;
        text_init(&ctr_ctx, config.font_path, 80);
        char counter_str[32];
        snprintf(counter_str, sizeof(counter_str), "#%d", config.preview_counter);
        text_render_centered_alpha(&ctr_ctx, rgb_buffer, config.video.width, config.video.height,
                                   counter_str, 1020,
                                   active_colors.question_text.r,
                                   active_colors.question_text.g,
                                   active_colors.question_text.b, 1.0f);
        text_close(&ctr_ctx);

        /* Write preview video frame */
        muxer_write_video_frame(muxer, rgb_buffer);

        /* Write 1 frame of silence to keep A/V in sync */
        int silence_samples = config.audio.sample_rate / config.video.fps;
        float *silence = calloc(silence_samples, sizeof(float));
        muxer_write_audio_samples(muxer, silence, silence_samples);
        free(silence);

        global_frame++;
        printf("Preview frame rendered: %s #%d\n", config.preview_category, config.preview_counter);
    }

      for (int q = 0; q < quiz.num_questions; q++) {
        QuizQuestion *question = &quiz.questions[q];
        int frames_for_question = (int)(question->total_duration * config.video.fps);

        printf("Question %d/%d: %s\n", q + 1, quiz.num_questions, question->question);
        printf("  Audio: %.2fs, Total: %.2fs (%d frames)\n",
               question->question_duration, question->total_duration, frames_for_question);

        /* Calculate timing phases */
        float audio_start_time = config.animation.question_delay +
                                config.animation.question_fade_duration;

        /* Build complete audio track for this question */
        AudioSource *question_audio_track = NULL;

        if (background_loop) {
            /* Create background for full question duration */
            AudioSource *bg_full = audio_adjust_duration(background_loop,
                                                        question->question_duration,
                                                        config.audio.sample_rate);

            if (question->audio && bg_full) {
                /* Create silence before voiceover */
                int silence_before = (int)(audio_start_time * config.audio.sample_rate);
                float *silence_start = calloc(silence_before, sizeof(float));

                /* Combine: silence + voiceover */
                int voice_total_samples = silence_before + question->audio->num_samples;
                float *voice_track = calloc(voice_total_samples, sizeof(float));
                if (voice_track && silence_start) {
                    memcpy(voice_track + silence_before, question->audio->samples,
                           question->audio->num_samples * sizeof(float));
                    free(silence_start);
                }

                /* Create temporary AudioSource for voice with silence */
                AudioSource voice_with_silence = {
                    .samples = voice_track,
                    .num_samples = voice_total_samples,
                    .sample_rate = config.audio.sample_rate,
                    .channels = 1,
                    .duration = (float)voice_total_samples / config.audio.sample_rate
                };

                /* Apply volume ducking to background */
                float voice_end_time = audio_start_time + question->audio->duration;
                for (int i = 0; i < bg_full->num_samples; i++) {
                    float time = (float)i / config.audio.sample_rate;
                    float bg_volume = (time >= audio_start_time && time < voice_end_time) ?
                                     config.audio.background.volume_with_voice :
                                     config.audio.background.volume_without_voice;
                    bg_full->samples[i] *= bg_volume;
                }

                /* Mix voice and background */
                question_audio_track = audio_mix(&voice_with_silence, 1.0f, bg_full, 1.0f);

                free(voice_track);
                audio_free(bg_full);

                if (question_audio_track) {
                    int reveal_samples = (int)(question->reveal_duration * config.audio.sample_rate);
                    int new_total = question_audio_track->num_samples + reveal_samples;
                    float *extended = calloc(new_total, sizeof(float));
                    if (extended) {
                        memcpy(extended, question_audio_track->samples,
                               question_audio_track->num_samples * sizeof(float));
                        free(question_audio_track->samples);
                        question_audio_track->samples = extended;
                        question_audio_track->num_samples = new_total;
                        question_audio_track->duration = question->total_duration;
                    }
                }
            } else if (bg_full) {
                /* No voice, just background */
                for (int i = 0; i < bg_full->num_samples; i++) {
                    bg_full->samples[i] *= config.audio.background.volume_without_voice;
                }
                question_audio_track = bg_full;
            }
        } else if (question->audio) {
            /* No background, just add silence before voice */
            int silence_before = (int)(audio_start_time * config.audio.sample_rate);
            int total_samples = (int)(question->total_duration * config.audio.sample_rate);

            question_audio_track = calloc(1, sizeof(AudioSource));
            question_audio_track->samples = calloc(total_samples, sizeof(float));
            question_audio_track->num_samples = total_samples;
            question_audio_track->sample_rate = config.audio.sample_rate;
            question_audio_track->channels = 1;
            question_audio_track->duration = question->total_duration;

            memcpy(question_audio_track->samples + silence_before,
                   question->audio->samples,
                   question->audio->num_samples * sizeof(float));
        }

        /* Write mixed audio */
        if (question_audio_track) {
            muxer_write_audio_samples(muxer, question_audio_track->samples,
                                     question_audio_track->num_samples);
            audio_free(question_audio_track);
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
    if (background_loop) audio_free(background_loop);
    audio_cleanup();
    config_free(&config);

    printf("\nQuiz video with audio created successfully!\n");
    printf("Output: %s\n", config.output_file);
    printf("Total duration: %.1f seconds\n", total_duration);

    return 0;
}
