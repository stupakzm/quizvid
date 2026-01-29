#include <stdio.h>
#include "video.h"
#include "text.h"
#include "quiz.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("QuizVid - Quiz JSON Parser Test\n\n");

    /* Load quiz data */
    QuizData quiz = {0};
    if (quiz_load(&quiz, "examples/sample_quiz.json") < 0) {
        fprintf(stderr, "Failed to load quiz\n");
        return 1;
    }

    /* Print loaded data */
    printf("Quiz Configuration:\n");
    printf("  Question duration: %d seconds\n", quiz.question_duration);
    printf("  Reveal duration: %d seconds\n", quiz.reveal_duration);
    printf("  Number of questions: %d\n\n", quiz.num_questions);

    /* Print each question */
    for (int i = 0; i < quiz.num_questions; i++) {
        QuizQuestion *q = &quiz.questions[i];
        printf("Question %d: %s\n", i + 1, q->question);

        for (int j = 0; j < q->num_answers; j++) {
            char letter = 'A' + j;
            const char *marker = (j == q->correct_answer) ? " ✓" : "";
            printf("  %c) %s%s\n", letter, q->answers[j], marker);
        }
        printf("\n");
    }

    /* Cleanup */
    quiz_free(&quiz);
    printf("Quiz data freed successfully.\n");

    return 0;
}
