#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text.h"

int text_init(TextContext *ctx, const char *font_path, int font_size) {
  FT_Error error;

  /* Initialize FreeType lib */
  error = FT_Init_FreeType(&ctx->library);
  if(error){
    fprintf(stderr, "Failed to initialize FreeType\n");
    return -1;
  }

  /* Load font */
  error = FT_New_Face(ctx->library, font_path, 0, &ctx->face);
  if(error){
    fprintf(stderr, "Failed to load font: %s\n", font_path);
    FT_Done_FreeType(ctx->library);
    return -1;
  }

  error = FT_Set_Pixel_Sizes(ctx->face, 0, font_size);
  if(error){
    fprintf(stderr, "Failed to set font size\n");
    FT_Done_Face(ctx->face);
    FT_Done_FreeType(ctx->library);
    return -1;
  }

  ctx->font_size = font_size;
  printf("Text renderer initialized: %s @ %dpx\n", font_path, font_size);

  return 0;
}

void text_close(TextContext *ctx){
  if(ctx->face){
    FT_Done_Face(ctx->face);
  }
  if(ctx->library){
    FT_Done_FreeType(ctx->library);
  }
}

int text_render(TextContext *ctx, uint8_t *rgb_buffer, int buffer_width, int buffer_height,
                const char *text, int x, int y, uint8_t r, uint8_t g, uint8_t b){
  FT_Error error;
  FT_GlyphSlot slot = ctx->face->glyph;
  int pen_x = x;
  int pen_y = y;

  /* Render each character */
  for(size_t i = 0; i < strlen(text); i++){
    error = FT_Load_Char(ctx->face, text[i], FT_LOAD_RENDER);
    if(error){
      fprintf(stderr, "Failed to load character '%c'\n", text[i]);
      continue;
    }

    FT_Bitmap *bitmap = &slot->bitmap;

    int draw_x = pen_x + slot->bitmap_left;
    int draw_y = pen_y - slot->bitmap_top;

    /* Blend bitmap onto RGB buffer */
    for (unsigned int row = 0; row < bitmap->rows; row++){
      for(unsigned int col = 0; col < bitmap->width; col++){
        int pixel_x = draw_x + col;
        int pixel_y = draw_y + row;

        if(pixel_x < 0 || pixel_x >= buffer_width ||
           pixel_y < 0 || pixel_y >= buffer_height){
          continue;
        }

        uint8_t alpha = bitmap->buffer[row*bitmap->pitch + col];

        int buffer_index = (pixel_y * buffer_width + pixel_x) * 3;

        float alpha_f = alpha / 255.0f;
        rgb_buffer[buffer_index + 0] = (uint8_t)(r * alpha_f + rgb_buffer[buffer_index + 0] * (1.0f - alpha_f));
        rgb_buffer[buffer_index + 1] = (uint8_t)(g * alpha_f + rgb_buffer[buffer_index + 1] * (1.0f - alpha_f));
        rgb_buffer[buffer_index + 2] = (uint8_t)(b * alpha_f + rgb_buffer[buffer_index + 2] * (1.0f - alpha_f));
      }
    }
    pen_x += slot->advance.x >> 6;
  }
  return 0;
}

int text_measure_width(TextContext *ctx, const char *text) {
    FT_Error error;
    FT_GlyphSlot slot = ctx->face->glyph;
    int width = 0;

    for (size_t i = 0; i < strlen(text); i++) {
        error = FT_Load_Char(ctx->face, text[i], FT_LOAD_DEFAULT);
        if (error) continue;

        width += slot->advance.x >> 6;
    }

    return width;
}

int text_render_centered(TextContext *ctx, uint8_t *rgb_buffer,
                         int buffer_width, int buffer_height,
                         const char *text, int y,
                         uint8_t r, uint8_t g, uint8_t b) {
    int text_width = text_measure_width(ctx, text);
    int x = (buffer_width - text_width) / 2;

    return text_render(ctx, rgb_buffer, buffer_width, buffer_height,
                       text, x, y, r, g, b);
}
