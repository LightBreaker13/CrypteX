#pragma once

#include <stdint.h>

void fb_init(void *mb2);
void fb_clear(uint32_t color);
void fb_putpx(int x, int y, uint32_t color);
void fb_fillrect(int x, int y, int w, int h, uint32_t color);
void fb_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg);
void fb_draw_text(int x, int y, const char *text, uint32_t fg, uint32_t bg);
int fb_width(void);
int fb_height(void);

