#include "colors.h"

/* Active color scheme (global) */
ColorScheme active_colors;

/* Grayscale color scheme (minimalistic) */
const ColorScheme COLOR_SCHEME_GRAYSCALE = {
    .background = {15, 15, 15},           /* Very dark gray */
    .timer_background = {40, 40, 40},     /* Dark gray */
    .timer_fill = {200, 200, 200},        /* Light gray */
    .timer_text = {255, 255, 255},        /* White */
    .question_text = {255, 255, 255},     /* White */
    .answer_text = {200, 200, 200},       /* Light gray */
    .answer_correct = {255, 255, 255},    /* White (bold highlight) */
    .answer_incorrect = {100, 100, 100},  /* Medium gray */
    .accent = {180, 180, 180}             /* Light gray */
};

/* Colorblind-friendly scheme (gold and purple) */
const ColorScheme COLOR_SCHEME_COLORBLIND = {
    .background = {20, 15, 35},           /* Dark purple-blue */
    .timer_background = {40, 35, 50},     /* Darker purple */
    .timer_fill = {255, 200, 50},         /* Gold */
    .timer_text = {255, 255, 255},        /* White */
    .question_text = {255, 255, 255},     /* White */
    .answer_text = {220, 220, 255},       /* Light purple-white */
    .answer_correct = {255, 215, 0},      /* Bright gold */
    .answer_incorrect = {150, 130, 180},  /* Muted purple */
    .accent = {180, 120, 255}             /* Purple accent */
};

/* Default scheme (original blue theme) */
const ColorScheme COLOR_SCHEME_DEFAULT = {
    .background = {20, 30, 60},           /* Dark blue */
    .timer_background = {40, 40, 40},     /* Dark gray */
    .timer_fill = {0, 255, 0},            /* Green */
    .timer_text = {255, 255, 255},        /* White */
    .question_text = {255, 255, 255},     /* White */
    .answer_text = {255, 255, 255},       /* White */
    .answer_correct = {0, 255, 0},        /* Green */
    .answer_incorrect = {255, 50, 50},    /* Red */
    .accent = {0, 150, 255}               /* Blue */
};

void colors_init(const ColorScheme *scheme) {
    if (scheme) {
        active_colors = *scheme;
    } else {
        /* Default to colorblind-friendly if none specified */
        active_colors = COLOR_SCHEME_COLORBLIND;
    }
}
