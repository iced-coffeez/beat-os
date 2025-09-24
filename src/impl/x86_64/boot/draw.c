#include "draw.h"
#include <stddef.h>
#include <stdint.h>

static uint32_t* framebuffer = 0;
static size_t screen_width = 0;
static size_t screen_height = 0;
static size_t stride = 0; // pixels per scanline

void gfx_init(void* framebuffer_base, size_t width, size_t height, size_t pixels_per_scanline) {
    framebuffer = (uint32_t*)framebuffer_base;
    screen_width = width;
    screen_height = height;
    stride = pixels_per_scanline;
}

void putpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!framebuffer) return;
    if (x >= screen_width || y >= screen_height) return;

    framebuffer[y * stride + x] = color;
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t j = 0; j < h; j++) {
        for (uint32_t i = 0; i < w; i++) {
            putpixel(x + i, y + j, color);
        }
    }
}

void clear_screen(uint32_t color) {
    for (uint32_t y = 0; y < screen_height; y++) {
        for (uint32_t x = 0; x < screen_width; x++) {
            putpixel(x, y, color);
        }
    }
}
