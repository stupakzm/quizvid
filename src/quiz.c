#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "config.h"
#include "quiz.h"
#include "video.h"
#include "text.h"
#include "colors.h"

int quiz_load(QuizData *quiz, const char *json_file) {
    /* Read JSON file */
    struct json_object *root = json_object_from_file(json_file);
    if (!root) {
        fprintf(stderr, "Failed to parse JSON file: %s\n", json_file);
        return -1;
    }

    /* Get config */
    struct json_object *config;
    if (json_object_object_get_ex(root, "config", &config)) {
        struct json_object *duration;
        if (json_object_object_get_ex(config, "question_duration", &duration)) {
            quiz->question_duration = json_object_get_int(duration);
        }
        if (json_object_object_get_ex(config, "reveal_duration", &duration)) {
            quiz->reveal_duration = json_object_get_int(duration);
        }
    }

    /* Get questions array */
    struct json_object *questions_array;
    if (!json_object_object_get_ex(root, "questions", &questions_array)) {
        fprintf(stderr, "No 'questions' array in JSON\n");
        json_object_put(root);
        return -1;
    }

    quiz->num_questions = json_object_array_length(questions_array);
    quiz->questions = malloc(quiz->num_questions * sizeof(QuizQuestion));
    if (!quiz->questions) {
        fprintf(stderr, "Failed to allocate questions array\n");
        json_object_put(root);
        return -1;
    }

    /* Parse each question */
    for (int i = 0; i < quiz->num_questions; i++) {
        struct json_object *q_obj = json_object_array_get_idx(questions_array, i);
        QuizQuestion *q = &quiz->questions[i];

        /* Get question text */
        struct json_object *text;
        if (json_object_object_get_ex(q_obj, "question", &text)) {
            strncpy(q->question, json_object_get_string(text), MAX_QUESTION_LEN - 1);
            q->question[MAX_QUESTION_LEN - 1] = '\0';
        }

        /* Get answers */
        struct json_object *answers_array;
        if (json_object_object_get_ex(q_obj, "answers", &answers_array)) {
            q->num_answers = json_object_array_length(answers_array);
            if (q->num_answers > MAX_ANSWERS) q->num_answers = MAX_ANSWERS;

            for (int j = 0; j < q->num_answers; j++) {
                struct json_object *ans = json_object_array_get_idx(answers_array, j);
                strncpy(q->answers[j], json_object_get_string(ans), MAX_ANSWER_LEN - 1);
                q->answers[j][MAX_ANSWER_LEN - 1] = '\0';
            }
        }

        /* Get correct answer */
        struct json_object *correct;
        if (json_object_object_get_ex(q_obj, "correct", &correct)) {
            q->correct_answer = json_object_get_int(correct);
        }
    }

    json_object_put(root);
    printf("Loaded %d quiz questions from %s\n", quiz->num_questions, json_file);

    return 0;
}

void quiz_free(QuizData *quiz) {
    if (quiz->questions) {
        free(quiz->questions);
        quiz->questions = NULL;
    }
    quiz->num_questions = 0;
}

int quiz_render_frame(QuizData *quiz, int question_index,
                      float time_in_question,
                      uint8_t *rgb_buffer, int width, int height,
                      const LayoutConfig *layout,
                      const AnimationConfig *animation) {
    /* Validate inputs */
    if (question_index < 0 || question_index >= quiz->num_questions) {
        return -1;
    }

    QuizQuestion *q = &quiz->questions[question_index];

    /* Calculate timer progress (0.0 to 1.0) */
    float progress = time_in_question / (float)quiz->question_duration;
    if (progress > 1.0f) progress = 1.0f;

    /* Determine if answer should be revealed */
    int reveal = (time_in_question >= quiz->question_duration);

    /* Fill background with dark blue */
    video_fill_rgb_color(rgb_buffer, width, height, active_colors.background);

    /* Draw timer bar at top */
    video_draw_timer_bar(rgb_buffer, width, height, progress, layout->timer_bar_height);

  /* Calculate question alpha (fade in) */
    float question_start = animation->question_delay;
    float question_end = question_start + animation->question_fade_duration;
    float question_alpha = 0.0f;

    if (time_in_question >= question_end) {
        question_alpha = 1.0f;
    } else if (time_in_question > question_start) {
        question_alpha = (time_in_question - question_start) / animation->question_fade_duration;
    }

    /* Render question with fade */
    if (question_alpha > 0.0f) {
        TextContext text_ctx;
        if (text_init(&text_ctx, "assets/fonts/Roboto-Bold.ttf", layout->question_font_size) < 0) {
            return -1;
        }

        Color q_color = active_colors.question_text;
        text_render_centered_alpha(&text_ctx, rgb_buffer, width, height,
                                   q->question, layout->question_y_position,
                                   q_color.r, q_color.g, q_color.b, question_alpha);
        text_close(&text_ctx);
    }

    /* Initialize text context */
    TextContext text_ctx;
    if (text_init(&text_ctx, "assets/fonts/Roboto-Bold.ttf", layout->answer_font_size) < 0) {
        return -1;
    }

    char answer_text[MAX_ANSWER_LEN + 4];
    int button_width = width - (2 * layout->button_margin);

    for (int i = 0; i < q->num_answers; i++) {
     /* Calculate fade timing for this answer */
        float answer_start = question_end + (i * animation->answer_delay_between);
        float answer_end = answer_start + animation->answer_fade_duration;
        float answer_alpha = 0.0f;

        if (time_in_question >= answer_end) {
            answer_alpha = 1.0f;
        } else if (time_in_question > answer_start) {
            answer_alpha = (time_in_question - answer_start) / animation->answer_fade_duration;
        }

        /* Skip if not visible yet */
        if (answer_alpha <= 0.0f) continue;

        char letter = 'A' + i;
        snprintf(answer_text, sizeof(answer_text), "%c) %s", letter, q->answers[i]);

        int button_y = layout->answer_y_start + (i * layout->answer_spacing);

        /* Determine button background color */
        Color button_bg;
        if (reveal && i == q->correct_answer) {
            button_bg = active_colors.answer_button_correct;
        } else if (reveal && i != q->correct_answer) {
            button_bg = active_colors.answer_button_incorrect;
        } else {
            button_bg = active_colors.answer_button_normal;
        }

        /* Draw button background */
        video_draw_rounded_rect_alpha(rgb_buffer, width, height,
                               layout->button_margin, button_y,
                               button_width, layout->button_height,
                               layout->button_radius, button_bg, answer_alpha);

        /* Render text on top of button (centered vertically in button) */
        Color text_color = active_colors.answer_text;
        int text_y = button_y + (layout->button_height / 2) + 8;  /* Adjust for vertical centering */

        text_render_alpha(&text_ctx, rgb_buffer, width, height,
                   answer_text, layout->button_margin + layout->button_text_padding, text_y,
                   text_color.r, text_color.g, text_color.b, answer_alpha);
    }

    text_close(&text_ctx);

    return 0;
}
