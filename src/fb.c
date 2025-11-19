#include "fb.h"
#include "font8x16.h"
#include "multiboot2.h"
#include "common.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint8_t *addr;
    uint32_t clear_color;
} fb_state_t;

static fb_state_t fb;

static uint32_t blend_color(uint32_t src, uint32_t dst, uint8_t alpha) {
    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8) & 0xFF;
    uint32_t sb = src & 0xFF;
    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;
    uint32_t rr = (sr * alpha + dr * (255 - alpha)) / 255;
    uint32_t rg = (sg * alpha + dg * (255 - alpha)) / 255;
    uint32_t rb = (sb * alpha + db * (255 - alpha)) / 255;
    return (rr << 16) | (rg << 8) | rb;
}

void fb_init(void *mb2) {
    kmemset(&fb, 0, sizeof(fb));
    font8x16_init();

    if (!mb2) {
        return;
    }

    mb2_header_t *hdr = (mb2_header_t *)mb2;
    uint8_t *tag_ptr = (uint8_t *)mb2 + 8;
    uint8_t *end = (uint8_t *)mb2 + hdr->total_size;

    while (tag_ptr < end) {
        mb2_tag_t *tag = (mb2_tag_t *)tag_ptr;
        if (tag->type == 8) {
            mb2_tag_fb_t *fb_tag = (mb2_tag_fb_t *)tag;
            fb.addr = (uint8_t *)(uintptr_t)fb_tag->addr;
            fb.width = fb_tag->width;
            fb.height = fb_tag->height;
            fb.pitch = fb_tag->pitch;
            fb.bpp = fb_tag->bpp;
            break;
        }
        if (tag->size == 0) {
            break;
        }
        tag_ptr += (tag->size + 7) & ~7;
    }

    if (!fb.addr) {
        fb.width = 640;
        fb.height = 480;
        fb.pitch = 640 * 4;
        static uint32_t fallback[640 * 480];
        fb.addr = (uint8_t *)fallback;
    }

    fb.clear_color = 0x00102030;
    fb_clear(fb.clear_color);
}

void fb_clear(uint32_t color) {
    fb.clear_color = color;
    for (uint32_t y = 0; y < fb.height; ++y) {
        uint32_t *row = (uint32_t *)(fb.addr + y * fb.pitch);
        for (uint32_t x = 0; x < fb.width; ++x) {
            row[x] = color;
        }
    }
}

void fb_putpx(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || (uint32_t)x >= fb.width || (uint32_t)y >= fb.height) {
        return;
    }
    uint32_t *row = (uint32_t *)(fb.addr + y * fb.pitch);
    row[x] = color;
}

void fb_fillrect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) {
        return;
    }
    for (int yy = 0; yy < h; ++yy) {
        int py = y + yy;
        if (py < 0 || (uint32_t)py >= fb.height) {
            continue;
        }
        uint32_t *row = (uint32_t *)(fb.addr + py * fb.pitch);
        for (int xx = 0; xx < w; ++xx) {
            int px = x + xx;
            if (px < 0 || (uint32_t)px >= fb.width) {
                continue;
            }
            row[px] = color;
        }
    }
}

void fb_draw_char(int x, int y, char ch, uint32_t fg, uint32_t bg) {
    char glyph = kupper(ch);
    const uint8_t *rows = font8x16[(uint8_t)glyph];
    for (int row = 0; row < 16; ++row) {
        uint8_t bits = rows[row];
        for (int col = 0; col < 8; ++col) {
            uint32_t color = (bits & (1 << (7 - col))) ? fg : bg;
            if (color == 0xFFFFFFFF) {
                continue;
            }
            fb_putpx(x + col, y + row, color);
        }
    }
}

void fb_draw_text(int x, int y, const char *text, uint32_t fg, uint32_t bg) {
    int cursor_x = x;
    int cursor_y = y;
    while (*text) {
        if (*text == '\n') {
            cursor_y += 16;
            cursor_x = x;
        } else {
            fb_draw_char(cursor_x, cursor_y, *text, fg, bg);
            cursor_x += 8;
        }
        text++;
    }
}

int fb_width(void) {
    return (int)fb.width;
}

int fb_height(void) {
    return (int)fb.height;
}

