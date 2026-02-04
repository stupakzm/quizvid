#ifndef QUIZ_H
#define QUIZ_H

#include <stdint.h>

/* Maximum lengths */
#define MAX_QUESTION_LEN 256
#define MAX_ANSWER_LEN 128
#define MAX_ANSWERS 4

/* Single quiz question */
typedef struct {
    char question[MAX_QUESTION_LEN];
    char answers[MAX_ANSWERS][MAX_ANSWER_LEN];
    int correct_answer;  /* 0-3 (A-D) */
    int num_answers;     /* How many answers (2-4) */
} QuizQuestion;

/* Quiz configuration */
typedef struct {
    QuizQuestion *questions;
    int num_questions;
    int question_duration;  /* seconds per question */
    int reveal_duration;    /* seconds to show answer */
} QuizData;

/* Load quiz from JSON file */
int quiz_load(QuizData *quiz, const char *json_file);

/* Free quiz data */
void quiz_free(QuizData *quiz);

/* Render quiz frame at specific time */
int quiz_render_frame(QuizData *quiz, int question_index,
                      float time_in_question,
                      uint8_t *rgb_buffer, int width, int height);

#endif // QUIZ_H
