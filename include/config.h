#ifndef CONFIG_H
#define CONFIG_H

#include "colors.h"

/* Layout configuration */
typedef struct {
    /* Question layout */
    int question_font_size;
    int question_y_position;

    /* Answer button layout */
    int answer_font_size;
    int answer_y_start;
    int answer_spacing;
    int button_margin;
    int button_height;
    int button_radius;
    int button_text_padding;

    /* Timer bar */
    int timer_bar_height;
} LayoutConfig;

/* Video configuration */
typedef struct {
    int width;
    int height;
    int fps;
} VideoSettings;

/* Complete application configuration */
typedef struct {
    VideoSettings video;
    LayoutConfig layout;
    const char *color_scheme;  /* "grayscale", "colorblind", "default" */
    const char *font_path;
    const char *quiz_file;
    const char *output_file;
} AppConfig;

/* Load configuration from JSON file */
int config_load(AppConfig *config, const char *config_file);

/* Free configuration resources */
void config_free(AppConfig *config);

/* Apply loaded configuration (set active colors, etc.) */
int config_apply(const AppConfig *config);

/* Get default configuration */
AppConfig config_get_default(void);

#endif // CONFIG_H
