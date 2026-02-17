#ifndef QUIZ_H
#define QUIZ_H

#include <stdint.h>
#include "config.h"
/* Maximum lengths */
#define MAX_QUESTION_LEN 256
#define MAX_ANSWER_LEN 128
#define MAX_ANSWERS 6

typedef enum {
    QUIZ_TYPE_STANDARD,
    QUIZ_TYPE_TRUEFALSE,
    QUIZ_TYPE_MULTI
} QuizType;

/* Single quiz question */
typedef struct {
    QuizType type;
    char question[MAX_QUESTION_LEN];
    char answers[MAX_ANSWERS][MAX_ANSWER_LEN];
    int correct_answers[MAX_ANSWERS];
    int num_correct;
    int num_answers;
} QuizQuestion;

/* Quiz configuration */
typedef struct {
    QuizQuestion *questions;
    int num_questions;
    int question_duration;
    int reveal_duration;
} QuizData;

/* Load quiz from JSON file */
int quiz_load(QuizData *quiz, const char *json_file);

/* Free quiz data */
void quiz_free(QuizData *quiz);

/* Render quiz frame at specific time */
int quiz_render_frame(QuizData *quiz, int question_index,
                      float time_in_question,
                      uint8_t *rgb_buffer, int width, int height,
                      const LayoutConfig *layout,
                      const AnimationConfig *animation);

#endif // QUIZ_H
