#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
  FT_Library library;
  FT_Face face;
  int font_size;
} TextContext;

int text_init(TextContext *ctx, const char *font_path, int font_size);

int text_render_alpha(TextContext *ctx, uint8_t *rgb_buffer, int buffer_width,
                int buffer_height, const char *text, int x, int y,
                uint8_t r, uint8_t g, uint8_t b, float alpha);

int text_measure_width(TextContext *ctx, const char *text);

int text_render_centered_alpha(TextContext *ctx, uint8_t *rgb_buffer,
                         int buffer_width, int buffer_height,
                         const char *text, int y,
                         uint8_t r, uint8_t g, uint8_t b, float alpha);

/* Word-wrap helpers */
int text_measure_wrapped(TextContext *ctx, const char *text, int max_width);

int text_render_wrapped_centered(TextContext *ctx, uint8_t *rgb_buffer,
                         int buffer_width, int buffer_height,
                         const char *text, int y,
                         int max_width, int line_height,
                         uint8_t r, uint8_t g, uint8_t b, float alpha);

void text_close(TextContext *ctx);

#endif // TEXT_H
