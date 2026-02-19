#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

/* RGB Color structure */
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

/* Color scheme configuration */
typedef struct {
    /* Background colors */
    Color background;
    Color timer_background;

    /* Timer bar colors */
    Color timer_fill;
    Color timer_text;

    /* Text colors */
    Color question_text;
    Color answer_text;
    Color answer_correct;
    Color answer_incorrect;  /* For future use */

    /* Answer button background */
    Color answer_button_normal;
    Color answer_button_correct;
    Color answer_button_incorrect;

    /* UI colors */
    Color accent;  /* For future decorative elements */
} ColorScheme;

/* Predefined color schemes */
extern const ColorScheme COLOR_SCHEME_GRAYSCALE;
extern const ColorScheme COLOR_SCHEME_COLORBLIND;
extern const ColorScheme COLOR_SCHEME_COLORBLIND_ALTERNATIVE;
extern const ColorScheme COLOR_SCHEME_DEFAULT;

/* Active color scheme (can be changed at runtime) */
extern ColorScheme active_colors;

/* Initialize with a specific scheme */
void colors_init(const ColorScheme *scheme);

/* Helper to create Color from RGB values */
static inline Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    Color c = {r, g, b};
    return c;
}

#endif // COLORS_H
