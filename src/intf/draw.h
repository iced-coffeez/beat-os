#pragma once
#include <stdint.h>
#include <stddef.h>

void gfx_init(void* framebuffer_base, size_t width, size_t height, size_t pixels_per_scanline);
void putpixel(uint32_t x, uint32_t y, uint32_t color);
void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void clear_screen(uint32_t color);

// optional color constants
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xFFFFFF
#define COLOR_RED   0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE  0x0000FF
