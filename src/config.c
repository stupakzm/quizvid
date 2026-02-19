#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "config.h"
#include "colors.h"

/* Helper to get int from JSON object */
static int get_json_int(struct json_object *obj, const char *key, int default_value) {
    struct json_object *value;
    if (json_object_object_get_ex(obj, key, &value)) {
        return json_object_get_int(value);
    }
    return default_value;
}

/* Helper to get string from JSON object */
static const char *get_json_string(struct json_object *obj, const char *key, const char *default_value) {
    struct json_object *value;
    if (json_object_object_get_ex(obj, key, &value)) {
        return json_object_get_string(value);
    }
    return default_value;
}

/* Helper to duplicate string */
static char *strdup_safe(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

AppConfig config_get_default(void) {
    AppConfig config = {
        .video = {1080, 1920, 30},
        .layout = {
            .question_font_size = 64,
            .question_y_position = 400,
            .answer_font_size = 48,
            .answer_y_start = 700,
            .answer_spacing = 150,
            .button_margin = 100,
            .button_height = 120,
            .button_radius = 20,
            .button_text_padding = 40,
            .timer_bar_height = 80
        },
        .animation = {
          .question_fade_duration = 0.5f,
          .answer_fade_duration = 0.3f,
          .answer_delay_between = 0.3f,
          .question_delay = 0.0f
        },
        .color_scheme = "colorblind",
        .font_path = "assets/fonts/Roboto-Bold.ttf",
        .quiz_file = "examples/sample_quiz.json",
        .output_file = "quiz_video.mp4"
    };
    return config;
}

int config_load(AppConfig *config, const char *config_file) {
    /* Read JSON file */
    struct json_object *root = json_object_from_file(config_file);
    if (!root) {
        fprintf(stderr, "Failed to parse config file: %s\n", config_file);
        fprintf(stderr, "Using default configuration.\n");
        *config = config_get_default();
        return -1;
    }

    /* Start with defaults */
    *config = config_get_default();

    /* Parse video settings */
    struct json_object *video;
    if (json_object_object_get_ex(root, "video", &video)) {
        config->video.width = get_json_int(video, "width", 1080);
        config->video.height = get_json_int(video, "height", 1920);
        config->video.fps = get_json_int(video, "fps", 30);
    }

    /* Parse layout settings */
    struct json_object *layout;
    if (json_object_object_get_ex(root, "layout", &layout)) {
        config->layout.question_font_size = get_json_int(layout, "question_font_size", 64);
        config->layout.question_y_position = get_json_int(layout, "question_y_position", 400);
        config->layout.answer_font_size = get_json_int(layout, "answer_font_size", 48);
        config->layout.answer_y_start = get_json_int(layout, "answer_y_start", 700);
        config->layout.answer_spacing = get_json_int(layout, "answer_spacing", 150);
        config->layout.button_margin = get_json_int(layout, "button_margin", 100);
        config->layout.button_height = get_json_int(layout, "button_height", 120);
        config->layout.button_radius = get_json_int(layout, "button_radius", 20);
        config->layout.button_text_padding = get_json_int(layout, "button_text_padding", 40);
        config->layout.timer_bar_height = get_json_int(layout, "timer_bar_height", 80);
    }

    /* Parse appearance settings */
    struct json_object *appearance;
    if (json_object_object_get_ex(root, "appearance", &appearance)) {
        const char *scheme = get_json_string(appearance, "color_scheme", "colorblind");
        config->color_scheme = strdup_safe(scheme);

        const char *font = get_json_string(appearance, "font_path", "assets/fonts/Roboto-Bold.ttf");
        config->font_path = strdup_safe(font);
    } else {
        config->color_scheme = strdup_safe("colorblind");
        config->font_path = strdup_safe("assets/fonts/Roboto-Bold.ttf");
    }

    /* Parse input settings */
    struct json_object *input;
    if (json_object_object_get_ex(root, "input", &input)) {
        const char *quiz = get_json_string(input, "quiz_file", "examples/sample_quiz.json");
        config->quiz_file = strdup_safe(quiz);
    } else {
        config->quiz_file = strdup_safe("examples/sample_quiz.json");
    }

    /* Parse output settings */
    struct json_object *output;
    if (json_object_object_get_ex(root, "output", &output)) {
        const char *file = get_json_string(output, "file", "quiz_video.mp4");
        config->output_file = strdup_safe(file);
    } else {
        config->output_file = strdup_safe("quiz_video.mp4");
    }

    /* Parse animation settings */
    struct json_object *animation;
    if (json_object_object_get_ex(root, "animation", &animation)) {
        struct json_object *val;

        if (json_object_object_get_ex(animation, "question_fade_duration", &val)) {
            config->animation.question_fade_duration = (float)json_object_get_double(val);
        }
        if (json_object_object_get_ex(animation, "answer_fade_duration", &val)) {
            config->animation.answer_fade_duration = (float)json_object_get_double(val);
        }
        if (json_object_object_get_ex(animation, "answer_delay_between", &val)) {
            config->animation.answer_delay_between = (float)json_object_get_double(val);
        }
        if (json_object_object_get_ex(animation, "question_delay", &val)) {
            config->animation.question_delay = (float)json_object_get_double(val);
        }
    }

    json_object_put(root);
    printf("Configuration loaded from %s\n", config_file);

    return 0;
}

void config_free(AppConfig *config) {
    if (config->color_scheme) {
        free((void *)config->color_scheme);
        config->color_scheme = NULL;
    }
    if (config->font_path) {
        free((void *)config->font_path);
        config->font_path = NULL;
    }
    if (config->quiz_file) {
        free((void *)config->quiz_file);
        config->quiz_file = NULL;
    }
    if (config->output_file) {
        free((void *)config->output_file);
        config->output_file = NULL;
    }
}

int config_apply(const AppConfig *config) {
    /* Apply color scheme */
    if (strcmp(config->color_scheme, "grayscale") == 0) {
        colors_init(&COLOR_SCHEME_GRAYSCALE);
    } else if (strcmp(config->color_scheme, "colorblind") == 0) {
        colors_init(&COLOR_SCHEME_COLORBLIND_ALTERNATIVE);
    } else if (strcmp(config->color_scheme, "default") == 0) {
        colors_init(&COLOR_SCHEME_DEFAULT);
    } else {
        fprintf(stderr, "Unknown color scheme: %s, using colorblind\n", config->color_scheme);
        colors_init(&COLOR_SCHEME_COLORBLIND);
        return -1;
    }

    printf("Applied configuration:\n");
    printf("  Video: %dx%d @ %d fps\n", config->video.width, config->video.height, config->video.fps);
    printf("  Color scheme: %s\n", config->color_scheme);
    printf("  Font: %s\n", config->font_path);
    printf("  Quiz: %s\n", config->quiz_file);
    printf("  Output: %s\n\n", config->output_file);

    return 0;
}
