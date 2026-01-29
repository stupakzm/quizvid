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

int text_render(TextContext *ctx, uint8_t *rgb_buffer, int buffer_width,
                int buffer_height, const char *text, int x, int y,
                uint8_t r, uint8_t g, uint8_t b);

int text_measure_width(TextContext *ctx, const char *text);

int text_render_centered(TextContext *ctx, uint8_t *rgb_buffer,
                         int buffer_width, int buffer_height,
                         const char *text, int y,
                         uint8_t r, uint8_t g, uint8_t b);

void text_close(TextContext *ctx);

#endif // TEXT_H
